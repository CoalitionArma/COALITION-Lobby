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

		// GM possession-slot short-circuit: if this player's slot maps to a pre-existing, GM-placed
		// AI body (see COA_SlottingManager.RegisterGMPossessionGroup), possess that specific entity
		// instead of running the spawn-a-fresh-character flow below.
		int gmSlotId = m_SlottingManager.GetPlayerSlotID(playerId);
		RplId possessionTargetId;
		if (gmSlotId >= 0 && COA_GMPossessionManager.GetInstance().TryGetPossessionTarget(gmSlotId, possessionTargetId))
			return InitilizePossessionPlayer(playerId, gmSlotId, possessionTargetId, playerController);

		COA_PlayerCharacter playerCharacter = null;
		Faction faction = null;
		bool alreadyCreated;
		
		// Determine if player should be spectator or playable character
		if (!m_SlottingManager.IsPlayerInASlot(playerId) || m_SlottingManager.IsPlayerConsideredDead(playerId))
		{
			// SPECTATOR PATH: Create initial entity for spectators
			playerCharacter = GetOrCreateSpectatorEntity(playerId, playerController);
	
			faction = GetGame().GetFactionManager().GetFactionByKey("SPEC");
			
			COA_PlayerHelper.RemovePlayerFromCurrentGroup(playerId);
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
			// GEAR IS APPLIED AFTER POSSESSION, NOT BEFORE. Do not "fix" this ordering.
			//
			// An earlier version equipped the character here, before handover, citing vanilla's
			// PrepareEntity_S -> AssignEntity_S ordering. That was the wrong precedent: the vanilla
			// hook that actually spawns a player's loadout is SCR_BasePlayerLoadout.OnLoadoutSpawned,
			// which fires from OnPlayerSpawnFinalize_S - and SCR_SpawnHandlerComponent calls
			// AssignEntity_S (line 156) BEFORE OnPlayerSpawnFinalize_S (line 160). So vanilla also
			// equips after assignment.
			//
			COA_PlayerHelper.AssignFactionToPlayer(playerController, faction);

			// AI deactivation is part of possession: SCR_PlayerController.SetInitialMainEntity()
			// calls SetAIActivation(entity, false) itself, and the possess-spawn pipeline routes
			// through the same call. Deactivating here as well happened before ownership transfer,
			// leaving a window where the character was AI-active and unowned - which is what the
			// next-frame DisableAIWrap re-check in COA_PlayerCharacter was compensating for.
			COA_PlayerHelper.AssignCharacterToPlayer(playerController, playerCharacter);

			if (!COA_EntityHelper.IsSpectator(playerCharacter))
			{
				// Radios are set up by SetEntityGear() itself, once the gearscript has been applied
				// and the player controls the character - both of which are true again now that gear
				// is back to running after handover.
				AssignPlayerToGroup(playerId);
			}
			else
				//Sends the player the respawn screen if they reconnect while dead
				if (m_SlottingManager.IsPlayerInASlot(playerId) && m_SlottingManager.IsPlayerConsideredDead(playerId) && m_RespawnManager.CanPlayerRespawn(playerCharacter, faction.GetFactionKey(), playerId))
					m_RplBroadcastManager.SendRespawnScreen(playerId);

			m_RplBroadcastManager.InitilizePlayerBroadcast(playerId, playerRplComp.Id());
		};

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Hands a GM-placed AI body over to a player instead of spawning a fresh character. If the body
	//! has already died before anyone claimed the slot, the possession entry is consumed anyway and
	//! initialization is retried, which now falls through to the normal role-based spawn (using the
	//! role COA_GMRoleGuessHelper guessed at registration time) - so nobody gets stranded.
	protected bool InitilizePossessionPlayer(int playerId, int slotId, RplId targetEntityId, SCR_PlayerController playerController)
	{
		SCR_ChimeraCharacter targetCharacter = COA_EntityHelper.GetCharacterFromRplId(targetEntityId);

		if (!targetCharacter || !COA_DamageHelper.CheckIfEntityAlive(targetCharacter))
		{
			COA_GMPossessionManager.GetInstance().ConsumePossessionSlot(slotId);
			return InitilizePlayer(playerId);
		}

		Faction faction = m_SlottingManager.GetPlayerSlotFaction(playerId);
		m_MenuManager.RemovePlayerFromAnyChannel(playerId, false);

		COA_PlayerHelper.AssignFactionToPlayer(playerController, faction);
		COA_PossessionHelper.PossessExistingEntity(playerController, targetCharacter);

		AssignPlayerToGroup(playerId);

		m_RplBroadcastManager.InitilizePlayerBroadcast(playerId, targetEntityId);

		// One-shot: never intercept this slot again, regardless of what happens to the player from here.
		COA_GMPossessionManager.GetInstance().ConsumePossessionSlot(slotId);

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
		
		// Update character faction
		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(playerCharacter.FindComponent(FactionAffiliationComponent));
		facComp.SetAffiliatedFaction(m_SlottingManager.GetPlayerSlotFaction(playerId));
	
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
	//! Assign player to their slotted group.
	//! \param[in] playerId ID of the player to assign
	protected void AssignPlayerToGroup(int playerId)
	{
		SCR_AIGroup group = m_SlottingManager.GetPlayerSlotGroup(playerId);
		if (!group)
			return;

		int groupId = group.GetGroupID();
		if (groupId == -1)
			return;

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