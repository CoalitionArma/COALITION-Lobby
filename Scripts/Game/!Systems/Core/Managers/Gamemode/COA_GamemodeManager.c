class COA_GamemodeManagerClass : SCR_BaseGameModeComponentClass {}

class COA_GamemodeManager : SCR_BaseGameModeComponent
{	
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================
	protected ref COA_ResourceCache m_ResourceCache;

	protected SCR_GroupsManagerComponent m_GroupsManagerComponent;
	// SendRespawnScreen stayed on the lobby-side broadcast manager (COA_Gamemode's own respawn-into-slot
	// flow calls it too), so this needs its own reference to that class.
	protected COA_RplBroadcastManager m_RplBroadcastManager;
	protected COA_SlottingManager m_SlottingManager;
	protected COA_RespawnManager m_RespawnManager;
	protected COA_MenuManager m_MenuManager;
	protected COA_Gamemode m_Gamemode;
	
	protected const int STATS_TRACKING_INIT_RETRY_DELAY_MS = 250;
	protected const int STATS_TRACKING_INIT_MAX_RETRIES = 20;

	protected const int CHARACTER_ASSIGN_RETRY_DELAY_MS = 250;
	protected const int CHARACTER_ASSIGN_MAX_RETRIES = 15;
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{	
		super.OnPostInit(owner);
		
		if (RplSession.Mode() != RplMode.Client)
		{
			// Initialize all required manager references
			InitializeManagers();
		
			m_ResourceCache = new COA_ResourceCache;
			GetGame().GetCallqueue().Call(m_ResourceCache.PreLoadCharacterResources);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initialize all manager references needed for this component
	protected void InitializeManagers()
	{
		m_GroupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		m_RplBroadcastManager = COA_RplBroadcastManager.GetInstance();
		m_SlottingManager = COA_SlottingManager.GetInstance();
		m_RespawnManager = COA_RespawnManager.GetInstance();
		m_MenuManager = COA_MenuManager.GetInstance();
		m_Gamemode = COA_Gamemode.GetInstance();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 PLAYER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Initialize a player into the game either as a playable character or spectator
	//! \param[in] playerId ID of the player to initialize
	//! \param[in] spawnPointID the ID of the spawn point we want to spawn this player at (either set manually with the respawn screen or automatic if -1;
	//! \param[in] entityRplID the rplID of the entity  we want to spawn this player at (either set manually or automatic if invalid rpl id;
	bool InitilizePlayer(int playerId, int spawnPointID = -1, RplId entityRplID = RplId.Invalid())
	{	
		if (playerId <= 0)
			return true;
		
		if (!EnsureManagersReady())
			return false;
		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!playerController)
			return false;
			
		COA_PlayerCharacter playerCharacter = null;
		Faction faction = null;
		bool alreadyCreated;
		
		// Determine if player should be spectator or playable character
		if (!m_SlottingManager.IsPlayerInASlot(playerId) || m_SlottingManager.IsPlayerConsideredDead(playerId))
		{
			// SPECTATOR PATH: Create initial entity for spectators
			playerCharacter = GetOrCreateSpectatorEntity(playerId, playerController);

			faction = GetGame().GetFactionManager().GetFactionByKey("SPEC");

			COA_InitializationHelper.RemovePlayerFromCurrentGroup(playerId);
		} else {
			// PLAYABLE CHARACTER PATH: Skip initial entity, spawn real character directly
			playerCharacter = GetOrCreatePlayableCharacter(playerId, spawnPointID, entityRplID, alreadyCreated);
			faction = m_SlottingManager.GetPlayerSlotFaction(playerId);
			
			m_MenuManager.RemovePlayerFromAnyChannel(playerId, false);
		}
		
		if (!playerCharacter)
			return false;
		
		RplComponent playerRplComp = RplComponent.Cast(playerCharacter.FindComponent(RplComponent));
		if (!playerRplComp)
			return false;
		
		if (playerCharacter && playerRplComp)
		{
			playerCharacter.DisableAI();

			if (!COA_EntityHelper.IsSpectator(playerCharacter))
			{
				// Playable characters wait for their gear before being handed over — see
				// ScheduleAssignPlayerToCharacter, which owns the rest of the sequence.
				ScheduleAssignPlayerToCharacter(playerCharacter, playerId, playerController, playerRplComp.Id(), 0);
			} else {
				//Sends the player the respawn screen if they reconnect while dead
				if (m_SlottingManager.IsPlayerInASlot(playerId) && m_SlottingManager.IsPlayerConsideredDead(playerId) && m_RespawnManager.CanPlayerRespawn(playerCharacter, faction.GetFactionKey(), playerId))
					m_RplBroadcastManager.SendRespawnScreen(playerId);

				COA_InitializationHelper.AssignCharacterToPlayer(playerController, playerCharacter);
				COA_InitializationHelper.AssignFactionToPlayer(playerController, faction);

				m_RplBroadcastManager.InitilizePlayerBroadcast(playerId, playerRplComp.Id());
			};
		};

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Re-acquire manager references if init ordering delayed singleton availability.
	protected bool EnsureManagersReady()
	{
		if (!m_RplBroadcastManager || !m_SlottingManager || !m_RespawnManager || !m_MenuManager || !m_Gamemode)
			InitializeManagers();

		return m_RplBroadcastManager && m_SlottingManager && m_RespawnManager && m_MenuManager && m_Gamemode;
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 PLAYER CHARACTER HELPERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Get existing character or create a new one for playable roles
	//! \param[in] playerId ID of the player
	//! \param[in] spawnPointID Optional spawn location
	//! \return The character entity
	protected COA_PlayerCharacter GetOrCreatePlayableCharacter(int playerId, int spawnPointID, RplId entityRplID, out bool alreadyCreated)
	{
		alreadyCreated = true;
		COA_PlayerCharacter playerCharacter = m_SlottingManager.GetPlayerSlotCharacter(playerId);
		
		if (!playerCharacter || playerCharacter.GetCharacterController().IsDead())
		{
			alreadyCreated = false;
			
			m_RplBroadcastManager.SendCharacterLoadingScreen(playerId);
			playerCharacter = SpawnPlayableCharacter(playerId, spawnPointID, entityRplID);
			
			if (!playerCharacter)
				Print(string.Format("[COA_GamemodeManager] ERROR: Failed to spawn character for player %1", playerId), LogLevel.ERROR);
		}
			
		return playerCharacter;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Create a new character for player
	//! \param[in] playerId ID of the player
	//! \param[in] spawnPointID spawn location
	//! \return The character entity
	protected COA_PlayerCharacter SpawnPlayableCharacter(int playerId, int spawnPointID, RplId EntityRplID)
	{
		int slotId = m_SlottingManager.GetPlayerSlotID(playerId);
		if (slotId < 0)
			return null;
			
		ResourceName resourceName = m_SlottingManager.GetPlayerSlotResource(playerId);
		if (resourceName.IsEmpty())
			return null;
		
		COA_SpawnPointData spawnPointData;
		
		if (spawnPointID == -1 && EntityRplID == RplId.Invalid())
			spawnPointData = m_RespawnManager.FindInitalFactionSpawnpoint(m_SlottingManager.GetPlayerSlotFaction(playerId).GetFactionKey(), m_SlottingManager.GetPlayerSlotGroup(playerId));
		else if (EntityRplID == RplId.Invalid())
			spawnPointData = m_RespawnManager.GetSpawnPoint(spawnPointID);
		else
		{
			spawnPointData = new COA_SpawnPointData();
			spawnPointData.SetSpawnPointEntity(EntityRplID);
		}

		// Setup spawn parameters
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		GetSafeSpawnTransform(spawnPointData, spawnParams.Transform);
		
		COA_PlayerCharacter playerCharacter = COA_PlayerCharacter.Cast(
			GetGame().SpawnEntityPrefab(m_ResourceCache.GetCachedResource(resourceName), GetGame().GetWorld(), spawnParams)
		);
		
		if (!playerCharacter)
			return null;
		
		// Update character faction.
		// Not every playable prefab carries a faction component — the Zeus and VIP prefabs in
		// particular declare a bare FactionAffiliationComponent rather than inheriting the
		// SCR_CharacterFactionAffiliationComponent the rest of the roles get. Missing it is not
		// fatal to the spawn, so warn loudly (naming the prefab) and carry on rather than
		// hard-crashing player initialization.
		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(playerCharacter.FindComponent(FactionAffiliationComponent));
		if (facComp)
			facComp.SetAffiliatedFaction(m_SlottingManager.GetPlayerSlotFaction(playerId));
		else
			Print(string.Format("[COA_GamemodeManager] WARNING: '%1' has no FactionAffiliationComponent — spawning player %2 without a faction affiliation", resourceName, playerId), LogLevel.WARNING);
	
		// Update slot data
		RplComponent charRplComp = RplComponent.Cast(playerCharacter.FindComponent(RplComponent));
		if (charRplComp)
			m_SlottingManager.UpdateSlotCharacter(slotId, charRplComp.Id());
		
		return playerCharacter;
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 SPECTATOR CHARACTER HELPERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Create a spectator entity in the world
	//! \param[in] playerId ID of the player
	//! \return The created spectator character
	protected COA_PlayerCharacter GetOrCreateSpectatorEntity(int playerId, SCR_PlayerController playerController)
	{
		COA_PlayerCharacter playerSpectator = COA_PlayerCharacter.Cast(playerController.GetMainEntity());
		
		if (playerSpectator && COA_EntityHelper.IsSpectator(playerSpectator))
		{
			if (!COA_DamageHelper.CheckIfEntityAlive(playerSpectator))
				SCR_EntityHelper.DeleteEntityAndChildren(playerSpectator);
			else
				return playerSpectator;
		}
		
		Resource spectatorRes = Resource.Load(COA_EntityHelper.GetSpectatorResource());
		EntitySpawnParams spawnParams = COA_EntityHelper.CreateSpawnParams(m_Gamemode.GetGenericSpawn());
		
		playerSpectator = COA_PlayerCharacter.Cast(GetGame().SpawnEntityPrefab(spectatorRes, GetGame().GetWorld(), spawnParams));
		
		if (!playerSpectator)
			Print(string.Format("[COA_GamemodeManager] ERROR: Failed to spawn spectator for player %1", playerId), LogLevel.ERROR);
		
		return playerSpectator;
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 PLAYER INIT HELPERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Utilize spawn point information to get positional data for spawning a character entity
	//! \param[in] spawnPointData data of the spawn point we are spawning at
	//! \param[out] trasnformOut spawn location
	protected void GetSafeSpawnTransform(COA_SpawnPointData spawnPointData, out vector trasnformOut[4])
	{
		vector baseTransform[4];
		IEntity spawnPointEnt = COA_EntityHelper.GetEntityFromRplId(spawnPointData.GetSpawnPointEntity());
		if (spawnPointEnt)
			spawnPointEnt.GetWorldTransform(baseTransform);
		
		// Add random offset to prevent exact position overlap
		float angle = Math.RandomFloat01() * Math.PI2;
		float dist = Math.RandomFloat01() * spawnPointData.GetSpawnPointRadius();
		vector offset = Vector(Math.Cos(angle) * dist, 0, Math.Sin(angle) * dist);
		
		baseTransform[3] = baseTransform[3] + offset;
		
		vector surface;
		// Snap to terrain geometry
		if (spawnPointData.GetIfSpawnPointSafetyCheck())
			SCR_TerrainHelper.SnapToGeometry(surface, baseTransform[3], {}, GetGame().GetWorld());
		
		if (surface != vector.Zero && spawnPointData.GetIfSpawnPointConformsToTerrain())
		{
			baseTransform[3] = surface;
			SCR_TerrainHelper.OrientToTerrain(baseTransform);
		}
		
		trasnformOut = baseTransform;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Wait until the character is ready to be handed over, then run the whole server-side
	//! handover in one place and in a fixed order.
	//!
	//! Order matters and mirrors the vanilla spawn sequence (SCR_SpawnHandlerComponent):
	//! gear is applied to the entity first, then the entity is assigned to the player
	//! (SetInitialMainEntity, which is also what transfers ownership), and only then does
	//! anything that depends on the player actually controlling that entity run.
	//!
	//! Everything here is authority-side on purpose. Faction, group membership and radio tuning
	//! are all server-authoritative; a client asking for them cannot confirm the result, and the
	//! radio path in particular is a silent no-op off the authority. The client is told to do its
	//! own local presentation work last, via InitilizePlayerBroadcast.
	//! \param[in] playerCharacter the character being handed over
	//! \param[in] playerId ID of the player to assign
	//! \param[in] playerController Player controller to assign to
	//! \param[in] playerEntityRplId RplId of the character this assignment was issued for, so a
	//!            stale retry (player died/respawned again before this resolved) doesn't fire late
	//! \param[in] attempt current retry count
	protected void ScheduleAssignPlayerToCharacter(COA_PlayerCharacter playerCharacter, int playerId, SCR_PlayerController playerController, RplId playerEntityRplId, int attempt)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerCharacter || !playerController || !playerManager || !playerManager.IsPlayerConnected(playerId))
			return;

		// If a newer InitilizePlayer call has already put this player into a different slot
		// character, that call owns the handover now — drop this one. Compared against the slot
		// rather than the controlled entity: the player has not been given the character yet at
		// this point, so their current controlled entity is still the previous/spectator one and
		// is expected to differ.
		COA_PlayerCharacter slotCharacter = m_SlottingManager.GetPlayerSlotCharacter(playerId);
		if (slotCharacter && slotCharacter != playerCharacter)
			return;

		//--------------------------------------------------------------------------- GROUP ---------------------------------------------------------------------------
		// A slot can legitimately have no group at all (Zeus and other unattached roles), which is
		// different from "the slot has a group but it hasn't replicated into an entity yet". Only
		// the latter is worth waiting for — gating on the group unconditionally would stall
		// group-less slots for the full retry budget and then strand them with no character.
		COA_SlotData slotData = m_SlottingManager.GetPlayerSlotData(playerId);
		bool slotExpectsGroup = slotData && slotData.GetSlotCurrentGroup() != RplId.Invalid();

		SCR_AIGroup group = m_SlottingManager.GetPlayerSlotGroup(playerId);
		int groupId = -1;
		if (group)
			groupId = group.GetGroupID();

		bool isGroupReady = !slotExpectsGroup || (group && groupId != -1);

		SCR_PlayerControllerGroupComponent groupComponent = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId);

		//--------------------------------------------------------------------------- GEARSCRIPT ---------------------------------------------------------------------------
		// Non-gearscript playable prefabs have no gear state to wait on, so treat them as ready.
		bool isCharacterGearSet = true;
		COA_GearscriptCharacter gearscriptCharacter = COA_GearscriptCharacter.Cast(playerCharacter);
		if (gearscriptCharacter)
			isCharacterGearSet = gearscriptCharacter.GetCharacterGearState();

		//--------------------------------------------------------------------------- CHECK ---------------------------------------------------------------------------
		if (!groupComponent || !isCharacterGearSet || !isGroupReady)
		{
			if (attempt + 1 < CHARACTER_ASSIGN_MAX_RETRIES)
			{
				GetGame().GetCallqueue().CallLater(ScheduleAssignPlayerToCharacter, CHARACTER_ASSIGN_RETRY_DELAY_MS, false, playerCharacter, playerId, playerController, playerEntityRplId, attempt + 1);
				return;
			}

			// Out of retries. Hand the character over anyway — a player stuck with no character at
			// all is far worse than one who spawns with a missing group or an untuned radio, and
			// those can still be recovered by re-slotting. Log what was missing.
			Print(string.Format("[COA_GamemodeManager] ERROR: Player %1 not ready after %2 attempts (gearSet: %3, groupComponent: %4, groupReady: %5) — assigning character anyway", playerId, CHARACTER_ASSIGN_MAX_RETRIES, isCharacterGearSet, groupComponent != null, isGroupReady), LogLevel.ERROR);
		}

		//--------------------------------------------------------------------------- CHECK PASS ---------------------------------------------------------------------------
		// 1. Hand the geared character over and transfer ownership to the player.
		COA_InitializationHelper.AssignCharacterToPlayer(playerController, playerCharacter);

		// 2. Faction on the player controller, from the authority.
		COA_InitializationHelper.AssignFactionToPlayer(playerController, m_SlottingManager.GetPlayerSlotFaction(playerId));

		// 3. Group membership. Drives nametags, group VON frequency and squad UI. A slot without a
		//    group (Zeus and similar unattached roles) is legitimate, so this is not a hard gate on
		//    the handover — it just gets skipped.
		if (group && groupId != -1)
			AssignPlayerToGroup(playerId);

		// 4. Radios. Must run after the player controls the character: the radio list is built from
		//    the controlled entity's inventory, and tuning is authority-only.
		COA_InitializationHelper.SetupPlayerRadios(playerId, playerCharacter);

		// 5. Finally let the owning client do its local presentation work.
		m_RplBroadcastManager.InitilizePlayerBroadcast(playerId, playerEntityRplId);
	}

	//------------------------------------------------------------------------------------------------
	//! Assign player to their slotted group. AUTHORITY ONLY — SCR_GroupsManagerComponent is
	//! server-authoritative, and RPC_AskJoinGroup is what pushes the resulting group ID back down
	//! to the owning client. Kept as its own method so mods can hook the moment a player is
	//! grouped (see CRF_CSI_ColorTeam.c) rather than having to re-implement the sequence.
	//! \param[in] playerId ID of the player to assign
	protected void AssignPlayerToGroup(int playerId)
	{
		SCR_AIGroup group = m_SlottingManager.GetPlayerSlotGroup(playerId);
		if (!group)
			return;

		int groupId = group.GetGroupID();
		if (groupId == -1)
			return;

		// Not part of EnsureManagersReady() — a missing groups manager should degrade grouping, not
		// block the player from getting their character.
		if (!m_GroupsManagerComponent)
			m_GroupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();

		if (!m_GroupsManagerComponent)
		{
			Print(string.Format("[COA_GamemodeManager] WARNING: No SCR_GroupsManagerComponent — player %1 will not be grouped", playerId), LogLevel.WARNING);
			return;
		}

		m_GroupsManagerComponent.AddPlayerToGroup(groupId, playerId);

		SCR_PlayerControllerGroupComponent groupComponent = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId);
		if (groupComponent)
			groupComponent.RPC_AskJoinGroup(groupId);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static COA_GamemodeManager m_sInstance;
	void COA_GamemodeManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_GamemodeManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static COA_GamemodeManager GetInstance()
	{
		return m_sInstance;
	}
}