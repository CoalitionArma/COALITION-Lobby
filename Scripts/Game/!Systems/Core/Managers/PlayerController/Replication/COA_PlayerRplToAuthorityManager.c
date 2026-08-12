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

	//------------------------------------------------------------------------------------------------
	//! Resolve a replicated entity without dereferencing a missing or stale replication item.
	protected IEntity ResolveReplicatedEntity(RplId entityId)
	{
		if (entityId == RplId.Invalid())
			return null;

		RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(entityId));
		if (!rplComponent)
			return null;

		return rplComponent.GetEntity();
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 CLIENT/RPC METHODS (lobby-scoped: player init, gamemode/slotting phase, slot mutation, spawn, spectator cam, slot lottery, gear apply/hot-swap, VON channel join)
//=============================================================================================================================================================================================================================================================================================================================================================

	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 CLIENT REPLICATION ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void RequestInitilizePlayer(int playerId)
	{
		Rpc(RpcAsk_RequestInitilizePlayer, playerId); 
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
	void RequestForwardDeploy(vector cursorWorldPos, string factionKey, int playerId)
	{
		Rpc(RpcAsk_RequestForwardDeploy, cursorWorldPos, factionKey, playerId);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 REPLICATION METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestInitilizePlayer(int playerId)
	{
		// Telemetry: int
		LogTelemetry("RpcAsk_RequestInitilizePlayer", COA_BandwidthTelemetryManager.EstimateSize_Int());
		
		// Use staggered initialization system to prevent server overload
		m_Gamemode.QueuePlayerInitialization(playerId);
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
		
		m_SafestartManager.ToggleSideReady(setReady, playerName, adminForced);
	}


	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestAdvanceGamemodeState(bool overriden, string winningFaction)
	{
		// Telemetry: bool
		LogTelemetry("RpcAsk_RequestAdvanceGamemodeState", COA_BandwidthTelemetryManager.EstimateSize_Bool());
		
		m_Gamemode.AdvanceGamemodeState(overriden);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestAdvanceSlottingPhase()
	{
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
		
		m_SlottingManager.UpdateSlotPlayerID(slotId, playerId);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSlotLockedState(int slotId, bool input)
	{
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
		// Telemetry: 2 ints
		LogTelemetry("RpcAsk_RequestToJoinChannel", COA_BandwidthTelemetryManager.EstimateSize_Int() * 2);
		
		Print(string.Format("[VON] Server processing join request: channel=%1, requestId=%2", channel, requestId), LogLevel.NORMAL);
		
		// Instead of using BroadcastManager, handle the request directly on the server
		if (channel < 0 || channel >= m_MenuManager.m_aVONChannels.Count())
			return;
		
		// Extract channel creator ID from channel name
		// Channel name format: "PlayerName's Channel (PlayerID)|players..."
		string channelString = m_MenuManager.m_aVONChannels[channel];
		array<string> channelSplit = {};
		channelString.Split("|", channelSplit, true);
		
		if (channelSplit.Count() == 0)
			return;
		
		string channelName = channelSplit[0];
		
		// Find the creator ID from the channel name format: "Name's Channel (ID)"
		int openParen = channelName.IndexOf("(");
		int closeParen = channelName.IndexOf(")");
		
		if (openParen == -1 || closeParen == -1 || closeParen <= openParen)
			return;
		
		string creatorIdStr = channelName.Substring(openParen + 1, closeParen - openParen - 1);
		int creatorId = creatorIdStr.ToInt();
		
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
		
		m_MenuManager.AddPlayerToChannel(playerId, channel, false);
	}


	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SpawnOnGroup(int playerId, int playerIDToSpawnOn, int groupID, bool logAction)
	{
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
				string logMessage = string.Format("%1 was respawned to %2", playerName, group.m_faction);
				m_RplBroadcastManager.LogAdminAction(logMessage, playerId, true, COA_EAdminLogLevel.Low);
			}
		}
	}


	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ResetGear(int playerId, ResourceName prefab, bool logAction)
	{
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
			string logMessage = string.Format("%1's gear was set to %2", playerName, prefabName);
			m_RplBroadcastManager.LogAdminAction(logMessage, playerId, true, COA_EAdminLogLevel.Low);
		}
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateGearSet(string faction, ResourceName path)
	{
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
			Print("ERROR: AIworld not found, can't update gear sets");
			return;
		}
		
		array<AIAgent> aiAgents = {};
		array<RplId> entityIds = {};

		//Get entities in the faction and store them
		aiWorld.GetAIAgents(aiAgents);
		foreach (AIAgent agent : aiAgents)
		{
			IEntity entity = agent.GetControlledEntity();
			if (!entity)
				continue;

			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
			if (!character)
				continue;

			if (character.GetFactionKey() == faction)
			{
				RplId entityId = Replication.FindItemId(entity);
				if (entityId != RplId.Invalid())
					entityIds.Insert(entityId);
			}
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
			if (!playerCharacter)
				continue;

			if (playerCharacter.GetFactionKey() == faction)
			{
				RplId entityId = Replication.FindItemId(playerEntity);
				if (entityId != RplId.Invalid())
					entityIds.Insert(entityId);
			}
		}

		// Queue changes to prevent server freezing
		UpdateGearSetQueue(entityIds);
		
		string logMessage = string.Format("%1 was changed to %2", faction, path);
		m_RplBroadcastManager.LogAdminAction(logMessage, -1 , false, COA_EAdminLogLevel.Low)
	}

	
	//------------------------------------------------------------------------------------------------
	protected void UpdateGearSetQueue(array<RplId> entityIds, int lastIndex = 0)
	{
		if (!entityIds || lastIndex >= entityIds.Count())
			return;
		
		IEntity entity = ResolveReplicatedEntity(entityIds[lastIndex]);
		if (!entity)
		{
			GetGame().GetCallqueue().CallLater(UpdateGearSetQueue, 50, false, entityIds, lastIndex + 1);
			return;
		}
		
		// Grab prefab name and check if its a valid gearscript
		EntityPrefabData prefabData = entity.GetPrefabData();
		if (!prefabData)
		{
			GetGame().GetCallqueue().CallLater(UpdateGearSetQueue, 50, false, entityIds, lastIndex + 1);
			return;
		}

		ResourceName prefab = prefabData.GetPrefabName();
		if (!COA_RoleHelper.IsValidGearscriptResource(prefab))
		{
			GetGame().GetCallqueue().CallLater(UpdateGearSetQueue, 50, false, entityIds, lastIndex + 1);
			return;
		}
		
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
		
		// Queue next entity
		GetGame().GetCallqueue().CallLater(UpdateGearSetQueue, 50, false, entityIds, lastIndex + 1);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_MoveSpecCamToSlot(int slotID, int playerId)
	{
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
		
		// Broadcast a new ticket/message to admins
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
		
		m_AdminMenuManager.AssignAdminTicket(ticketID, adminID);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_GetOpenTickets(int playerID)
	{
		m_RplBroadcastManager.GetOpenTickets(playerID);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_GetTicketMessages(int playerID, int ticketID)
	{
		m_RplBroadcastManager.GetTicketMessages(playerID, ticketID);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RespawnFaction(FactionKey faction, bool logAction)
	{
		// Telemetry: string (FactionKey) + bool
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_String(faction);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_RespawnFaction", bytes);
		
		m_RespawnManager.RespawnSide(faction);
		
		if (logAction)
		{
			string logMessage = string.Format("%1 was respawned", faction);
			m_RplBroadcastManager.LogAdminAction(logMessage, -1, false, COA_EAdminLogLevel.Low);
		}
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AddItem(int playerId, string prefab, bool logAction)
	{
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
			string logMessage = string.Format("%2 was added to %1's inventory", playerName, itemName);
			m_RplBroadcastManager.LogAdminAction(logMessage, playerId, true, COA_EAdminLogLevel.Low);
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
				COA_PlayerRplToOwnerManager ownerManager = COA_PlayerRplToOwnerManager.GetInstance();
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
		// Telemetry: 2 strings + int
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_String(data);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_String(factionKey);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Int();
		LogTelemetry("RpcAsk_SendHint", bytes);
		
		m_RplBroadcastManager.SendHint(data, playerId, factionKey);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_Heal(int playerId, bool logAction, bool isVehicle)
	{
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
			string logMessage = string.Format("%1 was healed/vehicle repaired", playerName);
			m_RplBroadcastManager.LogAdminAction(logMessage, playerId, true, COA_EAdminLogLevel.Low);
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
		
		m_RplBroadcastManager.LogAdminAction(data, playerId, sendToPlayer, COA_EAdminLogLevel.Low);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateTimer(int delta)
	{
		// Telemetry: int
		LogTelemetry("RpcAsk_UpdateTimer", COA_BandwidthTelemetryManager.EstimateSize_Int());
		
		// Get current end time
		int currentEndTime = COA_GameTimerManager.GetInstance().m_iTimeMissionEnds;
		if ((currentEndTime + delta) < 0 || m_SafestartManager.GetSafestartStatus())
			return;

		// Set the new time, broadcast is handled by rplprop
		COA_GameTimerManager.GetInstance().m_iTimeMissionEnds = currentEndTime + delta;
		
		string logMessage = string.Format("Game timer adjusted by %1 mins", delta/60000);
		m_RplBroadcastManager.LogAdminAction(logMessage, -1, false, COA_EAdminLogLevel.Low);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateTicket(string action, FactionKey faction, int delta)
	{
		// Telemetry: 2 strings + int
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_String(action);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_String(faction);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Int();
		LogTelemetry("RpcAsk_UpdateTicket", bytes);
		
		if (action == "Add")
			m_RespawnManager.AddTicket(faction, delta, true);
		else if (action == "Subtract")
			m_RespawnManager.SubtractTicket(faction, delta, true);
		
		string logMessage = string.Format("%1 tickets was subtracted from %2", delta, faction);
		m_RplBroadcastManager.LogAdminAction(logMessage, -1, false, COA_EAdminLogLevel.Low);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ToggleWaveRespawn()
	{
		// Telemetry: no parameters
		LogTelemetry("RpcAsk_ToggleWaveRespawn", 0);
		
		COA_RespawnManager.GetInstance().ToggleRespawnWave();
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ToggleRespawn()
	{
		// Telemetry: no parameters
		LogTelemetry("RpcAsk_ToggleRespawn", 0);
		
		COA_RespawnManager.GetInstance().ToggleRespawn();
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SetRespawnTime(int seconds)
	{
		// Telemetry: int
		LogTelemetry("RpcAsk_SetRespawnTime", COA_BandwidthTelemetryManager.EstimateSize_Int());
		
		COA_RespawnManager.GetInstance().SetRespawnTime(seconds);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CleanUpBodies()
	{
		// Telemetry: no parameters
		LogTelemetry("RpcAsk_CleanUpBodies", 0);
		
		COA_GarbageManager.GetInstance().CleanUpBodies();
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_TogglePlayerLisntening(int playerId, bool input)
	{
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
	protected void RpcAsk_RequestForwardDeploy(vector cursorWorldPos, string factionKey, int playerId)
	{
		LogTelemetry("RpcAsk_RequestForwardDeploy", COA_BandwidthTelemetryManager.EstimateSize_Vector() + COA_BandwidthTelemetryManager.EstimateSize_String(factionKey) + COA_BandwidthTelemetryManager.EstimateSize_Int());
		IEntity polyzone;
		cursorWorldPos[1] = SCR_TerrainHelper.GetTerrainY(cursorWorldPos);
		foreach (IEntity zone: COA_ForwardDeployManager.GetInstance().GetForwardDeployZones())
		{
			COA_PolyZone zoneComp = COA_PolyZone.Cast(zone.FindComponent(COA_PolyZone));
			if (!zoneComp)
				continue;
			
			if (!zoneComp.IsInsidePolygon(Vector(cursorWorldPos[0], 0, cursorWorldPos[2])))
				continue;
			
			if (!zoneComp.m_aVisibleForFactions.Contains(factionKey))
				continue;
			
			polyzone = zone;
			break;
		}
		
		if (!polyzone)
		{
			COA_PlayerRplToOwnerManager.GetInstance().ForwardDeployRequestRejected();
			return;
		}
		
		array<IEntity> entities = {};

		SCR_GroupsManagerComponent groupMan = SCR_GroupsManagerComponent.GetInstance();
		SCR_AIGroup playerGroup = groupMan.GetPlayerGroup(playerId);
		if (!playerGroup)
		{
		    COA_PlayerRplToOwnerManager.GetInstance().ForwardDeployRequestRejected();
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
