//----------------------------------------------------------------
// Kill feed suppression for admins: an admin who is alive and playing
// (not in Game Master, not spectating) doesn't get the kill feed, so it
// can't give them information players don't have. Spectating and
// Game Master admins see it like everyone else in those modes.
// Used by SCR_NotificationsComponent and SCR_NotificationSenderComponent.
//----------------------------------------------------------------
class COA_AdminKillfeed
{
	static bool ShouldSuppressForLocalPlayer()
	{
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (!editorManager || editorManager.IsLimited())
			return false;

		// Game Master open - show
		if (editorManager.IsOpened())
			return false;

		// Spectating - show. Spectators control a COA_SpectatorCharacter, so "has a controlled
		// entity" alone is not "alive" (that check is what hid it from spectating admins).
		if (COA_EntityHelper.IsSpectator())
			return false;

		// No body at all (between states) - nothing to protect
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc || !pc.GetControlledEntity())
			return false;

		// Alive and playing - suppress
		return true;
	}

	//----------------------------------------------------------------
	//! The vanilla notification log shows kills only to a Game Master with the editor open. Never to
	//! spectators (they have the COALITION kill feed) or to an admin between dying and spectating -
	//! vanilla sends unlimited-editor accounts the full feed even then.
	static bool ShowKillsInVanillaLog()
	{
		if (COA_EntityHelper.IsSpectator())
			return false;

		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		return editorManager && !editorManager.IsLimited() && editorManager.IsOpened();
	}

	//----------------------------------------------------------------
	//! Notifications that make up the vanilla kill feed - every death and kill notification
	static bool IsKillfeedNotification(ENotification notificationID)
	{
		switch (notificationID)
		{
			case ENotification.PLAYER_DIED:
			case ENotification.PLAYER_KILLED_PLAYER:
			case ENotification.AI_KILLED_PLAYER:
			case ENotification.POSSESSED_AI_DIED:
			case ENotification.POSSESSED_AI_KILLED_PLAYER:
			case ENotification.PLAYER_KILLED_POSSESSED_AI:
			case ENotification.AI_KILLED_POSSESSED_AI:
			case ENotification.POSSESSED_AI_KILLED_POSSESSED_AI:
			case ENotification.PLAYER_KILLED_BY_EDITOR:
			case ENotification.PLAYER_KILLED_BY_EDITOR_PLAYER:
			case ENotification.POSSESSED_AI_KILLED_BY_EDITOR:
			case ENotification.POSSESSED_AI_KILLED_BY_EDITOR_PLAYER:
			case ENotification.AI_IN_GROUP_DIED:
			case ENotification.PLAYER_KILLED_AI_IN_GROUP:
			case ENotification.AI_KILLED_AI_IN_GROUP:
			case ENotification.POSSESSED_AI_KILLED_AI_IN_GROUP:
				return true;
		}

		return false;
	}
}

modded class SCR_NotificationsComponent
{
	//----------------------------------------------------------------
	private static bool IsLocalPlayerUnlimitedEditor()
	{
		return COA_AdminKillfeed.ShouldSuppressForLocalPlayer();
	}
	
	//----------------------------------------------------------------
	// Allow killfeed through for zeus-open admins; block everything else
	//----------------------------------------------------------------
	override static bool SendLocalUnlimitedEditor(ENotification notificationID, vector position, int param1 = 0, int param2 = 0, int param3 = 0, int param4 = 0, int param5 = 0, int param6 = 0)
	{
		if (IsKillfeedNotification(notificationID))
			return super.SendLocalUnlimitedEditor(notificationID, position, param1, param2, param3, param4, param5, param6);
		return false;
	}
	
	//----------------------------------------------------------------
	// Override SendLocal to block killfeed for admins who are alive
	// and playing without zeus open. Normal players are unaffected -
	// their killfeed is already disabled by base game config and
	// IsLocalPlayerUnlimitedEditor() returns false for non-admins.
	//----------------------------------------------------------------
	override static bool SendLocal(ENotification notificationID, vector position, int param1 = 0, int param2 = 0, int param3 = 0, int param4 = 0, int param5 = 0, int param6 = 0)
	{
		if (IsKillfeedNotification(notificationID) && IsLocalPlayerUnlimitedEditor())
			return false;
		
		return super.SendLocal(notificationID, position, param1, param2, param3, param4, param5, param6);
	}
	
	//----------------------------------------------------------------
	// Helper method to identify killfeed-related notifications
	//----------------------------------------------------------------
	private static bool IsKillfeedNotification(ENotification notificationID)
	{
		return COA_AdminKillfeed.IsKillfeedNotification(notificationID);
	}
}