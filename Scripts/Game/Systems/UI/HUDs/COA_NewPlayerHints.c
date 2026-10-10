//------------------------------------------------------------------------------------------------
//! One-time hints for players in their first few sessions on this machine (COA_PlayerProfile).
//! Each hint is shown at most once ever, at the moment it is relevant.
class COA_NewPlayerHints
{
	static const string SLOTTING = "slotting";
	static const string MISSION_START = "mission_start";
	static const string SPECTATOR = "spectator";

	// Players past this many sessions never see these
	protected static const int MAX_SESSIONS = 5;

	//------------------------------------------------------------------------------------------------
	//! \param[in] hintId One of the constants above
	//! \param[in] delayMs Wait before showing, e.g. to let a menu's entrance animation finish
	static void Show(string hintId, int delayMs = 0)
	{
		if (RplSession.Mode() == RplMode.Dedicated)
			return;

		if (COA_PlayerProfile.GetSessionCount() > MAX_SESSIONS || COA_PlayerProfile.HasSeenHint(hintId))
			return;

		if (delayMs > 0)
		{
			GetGame().GetCallqueue().CallLater(ShowNow, delayMs, false, hintId);
			return;
		}

		ShowNow(hintId);
	}

	//------------------------------------------------------------------------------------------------
	protected static void ShowNow(string hintId)
	{
		string text = GetText(hintId);
		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (text.IsEmpty() || !broadcastManager || COA_PlayerProfile.HasSeenHint(hintId))
			return;

		COA_PlayerProfile.MarkHintSeen(hintId);

		// Same display path the server's hints use, run locally for this player only ("[Tip]" is the card's heading)
		broadcastManager.RpcDo_SendHint("[Tip] " + text, -1, string.Empty);

		// These can fire while a full-screen lobby menu is open - keep the hint above it
		COA_PlayerControllerManager playerControllerManager = COA_PlayerControllerManager.GetInstance();
		if (playerControllerManager && playerControllerManager.m_wSavedHintWidget)
			playerControllerManager.m_wSavedHintWidget.SetZOrder(1000);
	}

	//------------------------------------------------------------------------------------------------
	protected static string GetText(string hintId)
	{
		switch (hintId)
		{
			case SLOTTING:
				return "Hover a slot to see the loadout it spawns with. Once you've played a role, the Quick slot button finds it for you next time.";

			case MISSION_START:
				return "Open the radial menu (Ctrl+F by default) to request a medic, check your gear or see your radio frequencies. Type /help in chat for commands. New to COALITION? coalitiongroup.net/getting-started";

			case SPECTATOR:
				return "Spectating: G follows your squad, Tab shows the damage report, R jumps to your last kill.";
		}

		return string.Empty;
	}
}
