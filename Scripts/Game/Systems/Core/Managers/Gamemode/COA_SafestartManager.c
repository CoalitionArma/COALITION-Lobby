class COA_SafestartManagerClass : ScriptComponentClass {}

class COA_SafestartManager : ScriptComponent
{

//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================

	[RplProp(onRplName: "OnSafeStartChange")]
	protected bool m_bSafeStartEnabled = false;
	ref ScriptInvoker m_OnSafeStartChange = new ScriptInvoker();

	[RplProp()]
	protected ref array<string> m_aFactionsStatusArray;

	//! Who readied each side, same order as m_aFactionsStatusArray (BLUFOR, OPFOR, INDFOR, CIV); "" while not ready
	[RplProp()]
	protected ref array<string> m_aReadyBy = {"", "", "", ""};
	protected static const ref array<string> READY_FACTION_KEYS = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
	protected ref array<SCR_Faction> m_aPlayedFactionsArray = {};

	[RplProp()]
	protected bool m_bKillRedundantUnitsBool;

	[RplProp()]
	int m_iSafeStartTimeRemaining;
	
	[RplProp()]
	protected bool m_bCountdownMode = false; // True if using time limit countdown instead of ready-up countdown
	
	[RplProp()]
	protected bool m_bGoingLive = false; // Every side is ready and the final go-live countdown is running
	
	//! Length of the go-live countdown once every side is ready (both modes)
	static const int GO_LIVE_SECONDS = 30;
	
	protected int m_iStoredTimeBeforeReadyUp = 0; // Stores the time remaining before all sides readied up

	protected bool m_bBluforReady = false;
	protected bool m_bOpforReady = false;
	protected bool m_bIndforReady = false;
	protected bool m_bCivReady = false;

	protected bool m_bAdminForcedReady = false;

	protected int m_iPlayedFactionsCount;
	protected ref map<IEntity, bool> m_mEntitiesWithEHsMap = new map<IEntity, bool>();
	protected ref array<IEntity> m_aSafestartZones = {};

	protected SCR_PopUpNotification m_PopUpNotification = null;
	
	protected COA_Gamemode m_Gamemode;
	protected COA_GameTimerManager m_GameTimerManager;
	protected COA_SlottingManager m_SlottingManager;
	protected COA_RplBroadcastManager m_RplBroadcastManager;
	
	protected bool m_bInitComplete = false;
	protected bool m_bUpdatedServerWorldTime = false;
	protected bool m_bUpdatePlayedFactions = false;
	protected bool m_bActivateSafeStartEHs = false;
	protected bool m_bUpdateMissionEndTimer = false;

//=============================================================================================================================================================================================================================================================================================================================================================
//	 MANAGER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Only initialize in actual gameplay
		if (!GetGame().InPlayMode())
			return;
		
		// Get all instances we need for this manager.
		m_Gamemode = COA_Gamemode.GetInstance();
		m_GameTimerManager = COA_GameTimerManager.GetInstance();
		m_SlottingManager = COA_SlottingManager.GetInstance();
		m_RplBroadcastManager = COA_RplBroadcastManager.GetInstance();

		if (RplSession.Mode() != RplMode.Client) // Supports both workbench and dedi
			SetEventMask(GetGame().GetGameMode(), EntityEvent.FIXEDFRAME);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 GETTERS/MISC METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	bool GetSafestartStatus()
	{
		return m_bSafeStartEnabled;
	};
	
	//------------------------------------------------------------------------------------------------
	bool GetCountdownMode()
	{
		return m_bCountdownMode;
	}
	
	//------------------------------------------------------------------------------------------------
	//! True while every side is ready and the go-live countdown runs (GetSafeStartTimeRemaining counts it down)
	bool GetGoingLive()
	{
		return m_bGoingLive;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetSafeStartTimeRemaining()
	{
		return m_iSafeStartTimeRemaining;
	}
	
	//------------------------------------------------------------------------------------------------
	string GetFormattedSafeStartTimeRemaining()
	{
		if (m_iSafeStartTimeRemaining <= 0)
			return "00:00";
			
		int minutes = m_iSafeStartTimeRemaining / 60;
		int seconds = m_iSafeStartTimeRemaining % 60;
		
		return string.Format("%1:%2", minutes.ToString(2), seconds.ToString(2));
	}

	//------------------------------------------------------------------------------------------------
	void OnSafeStartChange()
	{
		m_OnSafeStartChange.Invoke(m_bSafeStartEnabled);
	};
	
	//------------------------------------------------------------------------------------------------
	TStringArray GetWhosReady() {
		return m_aFactionsStatusArray;
	}

	//------------------------------------------------------------------------------------------------
	//! Who readied each side (same order as GetWhosReady); "" while that side is not ready
	TStringArray GetReadyBy()
	{
		return m_aReadyBy;
	}

	//------------------------------------------------------------------------------------------------
	protected void SetReadyBy(FactionKey factionKey, string playerName)
	{
		int index = READY_FACTION_KEYS.Find(factionKey);
		if (index != -1 && m_aReadyBy.IsIndexValid(index))
			m_aReadyBy[index] = playerName;
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearReadyBy(string playerName = "")
	{
		for (int i = 0; i < m_aReadyBy.Count(); i++)
			m_aReadyBy[i] = playerName;
	}

	//------------------------------------------------------------------------------------------------
	void AddSafestartZone(IEntity entity)
	{
		m_aSafestartZones.Insert(entity);
	}
	
	//------------------------------------------------------------------------------------------------
	void DeleteAllSafestartZones()
	{
		foreach(IEntity zone : m_aSafestartZones)
		{
			if(zone)
				SCR_EntityHelper.DeleteEntityAndChildren(zone);
		}
		
		m_aSafestartZones.Clear()
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 FRAME HANDLER
//=============================================================================================================================================================================================================================================================================================================================================================
	
	float m_fUpdateBuffer = 0;
	float m_fLongUpdateBuffer = 0;
	//------------------------------------------------------------------------------------------------
	override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		super.EOnFixedFrame(owner, timeSlice);
		
		if (m_fUpdateBuffer >= 1)
		{
			if (!m_bInitComplete)
				if (m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
				{
					m_bSafeStartEnabled = !m_Gamemode.m_bSafestartEnabledOnMissionStart;
					Replication.BumpMe();//Broadcast m_bSafeStartEnabled change
			
					GetGame().GetCallqueue().CallLater(ToggleSafeStartServer, 1000, false, m_Gamemode.m_bSafestartEnabledOnMissionStart);
					m_bInitComplete = true;
				}
			
			if (m_bUpdatedServerWorldTime)
				m_GameTimerManager.UpdateServerWorldTime();
			
			if (m_bUpdateMissionEndTimer)
				m_GameTimerManager.UpdateMissionEndTimer();
			
			// Time limit countdown, or the go-live countdown once every side is ready - both every second
			if (m_bSafeStartEnabled)
			{
				if (m_bCountdownMode)
					CheckCountdownMode();
				else
					TickReadyCountdown();
			}
			
			m_fUpdateBuffer = 0;
		}
		m_fUpdateBuffer += timeSlice;
		
		if (m_fLongUpdateBuffer >= 10)
		{
			if (m_bActivateSafeStartEHs)
			{
				SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
				ActivateSafeStartEHs(aiWorld);
			}
			
			if (m_bUpdatePlayedFactions)
				UpdatePlayedFactions();
			
			m_fLongUpdateBuffer = 0;	
		}
		m_fLongUpdateBuffer += timeSlice;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 ONFRAME METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected void UpdatePlayedFactions()
	{
		// Get faction manager and retrieve all factions
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return;

		// Get sorted factions
		SCR_SortedArray<SCR_Faction> sortedFactions = new SCR_SortedArray<SCR_Faction>();
		factionManager.GetSortedFactionsList(sortedFactions);

		if (!sortedFactions || sortedFactions.IsEmpty())
			return;

		// Convert to regular array for iteration
		array<SCR_Faction> factionArray = {};
		sortedFactions.ToArray(factionArray);

		// Reset faction tracking
		m_aPlayedFactionsArray.Clear();

		// Initialize default faction status strings
		string bluforStatus = "N/A";
		string opforStatus = "N/A";
		string indforStatus = "N/A";
		string civStatus = "N/A";

		// Process each faction
		foreach (SCR_Faction faction : factionArray)
		{
			string factionKey = faction.GetFactionKey();
			
			// Only process supported faction keys
			if (factionKey != "BLUFOR" && factionKey != "OPFOR" && 
			    factionKey != "INDFOR" && factionKey != "CIV")
				continue;
			
			// Check if faction has SLOTTED players (not just spawned ones)
			// This fixes the issue where factions with slotted but unspawned players weren't showing
			int slottedPlayerCount = m_SlottingManager.GetSlottedPlayerCountByFaction(factionKey);
			
			if (slottedPlayerCount == 0)
				continue;

			// Add to active factions list
			m_aPlayedFactionsArray.Insert(faction);

			// Set appropriate status string based on faction and ready state
			if (factionKey == "BLUFOR") {
				if (m_bBluforReady) {
					bluforStatus = "Ready";
				} else {
					bluforStatus = "Not Ready";
				}
			} else if (factionKey == "OPFOR") {
				if (m_bOpforReady) {
					opforStatus = "Ready";
				} else {
					opforStatus = "Not Ready";
				}
			} else if (factionKey == "INDFOR") {
				if (m_bIndforReady) {
					indforStatus = "Ready";
				} else {
					indforStatus = "Not Ready";
				}
			} else if (factionKey == "CIV") {
				if (m_bCivReady) {
					civStatus = "Ready";
				} else {
					civStatus = "Not Ready";
				}
			}
		}

		// Update faction status array
		m_aFactionsStatusArray = {bluforStatus, opforStatus, indforStatus, civStatus};

		// Count active factions
		m_iPlayedFactionsCount = 0;
		foreach (string factionStatus : m_aFactionsStatusArray)
		{
			if (factionStatus != "N/A")
				m_iPlayedFactionsCount++;
		}

		// Notify clients of changes
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	//! Ready-up mode, called every second: once every playing side is ready a GO_LIVE_SECONDS countdown
	//! starts; if any side un-readies it is cancelled and resets (previously it only paused, since the
	//! check only ran while everyone was ready, and it moved in 5 second steps clients never saw)
	protected void TickReadyCountdown()
	{
		string submessage = "Any leader can toggle ready to cancel the countdown";
		
		if (!AllSidesReady())
		{
			if (!m_bGoingLive)
				return;
			
			m_bGoingLive = false;
			m_iSafeStartTimeRemaining = GO_LIVE_SECONDS;
			Replication.BumpMe();
			m_RplBroadcastManager.PopUpNotification(4, "[LOBBY] : Game Live Countdown Canceled!", "A side is no longer ready");
			return;
		}
		
		if (!m_bGoingLive)
		{
			m_bGoingLive = true;
			m_iSafeStartTimeRemaining = GO_LIVE_SECONDS;
			Replication.BumpMe();
			m_RplBroadcastManager.PopUpNotification(4, string.Format("[LOBBY] : All Sides Ready! Game Live In: %1 Seconds!", GO_LIVE_SECONDS), submessage);
			return;
		}
		
		if (m_iSafeStartTimeRemaining > 0)
			m_iSafeStartTimeRemaining--;
		Replication.BumpMe();
		
		if (m_iSafeStartTimeRemaining == 0)
		{
			m_bGoingLive = false;
			ToggleSafeStartServer(false);
			m_RplBroadcastManager.PopUpNotification(8, "[LOBBY] : GAME LIVE!", "Safestart has ended, weapons are now live!");
			return;
		}
		
		// The HUD shows every second; popups only at a few marks
		if (m_iSafeStartTimeRemaining == 20 || m_iSafeStartTimeRemaining == 10 || m_iSafeStartTimeRemaining <= 5)
			m_RplBroadcastManager.PopUpNotification(1.5, string.Format("[LOBBY] : Game Live In: %1 Seconds!", m_iSafeStartTimeRemaining), submessage);
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool AllSidesReady()
	{
		int readyFactionsCount = FactionsReadyCount();
		return readyFactionsCount != 0 && m_iPlayedFactionsCount != 0 && readyFactionsCount == m_iPlayedFactionsCount;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void CheckCountdownMode()
	{
		string message;
		string submessage = "";
		float popupLife = 3.25;
		bool showMessage = false; // Only show messages at specific intervals
		
		// Check if all factions are ready - if so, skip to 30 second countdown
		int readyFactionsCount = FactionsReadyCount();
		if (readyFactionsCount != 0 && m_iPlayedFactionsCount != 0 && readyFactionsCount == m_iPlayedFactionsCount)
		{
			// All factions are ready - jump to 30 second countdown if we're above that
			if (m_iSafeStartTimeRemaining > 30)
			{
				// Store the current time before jumping to 30 seconds
				m_iStoredTimeBeforeReadyUp = m_iSafeStartTimeRemaining;
				
				m_iSafeStartTimeRemaining = GO_LIVE_SECONDS;
				m_bGoingLive = true;
				message = "[LOBBY] : All Factions Ready! Game Live In: 30 Seconds!";
				submessage = "Any leader can toggle ready to cancel the countdown";
				popupLife = 5;
				showMessage = true;
				Replication.BumpMe();
				
				// Send notification and continue countdown
				if (showMessage)
					m_RplBroadcastManager.PopUpNotification(popupLife, message, submessage);
				return;
			}
		}
		else if (m_iSafeStartTimeRemaining <= 30 && m_iStoredTimeBeforeReadyUp > 0)
		{
			// A faction unreadied during the 30 second countdown - restore the original time
			message = "[LOBBY] : Game Live Countdown Canceled!";
			submessage = "A faction has unreadied";
			popupLife = 5;
			showMessage = true;
			
			// Restore to the time we had before all sides readied up
			m_iSafeStartTimeRemaining = m_iStoredTimeBeforeReadyUp;
			m_iStoredTimeBeforeReadyUp = 0; // Reset the stored time
			m_bGoingLive = false;
			Replication.BumpMe();
			
			if (showMessage)
				m_RplBroadcastManager.PopUpNotification(popupLife, message, submessage);
			return;
		}
		
		// Countdown from time limit (every second) - only if above zero
		if (m_iSafeStartTimeRemaining > 0)
			m_iSafeStartTimeRemaining -= 1;
		
		// Clamp to zero to prevent negatives
		if (m_iSafeStartTimeRemaining < 0)
			m_iSafeStartTimeRemaining = 0;
		
		// Replicate the updated time to all clients
		Replication.BumpMe();
		
		// Only show popup warnings when 5 minutes or less remain
		if (m_iSafeStartTimeRemaining <= 300)
		{
			// Format time remaining as MM:SS
			int minutesRemaining = m_iSafeStartTimeRemaining / 60;
			int secondsRemaining = m_iSafeStartTimeRemaining % 60;
			string timeString;
			
			if (minutesRemaining > 0)
				timeString = string.Format("%1:%2", minutesRemaining, secondsRemaining.ToString(2)); // 2 digits for seconds
			else
				timeString = string.Format("%1 Seconds", secondsRemaining);
			
			message = string.Format("[LOBBY] : Safestart Ends In: %1", timeString);
			
			// Give warnings at specific intervals (5 minutes or less)
			if (m_iSafeStartTimeRemaining == 300) // 5 minutes
			{
				submessage = "5 minutes remaining";
				popupLife = 5;
				showMessage = true;
			}
			else if (m_iSafeStartTimeRemaining == 240) // 4 minutes
			{
				submessage = "4 minutes remaining";
				popupLife = 4;
				showMessage = true;
			}
			else if (m_iSafeStartTimeRemaining == 180) // 3 minutes
			{
				submessage = "3 minutes remaining";
				popupLife = 4;
				showMessage = true;
			}
			else if (m_iSafeStartTimeRemaining == 120) // 2 minutes
			{
				submessage = "2 minutes remaining";
				popupLife = 5;
				showMessage = true;
			}
			else if (m_iSafeStartTimeRemaining == 60) // 1 minute
			{
				submessage = "1 minute remaining";
				popupLife = 5;
				showMessage = true;
			}
			else if (m_iSafeStartTimeRemaining == 30) // 30 seconds
			{
				submessage = "30 seconds remaining";
				popupLife = 5;
				showMessage = true;
			}
			else if (m_iSafeStartTimeRemaining <= 10 && m_iSafeStartTimeRemaining > 0) // Final 10 seconds
			{
				submessage = "Get ready!";
				popupLife = 1;
				showMessage = true;
			}
		}
		
		// End safe start when countdown reaches zero
		if (m_iSafeStartTimeRemaining <= 0)
		{
			// Force all sides to be ready
			m_bBluforReady = true;
			m_bOpforReady = true;
			m_bIndforReady = true;
			m_bCivReady = true;
			m_bAdminForcedReady = true;
			UpdatePlayedFactions();
			
			ToggleSafeStartServer(false);
			m_bCountdownMode = false;
			m_bGoingLive = false;
			message = "[LOBBY] : GAME LIVE!";
			submessage = "Safestart timer expired, mission is now live!";
			popupLife = 8;
			showMessage = true;
		}
		
		// Only send popup notification if we have something to show
		if (showMessage)
			m_RplBroadcastManager.PopUpNotification(popupLife, message, submessage);
	}
	
	//------------------------------------------------------------------------------------------------
	protected int FactionsReadyCount()
	{
		int readyFactionsCount = 0;
		if (!m_aFactionsStatusArray)
			return 0;
		
		foreach (string factionStatus : m_aFactionsStatusArray)
		{
			if (factionStatus == "Ready")
				readyFactionsCount++;
		}
		
		return readyFactionsCount;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 PRIMARY SAFESTART METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void ToggleSideReady(FactionKey setReady, string playerName, bool adminForced) {
		if (!GetSafestartStatus())
			return;
		
		string message;

		// Handle admin force ready/unready all factions
		if (adminForced) {
			bool newReadyState = !m_bAdminForcedReady;
			m_bAdminForcedReady = newReadyState;

			// Set all factions to the same ready state
			m_bBluforReady = newReadyState;
			m_bOpforReady = newReadyState;
			m_bIndforReady = newReadyState;
			m_bCivReady = newReadyState;

			if (newReadyState)
				ClearReadyBy(string.Format("Admin (%1)", playerName));
			else
				ClearReadyBy();

			string actionText;
			if (newReadyState) {
				actionText = "Force Readied";
			} else {
				actionText = "Force Unreadied";
			}

			message = string.Format("An Admin (%1) Has %2 All Sides!", playerName, actionText);

			Replication.BumpMe();
			m_RplBroadcastManager.PopUpNotification(3.25, message);
			
			UpdatePlayedFactions();
			return;
		}

		// If admin forced ready is active, don't allow individual faction changes
		if (m_bAdminForcedReady)
			return;

		// Toggle faction ready status
		bool newStatus = false;
		string messageKey = "";

		// Update faction status and prepare message
		switch (setReady) {
			case "BLUFOR": {
				m_bBluforReady = !m_bBluforReady;
				newStatus = m_bBluforReady;
				if (newStatus) {
					messageKey = "[LOBBY] : BLUFOR READY";
				} else {
					messageKey = "[LOBBY] : BLUFOR NOT READY";
				}
				break;
			}
			case "OPFOR": {
				m_bOpforReady = !m_bOpforReady;
				newStatus = m_bOpforReady;
				if (newStatus) {
					messageKey = "[LOBBY] : OPFOR READY";
				} else {
					messageKey = "[LOBBY] : OPFOR NOT READY";
				}
				break;
			}
			case "INDFOR": {
				m_bIndforReady = !m_bIndforReady;
				newStatus = m_bIndforReady;
				if (newStatus) {
					messageKey = "[LOBBY] : INDFOR READY";
				} else {
					messageKey = "[LOBBY] : INDFOR NOT READY";
				}
				break;
			}
			case "CIV": {
				m_bCivReady = !m_bCivReady;
				newStatus = m_bCivReady;
				if (newStatus) {
					messageKey = "[LOBBY] : CIV READY";
				} else {
					messageKey = "[LOBBY] : CIV NOT READY";
				}
				break;
			}
		}

		if (newStatus)
			SetReadyBy(setReady, playerName);
		else
			SetReadyBy(setReady, "");

		message = string.Format("%1 - %2", messageKey, playerName);
		
		m_RplBroadcastManager.PopUpNotification(3.25, message, "Any leader can toggle ready to cancel the countdown");
		
		UpdatePlayedFactions();
	};

	//------------------------------------------------------------------------------------------------
	protected void ToggleSafeStartServer(bool status)
	{
		if (status)
		{ // Turn on safestart
			if (m_bSafeStartEnabled)
				return;
			
			//if (GetEventMask() != EntityEvent.FIXEDFRAME)
				//SetEventMask(GetGame().GetGameMode(), EntityEvent.FIXEDFRAME);

			COA_GameTimerManager.GetInstance().m_iTimeSafeStartBegan = GetGame().GetWorld().GetWorldTime();
			m_bSafeStartEnabled = true;
			
			// Check if using countdown mode (boolean enabled AND time limit > 0)
			if (m_Gamemode.m_bUseSafestartTimeLimit && m_Gamemode.m_iSafestartTimeLimit > 0)
			{
				m_bCountdownMode = true;
				m_iSafeStartTimeRemaining = m_Gamemode.m_iSafestartTimeLimit * 60; // Convert minutes to seconds
			}
			else
			{
				m_bCountdownMode = false;
				m_iSafeStartTimeRemaining = GO_LIVE_SECONDS;
			}
			m_bGoingLive = false;
			m_iStoredTimeBeforeReadyUp = 0;

			m_bUpdateMissionEndTimer = false;
			m_bUpdatedServerWorldTime = true;
			
			SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
			ActivateSafeStartEHs(aiWorld);
			m_bActivateSafeStartEHs = true;
			
			UpdatePlayedFactions();
			m_bUpdatePlayedFactions = true;

			Replication.BumpMe();//Broadcast m_bSafeStartEnabled change

		} else { // Turn off safestart
			if (!m_bSafeStartEnabled)
				return;
			
			UpdatePlayedFactions();

			m_bKillRedundantUnitsBool = true;
			m_bGoingLive = false;
			ClearReadyBy();
			m_bAdminForcedReady = false;
			m_bBluforReady = false;
			m_bOpforReady = false;
			m_bIndforReady = false;
			m_bCivReady = false;
			
			if(m_Gamemode.m_bLockUnusedSlots)
				m_SlottingManager.LockAllOpenSlots();

			m_bUpdatedServerWorldTime = false;
			m_bActivateSafeStartEHs = false;
			m_bUpdatePlayedFactions = false;
			
			COA_Gamemode gm = COA_Gamemode.GetInstance();

			if (m_Gamemode.m_iTimeLimitMinutes > 0) {
				COA_GameTimerManager.GetInstance().m_iTimeMissionEnds = GetGame().GetWorld().GetWorldTime() + (m_Gamemode.m_iTimeLimitMinutes * 60000);
				m_bUpdateMissionEndTimer = true;
			} else {
				m_GameTimerManager.SetServerWorldTime("N/A");
			};

			Replication.BumpMe();//Broadcast change

			DeactivateSafeStartEHs();

			// Use CallLater to delay the call for the removal of EHs so the changes so m_bSafeStartEnabled can propagate.
			GetGame().GetCallqueue().CallLater(DeactivateSafeStartEHs, 1500);
			
			// Delete Temp Group Spawn Points
			GetGame().GetCallqueue().CallLater(COA_RespawnManager.GetInstance().ClearGroupSpawnPoints, 8500);
			
			// Even longer delay just in case there's any edge cases we didnt anticipate.
			GetGame().GetCallqueue().CallLater(DeactivateSafeStartEHs, 12500);

			// Delay the change being broadcasted
			GetGame().GetCallqueue().CallLater(DelayChangeSafeStartDisabled, 250);
			
			DeleteAllSafestartZones();
			
			COA_ForwardDeployManager.GetInstance().DeleteAllForwardDeployZones();
			
			//ClearEventMask(GetGame().GetGameMode(), EntityEvent.FIXEDFRAME);
		}
	};

	//------------------------------------------------------------------------------------------------
	protected void DelayChangeSafeStartDisabled() {
		m_bSafeStartEnabled = false;
		Replication.BumpMe();//Broadcast m_bSafeStartEnabled change
	};

//=============================================================================================================================================================================================================================================================================================================================================================
//	 EVENT HANDLERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Activates safe start event handlers for all AI and player-controlled entities.
	//! Disables damage and weapon functionality during the safe start period.
	protected void ActivateSafeStartEHs(SCR_AIWorld aiWorld)
	{
		// Apply safe start to AI-controlled entities
		if (aiWorld)
		{
			array<AIAgent> aiAgents = {};
			aiWorld.GetAIAgents(aiAgents);

			foreach (AIAgent agent : aiAgents)
			{
				IEntity controlledEntity = agent.GetControlledEntity();
				if (controlledEntity)
					SetSafeStartEHs(controlledEntity);
			}
		}

		// Apply safe start to player-controlled entities
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);

		foreach (int playerId : playerIds)
		{
			IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (controlledEntity)
				SetSafeStartEHs(controlledEntity);
		}
	};

	//------------------------------------------------------------------------------------------------
	//! Deactivates all safe start event handlers and re-enables combat functionality
	//! for all entities that had safe start restrictions applied.
	protected void DeactivateSafeStartEHs()
	{
		foreach (IEntity controlledEntity, bool hasHandlers : m_mEntitiesWithEHsMap)
		{
			if (!controlledEntity)
				continue;

			// Re-enable damage handling
			SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(
				controlledEntity.FindComponent(SCR_CharacterDamageManagerComponent));
			if (damageManager)
				damageManager.EnableDamageHandling(true);

			// Turn off weapon safety
			CharacterControllerComponent charComp = CharacterControllerComponent.Cast(
				controlledEntity.FindComponent(CharacterControllerComponent));
			if (!charComp)
				continue;

			charComp.SetSafety(false, false);

			// Remove weapon event handlers
			EventHandlerManagerComponent eventHandler = EventHandlerManagerComponent.Cast(
				controlledEntity.FindComponent(EventHandlerManagerComponent));
			if (!eventHandler)
				continue;

			eventHandler.RemoveScriptHandler("OnProjectileShot", this, OnWeaponFired);
			eventHandler.RemoveScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);

			m_mEntitiesWithEHsMap.Remove(controlledEntity);
		}
	};

	//------------------------------------------------------------------------------------------------
	protected void SetSafeStartEHs(IEntity controlledEntity)
	{
		SCR_CharacterDamageManagerComponent damageHandler = SCR_CharacterDamageManagerComponent.Cast(controlledEntity.FindComponent(SCR_CharacterDamageManagerComponent));
		if (damageHandler)
			damageHandler.EnableDamageHandling(false);

		EventHandlerManagerComponent eventHandler = EventHandlerManagerComponent.Cast(controlledEntity.FindComponent(EventHandlerManagerComponent));
		CharacterControllerComponent charComp = CharacterControllerComponent.Cast(controlledEntity.FindComponent(CharacterControllerComponent));

		bool alreadyHasEventHandlers = m_mEntitiesWithEHsMap.Get(controlledEntity);

		if (!alreadyHasEventHandlers && charComp && eventHandler) {
			charComp.SetSafety(true, true);
			eventHandler.RegisterScriptHandler("OnProjectileShot", this, OnWeaponFired);
			eventHandler.RegisterScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);
			m_mEntitiesWithEHsMap.Set(controlledEntity, true);
		};
	};

	//------------------------------------------------------------------------------------------------
	protected void OnWeaponFired(int playerId, BaseWeaponComponent weapon, IEntity entity)
	{
		// Get projectile and delete it
		delete entity;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGrenadeThrown(int playerId, BaseWeaponComponent weapon, IEntity entity)
	{
		if (!weapon)
			return;

		// Get grenade and delete it
		delete entity;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static COA_SafestartManager m_sInstance;
	void COA_SafestartManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_SafestartManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static COA_SafestartManager GetInstance()
	{
		return m_sInstance;
	}
}
