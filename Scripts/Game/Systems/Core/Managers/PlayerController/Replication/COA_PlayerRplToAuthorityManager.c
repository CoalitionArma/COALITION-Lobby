//------------------------------------------------------------------------------------------------
// Data structure for batched slot updates to reduce network traffic
// Using individual parameters instead of complex serialization to match Enfusion's simpler RPC pattern
//------------------------------------------------------------------------------------------------

class COA_PlayerRplToAuthorityManagerClass : ScriptComponentClass {}

class COA_PlayerRplToAuthorityManager : ScriptComponent
{	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	// Manager references
	protected COA_Gamemode m_Gamemode;
	protected COA_MenuManager m_MenuManager;
	protected COA_RespawnManager m_RespawnManager;
	protected COA_PermissionManager m_PermissionManager;
	protected COA_SlottingManager m_SlottingManager;
	protected COA_SafestartManager m_SafestartManager;
	protected COA_AdminMenuManager m_AdminMenuManager;
	protected COA_GearscriptManager m_GearscriptManager;
	protected COA_RplBroadcastManager m_RplBroadcastManager;
	protected COA_BandwidthTelemetryManager m_TelemetryManager;
	protected SCR_GroupsManagerComponent m_GroupsManagerComponent;

	// How close (squared metres) a player must be to a rally point to destroy it. Generous compared
	// to the user-action range to allow for movement and latency.
	protected const float RALLYPOINT_INTERACT_DISTANCE_SQ = 100;

//=============================================================================================================================================================================================================================================================================================================================================================
//	 MANAGER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{	
		super.OnPostInit(owner);
		
		if(!Replication.IsServer())
			return;
		
		InitializeManagerReferences();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initializes all manager references needed by this component
	protected void InitializeManagerReferences()
	{
		m_Gamemode = COA_Gamemode.GetInstance();
		m_MenuManager = COA_MenuManager.GetInstance();
		m_RespawnManager = COA_RespawnManager.GetInstance();
		m_PermissionManager = COA_PermissionManager.GetInstance();
		m_SlottingManager = COA_SlottingManager.GetInstance();
		m_SafestartManager = COA_SafestartManager.GetInstance();
		m_AdminMenuManager = COA_AdminMenuManager.GetInstance();
		m_GearscriptManager = COA_GearscriptManager.GetInstance();
		m_RplBroadcastManager = COA_RplBroadcastManager.GetInstance();
		m_TelemetryManager = COA_BandwidthTelemetryManager.GetInstance();
		m_GroupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
	}

	//------------------------------------------------------------------------------------------------
	//! The player who sent the RPC being handled. This component sits on each player's own
	//! controller, so this is server-authoritative, unlike a player ID passed in by the client.
	protected int GetCallerPlayerId()
	{
		PlayerController ownerController = PlayerController.Cast(GetOwner());
		if (!ownerController)
			return 0;

		return ownerController.GetPlayerId();
	}

	//------------------------------------------------------------------------------------------------
	//! Server-side permission check for admin/moderator-only requests. The client-side checks in the
	//! accessors above are only UI convenience; a modified client can call any RPC directly.
	protected bool IsCallerStaff()
	{
		int callerId = GetCallerPlayerId();
		if (callerId <= 0)
			return false;

		if (SCR_Global.IsAdmin(callerId))
			return true;

		return m_PermissionManager && m_PermissionManager.IsModerator(callerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Creator of a custom VON channel, parsed from its name ("Name's Channel (PlayerID)|players...").
	//! \return the creator's player ID, or 0 if the channel is invalid or has no creator
	protected int GetChannelCreatorId(int channel)
	{
		if (!m_MenuManager || channel < 0 || channel >= m_MenuManager.m_aVONChannels.Count())
			return 0;

		array<string> channelSplit = {};
		m_MenuManager.m_aVONChannels[channel].Split("|", channelSplit, true);
		if (channelSplit.IsEmpty())
			return 0;

		string channelName = channelSplit[0];
		int openParen = channelName.IndexOf("(");
		int closeParen = channelName.IndexOf(")");
		if (openParen == -1 || closeParen == -1 || closeParen <= openParen)
			return 0;

		return channelName.Substring(openParen + 1, closeParen - openParen - 1).ToInt();
	}

	//------------------------------------------------------------------------------------------------
	//! Server-side check for admin-only requests (those the client UI gates on SCR_Global.IsAdmin()).
	protected bool IsCallerAdmin()
	{
		int callerId = GetCallerPlayerId();
		return callerId > 0 && SCR_Global.IsAdmin(callerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Server-side check for requests a player may make for themselves, and staff for anyone.
	protected bool IsCallerSelfOrStaff(int playerId)
	{
		int callerId = GetCallerPlayerId();
		if (callerId <= 0)
			return false;

		return playerId == callerId || IsCallerStaff();
	}

	//------------------------------------------------------------------------------------------------
	//! Tell the requesting player their forward deploy was rejected. Uses the owner manager on this
	//! same controller - COA_PlayerRplToOwnerManager.GetInstance() is a different, arbitrary player
	//! on the server.
	protected void RejectForwardDeploy()
	{
		COA_PlayerRplToOwnerManager ownerManager = COA_PlayerRplToOwnerManager.Cast(GetOwner().FindComponent(COA_PlayerRplToOwnerManager));
		if (ownerManager)
			ownerManager.ForwardDeployRequestRejected();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 TELEMETRY
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Log RPC call to telemetry system (server-side only)
	protected void LogTelemetry(string rpcName, int estimatedBytes)
	{
		if (!Replication.IsServer())
			return;
			
		if (!m_TelemetryManager)
			m_TelemetryManager = COA_BandwidthTelemetryManager.GetInstance();
			
		if (m_TelemetryManager)
			m_TelemetryManager.LogRPC(rpcName, estimatedBytes);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 CLIENT/RPC METHODS (lobby-scoped: player init, gamemode/slotting phase, slot mutation, spawn, spectator cam, slot lottery, gear apply/hot-swap, VON channel join)
//=============================================================================================================================================================================================================================================================================================================================================================

	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 CLIENT REPLICATION ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! \param[in] isMassJoinBatch Forwarded to COA_Gamemode.QueuePlayerInitialization - leave true for
	//! connect/round-start callers so they still get smoothed by the staggered batch queue; pass false
	//! only for a standalone request (e.g. a single mid-session slotting-menu confirm) that should run
	//! immediately instead of waiting behind that queue.
	void RequestInitilizePlayer(int playerId, bool isMassJoinBatch = true)
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.COA_CharacterLoading);
		Rpc(RpcAsk_RequestInitilizePlayer, playerId, isMassJoinBatch);
	}

	//------------------------------------------------------------------------------------------------
	void ToggleSideReady(string setReady, string playerName, bool adminForced)
	{
		Rpc(RpcAsk_ToggleSideReady, setReady, playerName, adminForced); 
	}
	
	//------------------------------------------------------------------------------------------------
	void RequestAdvanceGamemodeState(bool overriden, string winningFaction = "")
	{
		if (SCR_Global.IsAdmin())
			Rpc(RpcAsk_RequestAdvanceGamemodeState, overriden, winningFaction);
	}

	//------------------------------------------------------------------------------------------------
	void RequestAdvanceSlottingPhase()
	{
		if (SCR_Global.IsAdmin())
			Rpc(RpcAsk_RequestAdvanceSlottingPhase); 
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSlotPlayerID(int slotId, int playerId)
	{
		Rpc(RpcAsk_UpdateSlotPlayerID, slotId, playerId);
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSlotLockedState(int slotId, bool input)
	{
		// Direct manager call if BatchUpdateSlot unavailable
		Rpc(RpcAsk_UpdateSlotLockedState, slotId, input);
	}

	//------------------------------------------------------------------------------------------------
	void UpdateGroupLockedState(RplId groupRplId, bool input)
	{
		// Group locking is not part of slot batching, use direct RPC
		Rpc(RpcAsk_UpdateGroupLockedState, groupRplId, input); 
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSlotDeathState(int slotId, bool input)
	{
		// Direct manager call if BatchUpdateSlot unavailable
		Rpc(RpcAsk_UpdateSlotDeathState, slotId, input);
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSlotRole(int slotId, COA_EGearRole role)
	{
		// Direct manager call if BatchUpdateSlot unavailable
		Rpc(RpcAsk_UpdateSlotRole, slotId, role);
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSlotGroup(int slotId, RplId groupRplId)
	{
		// Direct manager call if BatchUpdateSlot unavailable
		Rpc(RpcAsk_UpdateSlotGroup, slotId, groupRplId);
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSlotCharacter(int slotId, RplId charId)
	{
		// Direct manager call if BatchUpdateSlot unavailable
		Rpc(RpcAsk_UpdateSlotCharacter, slotId, charId);
	}

	//------------------------------------------------------------------------------------------------
	void RespawnPlayer(int playerId, int spawnPointID)
	{
		Rpc(RpcAsk_RespawnPlayer, playerId, spawnPointID); 
	}	

	//------------------------------------------------------------------------------------------------
	void RequestToJoinChannel(int channel, int requestId)
	{
		Rpc(RpcAsk_RequestToJoinChannel, channel, requestId); 
	}

	//------------------------------------------------------------------------------------------------
	void CheckVONRegister(int playerId)
	{
		Rpc(RpcAsk_CheckVONRegister, playerId); 
	}

	//------------------------------------------------------------------------------------------------
	void CreateChannel(int playerId)
	{
		Rpc(RpcAsk_CreateChannel, playerId); 
	}

	//------------------------------------------------------------------------------------------------
	void JoinChannel(int playerId, int channel)
	{
		Rpc(RpcAsk_JoinChannel, playerId, channel); 
	}

	//------------------------------------------------------------------------------------------------
	void SpawnOnGroup(int playerId, int playerIDToSpawnOn, int groupID, bool logAction)
	{
		Rpc(RpcAsk_SpawnOnGroup, playerId, playerIDToSpawnOn, groupID, logAction); 
	}

	//------------------------------------------------------------------------------------------------
	void ResetGear(int playerId, ResourceName prefab, bool logAction)
	{
		Rpc(RpcAsk_ResetGear, playerId, prefab, logAction); 
	}

	//------------------------------------------------------------------------------------------------
	void UpdateGearSet(string faction, ResourceName path)
	{
		Rpc(RpcAsk_UpdateGearSet, faction, path); 
	}

	//------------------------------------------------------------------------------------------------
	void MoveSpecCamToSlot(int slotID, int playerID)
	{
		Rpc(RpcAsk_MoveSpecCamToSlot, slotID, playerID);
	}

	//------------------------------------------------------------------------------------------------
	void SendAdminMessage(string data, int playerID)
	{
		Rpc(RpcAsk_SendAdminMessage, data, playerID); 
	}

	//------------------------------------------------------------------------------------------------
	void ReplyAdminMessage(string data, int playerId, int adminID, bool logAction)
	{
		if (SCR_Global.IsAdmin() || m_PermissionManager.IsModerator())
			Rpc(RpcAsk_ReplyAdminMessage, data, playerId, adminID, logAction); 
	}

	//------------------------------------------------------------------------------------------------
	void CloseAdminTicket(int ticketID, int adminID, bool logAction)
	{
		Rpc(RpcAsk_CloseAdminTicket, ticketID, adminID, logAction); 
	}

	//------------------------------------------------------------------------------------------------
	void AssignAdminTicket(int ticketID, int adminID, bool logAction)
	{
		Rpc(RpcAsk_AssignAdminTicket, ticketID, adminID, logAction); 
	}

	//------------------------------------------------------------------------------------------------
	void GetOpenTickets(int playerID)
	{
		Rpc(RpcAsk_GetOpenTickets, playerID); 
	}

	//------------------------------------------------------------------------------------------------
	void GetTicketMessages(int playerID, int ticketID)
	{
		Rpc(RpcAsk_GetTicketMessages, playerID, ticketID); 
	}
	
	//------------------------------------------------------------------------------------------------
	void TeleportPlayers(int playerId1, int playerId2, bool logAction)
	{
		Rpc(RpcAsk_TeleportPlayers, playerId1, playerId2, logAction); 
	}

	//------------------------------------------------------------------------------------------------
	void SendHint(string data, int playerId = -1, string factionKey = "")
	{
		Rpc(RpcAsk_SendHint, data, playerId, factionKey);
	}

	//------------------------------------------------------------------------------------------------
	//! Sent by COA_GroupNamingDialog once a GM confirms a name for a group they're marking joinable.
	void RequestMarkGroupJoinable(RplId groupId, FactionKey factionKey, string customName)
	{
		Rpc(RpcAsk_MarkGroupJoinable, groupId, factionKey, customName);
	}

	//------------------------------------------------------------------------------------------------
	void Heal(int playerId, bool logAction, bool isVehicle = false)
	{
		Rpc(RpcAsk_Heal, playerId, logAction, isVehicle); 
	}

	//------------------------------------------------------------------------------------------------
	void LogAdminAction(string data, int playerId, bool sendToPlayer, COA_EAdminLogLevel level) 
	{
		Rpc(RpcAsk_LogAdminAction, data, playerId, sendToPlayer, level); 
	}

	//------------------------------------------------------------------------------------------------
	void UpdateTimer(int delta) 
	{
		Rpc(RpcAsk_UpdateTimer, delta); 
	}	

	//------------------------------------------------------------------------------------------------
	void UpdateTicket(string action, FactionKey faction, int delta) 
	{
		Rpc(RpcAsk_UpdateTicket, action, faction, delta); 
	}
	
	//------------------------------------------------------------------------------------------------
	void RespawnFaction(FactionKey faction, bool logAction)
	{
		Rpc(RpcAsk_RespawnFaction, faction, logAction); 
	}

	//------------------------------------------------------------------------------------------------
	void AddItem(int playerId, string prefab, bool logAction)
	{
		Rpc(RpcAsk_AddItem, playerId, prefab, logAction); 
	}
	
	//------------------------------------------------------------------------------------------------
	void SetRespawnTime(int seconds)
	{
		Rpc(RpcAsk_SetRespawnTime, seconds);
	}
	
	//------------------------------------------------------------------------------------------------
	void CleanUpBodies()
	{
		Rpc(RpcAsk_CleanUpBodies);
	}
	
	//------------------------------------------------------------------------------------------------
	void ToggleWaveRespawn()
	{
		Rpc(RpcAsk_ToggleWaveRespawn);
	}
	
	//------------------------------------------------------------------------------------------------
	void ToggleRespawn()
	{
		Rpc(RpcAsk_ToggleRespawn);
	}

	//------------------------------------------------------------------------------------------------
	void TogglePlayerListening(int playerId, bool input)
	{
		Rpc(RpcAsk_TogglePlayerLisntening, playerId, input);
	}
	
	//------------------------------------------------------------------------------------------------
	void BorderKill(int playerId)
	{
		Rpc(RpcAsk_BorderKill, playerId); 
	}
	
	//------------------------------------------------------------------------------------------------
	void RequestForwardDeploy(vector cursorWorldPos, string factionKey, int playerId)
	{
		Rpc(RpcAsk_RequestForwardDeploy, cursorWorldPos, factionKey, playerId);
	}

	//------------------------------------------------------------------------------------------------
	void RequestPlaceRallypoint(int playerId)
	{
		Rpc(RpcAsk_PlaceRallypoint, playerId);
	}

	//------------------------------------------------------------------------------------------------
	void RequestDestroyRallypoint(RplId rallyPointId)
	{
		Rpc(RpcAsk_DestroyRallypoint, rallyPointId);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 REPLICATION METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	// Default parameter values aren't supported on RPCs - RequestInitilizePlayer (below) always passes
	// both arguments explicitly, so this method doesn't need one.
	protected void RpcAsk_RequestInitilizePlayer(int playerId, bool isMassJoinBatch)
	{
		playerId = GetCallerPlayerId(); // act on the sender, never a client-supplied ID

		// Telemetry: int, bool
		LogTelemetry("RpcAsk_RequestInitilizePlayer", COA_BandwidthTelemetryManager.EstimateSize_Int() + COA_BandwidthTelemetryManager.EstimateSize_Bool());

		// Use staggered initialization system to prevent server overload during connect/round-start
		// bursts; a standalone request (isMassJoinBatch = false) skips straight to InitilizePlayer.
		m_Gamemode.QueuePlayerInitialization(playerId, isMassJoinBatch);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ToggleSideReady(string setReady, string playerName, bool adminForced)
	{
		// Telemetry: 2 strings + bool
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_String(setReady);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_String(playerName);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_ToggleSideReady", bytes);

		// Mirror COA_PlayerKeybindManager's rules on the server: only admins may force-ready, and
		// otherwise only a group leader may toggle their OWN faction. The announced name is the
		// sender's real name, not a client-supplied string.
		int callerId = GetCallerPlayerId();
		if (callerId <= 0)
			return;

		playerName = GetGame().GetPlayerManager().GetPlayerName(callerId);

		if (adminForced)
		{
			if (!IsCallerAdmin())
				return;
		}
		else
		{
			SCR_AIGroup callerGroup;
			if (m_GroupsManagerComponent)
				callerGroup = m_GroupsManagerComponent.GetPlayerGroup(callerId);
			if (!callerGroup || !callerGroup.IsPlayerLeader(callerId))
				return;

			SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			if (!factionManager)
				return;

			Faction callerFaction = factionManager.GetPlayerFaction(callerId);
			if (!callerFaction || callerFaction.GetFactionKey() != setReady)
				return;
		}

		m_SafestartManager.ToggleSideReady(setReady, playerName, adminForced);
	}


	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestAdvanceGamemodeState(bool overriden, string winningFaction)
	{
		if (!IsCallerAdmin())
			return;

		// Telemetry: bool
		LogTelemetry("RpcAsk_RequestAdvanceGamemodeState", COA_BandwidthTelemetryManager.EstimateSize_Bool());
		
		m_Gamemode.AdvanceGamemodeState(overriden);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestAdvanceSlottingPhase()
	{
		if (!IsCallerAdmin())
			return;

		// Telemetry: no parameters
		LogTelemetry("RpcAsk_RequestAdvanceSlottingPhase", 0);
		
		m_Gamemode.AdvanceSlottingState();
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSlotPlayerID(int slotId, int playerId)
	{
		// Telemetry: 2 ints
		LogTelemetry("RpcAsk_UpdateSlotPlayerID", COA_BandwidthTelemetryManager.EstimateSize_Int() * 2);

		// Admins may slot/kick anyone (COA_SlottingMenu admin controls). Everyone else may only
		// claim an unlocked slot for themselves, or leave the slot they are currently in.
		if (!IsCallerAdmin())
		{
			int callerId = GetCallerPlayerId();
			COA_SlotData slotData = m_SlottingManager.GetSlotData(slotId);
			if (callerId <= 0 || !slotData)
				return;

			bool claimingForSelf = playerId == callerId && !slotData.GetIsLockedSlot();
			bool leavingOwnSlot = playerId == 0 && slotData.GetSlotCurrentPlayerId() == callerId;
			if (!claimingForSelf && !leavingOwnSlot)
				return;
		}

		m_SlottingManager.UpdateSlotPlayerID(slotId, playerId);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSlotLockedState(int slotId, bool input)
	{
		if (!IsCallerAdmin())
			return;

		// Telemetry: int + bool
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_UpdateSlotLockedState", bytes);
		
		m_SlottingManager.UpdateSlotLockedState(slotId, input);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_UpdateGroupLockedState(RplId groupRplId, bool input)
	{
		if (!IsCallerAdmin())
			return;

		// Telemetry: RplId + bool
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_RplId();
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_UpdateGroupLockedState", bytes);
		
		RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(groupRplId));
		if (!rplComponent)
			return;
			
		SCR_AIGroup group = SCR_AIGroup.Cast(rplComponent.GetEntity());
		if (group)
			group.SetPrivate(input);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSlotDeathState(int slotId, bool input)
	{
		if (!IsCallerAdmin())
			return;

		// Telemetry: int + bool
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_UpdateSlotDeathState", bytes);
		
		m_SlottingManager.UpdateSlotDeathState(slotId, input); 
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSlotRole(int slotId, COA_EGearRole role)
	{
		if (!IsCallerAdmin())
			return;

		// Telemetry: int + int
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Int();
		LogTelemetry("RpcAsk_UpdateSlotRole", bytes);
		
		m_SlottingManager.UpdateSlotRole(slotId, role); 
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSlotGroup(int slotId, RplId groupRplId)
	{
		if (!IsCallerAdmin())
			return;

		// Telemetry: int + RplId
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += COA_BandwidthTelemetryManager.EstimateSize_RplId();
		LogTelemetry("RpcAsk_UpdateSlotGroup", bytes);
		
		m_SlottingManager.UpdateSlotGroup(slotId, groupRplId); 
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSlotCharacter(int slotId, RplId charId)
	{
		if (!IsCallerAdmin())
			return;

		// Telemetry: int + RplId
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += COA_BandwidthTelemetryManager.EstimateSize_RplId();
		LogTelemetry("RpcAsk_UpdateSlotCharacter", bytes);
		
		m_SlottingManager.UpdateSlotCharacter(slotId, charId); 
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RespawnPlayer(int playerId, int spawnPointID)
	{
		playerId = GetCallerPlayerId(); // act on the sender, never a client-supplied ID

		// Telemetry: int + int
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Int();
		LogTelemetry("RpcAsk_RespawnPlayer", bytes);
		
		m_RespawnManager.RespawnPlayer(playerId, spawnPointID);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestToJoinChannel(int channel, int requestId)
	{
		requestId = GetCallerPlayerId(); // act on the sender, never a client-supplied ID

		// Telemetry: 2 ints
		LogTelemetry("RpcAsk_RequestToJoinChannel", COA_BandwidthTelemetryManager.EstimateSize_Int() * 2);
		
		Print(string.Format("[VON] Server processing join request: channel=%1, requestId=%2", channel, requestId), LogLevel.NORMAL);
		
		// Instead of using BroadcastManager, handle the request directly on the server
		int creatorId = GetChannelCreatorId(channel);
		if (creatorId <= 0)
			return;

		// Don't send a request if the requester is the channel creator
		if (creatorId == requestId)
		{
			Print(string.Format("[VON] Player %1 tried to join their own channel %2, ignoring", requestId, channel), LogLevel.NORMAL);
			return;
		}
		
		// Send notification to the channel creator
		if (creatorId > 0)
		{
			Print(string.Format("[VON] Server sending join request notification to creator %1 from requester %2 for channel %3", creatorId, requestId, channel), LogLevel.NORMAL);
			m_RplBroadcastManager.NotifyChannelJoinRequest(creatorId, requestId, channel);
		}
	}


	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CheckVONRegister(int playerId)
	{
		playerId = GetCallerPlayerId(); // act on the sender, never a client-supplied ID

		// Telemetry: int
		LogTelemetry("RpcAsk_CheckVONRegister", COA_BandwidthTelemetryManager.EstimateSize_Int());
		
		int channelIndex;
		if (!m_MenuManager.IsPlayerInAnyChannel(playerId, channelIndex))
		{
			m_MenuManager.AddPlayerToChannel(playerId, 1, false);
		}
	}


	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CreateChannel(int playerId)
	{
		playerId = GetCallerPlayerId(); // act on the sender, never a client-supplied ID

		// Telemetry: int
		LogTelemetry("RpcAsk_CreateChannel", COA_BandwidthTelemetryManager.EstimateSize_Int());
		
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		// Include player ID in channel name to ensure uniqueness when players have same username
		string uniqueChannelName = playerName + "'s Channel (" + playerId + ")";
		int channelIndex = m_MenuManager.CreateChannel(uniqueChannelName, playerId);
	}


	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_JoinChannel(int playerId, int channel)
	{
		// Telemetry: 2 ints
		LogTelemetry("RpcAsk_JoinChannel", COA_BandwidthTelemetryManager.EstimateSize_Int() * 2);

		// A player may join Deafen/Global, a channel with no creator (e.g. AAR group channels) or
		// their own channel directly. Player-created channels otherwise go through a join request,
		// which the channel's creator accepts by adding the requester (COA_MenuManager.Accept).
		int callerId = GetCallerPlayerId();
		int creatorId = GetChannelCreatorId(channel);
		bool selfJoinAllowed = playerId == callerId && (channel <= 1 || creatorId <= 0 || creatorId == callerId);
		bool creatorAccepting = creatorId > 0 && creatorId == callerId;
		if (!selfJoinAllowed && !creatorAccepting && !IsCallerStaff())
			return;

		m_MenuManager.AddPlayerToChannel(playerId, channel, false);
	}


	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SpawnOnGroup(int playerId, int playerIDToSpawnOn, int groupID, bool logAction)
	{
		if (!IsCallerStaff())
			return;

		// Telemetry: 3 ints + bool
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_Int() * 3;
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_SpawnOnGroup", bytes);
		
		RplId entityRplID;
		
		if (playerIDToSpawnOn != -1)
		{
			// Use the player selected in the group as the spawn point
			COA_PlayerCharacter spawnEntity = COA_SlottingManager.GetInstance().GetPlayerSlotCharacter(playerIDToSpawnOn);
			if (!spawnEntity)
				return;
			
			entityRplID = spawnEntity.GetRplComponent().Id();
		}		
		else if (groupID != -1)
		{
			// Use the group leader as the spawn point
			SCR_AIGroup group = SCR_GroupsManagerComponent.GetInstance().FindGroup(groupID);
			if (!group)
				return;
			
			IEntity leaderEntity = group.GetLeaderEntity();
			if (!leaderEntity)
				return;
			
			RplComponent rplComp = RplComponent.Cast(leaderEntity.FindComponent(RplComponent));
			if (!rplComp)
				return;
			
			entityRplID = rplComp.Id();
		}
		else
			entityRplID = RplId.Invalid();
		
		m_RespawnManager.RespawnPlayer(playerId, -1, entityRplID);
		
		if (logAction)
		{
			SCR_AIGroup group = m_GroupsManagerComponent.FindGroup(groupID);
			if (group)
			{
				string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
				string logMessage = string.Format("%1 Was Respawned To %2", playerName, group.m_faction);
				m_RplBroadcastManager.LogAdminAction(logMessage, playerId, true, COA_EAdminLogLevel.Medium);
			}
		}
	}


	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ResetGear(int playerId, ResourceName prefab, bool logAction)
	{
		if (!IsCallerSelfOrStaff(playerId))
			return;

		// Telemetry: int + ResourceName + bool
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += COA_BandwidthTelemetryManager.EstimateSize_ResourceName(prefab);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_ResetGear", bytes);
		
		// Prevent stuck on map
		m_RplBroadcastManager.Closemap(playerId);
		
		// Prevent invisible gun 
		m_RplBroadcastManager.HolsterGun(playerId);
		
		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!entity)
			return;

		// Schedule gear setup with appropriate delay
		GetGame().GetCallqueue().Call(
			m_GearscriptManager.SetEntityGear, 
			entity, 
			prefab
		);
		
		COA_RolesConfig rolesConfig = COA_GearscriptManager.GetRolesConfig();
		COA_EGearRole role = COA_RoleHelper.ResourceToRole(prefab);
		
		int slotId = m_SlottingManager.GetPlayerSlotID(playerId);
		COA_SlotData slotData = m_SlottingManager.GetSlotData(slotId);
		
		// Use delta updates for individual field changes (90%+ bandwidth savings)
		slotData.SetSlotRole(role);
		m_RplBroadcastManager.UpdateSlotRoleDelta(slotId, role);
		
		// Note: Name, Type, and Icon don't have delta updates as they rarely change
		// If they change frequently in the future, add delta methods for them too
		
		if (logAction)
		{
			string prefabName = prefab.Substring(prefab.LastIndexOf("/") + 1, prefab.LastIndexOf(".") - prefab.LastIndexOf("/") - 1);
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			string logMessage = string.Format("%1's Gear Was Set To %2", playerName, prefabName);
			m_RplBroadcastManager.LogAdminAction(logMessage, playerId, true, COA_EAdminLogLevel.Medium);
		}
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateGearSet(string faction, ResourceName path)
	{
		if (!IsCallerStaff())
			return;

		// Telemetry: string + ResourceName
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_String(faction);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_ResourceName(path);
		LogTelemetry("RpcAsk_UpdateGearSet", bytes);
		
		// Update gearscript in the gamemode
		COA_Gamemode.GetInstance().UpdateGearscriptResource(faction, path);

		// Load the AI world
		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld)
		{
			Print("[COA_PlayerRplToAuthorityManager] AIworld not found, can't update gear sets", LogLevel.ERROR);
			return;
		}
		
		array<AIAgent> aiAgents = {};
		array<IEntity> entities = {};
		int delayTime;
		int delay;

		//Get entities in the faction and store them
		aiWorld.GetAIAgents(aiAgents);
		foreach (AIAgent agent : aiAgents)
		{
			IEntity entity = agent.GetControlledEntity();
			if (!entity)
				continue;

			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
			if (character && character.GetFactionKey() == faction)
				entities.Insert(character);
		}

		// Also gather connected human players in the faction, so their gear updates immediately instead of only on reinitialize
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!playerEntity)
				continue;

			SCR_ChimeraCharacter playerCharacter = SCR_ChimeraCharacter.Cast(playerEntity);
			if (playerCharacter && playerCharacter.GetFactionKey() == faction && !entities.Contains(playerCharacter))
				entities.Insert(playerCharacter);
		}

		// SET GEAR SCRIPTS
		foreach (IEntity entity : entities)
		{
			if (entity)
			{
				EntityPrefabData prefabData = entity.GetPrefabData();
				if (prefabData)
				{
					ResourceName prefab = prefabData.GetPrefabName();
					if (COA_RoleHelper.IsValidGearscriptResource(prefab))
					{
						delay++;
						delayTime = (delay * 65); // need a delay to prevent massive lag spikes on mission load (and to see what role actually casued the error lol)
						GetGame().GetCallqueue().CallLater(UpdateGearSetQueue, delayTime, false, entity, prefab);
					}	
				};
			};
		}
		
		string logMessage = string.Format("%1 Was Changed To %2", faction, path);
		m_RplBroadcastManager.LogAdminAction(logMessage, -1 , false, COA_EAdminLogLevel.High)
	}

	
	//------------------------------------------------------------------------------------------------
	protected void UpdateGearSetQueue(IEntity entity, ResourceName prefab)
	{
		if (!entity || !COA_RoleHelper.IsValidGearscriptResource(prefab))
			return;
		
		// Prevent Lockup and invisible weapon if player
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
		if (playerId)
		{
			m_RplBroadcastManager.Closemap(playerId);
			//Causes issues when trying to holster a weapon we are deleting.
			//m_RplBroadcastManager.HolsterGun(playerId);
		}
		
		COA_GearscriptManager gearscriptManager = COA_GearscriptManager.GetInstance();
		if (gearscriptManager)
			gearscriptManager.SetEntityGear(entity, prefab);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_MoveSpecCamToSlot(int slotID, int playerId)
	{
		playerId = GetCallerPlayerId(); // act on the sender, never a client-supplied ID

		// Get slot data from the slotting manager
		COA_SlotData slotData = COA_SlottingManager.GetInstance().GetSlotData(slotID);
		if (!slotData)
			return;
		
		// Find the entity associated with the slot and set it as the spectator target
		RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(slotData.GetSlotCurrentCharacter()));
		if (!rplComponent)
			return;
		
		// Get slot origin
		IEntity slotEntity = rplComponent.GetEntity();
		if (!slotEntity)
			return;

		vector slotPos = slotEntity.GetOrigin();
				
		m_RplBroadcastManager.MoveSpecCamToSlot(slotPos, playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SendAdminMessage(string data, int playerID)
	{
		// Telemetry: string + int
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_String(data);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Int();
		LogTelemetry("RpcAsk_SendAdminMessage", bytes);

		// A ticket is always from the player who sent it - never trust the client-supplied ID,
		// or a modified client could open tickets in someone else's name.
		playerID = GetCallerPlayerId();
		if (playerID <= 0)
			return;

		// Send the new ticket/message to admins
		bool ticketExists = m_AdminMenuManager.TicketExists(playerID);
		m_RplBroadcastManager.SendAdminMessage(data, playerID, ticketExists);
		
		// Create a new ticket or/and add reply to existing ticket if not a admin/mod
		if (!SCR_Global.IsAdmin(playerID) && !m_PermissionManager.IsModerator(playerID))
			m_AdminMenuManager.NewTicketMessage(playerID, playerID, data);
	}	
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ReplyAdminMessage(string data, int playerId, int adminID, bool logAction)
	{
		// Telemetry: string + 2 ints + bool
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_String(data);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Int() * 2;
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_ReplyAdminMessage", bytes);

		if (!IsCallerStaff())
			return;
		adminID = GetCallerPlayerId();

		// Create a new ticket or/and add reply to existing ticket
		m_AdminMenuManager.NewTicketMessage(playerId, adminID, data);
		
		// Broadcast to the reply to the player
		m_RplBroadcastManager.ReplyAdminMessage(data, playerId, adminID, logAction);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CloseAdminTicket(int ticketID, int adminID, bool logAction)
	{
		// Telemetry: 2 ints + bool
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_Int() * 2;
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_CloseAdminTicket", bytes);

		if (!IsCallerStaff())
			return;
		adminID = GetCallerPlayerId();

		m_AdminMenuManager.CloseTicket(ticketID);
		
		// Broadcast to admins that ticket was clsoed
		m_RplBroadcastManager.CloseAdminTicket(ticketID, adminID, true);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AssignAdminTicket(int ticketID, int adminID, bool logAction)
	{
		// Telemetry: 2 ints + bool
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_Int() * 2;
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_AssignAdminTicket", bytes);

		if (!IsCallerStaff())
			return;
		adminID = GetCallerPlayerId();

		m_AdminMenuManager.AssignAdminTicket(ticketID, adminID);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_GetOpenTickets(int playerID)
	{
		if (!IsCallerStaff())
			return;

		m_RplBroadcastManager.GetOpenTickets(GetCallerPlayerId());
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_GetTicketMessages(int playerID, int ticketID)
	{
		if (!IsCallerStaff())
			return;

		m_RplBroadcastManager.GetTicketMessages(GetCallerPlayerId(), ticketID);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RespawnFaction(FactionKey faction, bool logAction)
	{
		if (!IsCallerStaff())
			return;

		// Telemetry: string (FactionKey) + bool
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_String(faction);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_RespawnFaction", bytes);
		
		m_RespawnManager.RespawnSide(faction);
		
		if (logAction)
		{
			string logMessage = string.Format("%1 Was Respawned", faction);
			m_RplBroadcastManager.LogAdminAction(logMessage, -1, false, COA_EAdminLogLevel.High);
		}
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AddItem(int playerId, string prefab, bool logAction)
	{
		if (!IsCallerStaff())
			return;

		// Telemetry: int + string + bool
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += COA_BandwidthTelemetryManager.EstimateSize_String(prefab);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_AddItem", bytes);
		
		if (playerId == 0 || prefab.IsEmpty())
			return;

		if (logAction)
		{
			string itemName = prefab.Substring(prefab.LastIndexOf("/") + 1, prefab.LastIndexOf(".") - prefab.LastIndexOf("/") - 1);
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			string logMessage = string.Format("%2 Was Added To %1's Inventory", playerName, itemName);
			m_RplBroadcastManager.LogAdminAction(logMessage, playerId, true, COA_EAdminLogLevel.Medium);
		}
		
		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!entity)
			return;
			
		SCR_InventoryStorageManagerComponent entityInventoryManager = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!entityInventoryManager)
			return;
		
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = entity.GetOrigin();
		
		Resource resource = Resource.Load(prefab);
		IEntity resourceSpawned = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
		if (!resourceSpawned)
			return;

		if (!entityInventoryManager.TryInsertItem(resourceSpawned))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(resourceSpawned);
			return;
		}
		
		if (resourceSpawned)
			if (resourceSpawned.FindComponent(CVON_RadioComponent))
			{
				GetGame().GetCallqueue().CallLater(InitializePlayerRadiosDelayed, 500, false, playerId);
				COA_PlayerRplToOwnerManager ownerManager = COA_PlayerRplToOwnerManager.GetForPlayer(playerId);
				if (ownerManager)
					ownerManager.InitializeRadioFromServer();
			
			}
	}
	
	//------------------------------------------------------------------------------------------------
	static void InitializePlayerRadiosDelayed(int playerId)
	{
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!player)
			return;

		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (groupsManager)
			groupsManager.TuneFreqDelayWithPresets(playerId, player);

		COA_PlayerController playerController = COA_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (playerController)
			playerController.InitializeRadios(player);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_TeleportPlayers(int playerId1, int playerId2, bool logAction)
	{
		if (!IsCallerStaff())
			return;

		// Telemetry: 2 ints + bool
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_Int() * 2;
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_TeleportPlayers", bytes);
		
		m_RplBroadcastManager.TeleportPlayers(playerId1, playerId2, logAction);
	}


	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SendHint(string data, int playerId, string factionKey)
	{
		if (!IsCallerStaff())
			return;

		// Telemetry: 2 strings + int
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_String(data);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_String(factionKey);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Int();
		LogTelemetry("RpcAsk_SendHint", bytes);
		
		m_RplBroadcastManager.SendHint(data, playerId, factionKey);
	}

	//------------------------------------------------------------------------------------------------
	//! Server-side handler for COA_GroupNamingDialog's confirm button. This is a direct client-
	//! reachable RPC (not gated by the editor context action's CanBeShown/CanBePerformed, which are
	//! only client-side UI hints), so the caller's Game Master status is re-checked here using
	//! EPlayerRole.GAME_MASTER - a server-authoritative role flag SCR_EditorManagerEntity itself
	//! maintains, not something a modified client can forge. The caller's identity is resolved from
	//! this component's own owning PlayerController (GetOwner()) rather than trusting any
	//! client-supplied player ID, since this component is attached per-player and the RPC always
	//! arrives in the context of whichever controller actually sent it.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_MarkGroupJoinable(RplId groupId, FactionKey factionKey, string customName)
	{
		// Telemetry: RplId + 2 strings
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_RplId();
		bytes += COA_BandwidthTelemetryManager.EstimateSize_String(factionKey);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_String(customName);
		LogTelemetry("RpcAsk_MarkGroupJoinable", bytes);

		PlayerController ownerController = PlayerController.Cast(GetOwner());
		if (!ownerController)
			return;

		int callerId = ownerController.GetPlayerId();
		if (!GetGame().GetPlayerManager().HasPlayerRole(callerId, EPlayerRole.GAME_MASTER))
			return;

		SCR_AIGroup group = COA_EntityHelper.GetGroupFromRplId(groupId);
		if (!group)
			return;

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (slottingManager)
			slottingManager.RegisterGMPossessionGroup(group, factionKey, customName);
	}


	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_Heal(int playerId, bool logAction, bool isVehicle)
	{
		if (!IsCallerStaff())
			return;

		// Telemetry: int + 2 bools
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Bool() * 2;
		LogTelemetry("RpcAsk_Heal", bytes);
		
		IEntity entityToFix = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!entityToFix)
			return;
		
		if (isVehicle)
		{
			entityToFix = SCR_CompartmentAccessComponent.GetVehicleIn(entityToFix);
			if (!entityToFix)
				return;
		}

		SCR_DamageManagerComponent damageComponent = SCR_DamageManagerComponent.Cast(entityToFix.FindComponent(SCR_DamageManagerComponent));
		if (!damageComponent)
			return;

		damageComponent.FullHeal();
		damageComponent.SetHealthScaled(1);

		if (logAction)
		{
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			string logMessage = string.Format("%1 Was Healed/Vehicle Repaired", playerName);
			m_RplBroadcastManager.LogAdminAction(logMessage, playerId, true, COA_EAdminLogLevel.Medium);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_LogAdminAction(string data, int playerId, bool sendToPlayer, COA_EAdminLogLevel level)
	{
		// Telemetry: string + int + bool
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_String(data);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_LogAdminAction", bytes);
		
		// Regular players do log actions (e.g. taking arsenal items in CRF_SCR_InventoryMenuUI), but
		// only staff may push a log line into another player's chat.
		if (!IsCallerStaff())
			sendToPlayer = false;

		m_RplBroadcastManager.LogAdminAction(data, playerId, sendToPlayer, level);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateTimer(int delta)
	{
		if (!IsCallerStaff())
			return;

		// Telemetry: int
		LogTelemetry("RpcAsk_UpdateTimer", COA_BandwidthTelemetryManager.EstimateSize_Int());
		
		// Get current end time
		int currentEndTime = COA_GameTimerManager.GetInstance().m_iTimeMissionEnds;
		if ((currentEndTime + delta) < 0 || m_SafestartManager.GetSafestartStatus())
			return;

		// Set the new time, broadcast is handled by rplprop
		COA_GameTimerManager.GetInstance().m_iTimeMissionEnds = currentEndTime + delta;
		
		string logMessage = string.Format("Game Timer Adjusted By %1 Mins", delta/60000);
		m_RplBroadcastManager.LogAdminAction(logMessage, -1, false, COA_EAdminLogLevel.High);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateTicket(string action, FactionKey faction, int delta)
	{
		if (!IsCallerStaff())
			return;

		// Telemetry: 2 strings + int
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_String(action);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_String(faction);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Int();
		LogTelemetry("RpcAsk_UpdateTicket", bytes);
		
		if (action == "Add")
			m_RespawnManager.AddTicket(faction, delta, true);
		else if (action == "Subtract")
			m_RespawnManager.SubtractTicket(faction, delta, true);
		
		string logMessage = string.Format("%1 Tickets Was %3ed For %2", delta, faction, action);
		m_RplBroadcastManager.LogAdminAction(logMessage, -1, false, COA_EAdminLogLevel.High);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ToggleWaveRespawn()
	{
		if (!IsCallerStaff())
			return;

		// Telemetry: no parameters
		LogTelemetry("RpcAsk_ToggleWaveRespawn", 0);
		
		COA_RespawnManager.GetInstance().ToggleRespawnWave();
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ToggleRespawn()
	{
		if (!IsCallerStaff())
			return;

		// Telemetry: no parameters
		LogTelemetry("RpcAsk_ToggleRespawn", 0);
		
		COA_RespawnManager.GetInstance().ToggleRespawn();
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SetRespawnTime(int seconds)
	{
		if (!IsCallerStaff())
			return;

		// Telemetry: int
		LogTelemetry("RpcAsk_SetRespawnTime", COA_BandwidthTelemetryManager.EstimateSize_Int());
		
		COA_RespawnManager.GetInstance().SetRespawnTime(seconds);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CleanUpBodies()
	{
		if (!IsCallerStaff() && !GetGame().GetPlayerManager().HasPlayerRole(GetCallerPlayerId(), EPlayerRole.GAME_MASTER))
			return;

		// Telemetry: no parameters
		LogTelemetry("RpcAsk_CleanUpBodies", 0);
		
		COA_GarbageManager.GetInstance().CleanUpBodies();
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_TogglePlayerLisntening(int playerId, bool input)
	{
		playerId = GetCallerPlayerId(); // act on the sender, never a client-supplied ID

		// Telemetry: int + bool
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_TogglePlayerLisntening", bytes);
		
		CVON_VONGameModeComponent cvon = CVON_VONGameModeComponent.GetInstance();
		if (cvon)
			cvon.TogglePlayerListening(playerId, input);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_BorderKill(int playerId)
	{
		playerId = GetCallerPlayerId(); // act on the sender, never a client-supplied ID

		// Telemetry: int + 2 bools
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_Int();
		LogTelemetry("RpcAsk_BorderKill", bytes);
		
		IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!playerEntity)
			return;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(playerEntity.FindComponent(SCR_DamageManagerComponent));
		if (!damageManager || damageManager.GetState() == EDamageState.DESTROYED)
			return;
		damageManager.SetHealthScaled(0);

		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		string logMessage = string.Format("%1 Was Killed By A Game Border", playerName);
		m_RplBroadcastManager.LogAdminAction(logMessage, playerId, true, COA_EAdminLogLevel.High);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestForwardDeploy(vector cursorWorldPos, string factionKey, int playerId)
	{
		playerId = GetCallerPlayerId(); // act on the sender, never a client-supplied ID

		LogTelemetry("RpcAsk_RequestForwardDeploy", COA_BandwidthTelemetryManager.EstimateSize_Vector() + COA_BandwidthTelemetryManager.EstimateSize_String(factionKey) + COA_BandwidthTelemetryManager.EstimateSize_Int());
		IEntity GameBorder;
		cursorWorldPos[1] = SCR_TerrainHelper.GetTerrainY(cursorWorldPos);
		foreach (IEntity zone: COA_ForwardDeployManager.GetInstance().GetForwardDeployZones())
		{
			COA_GameBorder border = COA_GameBorder.Cast(zone);
			
			if (!border || !border.IsInsidePolygon(Vector(cursorWorldPos[0], 0, cursorWorldPos[2])) || !border.m_aVisibleForFactions.Contains(factionKey))
				continue;
			
			GameBorder = border;
			break;
		}
		
		if (!GameBorder)
		{
			RejectForwardDeploy();
			return;
		}
		
		array<IEntity> entities = {};

		SCR_GroupsManagerComponent groupMan = SCR_GroupsManagerComponent.GetInstance();
		SCR_AIGroup playerGroup = groupMan.GetPlayerGroup(playerId);
		if (!playerGroup)
		{
		    RejectForwardDeploy();
		    return;
		}

		//Only the group leader may forward deploy, and only while SafeStart is active - re-validated
		//here since the client-side menu option is just a UI convenience, not an authority check.
		if (!playerGroup.IsPlayerLeader(playerId) || !m_SafestartManager || !m_SafestartManager.GetSafestartStatus())
		{
		    RejectForwardDeploy();
		    return;
		}

		//SCR_AIGroup.GetAgents() only yields AI-controlled members, human players have to be found via the player manager.
		array<int> groupPlayerIds = {};
		GetGame().GetPlayerManager().GetPlayers(groupPlayerIds);
		foreach (int groupMemberId : groupPlayerIds)
		{
			if (groupMan.GetPlayerGroup(groupMemberId) != playerGroup)
				continue;

			IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(groupMemberId);
			if (!entity)
				continue;

			if (!SCR_ChimeraCharacter.Cast(entity))
				continue;

			entities.Insert(entity);
		}

		array<AIAgent> aiAgents = {};
		playerGroup.GetAgents(aiAgents);
		foreach (AIAgent agent : aiAgents)
		{
			IEntity entity = agent.GetControlledEntity();
			if (!entity)
				continue;

			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
			if (!character)
				continue;

			entities.Insert(entity);
		}
		foreach (IEntity entity: entities)
		{
			int currentPlayerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
			if (currentPlayerId <= 0)
				continue;
			SCR_CompartmentAccessComponent compartmentAccess = SCR_CompartmentAccessComponent.Cast(entity.FindComponent(SCR_CompartmentAccessComponent));
			if (compartmentAccess)
			{
				IEntity vehicle = compartmentAccess.GetVehicle();

				if (vehicle)
				{
					SCR_BaseCompartmentManagerComponent compartmentMan = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
					array<BaseCompartmentSlot> slots = {};
					compartmentMan.GetCompartments(slots);
					//Check if majority of the vic is the same group, if not don't teleport.
					int amountInGroup = 0;
					int amountNotInGroup = 0;
					foreach (BaseCompartmentSlot slot: slots)
					{
						if (!slot.IsOccupied())
							continue;

						if (!slot.GetOccupant().FindComponent(FactionAffiliationComponent))
							continue;

						if (entities.Contains(slot.GetOccupant()))
							amountInGroup++;
						else
							amountNotInGroup++;
					}
					if (amountInGroup < amountNotInGroup)
						continue;
					COA_ForwardDeployManager.GetInstance().CreateForwardDeployRequest(currentPlayerId, cursorWorldPos);
					continue;
				}
			}
			COA_ForwardDeployManager.GetInstance().CreateForwardDeployRequest(currentPlayerId, cursorWorldPos);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_PlaceRallypoint(int playerId)
	{
		playerId = GetCallerPlayerId(); // act on the sender, never a client-supplied ID

		ResourceName rallyPrefabName;

		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!character)
			return;

		if (!COA_RoleHelper.IsSquadLeaderRole(character))
			return;

		Faction faction = COA_SlottingManager.GetInstance().GetPlayerSlotFaction(playerId);
		if (!faction)
			return;

		switch(faction.GetFactionKey())
		{
			case SCR_Enum.GetEnumName(COA_EFactions, 0): rallyPrefabName = m_Gamemode.m_rBLUFORRallyPrefab; break;
			case SCR_Enum.GetEnumName(COA_EFactions, 1): rallyPrefabName = m_Gamemode.m_rOPFORRallyPrefab; break;
			case SCR_Enum.GetEnumName(COA_EFactions, 2): rallyPrefabName = m_Gamemode.m_rINDFORRallyPrefab; break;
			case SCR_Enum.GetEnumName(COA_EFactions, 3): rallyPrefabName = m_Gamemode.m_rCIVILIANRallyPrefab; break;
		}

		Resource rallyPrefab = Resource.Load(rallyPrefabName);
		if (!rallyPrefab)
			return;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;

		COA_EntityHelper.GetSafeSpawnTransform(character, 2, true, true, spawnParams.Transform);

		IEntity rallyEntity = GetGame().SpawnEntityPrefab(rallyPrefab, GetGame().GetWorld(), spawnParams);
		if (!rallyEntity)
			return;
		
		COA_RallyPoint rallyPoint = COA_RallyPoint.Cast(rallyEntity);
		if (!rallyPoint)
			return;

		rallyPoint.SetupRallyPoint(playerId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_DestroyRallypoint(RplId rallyPointId)
	{
		IEntity rallyPoint = COA_EntityHelper.GetEntityFromRplId(rallyPointId);
		if (!rallyPoint)
			return;

		// Anyone may destroy a rally point (COA_DestroyRallypointAction), but only by interacting
		// with it - so the sender must actually be standing near it.
		if (!IsCallerStaff())
		{
			IEntity callerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(GetCallerPlayerId());
			if (!callerEntity || vector.DistanceSq(callerEntity.GetOrigin(), rallyPoint.GetOrigin()) > RALLYPOINT_INTERACT_DISTANCE_SQ)
				return;
		}

		COA_RallyPoint rallyPointComponent = COA_RallyPoint.Cast(rallyPoint);
		if (!rallyPointComponent)
			return;

		rallyPointComponent.DestroyRallPoint();
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================

	protected static COA_PlayerRplToAuthorityManager m_sInstance;
	void COA_PlayerRplToAuthorityManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_PlayerRplToAuthorityManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static COA_PlayerRplToAuthorityManager GetInstance()
	{
		return m_sInstance;
	}
};
