//----------------------------------------------------------------
// The vanilla notification log draws kills only for a Game Master with the
// editor open (COA_AdminKillfeed.ShowKillsInVanillaLog). Spectators get the
// COALITION kill feed (COA_SpectatorMenu) instead - the kill notifications
// still arrive, the spectator's own feed is built from them, they just aren't
// drawn here - and an admin between dying and spectating sees none.
//----------------------------------------------------------------
modded class SCR_NotificationsLogComponent
{
	override protected bool OnNotification(SCR_NotificationData data)
	{
		if (data && COA_AdminKillfeed.IsKillfeedNotification(data.GetID()) && !COA_AdminKillfeed.ShowKillsInVanillaLog())
			return false;

		return super.OnNotification(data);
	}
}
