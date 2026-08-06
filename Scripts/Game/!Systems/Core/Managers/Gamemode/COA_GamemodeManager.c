class COA_GamemodeManagerClass : SCR_BaseGameModeComponentClass {}

class COA_GamemodeManager : SCR_BaseGameModeComponent
{	
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================
	protected ref COA_ResourceCache m_ResourceCache;
	
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
				ScheduleAssignPlayerToCharacter(playerCharacter, playerId, playerController, playerRplComp.Id(), 0);
			} else {
				//Sends the player the respawn screen if they reconnect while dead
				if (m_SlottingManager.IsPlayerInASlot(playerId) && m_SlottingManager.IsPlayerConsideredDead(playerId) && m_RespawnManager.CanPlayerRespawn(playerCharacter, faction.GetFactionKey(), playerId))
					m_RplBroadcastManager.SendRespawnScreen(playerId);
				
				COA_InitializationHelper.AssignCharacterToPlayer(playerController, playerCharacter);
				
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
	//! Poll until all parameters are met for a player to be assigned a character, then do it.
	//! \param[in] playerId ID of the player to assign
	//! \param[in] playerController Player controller to check against
	//! \param[in] playerEntityRplId RplId of the character this assignment was issued for, so a
	//!            stale retry (player died/respawned again before this resolved) doesn't fire late
	//! \param[in] attempt current retry count
	protected void ScheduleAssignPlayerToCharacter(COA_PlayerCharacter playerCharacter, int playerId, SCR_PlayerController playerController, RplId playerEntityRplId, int attempt)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerCharacter || !playerManager || !playerManager.IsPlayerConnected(playerId))
			return;

		// If the player already moved on to a different character (e.g. respawned again
		// before this resolved), let that newer InitilizePlayer call own the group assignment.
		IEntity controlledEntity = playerManager.GetPlayerControlledEntity(playerId);
		if (controlledEntity && !COA_EntityHelper.IsSpectator(controlledEntity))
		{
			RplComponent controlledRplComp = RplComponent.Cast(controlledEntity.FindComponent(RplComponent));
			if (!controlledRplComp || controlledRplComp.Id() != playerEntityRplId)
				return;
		};

		//--------------------------------------------------------------------------- GROUP ---------------------------------------------------------------------------
		SCR_AIGroup group = m_SlottingManager.GetPlayerSlotGroup(playerId);
		int groupId = -1;
		if (group)
			groupId = group.GetGroupID();

		SCR_PlayerControllerGroupComponent groupComponent = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId);
		
		//--------------------------------------------------------------------------- GEARSCRIPT ---------------------------------------------------------------------------
		bool isCharacterGearSet = COA_GearscriptCharacter.Cast(playerCharacter).GetCharacterGearState();

		//--------------------------------------------------------------------------- CHECK ---------------------------------------------------------------------------
		if (!group || groupId == -1 || !groupComponent || !isCharacterGearSet)
		{
			if (attempt + 1 >= CHARACTER_ASSIGN_MAX_RETRIES)
			{
				Print(string.Format("[COA_GamemodeManager] WARNING: Failed to assign player %1 to character after %2 attempts (this is very bad)", playerId, CHARACTER_ASSIGN_MAX_RETRIES), LogLevel.ERROR);
				return;
			}

			GetGame().GetCallqueue().CallLater(ScheduleAssignPlayerToCharacter, CHARACTER_ASSIGN_RETRY_DELAY_MS, false, playerCharacter, playerId, playerController, playerEntityRplId, attempt + 1);
			return;
		}
		
		//--------------------------------------------------------------------------- CHECK PASS ---------------------------------------------------------------------------
		COA_InitializationHelper.AssignCharacterToPlayer(playerController, playerCharacter);
		
		m_RplBroadcastManager.InitilizePlayerBroadcast(playerId, playerEntityRplId);
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