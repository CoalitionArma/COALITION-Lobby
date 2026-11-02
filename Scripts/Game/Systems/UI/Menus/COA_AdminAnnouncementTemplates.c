//------------------------------------------------------------------------------------------------
//! One-click announcements in the admin menu's Hints tab (TEMPLATES list). Clicking one fills the
//! message box; it's then sent with the usual Send to player / faction / everyone buttons.
//! A leading "[Tag]" becomes the hint card's heading (see COA_Hint). Edit the list below to change them.
class COA_AdminAnnouncementTemplates
{
	//------------------------------------------------------------------------------------------------
	static void GetTemplates(notnull array<string> outTemplates)
	{
		outTemplates.Clear();

		outTemplates.Insert("[Safe Start] Safe start ends in 2 minutes - get ready.");
		outTemplates.Insert("[Safe Start] Safe start ends in 30 seconds.");
		outTemplates.Insert("[Command] Regroup at the FOB.");
		outTemplates.Insert("[Command] Hold fire.");
		outTemplates.Insert("[Mission] Mission ending in 10 minutes.");
		outTemplates.Insert("[Mission] Mission ending in 5 minutes.");
		outTemplates.Insert("[Mission] Final push - last minute of the mission.");
		outTemplates.Insert("[Admin] An admin is checking a report - please stand by.");
		outTemplates.Insert("[Admin] Pause - stay where you are until the admin says go.");
		outTemplates.Insert("[Admin] Resume - carry on.");
	}
}
