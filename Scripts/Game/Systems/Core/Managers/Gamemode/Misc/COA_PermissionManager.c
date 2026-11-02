class COA_PermissionManagerClass : ScriptComponentClass {}

class COA_PermissionManager : ScriptComponent
{	
	[RplProp()]
	ref array<int> m_aModerators = {}; 
	
	[RplProp()]
	ref array<int> m_aDonators = {};

	//! Connected players with the ADMINISTRATOR or SESSION_ADMINISTRATOR role. Kept by the server
	//! (role changes, joins, leaves) so clients can list admins without relying on role replication.
	[RplProp()]
	protected ref array<int> m_aAdmins = {};

//=============================================================================================================================================================================================================================================================================================================================================================
//	 ADMIN TRACKING
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Every machine subscribes; the handlers only act on the server
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(owner);
		if (!gameMode)
			return;

		gameMode.GetOnPlayerRoleChange().Insert(OnPlayerRoleChanged);
		gameMode.GetOnPlayerConnected().Insert(OnPlayerConnected);
		gameMode.GetOnPlayerDisconnected().Insert(OnPlayerDisconnected);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlayerRoleChanged(int playerId, EPlayerRole roleFlags)
	{
		RefreshAdmins(-1);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlayerConnected(int playerId)
	{
		RefreshAdmins(-1);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		RefreshAdmins(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: rebuild the admin list from the connected players and replicate it if it changed
	//! \param[in] leavingPlayerId Player who is disconnecting (may still be listed), or -1
	protected void RefreshAdmins(int leavingPlayerId)
	{
		if (!Replication.IsServer())
			return;

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);

		array<int> admins = {};
		foreach (int playerId : playerIds)
		{
			if (playerId != leavingPlayerId && SCR_Global.IsAdmin(playerId))
				admins.Insert(playerId);
		}

		bool changed = admins.Count() != m_aAdmins.Count();
		if (!changed)
		{
			foreach (int adminId : admins)
			{
				if (!m_aAdmins.Contains(adminId))
				{
					changed = true;
					break;
				}
			}
		}

		if (!changed)
			return;

		m_aAdmins.Copy(admins);
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	//! Any machine: whether a player is a connected admin (replicated list, or the local role check)
	bool IsAdminOnline(int playerId)
	{
		return m_aAdmins.Contains(playerId) || SCR_Global.IsAdmin(playerId);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 PERMISSION GETTERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Check if a given player is a moderator
	//! \param[in] playerId ID of the player to check
	//! \return True if player is a moderator, false otherwise
	bool IsModerator(int playerId)
	{
		return m_aModerators.Contains(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if local player is a moderator
	//! \return True if local player is a moderator, false otherwise
	bool IsModerator()
	{
		return m_aModerators.Contains(SCR_PlayerController.GetLocalPlayerId());
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if local player is a donator
	//! \param[in] playerId ID of the player to check
	//! \return True if local player is a donator, false otherwise
	bool IsDonator(int playerId)
	{
		return m_aDonators.Contains(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if local player is a donator
	//! \return True if local player is a donator, false otherwise
	bool IsDonator()
	{
		return m_aDonators.Contains(SCR_PlayerController.GetLocalPlayerId());
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATUS UPDATE METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Set a player status
	//! \param[in] playerId ID of the player to set as moderator or donator
	void SetPlayerStatus(int playerId, string role)
	{
		if (!Replication.IsServer())
			return;
		
		if (m_aModerators.Contains(playerId) || m_aDonators.Contains(playerId))
			return;
		
		bool statusChanged = false;
		switch (role) {
			case "mod": {
				m_aModerators.Insert(playerId);
				statusChanged = true;
				break;
			}
			case "don": {
				m_aDonators.Insert(playerId);
				statusChanged = true;
				break;
			}
		}
			
		if (statusChanged)
			Replication.BumpMe();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	static protected COA_PermissionManager m_sInstance;
	void COA_PermissionManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	static COA_PermissionManager GetInstance()
	{
		return m_sInstance;
	}
}