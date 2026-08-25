class COA_RespawnManagerClass : ScriptComponentClass {}

class COA_RespawnManager : ScriptComponent
{

//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================

	// Replicated Properties
	[RplProp(onRplName: "WaveRespawnTimer")]
	private int m_iRespawnWaveCurrentTime;
	float m_fRespawnTimer;
	
	// Ticket System - Moved from COA_Gamemode for efficient replication
	[RplProp()]
	int m_iBLUFORTickets;
	[RplProp()]
	int m_iOPFORTickets;
	[RplProp()]
	int m_iINDFORTickets;
	[RplProp()]
	int m_iCIVTickets;
	
	//Respawn variables
	[RplProp()] bool m_bCurrentRespawnEnabled;
	[RplProp()] bool m_bCurrentWaveRespawn;
	[RplProp()] int m_iCurrentTimeToRespawn;
	int m_iLocalTimeToRespawn = 0;

	// Internal flag to prevent redundant replication updates
	protected bool m_bSuppressReplication = false;
	
	// Store state for UI selection
	bool m_RespawnConfirmed = false;
	COA_SpawnPointData m_SelectedSpawnPoint;
	
	//For vehicle respawning, only tracked on the server
	protected ref array<COA_VehicleSpawner> m_aVehicleSpawners = {};
	
	protected ref map<int, ref COA_SpawnPointData> m_mSpawnPointMap = new map<int, ref COA_SpawnPointData>;
	
	protected COA_Gamemode m_Gamemode;
	protected COA_GamemodeManager m_GamemodeManager;
	protected COA_SafestartManager m_SafestartManager;
	protected COA_SlottingManager m_SlottingManager;
	protected COA_RplBroadcastManager m_RplBroadcastManager;
	
	protected bool m_bNeedsRespawn = false;
	protected bool m_bRespawnInit = false;
	
	// Latest Spawn ID used
	protected int m_iLatestSpawnPointID;
	
	// Invoker for data updates
	protected ref ScriptInvoker m_OnSpawnPointsUpdate;

//=============================================================================================================================================================================================================================================================================================================================================================
//	 MANAGER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		InitializeManagers();
		SetEventMask(owner, EntityEvent.FIXEDFRAME);
		if (!Replication.IsServer())
			return;

		InitializeTicketsFromGamemode();
	}
	
	//------------------------------------------------------------------------------------------------
	private void InitializeManagers()
	{
		m_Gamemode = COA_Gamemode.GetInstance();
		m_GamemodeManager = COA_GamemodeManager.GetInstance();
		m_SafestartManager = COA_SafestartManager.GetInstance();
		m_SlottingManager = COA_SlottingManager.GetInstance();
		m_RplBroadcastManager = COA_RplBroadcastManager.GetInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	private void InitializeRespawnTimers()
	{
		m_iRespawnWaveCurrentTime = m_iCurrentTimeToRespawn;
		m_fRespawnTimer = m_iRespawnWaveCurrentTime;
		m_bRespawnInit = true;
	}
	
	//------------------------------------------------------------------------------------------------
	private void InitializeTicketsFromGamemode()
	{
		if (!m_Gamemode)
			return;
			
		// Copy ticket values from gamemode attributes to local replicated properties
		m_iBLUFORTickets = m_Gamemode.m_iBLUFORTickets;
		m_iOPFORTickets = m_Gamemode.m_iOPFORTickets;
		m_iINDFORTickets = m_Gamemode.m_iINDFORTickets;
		m_iCIVTickets = m_Gamemode.m_iCIVTickets;
		m_iCurrentTimeToRespawn = m_Gamemode.m_iTimeToRespawn;
		m_bCurrentWaveRespawn = m_Gamemode.m_bWaveRespawn;
		m_bCurrentRespawnEnabled = m_Gamemode.m_bRespawnEnabled;
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 INVOKERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void ForceSpawnPointsUpdated()
	{
		if (m_OnSpawnPointsUpdate)
			m_OnSpawnPointsUpdate.Invoke();
	}
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnSpawnPointsUpdated()
	{
		if (!m_OnSpawnPointsUpdate)
			m_OnSpawnPointsUpdate = new ScriptInvoker();

		return m_OnSpawnPointsUpdate;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 GETTERS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	bool TicketsRemaining(string faction)
	{
		int tickets = GetFactionTickets(faction);
		return (tickets > 0 || tickets == -1);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if respawns are allowed based on time cutoff setting
	//! \return True if respawns are allowed, false if past the cutoff time
	bool IsRespawnTimeAllowed()
	{
		// No cutoff configured (0 = never disable)
		if (m_Gamemode.m_iRespawnCutoffMinutes <= 0)
			return true;
		
		// Check if we're within the cutoff window
		int currentTime = GetGame().GetWorld().GetWorldTime();
		int missionEndTime = COA_GameTimerManager.GetInstance().m_iTimeMissionEnds;
		int cutoffTime = missionEndTime - (m_Gamemode.m_iRespawnCutoffMinutes * 60000);
		
		// If current time is past the cutoff, disable respawns
		return currentTime < cutoffTime;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetFactionTickets(string faction)
	{
		switch (faction)
		{
			case "BLUFOR": return m_iBLUFORTickets;
			case "OPFOR": return m_iOPFORTickets;
			case "INDFOR": return m_iINDFORTickets;
			case "CIV": return m_iCIVTickets;
		}
		return 0;
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 GROUP SPAWNPOINT METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void ClearGroupSpawnPoints()
	{
		foreach (int spawnPointId, COA_SpawnPointData spawnPointData : m_mSpawnPointMap)
			if (spawnPointData && spawnPointData.GetIsTempSpawnPoint())
				UnRegisterRespawnPoint(spawnPointId);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 TICKET METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Checks if the player can respawn
	//! \param[in] playerEntity the entity of the player
	//! \param[in] factionKey the faction key of the player
	//! \param[in] playerId the player's numeric ID, required when the mission is using COA_ERespawnMode.SLOT
	//! \return True if the player can respawn
	bool CanPlayerRespawn(IEntity playerEntity, string factionKey, int playerId = -1)
	{
		bool respawnsAvailable;
		if (m_Gamemode.m_eRespawnMode == COA_ERespawnMode.SLOT)
			respawnsAvailable = SlotRespawnsRemaining(playerId);
		else
			respawnsAvailable = TicketsRemaining(factionKey);

		//This is a mess theres got to be a better way, one day we'll find it - Salami
		if (m_bCurrentRespawnEnabled &&
			!COA_EntityHelper.IsSpectator(playerEntity) &&
			m_Gamemode.m_GamemodeState != COA_EGamemodeState.AAR &&
			respawnsAvailable &&
			IsRespawnTimeAllowed() &&
			!GetFactionSpawnpoints(factionKey).IsEmpty() &&
			!factionKey.IsEmpty())
				return true;

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Checks if a player's own slot/group still has respawns available (COA_ERespawnMode.SLOT only)
	//! \param[in] playerId the player's numeric ID
	//! \return True if the player's slot/group has respawns remaining, or is unlimited (-1)
	bool SlotRespawnsRemaining(int playerId)
	{
		COA_SlotData slotData = m_SlottingManager.GetPlayerSlotData(playerId);
		if (!slotData)
			return false;

		int remaining = slotData.GetSlotRespawnsRemaining();
		return (remaining > 0 || remaining == -1);
	}
	//------------------------------------------------------------------------------------------------
	void SubtractTicket(FactionKey faction, int amount, bool force = false)
	{
		bool changed = SubtractTicketSilent(faction, amount, force);
		if (changed && !m_bSuppressReplication)
			Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	//! Deducts a player respawn, routing to the faction ticket pool or the player's slot/group pool
	//! depending on the mission's COA_ERespawnMode. Use this instead of SubtractTicket directly.
	//! \param[in] faction Faction to subtract tickets from (COA_ERespawnMode.TEAM only)
	//! \param[in] playerId Player to deduct a slot/group respawn from (COA_ERespawnMode.SLOT only)
	//! \param[in] amount Number of respawns to deduct
	//! \param[in] force Force deduction even during safestart
	void DeductPlayerRespawn(FactionKey faction, int playerId, int amount = 1, bool force = false)
	{
		if (m_Gamemode.m_eRespawnMode == COA_ERespawnMode.SLOT)
			SubtractSlotRespawn(playerId, amount, force);
		else
			SubtractTicket(faction, amount, force);
	}

	//------------------------------------------------------------------------------------------------
	//! Subtract tickets silently without triggering replication
	//! \param[in] faction Faction to subtract tickets from
	//! \param[in] amount Number of tickets to subtract
	//! \param[in] force Force subtraction even during safestart
	//! \return True if tickets were actually subtracted
	protected bool SubtractTicketSilent(FactionKey faction, int amount, bool force = false)
	{
		// Don't subtract tickets during safestart
		if (m_SafestartManager.GetSafestartStatus() && !force)
			return false;
			
		int currentTickets = GetFactionTickets(faction);
		
		// Don't subtract if tickets are unlimited (-1) or already at 0
		if (currentTickets == -1 || currentTickets <= 0)
			return false;

		// Update the appropriate faction's tickets
		switch (faction)
		{
			case "BLUFOR": m_iBLUFORTickets -= amount;
				if (force && m_iBLUFORTickets < 0)
					m_iBLUFORTickets = 0;
				else if (m_iBLUFORTickets < 0 && m_iBLUFORTickets != -1) 
					m_iBLUFORTickets = 0;
				break;
			
			case "OPFOR": m_iOPFORTickets -= amount;
				if (force && m_iOPFORTickets < 0)
					m_iOPFORTickets = 0;
				else if (m_iOPFORTickets < 0 && m_iOPFORTickets != -1) 
					m_iOPFORTickets = 0;
				break;
			
			case "INDFOR": m_iINDFORTickets -= amount;
				if (force && m_iINDFORTickets < 0)
					m_iINDFORTickets = 0;
				else if (m_iINDFORTickets < 0 && m_iINDFORTickets != -1) 
					m_iINDFORTickets = 0;
				break;
			
			case "CIV": m_iCIVTickets -= amount;
				if (force && m_iCIVTickets < 0)
					m_iCIVTickets = 0;
				else if (m_iCIVTickets < 0 && m_iCIVTickets != -1) 
					m_iCIVTickets = 0;
				break;
		}
		
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Deducts respawns from a player's slot/group pool (COA_ERespawnMode.SLOT).
	//! For PER_GROUP pools, propagates the new remaining count to every slot in the group so the
	//! whole squad sees the shared value update.
	//! \param[in] playerId Player whose slot/group pool to subtract from
	//! \param[in] amount Number of respawns to deduct
	//! \param[in] force Force subtraction even during safestart
	protected void SubtractSlotRespawn(int playerId, int amount, bool force = false)
	{
		// Don't subtract respawns during safestart
		if (m_SafestartManager.GetSafestartStatus() && !force)
			return;

		COA_SlotData slotData = m_SlottingManager.GetPlayerSlotData(playerId);
		if (!slotData)
			return;

		int remaining = slotData.GetSlotRespawnsRemaining();

		// Unlimited or already exhausted, nothing to do
		if (remaining == -1 || remaining <= 0)
			return;

		int newRemaining = remaining - amount;
		if (newRemaining < 0)
			newRemaining = 0;

		if (slotData.GetRespawnPoolType() == COA_ERespawnPoolType.PER_GROUP)
		{
			array<int> groupSlotIds = m_SlottingManager.GetAllSlotIDsForGroup(slotData.GetSlotCurrentGroup());
			foreach (int groupSlotId : groupSlotIds)
				m_SlottingManager.UpdateSlotRespawnsRemaining(groupSlotId, newRemaining);
		}
		else
		{
			m_SlottingManager.UpdateSlotRespawnsRemaining(slotData.GetSlotId(), newRemaining);
		}
	}

	//------------------------------------------------------------------------------------------------
	void AddTicket(FactionKey faction, int amount, bool force = false)
	{
		bool changed = AddTicketSilent(faction, amount, force);
		if (changed && !m_bSuppressReplication)
			Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Add tickets silently without triggering replication
	//! \param[in] faction Faction to add tickets to
	//! \param[in] amount Number of tickets to add
	//! \param[in] force Force addition even during safestart
	//! \return True if tickets were actually added
	protected bool AddTicketSilent(FactionKey faction, int amount, bool force = false)
	{
		// Don't add tickets during safestart
		if (m_SafestartManager.GetSafestartStatus() && !force)
			return false;
			
		int currentTickets = GetFactionTickets(faction);

		// Update the appropriate faction's tickets
		switch (faction)
		{
			case "BLUFOR": m_iBLUFORTickets += amount; break;
			case "OPFOR": m_iOPFORTickets += amount; break;
			case "INDFOR": m_iINDFORTickets += amount; break;
			case "CIV": m_iCIVTickets += amount; break;
		}
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Batch subtract tickets for multiple operations to minimize replication calls
	//! \param[in] ticketChanges Array of ticket changes {faction, amount, force}
	void BatchSubtractTickets(array<ref array<string>> ticketChanges)
	{
		bool anyChanged = false;
		m_bSuppressReplication = true;
		
		foreach (ref array<string> change : ticketChanges)
		{
			if (change.Count() < 2)
				continue;
				
			FactionKey faction = change[0];
			int amount = change[1].ToInt();
			bool force = (change.Count() > 2 && change[2] == "true");
			
			if (SubtractTicketSilent(faction, amount, force))
				anyChanged = true;
		}
		
		m_bSuppressReplication = false;
		if (anyChanged)
			Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Batch add tickets for multiple operations to minimize replication calls
	//! \param[in] ticketChanges Array of ticket changes {faction, amount, force}
	void BatchAddTickets(array<ref array<string>> ticketChanges)
	{
		bool anyChanged = false;
		m_bSuppressReplication = true;
		
		foreach (ref array<string> change : ticketChanges)
		{
			if (change.Count() < 2)
				continue;
				
			FactionKey faction = change[0];
			int amount = change[1].ToInt();
			bool force = (change.Count() > 2 && change[2] == "true");
			
			if (AddTicketSilent(faction, amount, force))
				anyChanged = true;
		}
		
		m_bSuppressReplication = false;
		if (anyChanged)
			Replication.BumpMe();
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 TIMER METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	float m_fUpdateBuffer = 0;
	override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		super.EOnFixedFrame(owner, timeSlice);
		#ifdef WORKBENCH
		if (!m_bRespawnInit && m_bCurrentRespawnEnabled)
			InitializeRespawnTimers();
		if (m_fUpdateBuffer >= 1)
		{
			if (m_bCurrentWaveRespawn)
				WaveRespawnTimer();
			m_fUpdateBuffer = 0;
		}
		m_fUpdateBuffer += timeSlice;
		#else
		if (System.IsConsoleApp())
		{
			if (!m_bRespawnInit && m_bCurrentRespawnEnabled)
				InitializeRespawnTimers();
			if (m_fUpdateBuffer >= 1)
			{
				if (m_bCurrentWaveRespawn)
					WaveRespawnTimer();
				m_fUpdateBuffer = 0;
			}
			m_fUpdateBuffer += timeSlice;
			return;
		}
		#endif
		if (m_fRespawnTimer > 0 || m_bNeedsRespawn)
			RespawnTimer(timeSlice);
	}
	
	//------------------------------------------------------------------------------------------------
	void WaveRespawnTimer()
	{
		// Client-side: Just update local timer display
		if (!Replication.IsServer())
		{
			// Timer value already updated via replication
			// Update local display time if needed
			if (m_iRespawnWaveCurrentTime == 0)
			{
				m_iLocalTimeToRespawn = m_iCurrentTimeToRespawn;
			}
			return;
		}
		
		// Server-side: Update timer and trigger replication
		if (m_Gamemode.m_GamemodeState != COA_EGamemodeState.GAME)
			return;

		m_iRespawnWaveCurrentTime--;
		
		if (m_iRespawnWaveCurrentTime == 0)
		{
			m_iRespawnWaveCurrentTime = m_iCurrentTimeToRespawn;
			m_iLocalTimeToRespawn = m_iCurrentTimeToRespawn;
		}
		
		if (!m_bSuppressReplication)
			Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	void RespawnTimer(float timeSlice)
	{
		string factionKey = m_SlottingManager.GetPlayerSlotFaction(SCR_PlayerController.GetLocalPlayerId()).GetFactionKey();
		int tickets = GetFactionTickets(factionKey);
		if (!m_bCurrentRespawnEnabled || GetFactionSpawnpoints(factionKey).IsEmpty())
		{
			GetGame().GetMenuManager().CloseAllMenus();
			m_fRespawnTimer = 0;
			m_SelectedSpawnPoint = null;
			m_RespawnConfirmed = false; 
			m_bNeedsRespawn = false;
			
			COA_PlayerRplToAuthorityManager.GetInstance().RequestInitilizePlayer(SCR_PlayerController.GetLocalPlayerId());
			return;
		}
		
		// Check if player is actually dead/needs respawn
		bool playerIsDead = m_SlottingManager.IsPlayerConsideredDead(SCR_PlayerController.GetLocalPlayerId());
		if (!playerIsDead)
		{
			// Player is alive, reset respawn state and allow slotting menu
			m_bNeedsRespawn = false;
			m_fRespawnTimer = 0;
			m_SelectedSpawnPoint = null;
			m_RespawnConfirmed = false;
			return;
		}
		
		if (!m_bNeedsRespawn)
			m_bNeedsRespawn = true;
		// Decrease the respawn timer
		if (m_fRespawnTimer > 0)
			m_fRespawnTimer -= timeSlice;
		//Handles adding more time to the players UI if more time is added.
		if (m_iLocalTimeToRespawn != m_iCurrentTimeToRespawn)
		{
			int timeToAdd = m_iCurrentTimeToRespawn - m_iLocalTimeToRespawn;
			m_fRespawnTimer += (float)timeToAdd;
			m_iLocalTimeToRespawn = m_iCurrentTimeToRespawn;
		}
		CloseSlottingMenu();
		// Check if timer expired or we're in AAR
		bool isTimerExpired = m_fRespawnTimer <= 0;
		bool isGameInAARState = (m_Gamemode.m_GamemodeState == COA_EGamemodeState.AAR);
		
		if (isTimerExpired || isGameInAARState)
		{
			// Check if Respawn Screen is open
			MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
			if (!topMenu)
				return;
			if (topMenu.IsInherited(COA_RespawnMenu))
			{
				// Check if respawn selection was confirmed in the UI
				COA_RespawnMenu respawnMenuUI = COA_RespawnMenu.Cast(topMenu);
				if (m_SelectedSpawnPoint != null && m_RespawnConfirmed)
				{
					// Reset the timer
					m_fRespawnTimer = m_iRespawnWaveCurrentTime;
					m_iLocalTimeToRespawn = m_iCurrentTimeToRespawn;
					m_bNeedsRespawn = false;
					// Only perform respawn if not in AAR state
					if (!isGameInAARState)
					{
						GetGame().GetMenuManager().CloseAllMenus();
						COA_PlayerRplToAuthorityManager.GetInstance().RespawnPlayer(SCR_PlayerController.GetLocalPlayerId(), m_SelectedSpawnPoint.GetSpawnPointId());
						
						// Set menu state back to default
						m_SelectedSpawnPoint = null;
						m_RespawnConfirmed = false; 
					}
	
					return;
				}
			}
		}

		// Handle respawn menu
		ShowRespawnMenuIfNeeded();
	}
	
	//------------------------------------------------------------------------------------------------
	private void ShowRespawnMenuIfNeeded()
	{
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		
		if (!topMenu)
			return;
			
		// If we're in spectator but not in respawn menu, open respawn menu
		if (!topMenu.IsInherited(COA_RespawnMenu) && topMenu.IsInherited(COA_SpectatorMenu))
		{
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.COA_RespawnMenu);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void CloseSlottingMenu()
	{
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu && topMenu.IsInherited(COA_SlottingMenu))
		{
			GetGame().GetMenuManager().CloseMenu(topMenu);
		}
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RESPAWN POINT METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	COA_SpawnPointData GetSpawnPoint(int spawnPointId)
	{
		return m_mSpawnPointMap.Get(spawnPointId);
	}
	
	//------------------------------------------------------------------------------------------------
	void RegisterRespawnPoint(COA_SpawnPointData spawnPointData, GenericEntity spawnPointEntity)
	{
		if (!spawnPointEntity || !spawnPointData)
			return;

		RplComponent rplComp = RplComponent.Cast(spawnPointEntity.FindComponent(RplComponent));
		if (!rplComp)
			return;
	
		// Retry until RplId is valid
		if (rplComp.Id() == RplId.Invalid())
		{
			GetGame().GetCallqueue().CallLater(RegisterRespawnPoint, 100, false, spawnPointData, spawnPointEntity);
			return;
		};
		
		m_iLatestSpawnPointID++;
		
		spawnPointData.SetSpawnPointEntity(rplComp.Id());
		spawnPointData.SetSpawnPointId(m_iLatestSpawnPointID);
		
		m_mSpawnPointMap.Set(m_iLatestSpawnPointID, spawnPointData);
		m_RplBroadcastManager.UpdateSpawnPointData(spawnPointData);
		SetSpawnId(spawnPointEntity, m_iLatestSpawnPointID);
		
		m_Gamemode.UpdateGenericSpawn();
	}

	//------------------------------------------------------------------------------------------------
	void UnRegisterRespawnPoint(int spawnPointId)
	{	
		if (spawnPointId <= 0)
			return;
		
		COA_SpawnPointData spawnPointData = GetSpawnPoint(spawnPointId);
		if (!spawnPointData)
			return;

		if (spawnPointData.GetIsTempSpawnPoint())
			SCR_EntityHelper.DeleteEntityAndChildren(COA_EntityHelper.GetEntityFromRplId(spawnPointData.GetSpawnPointEntity()));
		
		m_mSpawnPointMap.Remove(spawnPointId);
		m_RplBroadcastManager.RemoveSpawnPoint(spawnPointId);
	}
	
	//------------------------------------------------------------------------------------------------
	array<COA_SpawnPointData> GetFactionSpawnpoints(FactionKey factionKey, SCR_AIGroup group = null)
	{
		array<COA_SpawnPointData> sideSpawnPoints = {};

		foreach(int spawnPointId, COA_SpawnPointData spawnPointData : m_mSpawnPointMap)
		{
			if (!spawnPointData 
				|| !spawnPointData.GetIsActiveSpawnPoint()  
				|| spawnPointData.GetIsTempSpawnPoint() 
				|| spawnPointData.GetSpawnPointEntity() == RplId.Invalid()
				|| SCR_Enum.GetEnumName(COA_EFactions, spawnPointData.GetSpawnPointFaction()) != factionKey)
				continue;
			
			// Filter out group specific spawns
			if (group && (spawnPointData.GetRestrictedToGroup() != "" && spawnPointData.GetRestrictedToGroup() != group.GetCustomNameWithOriginal()))
					continue;

			if (IsDefaultSpawn(spawnPointData))
				sideSpawnPoints.InsertAt(spawnPointData, 0);
			else
				sideSpawnPoints.Insert(spawnPointData);
		}

		return sideSpawnPoints;
	}
	
	//------------------------------------------------------------------------------------------------
	COA_SpawnPointData FindInitalFactionSpawnpoint(FactionKey factionKey, SCR_AIGroup group = null)
	{	
		if (group)
		{
			string company, platoon, squad, character, format
			group.GetCallsigns(company, platoon, squad, character, format);
	
			foreach(int spawnPointId, COA_SpawnPointData spawnPointData : m_mSpawnPointMap)
			{
				if (!spawnPointData 
					|| !spawnPointData.GetIsActiveSpawnPoint() 
					|| !spawnPointData.GetIsTempSpawnPoint()
					|| spawnPointData.GetSpawnPointEntity() == RplId.Invalid()
					|| SCR_Enum.GetEnumName(COA_EFactions, spawnPointData.GetSpawnPointFaction()) != factionKey)
					continue;
	
				if (COA_GroupSpawnPoint.Cast(COA_EntityHelper.GetEntityFromRplId(spawnPointData.GetSpawnPointEntity())).m_sCallsignOfGroupToSpawn == squad)
					return spawnPointData;
			}
		};
		
		array<COA_SpawnPointData> factionSpawnDataArray = GetFactionSpawnpoints(factionKey);

		if (factionSpawnDataArray.IsEmpty())
			return null;

		if (m_Gamemode.GetRandomizeInitialSpawn(factionKey))
		{
			// Pool candidates are the spawn points flagged as a default spawn; mission makers
			// place multiple to give the faction a randomized set of initial spawn locations.
			array<COA_SpawnPointData> defaultSpawns = {};
			foreach (COA_SpawnPointData spawnPointData : factionSpawnDataArray)
				if (IsDefaultSpawn(spawnPointData))
					defaultSpawns.Insert(spawnPointData);

			if (!defaultSpawns.IsEmpty())
				return defaultSpawns.GetRandomElement();

			// No points flagged as default; fall back to randomizing across all registered points
			return factionSpawnDataArray.GetRandomElement();
		}

		return factionSpawnDataArray.Get(0);
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsDefaultSpawn(COA_SpawnPointData spawnPointData)
	{
		IEntity entity = COA_EntityHelper.GetEntityFromRplId(spawnPointData.GetSpawnPointEntity());
		
		COA_StaticSpawnPoint staticSpawn = COA_StaticSpawnPoint.Cast(entity);
		if (staticSpawn && staticSpawn.m_bIsDefaultSpawn)
			return true;
		
		 COA_MobileSpawnPoint vehSpawn =  COA_MobileSpawnPoint.Cast(entity);
		if (vehSpawn && vehSpawn.m_bIsDefaultSpawn)
			return true;
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSpawnId(GenericEntity spawnPointEntity, int spawnPointId)
	{		
		COA_StaticSpawnPoint staticSpawn = COA_StaticSpawnPoint.Cast(spawnPointEntity);
		if (staticSpawn)
			staticSpawn.SetLocalSpawnPointId(spawnPointId);
		
		 COA_MobileSpawnPoint vehSpawn =  COA_MobileSpawnPoint.Cast(spawnPointEntity);
		if (vehSpawn)
			vehSpawn.SetLocalSpawnPointId(spawnPointId);
		
		COA_RallyPoint rallyPoint = COA_RallyPoint.Cast(spawnPointEntity);
		if (rallyPoint)
			rallyPoint.SetLocalSpawnPointId(spawnPointId);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 RESPAWN METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void RespawnAllSides()
	{
		// Respawn each faction
		RespawnSide("BLUFOR");
		RespawnSide("INDFOR");
		RespawnSide("OPFOR");
		RespawnSide("CIV");
	}

	//------------------------------------------------------------------------------------------------
	void RespawnSide(FactionKey faction)
	{
		array<int> allPlayers = {};
		GetGame().GetPlayerManager().GetAllPlayers(allPlayers);

		// Collect all ticket changes for batching (COA_ERespawnMode.TEAM only)
		int totalTicketsToSubtract = 0;
		bool slotMode = (m_Gamemode.m_eRespawnMode == COA_ERespawnMode.SLOT);

		// Count player respawns first
		foreach (int playerId : allPlayers)
		{
			// Skip alive players or not in a slot
			if (!m_SlottingManager.IsPlayerConsideredDead(playerId) || !m_SlottingManager.IsPlayerInASlot(playerId))
				continue;

			// Get player's faction and verify it matches
			Faction playerFaction = m_SlottingManager.GetPlayerSlotFaction(playerId);
			if (!playerFaction || playerFaction.GetFactionKey() != faction)
				continue;

			if (slotMode)
			{
				// Slot-Based: deduct from this player's own slot/group pool directly
				if (SlotRespawnsRemaining(playerId))
					SubtractSlotRespawn(playerId, 1);
			}
			else if (TicketsRemaining(faction))
			{
				// Team-Based: accumulate into the batched faction ticket subtraction below
				totalTicketsToSubtract += 1;
			}
		}

		// Count vehicle respawns
		int vehicleTicketsToSubtract = 0;
		foreach (COA_VehicleSpawner vehicle: m_aVehicleSpawners)
		{
			if (vehicle.m_sFactionKey != faction)
				continue;
			
			// Check if we have enough tickets to respawn this vehicle
			int factionTickets = GetFactionTickets(faction);
			// Skip if we're out of tickets (but allow if unlimited -1)
			if (factionTickets == 0)
				continue;
			// Skip if we don't have enough tickets (but allow if unlimited -1)
			if (factionTickets > 0 && factionTickets < vehicle.m_iTicketsPerRespawn)
				continue;
			
			bool shouldRespawn = false;
			//Is the vehicle non existant anymore
			if (!vehicle.m_eVehicle && vehicle.m_bShouldRespawnOnSideRespawn)
			{
				shouldRespawn = true;
			}
			else if (vehicle.m_eVehicle && vehicle.m_eVehicle.FindComponent(SCR_VehicleDamageManagerComponent))
			{
				SCR_VehicleDamageManagerComponent vehicleDamageManager = SCR_VehicleDamageManagerComponent.Cast(vehicle.m_eVehicle.FindComponent(SCR_VehicleDamageManagerComponent));
				if (vehicleDamageManager.GetState() == EDamageState.DESTROYED)
					shouldRespawn = true;
			}
			
			if (shouldRespawn && TicketsRemaining(faction))
				vehicleTicketsToSubtract += vehicle.m_iTicketsPerRespawn;
		}
		
		// Apply all ticket changes in one batch if any changes needed
		if (totalTicketsToSubtract > 0 || vehicleTicketsToSubtract > 0)
		{
			m_bSuppressReplication = true;
			if (totalTicketsToSubtract > 0)
				SubtractTicketSilent(faction, totalTicketsToSubtract);
			if (vehicleTicketsToSubtract > 0)
				SubtractTicketSilent(faction, vehicleTicketsToSubtract);
			m_bSuppressReplication = false;
			Replication.BumpMe();
		}
		
		// Now perform the actual respawns (without additional ticket operations)
		foreach (int playerId : allPlayers)
		{
			// Skip alive players or not in a slot
			if (!m_SlottingManager.IsPlayerConsideredDead(playerId) || !m_SlottingManager.IsPlayerInASlot(playerId))
				continue;

			// Get player's faction and verify it matches
			Faction playerFaction = m_SlottingManager.GetPlayerSlotFaction(playerId);
			if (!playerFaction || playerFaction.GetFactionKey() != faction)
				continue;

			RespawnPlayer(playerId);
		}
	}

	//------------------------------------------------------------------------------------------------
	void RespawnPlayer(int playerId, int spawnPointID = -1, RplId entityRplID = RplId.Invalid())
	{
		// Skip on client
		if (RplSession.Mode() == RplMode.Client)
			return;

		// Validate player
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		if (!playerManager.IsPlayerConnected(playerId))
			return;

		// Get player faction
		FactionKey factionKey = "";
		Faction playerFaction = m_SlottingManager.GetPlayerSlotFaction(playerId);
		if (playerFaction)
			factionKey = playerFaction.GetFactionKey();
			
		if (factionKey.IsEmpty())
			return;

		// If no spawn location found, enter spectator mode
		if (GetFactionSpawnpoints(factionKey).IsEmpty())
		{
			m_SlottingManager.UpdateSlotDeathState(m_SlottingManager.GetPlayerSlotID(playerId), true);
			m_GamemodeManager.InitilizePlayer(playerId);
			return;
		}
		
		// Respawn the player
		int slotID = m_SlottingManager.GetPlayerSlotID(playerId);
		m_SlottingManager.UpdateSlotDeathState(slotID, false);
		m_GamemodeManager.InitilizePlayer(playerId, spawnPointID, entityRplID);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 GETTERS/MISC
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Getter for the current wave timer
	int GetCurrentWaveTimer()
	{
		return m_iRespawnWaveCurrentTime;
	}

	//------------------------------------------------------------------------------------------------
	int InsertVehicle(COA_VehicleSpawner spawner)
	{
		return m_aVehicleSpawners.Insert(spawner);
	}
	
	//------------------------------------------------------------------------------------------------
	array<COA_VehicleSpawner> GetVehicleSpawners()
	{
		return m_aVehicleSpawners;
	}
	
	//------------------------------------------------------------------------------------------------
	void ToggleRespawnWave()
	{
		m_bCurrentWaveRespawn = !m_bCurrentWaveRespawn;
		m_iRespawnWaveCurrentTime = m_iCurrentTimeToRespawn;
		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	void SetRespawnTime(int seconds)
	{
		m_iCurrentTimeToRespawn = seconds;
		m_iRespawnWaveCurrentTime = seconds;
		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	void ToggleRespawn()
	{
		m_bCurrentRespawnEnabled = !m_bCurrentRespawnEnabled;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	//! Explicitly enable or disable voluntary player respawn.
	//! Unlike ToggleRespawn, this is idempotent — calling SetRespawnEnabled(false) twice
	//! does not accidentally re-enable respawn. Used by PropHunt to suppress mid-round
	//! respawn screens while keeping the forced RespawnAllSides() round-reset path intact.
	void SetRespawnEnabled(bool enabled)
	{
		if (m_bCurrentRespawnEnabled == enabled)
			return;
		m_bCurrentRespawnEnabled = enabled;
		Replication.BumpMe();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 CLIENT SIDE REPLICATION METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Client-side: Update single spawn point from RPC (called by COA_RplBroadcastManager)
	//! Only updates if data actually changed (prevents unnecessary UI rebuilds)
	void UpdateSpawnPointDataClient(COA_SpawnPointData spawnPointData)
	{
		if (Replication.IsServer())
			return;  // Server doesn't receive these, only sends
		
		int spawnPointId = spawnPointData.GetSpawnPointId();
		COA_SpawnPointData oldSpawnPointData = m_mSpawnPointMap.Get(spawnPointId);

		if(!oldSpawnPointData)
			m_mSpawnPointMap.Set(spawnPointId, spawnPointData);
		else
			oldSpawnPointData.DataUpdate(spawnPointData);
		
		ForceSpawnPointsUpdated();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Client-side: Remove spawn point from RPC (called by COA_RplBroadcastManager)
	void RemoveSpawnPointClient(int spawnPointId)
	{
		if (Replication.IsServer())
			return;  // Server doesn't receive these, only sends
		
		COA_SpawnPointData spawnPointData = GetSpawnPoint(spawnPointId);
		spawnPointData.SetSpawnPointActive(false);
		
		m_mSpawnPointMap.Remove(spawnPointId);
		
		ForceSpawnPointsUpdated();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 REPLICATION
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	override protected bool RplSave(ScriptBitWriter writer)
	{
		// Save spawnPointData
		int spawnPointCount = m_mSpawnPointMap.Count();
		writer.WriteInt(spawnPointCount);
		foreach (int spawnPointId, COA_SpawnPointData spawnPointData : m_mSpawnPointMap)
		{
			spawnPointData.Save(writer);
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override protected bool RplLoad(ScriptBitReader reader)
	{
		// Load spawnPointData
		int spawnPointCount;
		reader.ReadInt(spawnPointCount);
		for (int i = 0; i < spawnPointCount; i++)
		{
			COA_SpawnPointData spawnPointData = new COA_SpawnPointData();
			spawnPointData.Load(reader);
			UpdateSpawnPointDataClient(spawnPointData);
		}

		return true;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static COA_RespawnManager m_sInstance;
	void COA_RespawnManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_RespawnManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static COA_RespawnManager GetInstance()
	{
		return m_sInstance;
	}
}
