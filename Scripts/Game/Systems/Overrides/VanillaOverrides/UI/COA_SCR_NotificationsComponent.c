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
		// Check for all killfeed-related notification types
		if (notificationID == ENotification.PLAYER_DIED ||
			notificationID == ENotification.PLAYER_KILLED_PLAYER ||
			notificationID == ENotification.AI_KILLED_PLAYER ||
			notificationID == ENotification.POSSESSED_AI_DIED ||
			notificationID == ENotification.POSSESSED_AI_KILLED_PLAYER ||
			notificationID == ENotification.POSSESSED_AI_KILLED_POSSESSED_AI ||
			notificationID == ENotification.AI_KILLED_POSSESSED_AI)
			return true;
		else
			return false;
	}
}