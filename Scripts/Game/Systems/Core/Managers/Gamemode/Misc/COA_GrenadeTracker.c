//------------------------------------------------------------------------------------------------
// Grenade tracker
//
// Server: every character (players and AI) gets an OnGrenadeThrown listener. Each throw is raised on
// GetOnGrenadeThrown() (the CRF VAAR recorder listens there) and broadcast to the clients.
// Clients: keep the thrown grenades (by RplId) until they explode, their smoke runs out or they time
// out, so the spectator can draw them in the world (COA_SpecWorldMarkers) and on its map
// (COA_SpectatorMapOverlay). Owned by COA_RplBroadcastManager.
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
enum COA_EGrenadeType
{
	EXPLOSIVE = 0,
	SMOKE = 1
}

//------------------------------------------------------------------------------------------------
//! A thrown grenade as a client knows it
class COA_ThrownGrenade
{
	RplId m_Id;
	int m_iType;			// COA_EGrenadeType
	string m_sFactionKey;	// thrower's faction
	float m_fExpireAt;		// world time (ms)
	bool m_bResolved;		// the entity has been seen on this client

	//------------------------------------------------------------------------------------------------
	//! The grenade entity, or null while it isn't streamed in / once it's gone
	IEntity GetEntity()
	{
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(m_Id));
		if (!rpl)
			return null;

		return rpl.GetEntity();
	}
}

//------------------------------------------------------------------------------------------------
//! Forwards one character's OnGrenadeThrown event to the tracker. The event's arguments don't say
//! who threw, so each character gets its own listener.
class COA_GrenadeThrowListener
{
	IEntity m_Character;	// weak: null once the character is deleted
	protected COA_GrenadeTracker m_Tracker;

	//------------------------------------------------------------------------------------------------
	void COA_GrenadeThrowListener(IEntity character, COA_GrenadeTracker tracker)
	{
		m_Character = character;
		m_Tracker = tracker;
	}

	//------------------------------------------------------------------------------------------------
	void Hook()
	{
		EventHandlerManagerComponent eventHandler = GetEventHandler();
		if (eventHandler)
			eventHandler.RegisterScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);
	}

	//------------------------------------------------------------------------------------------------
	void Unhook()
	{
		EventHandlerManagerComponent eventHandler = GetEventHandler();
		if (eventHandler)
			eventHandler.RemoveScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);
	}

	//------------------------------------------------------------------------------------------------
	protected EventHandlerManagerComponent GetEventHandler()
	{
		if (!m_Character)
			return null;

		return EventHandlerManagerComponent.Cast(m_Character.FindComponent(EventHandlerManagerComponent));
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGrenadeThrown(int playerID, BaseWeaponComponent weapon, IEntity entity)
	{
		if (m_Tracker)
			m_Tracker.OnThrown(m_Character, weapon, entity);
	}
}

//------------------------------------------------------------------------------------------------
class COA_GrenadeTracker
{
	static const float EXPLOSIVE_LIFETIME_MS = 20000;
	static const float SMOKE_LIFETIME_MS = 120000;

	protected static const int HOOK_INTERVAL_MS = 1000;
	protected static const int PRUNE_EVERY_TICKS = 60;

	protected static COA_GrenadeTracker s_Instance;

	// (IEntity thrower, IEntity grenade, int COA_EGrenadeType) - raised on the server
	protected static ref ScriptInvoker s_OnGrenadeThrown;

	protected ref map<EntityID, ref COA_GrenadeThrowListener> m_mListeners = new map<EntityID, ref COA_GrenadeThrowListener>();
	protected int m_iTicks;

	protected ref array<ref COA_ThrownGrenade> m_aGrenades = {};

	//------------------------------------------------------------------------------------------------
	void COA_GrenadeTracker()
	{
		s_Instance = this;

		// Server-ness is checked on each tick: replication may not be set up yet when this is created
		GetGame().GetCallqueue().CallLater(HookCharacters, HOOK_INTERVAL_MS, true);
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_GrenadeTracker()
	{
		if (s_Instance == this)
			s_Instance = null;

		GetGame().GetCallqueue().Remove(HookCharacters);

		// Characters that outlive the tracker must not call back into it
		foreach (EntityID characterId, COA_GrenadeThrowListener listener : m_mListeners)
		{
			if (listener)
				listener.Unhook();
		}
	}

	//------------------------------------------------------------------------------------------------
	static COA_GrenadeTracker GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	//! Red of the spectator's grenade icons (linear colour of sRGB #E5383B)
	static Color GetIconColor()
	{
		return Color.FromSRGBA(229, 56, 59, 255);
	}

	//------------------------------------------------------------------------------------------------
	//! Whether the spectator should draw this grenade under the "hide other spectator factions" setting
	static bool IsVisibleToFaction(COA_ThrownGrenade grenade, bool hideOtherFactions, Faction localFaction)
	{
		if (!hideOtherFactions || !localFaction || grenade.m_sFactionKey.IsEmpty())
			return true;

		return grenade.m_sFactionKey == localFaction.GetFactionKey();
	}

	//------------------------------------------------------------------------------------------------
	//! The grenade entity once it has left the thrower's hand, else null
	static IEntity GetThrownEntity(COA_ThrownGrenade grenade)
	{
		IEntity entity = grenade.GetEntity();
		if (!entity || entity.GetRootParent() != entity)
			return null;

		return entity;
	}

	//------------------------------------------------------------------------------------------------
	//! Server: raised for every thrown grenade with (IEntity thrower, IEntity grenade, int COA_EGrenadeType)
	static ScriptInvoker GetOnGrenadeThrown()
	{
		if (!s_OnGrenadeThrown)
			s_OnGrenadeThrown = new ScriptInvoker();

		return s_OnGrenadeThrown;
	}

	//================================================================================================
	// SERVER
	//================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Hook every character that doesn't have a listener yet (players and AI)
	protected void HookCharacters()
	{
		if (!Replication.IsServer())
			return;

		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return;

		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);
		foreach (AIAgent agent : agents)
		{
			IEntity character = agent.GetControlledEntity();
			if (!character || !ChimeraCharacter.Cast(character))
				continue;

			EntityID characterId = character.GetID();
			if (m_mListeners.Contains(characterId))
				continue;

			// Kept even without an event handler, so the character isn't looked up again every tick
			COA_GrenadeThrowListener listener = new COA_GrenadeThrowListener(character, this);
			listener.Hook();
			m_mListeners.Insert(characterId, listener);
		}

		m_iTicks++;
		if (m_iTicks % PRUNE_EVERY_TICKS == 0)
			PruneListeners();
	}

	//------------------------------------------------------------------------------------------------
	//! Forget listeners of characters that have been deleted
	protected void PruneListeners()
	{
		array<EntityID> stale = {};
		foreach (EntityID characterId, COA_GrenadeThrowListener listener : m_mListeners)
		{
			if (!listener || !listener.m_Character)
				stale.Insert(characterId);
		}

		foreach (EntityID staleId : stale)
		{
			m_mListeners.Remove(staleId);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Server: a character threw a grenade
	void OnThrown(IEntity thrower, BaseWeaponComponent weapon, IEntity thrownEntity)
	{
		if (!weapon || !Replication.IsServer())
			return;

		// The event hands over the thrown grenade; fall back to the grenade weapon's own entity
		IEntity grenade = thrownEntity;
		if (!grenade || grenade == thrower)
			grenade = weapon.GetOwner();
		if (!grenade)
			return;

		int grenadeType = COA_EGrenadeType.EXPLOSIVE;
		if (weapon.GetWeaponType() == EWeaponType.WT_SMOKEGRENADE)
			grenadeType = COA_EGrenadeType.SMOKE;

		GetOnGrenadeThrown().Invoke(thrower, grenade, grenadeType);

		// Only replicated grenades can be followed on the clients
		RplComponent rpl = RplComponent.Cast(grenade.FindComponent(RplComponent));
		if (!rpl || !rpl.Id().IsValid())
			return;

		RplId grenadeId = rpl.Id();

		string factionKey;
		FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(thrower.FindComponent(FactionAffiliationComponent));
		if (factionComponent && factionComponent.GetAffiliatedFaction())
			factionKey = factionComponent.GetAffiliatedFaction().GetFactionKey();

		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.BroadcastGrenadeThrown(grenadeId, grenadeType, factionKey);
	}

	//================================================================================================
	// CLIENT
	//================================================================================================

	//------------------------------------------------------------------------------------------------
	//! A grenade was thrown somewhere (COA_RplBroadcastManager.RpcDo_BroadcastGrenadeThrown)
	void ReceiveThrown(RplId grenadeId, int grenadeType, string factionKey)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		float lifetime = EXPLOSIVE_LIFETIME_MS;
		if (grenadeType == COA_EGrenadeType.SMOKE)
			lifetime = SMOKE_LIFETIME_MS;

		COA_ThrownGrenade grenade = new COA_ThrownGrenade();
		grenade.m_Id = grenadeId;
		grenade.m_iType = grenadeType;
		grenade.m_sFactionKey = factionKey;
		grenade.m_fExpireAt = world.GetWorldTime() + lifetime;
		m_aGrenades.Insert(grenade);
	}

	//------------------------------------------------------------------------------------------------
	//! Client: the grenades still in the air or on the ground. Drops grenades that have timed out, and
	//! ones that were seen and have since been deleted (exploded / smoke finished).
	int GetActiveGrenades(notnull array<COA_ThrownGrenade> outGrenades)
	{
		outGrenades.Clear();

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return 0;

		float now = world.GetWorldTime();
		for (int i = m_aGrenades.Count() - 1; i >= 0; i--)
		{
			COA_ThrownGrenade grenade = m_aGrenades[i];
			IEntity entity = grenade.GetEntity();
			if (entity)
				grenade.m_bResolved = true;

			if (now > grenade.m_fExpireAt || (grenade.m_bResolved && !entity))
			{
				m_aGrenades.Remove(i);
				continue;
			}

			outGrenades.Insert(grenade);
		}

		return outGrenades.Count();
	}
}
