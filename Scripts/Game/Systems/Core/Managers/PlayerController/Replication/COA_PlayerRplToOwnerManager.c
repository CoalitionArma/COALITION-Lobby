class COA_PlayerRplToOwnerManagerClass : ScriptComponentClass {}

class COA_PlayerRplToOwnerManager : ScriptComponent
{	
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 LOCAL REPLICATION ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void InitializeRadioFromServer()
	{
		Rpc(RpcDo_InitializeRadioFromServer);
	}
	
	//------------------------------------------------------------------------------------------------
	void ForwardDeployRequestRejected()
	{
		Rpc(RpcDo_ForwardDeployRequestRejected);
	}
	
	//------------------------------------------------------------------------------------------------
	void TeleportLocalPlayer(vector location)
	{
		Rpc(RpcDo_TeleportLocalPlayer, location);
	}

	//------------------------------------------------------------------------------------------------
	//! To a medic: a player on their faction asked for a medic (player radial menu)
	void ReceiveMedicRequest(string requesterName, string groupName, vector position)
	{
		if (IsLocallyOwned())
			RpcDo_ReceiveMedicRequest(requesterName, groupName, position);
		else
			Rpc(RpcDo_ReceiveMedicRequest, requesterName, groupName, position);
	}

	//------------------------------------------------------------------------------------------------
	//! To the requester: how many medics were alerted, or -1 if still on cooldown
	void ReceiveMedicRequestResult(int medicsReached)
	{
		if (IsLocallyOwned())
			RpcDo_ReceiveMedicRequestResult(medicsReached);
		else
			Rpc(RpcDo_ReceiveMedicRequestResult, medicsReached);
	}

	//------------------------------------------------------------------------------------------------
	//! Admin/ticket traffic is delivered only to the players allowed to see it (see
	//! COA_RplBroadcastManager), instead of being broadcast to every client and filtered there.
	//! Each handler forwards to the existing COA_RplBroadcastManager client-side logic.
	void ReceiveAdminMessage(string data, int playerID, bool ticketExists)
	{
		if (IsLocallyOwned())
			RpcDo_ReceiveAdminMessage(data, playerID, ticketExists);
		else
			Rpc(RpcDo_ReceiveAdminMessage, data, playerID, ticketExists);
	}

	//------------------------------------------------------------------------------------------------
	void ReceiveOpenTickets(int playerID, array<int> tickets)
	{
		if (IsLocallyOwned())
			RpcDo_ReceiveOpenTickets(playerID, tickets);
		else
			Rpc(RpcDo_ReceiveOpenTickets, playerID, tickets);
	}

	//------------------------------------------------------------------------------------------------
	void ReceiveTicketMessages(int playerID, array<ref COA_TicketMessageData> messages)
	{
		if (IsLocallyOwned())
			RpcDo_ReceiveTicketMessages(playerID, messages);
		else
			Rpc(RpcDo_ReceiveTicketMessages, playerID, messages);
	}

	//------------------------------------------------------------------------------------------------
	void ReceiveAdminLog(string data, int playerId, bool sendToPlayer, COA_EAdminLogLevel level)
	{
		if (IsLocallyOwned())
			RpcDo_ReceiveAdminLog(data, playerId, sendToPlayer, level);
		else
			Rpc(RpcDo_ReceiveAdminLog, data, playerId, sendToPlayer, level);
	}

	//------------------------------------------------------------------------------------------------
	void ReceiveAdminReply(string data, int playerId, int adminID)
	{
		if (IsLocallyOwned())
			RpcDo_ReceiveAdminReply(data, playerId, adminID);
		else
			Rpc(RpcDo_ReceiveAdminReply, data, playerId, adminID);
	}

	//------------------------------------------------------------------------------------------------
	//! True when the player owning this controller is the player on this machine (Workbench or a
	//! listen-server host). An owner RPC is not executed on the machine that sends it, so these
	//! calls are made directly instead.
	protected bool IsLocallyOwned()
	{
		PlayerController pc = PlayerController.Cast(GetOwner());
		return pc && pc.GetPlayerId() == SCR_PlayerController.GetLocalPlayerId();
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 REPLICATION METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_InitializeRadioFromServer()
	{
		COA_PlayerController pc = COA_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(SCR_PlayerController.GetLocalPlayerId()));
		
		if (pc)
			GetGame().GetCallqueue().CallLater(pc.InitializeRadios, 500, false, pc.GetLocalControlledEntity());
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ForwardDeployRequestRejected()
	{
		SCR_NotificationsComponent.GetInstance().SendLocal(SCR_NotificationsComponent.SendLocal(ENotification.FASTTRAVEL_PLAYER_LOCATION_WRONG));
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveMedicRequest(string requesterName, string groupName, vector position)
	{
		string where;
		IEntity medic = SCR_PlayerController.GetLocalControlledEntity();
		if (medic)
		{
			vector offset = position - medic.GetOrigin();
			int meters = Math.Round(vector.Distance(position, medic.GetOrigin()));
			where = string.Format("%1 m %2", meters, CompassDirection(offset));
		}

		string who = requesterName;
		if (!groupName.IsEmpty())
			who = string.Format("%1 (%2)", requesterName, groupName);

		string subtitle = who;
		if (!where.IsEmpty())
			subtitle = string.Format("%1  ·  %2", who, where);

		SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
		if (popup)
			popup.PopupMsg("MEDIC REQUESTED", 8, subtitle);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveMedicRequestResult(int medicsReached)
	{
		string text;
		if (medicsReached < 0)
			text = "Medic already requested - wait a moment";
		else if (medicsReached == 0)
			text = "No medics available on your side";
		else if (medicsReached == 1)
			text = "Medic request sent to 1 medic";
		else
			text = string.Format("Medic request sent to %1 medics", medicsReached);

		SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
		if (popup)
			popup.PopupMsg(text, 4);
	}

	//------------------------------------------------------------------------------------------------
	//! "N", "NE", ... for a world-space offset (+Z is north)
	protected static const ref array<string> COMPASS_DIRECTIONS = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};

	protected static string CompassDirection(vector offset)
	{
		float bearing = Math.Atan2(offset[0], offset[2]) * Math.RAD2DEG;
		if (bearing < 0)
			bearing += 360;

		int index = Math.Round(bearing / 45);
		return COMPASS_DIRECTIONS[index % 8];
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_TeleportLocalPlayer(vector location)
	{
		SCR_Global.TeleportPlayer(SCR_PlayerController.GetLocalPlayerId(), location);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveAdminMessage(string data, int playerID, bool ticketExists)
	{
		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.RpcDo_SendAdminMessage(data, playerID, ticketExists);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveOpenTickets(int playerID, array<int> tickets)
	{
		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.RpcDo_GetOpenTickets(playerID, tickets);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveTicketMessages(int playerID, array<ref COA_TicketMessageData> messages)
	{
		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.RpcDo_GetTicketMessages(playerID, messages);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveAdminLog(string data, int playerId, bool sendToPlayer, COA_EAdminLogLevel level)
	{
		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.RpcDo_LogAdminAction(data, playerId, sendToPlayer, level);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveAdminReply(string data, int playerId, int adminID)
	{
		// The server logs the reply once itself, so recipients never re-log it (logAction = false).
		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.RpcDo_ReplyAdminMessage(data, playerId, adminID, false);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static COA_PlayerRplToOwnerManager m_sInstance;
	void COA_PlayerRplToOwnerManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_PlayerRplToOwnerManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	//! The LOCAL player's manager. Only meaningful on a client: there is one of these per player
	//! controller, so on a server this is just whichever controller was created last. Server code
	//! must use GetForPlayer() instead.
	static COA_PlayerRplToOwnerManager GetInstance()
	{
		return m_sInstance;
	}

	//------------------------------------------------------------------------------------------------
	//! The manager on a specific player's controller. Use this on the server to send an owner RPC
	//! to one particular player.
	static COA_PlayerRplToOwnerManager GetForPlayer(int playerId)
	{
		if (playerId <= 0)
			return null;

		PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(playerId);
		if (!pc)
			return null;

		return COA_PlayerRplToOwnerManager.Cast(pc.FindComponent(COA_PlayerRplToOwnerManager));
	}

	//------------------------------------------------------------------------------------------------
	//! Tactical camera: an order from a leader for this player's squad (see COA_TacticalOrderClient)
	void ReceiveTacticalOrder(COA_ETacticalOrder order, vector position, string issuerName)
	{
		if (IsLocallyOwned())
			RpcDo_ReceiveTacticalOrder(order, position, issuerName);
		else
			Rpc(RpcDo_ReceiveTacticalOrder, order, position, issuerName);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveTacticalOrder(COA_ETacticalOrder order, vector position, string issuerName)
	{
		COA_TacticalOrderClient.Receive(order, position, issuerName);
	}
};
