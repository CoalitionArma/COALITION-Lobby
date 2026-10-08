//------------------------------------------------------------------------------------------------
// Per-player data kept on the player's own machine ($profile: on the client): the roles they
// played recently (slotting menu quick slot) and how many sessions they have joined (new-player
// hints). Nothing here is sent to the server - it only drives local UI conveniences.
//------------------------------------------------------------------------------------------------
class COA_PlayerProfile
{
	protected static const string FILE_PATH = "$profile:COA_PlayerProfile.json";
	protected static const int MAX_RECENT_ROLES = 5;

	protected static ref COA_PlayerProfileData s_Data;
	protected static bool s_bSessionCounted;

	//------------------------------------------------------------------------------------------------
	//! Most recent first
	static array<int> GetRecentRoles()
	{
		return GetData().m_aRecentRoles;
	}

	//------------------------------------------------------------------------------------------------
	//! Moves role to the front of the recent list (deduplicated) and saves
	static void RecordRole(COA_EGearRole role)
	{
		if (role == COA_EGearRole.UNARMED || role == COA_EGearRole.ZEUS)
			return;

		COA_PlayerProfileData data = GetData();
		data.m_aRecentRoles.RemoveItemOrdered(role);
		data.m_aRecentRoles.InsertAt(role, 0);

		while (data.m_aRecentRoles.Count() > MAX_RECENT_ROLES)
			data.m_aRecentRoles.Remove(data.m_aRecentRoles.Count() - 1);

		Save();
	}

	//------------------------------------------------------------------------------------------------
	//! Records the local player's current slot role. Called on clients when the round goes live.
	static void RecordLocalPlayerRole()
	{
		if (RplSession.Mode() == RplMode.Dedicated)
			return;

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return;

		COA_SlotData slotData = slottingManager.GetPlayerSlotData(SCR_PlayerController.GetLocalPlayerId());
		if (slotData)
			RecordRole(slotData.GetSlotRole());
	}

	//------------------------------------------------------------------------------------------------
	//! Sessions joined on this machine, counting the current one
	static int GetSessionCount()
	{
		COA_PlayerProfileData data = GetData();
		if (!s_bSessionCounted)
		{
			s_bSessionCounted = true;
			data.m_iSessions++;
			Save();
		}

		return data.m_iSessions;
	}

	//------------------------------------------------------------------------------------------------
	static bool HasSeenHint(string hintId)
	{
		return GetData().m_aSeenHints.Contains(hintId);
	}

	//------------------------------------------------------------------------------------------------
	static void MarkHintSeen(string hintId)
	{
		COA_PlayerProfileData data = GetData();
		if (data.m_aSeenHints.Contains(hintId))
			return;

		data.m_aSeenHints.Insert(hintId);
		Save();
	}

	//------------------------------------------------------------------------------------------------
	protected static COA_PlayerProfileData GetData()
	{
		if (s_Data)
			return s_Data;

		s_Data = new COA_PlayerProfileData();

		if (FileIO.FileExists(FILE_PATH))
		{
			JsonLoadContext loadContext = new JsonLoadContext();
			if (!loadContext.LoadFromFile(FILE_PATH) || !loadContext.ReadValue("", s_Data))
			{
				Print("[COA_PlayerProfile] Could not read " + FILE_PATH + " - starting fresh", LogLevel.WARNING);
				s_Data = new COA_PlayerProfileData();
			}
		}

		// Files written by older versions may lack newer fields
		if (!s_Data.m_aRecentRoles)
			s_Data.m_aRecentRoles = {};
		if (!s_Data.m_aSeenHints)
			s_Data.m_aSeenHints = {};

		return s_Data;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Save()
	{
		JsonSaveContext saveContext = new JsonSaveContext();
		saveContext.WriteValue("", s_Data);
		if (!saveContext.SaveToFile(FILE_PATH))
			Print("[COA_PlayerProfile] Could not write " + FILE_PATH, LogLevel.WARNING);
	}
}

//------------------------------------------------------------------------------------------------
class COA_PlayerProfileData
{
	ref array<int> m_aRecentRoles = {};
	int m_iSessions;
	ref array<string> m_aSeenHints = {};
}
