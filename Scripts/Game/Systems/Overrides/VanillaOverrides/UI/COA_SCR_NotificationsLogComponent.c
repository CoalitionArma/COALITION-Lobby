//----------------------------------------------------------------
// Spectators get the COALITION kill feed (COA_SpectatorMenu), so the vanilla
// notification log doesn't show kills on top of it. The kill notifications
// still arrive - the spectator's own feed is built from them - they just
// aren't drawn in the vanilla log while spectating.
//----------------------------------------------------------------
modded class SCR_NotificationsLogComponent
{
	override protected bool OnNotification(SCR_NotificationData data)
	{
		if (data && COA_AdminKillfeed.IsKillfeedNotification(data.GetID()) && COA_EntityHelper.IsSpectator())
			return false;

		return super.OnNotification(data);
	}
}
