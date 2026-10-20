modded class CVON_VONGameModeComponent
{
	// REPLICATION FIX: Removed [RplProp()] from array
	// Arrays with RplProp send ENTIRE array on every change = massive bandwidth waste
	// Solution: Use RPCs to send only the delta (changed player state)
	// Old: Every toggle sent all player IDs to all clients
	// New: Each toggle sends only 8 bytes (playerId + bool) to all clients
	protected ref set<int> m_sListeningPlayers = new set<int>;
	
	// Server-side: Toggle listening state and broadcast change
	void TogglePlayerListening(int playerId, bool input)
	{
		// Only server should modify state
		if (!Replication.IsServer())
			return;
		
		// Ensure set is initialized before accessing
		if (!m_sListeningPlayers)
			m_sListeningPlayers = new set<int>;
		
		if (!input)
			m_sListeningPlayers.RemoveItem(playerId);
		else
			m_sListeningPlayers.Insert(playerId);
		
		// Send only this change to all clients (not entire array)
		Rpc(RpcDo_PlayerListeningChanged, playerId, input);
	}
	
	// RPC: Notify all clients of single player's listening state change
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_PlayerListeningChanged(int playerId, bool isListening)
	{
		// Ensure set is initialized before accessing
		if (!m_sListeningPlayers)
			m_sListeningPlayers = new set<int>;
		
		// Update local state on all clients
		if (isListening)
			m_sListeningPlayers.Insert(playerId);
		else
			m_sListeningPlayers.RemoveItem(playerId);
	}
	
	// Helper: Check if player is listening
	bool IsPlayerListening(int playerId)
	{
		// Safety check
		if (!m_sListeningPlayers)
			return false;
			
		return m_sListeningPlayers.Contains(playerId);
	}
	
	// Helper: Get all listening players (if needed)
	set<int> GetListeningPlayers()
	{
		// Safety check
		if (!m_sListeningPlayers)
			m_sListeningPlayers = new set<int>;
			
		return m_sListeningPlayers;
	}

#ifdef CVON_RADIO_LISTEN
	//------------------------------------------------------------------------------------------------
	//! Spectators may listen to the radios of the living player they follow ("Listen to their radio" on the
	//! spectator's follow card). With the mission's "hide other spectator factions" on, only their own side's.
	//! Only with a CVON build that has radio listening (CVON_RADIO_LISTEN).
	override bool CanListenToRadios(int listenerId, int targetId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		IEntity listener = playerManager.GetPlayerControlledEntity(listenerId);
		IEntity target = playerManager.GetPlayerControlledEntity(targetId);
		if (!listener || !target || !COA_EntityHelper.IsSpectator(listener) || COA_EntityHelper.IsSpectator(target))
			return false;

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (gamemode && gamemode.m_bHideOtherSpectatorFactions)
		{
			COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
			if (slottingManager && slottingManager.GetPlayerSlotFaction(listenerId) != slottingManager.GetPlayerSlotFaction(targetId))
				return false;
		}

		return true;
	}
#endif
}
