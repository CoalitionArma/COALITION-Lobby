//------------------------------------------------------------------------------------------------
//! Two vanilla problems when a task (SCR_Task) is created while a player has a map open - the briefing,
//! respawn, AAR and in-game maps all load MapFullscreen.conf, which lists tasks:
//!  - OnTaskAdded reads SCR_FactionManager.SGetLocalPlayerFaction().GetFactionKey() without a null check,
//!    so a player with no faction yet (briefing, slotting, unslotted spectators) crashes.
//!  - It creates the map icon without checking IsTaskVisibleOnMap, so a list-only task still shows on a
//!    map that was open when it appeared (SCR_TaskMapUIComponent's own check is inverted and never fires).
//! Opening the map later goes through InitTaskIcons, which already checks both.
modded class SCR_MapUIElementContainer : SCR_MapUIBaseComponent
{
	//------------------------------------------------------------------------------------------------
	override protected void OnTaskAdded(SCR_Task task)
	{
		if (!task || !SCR_FactionManager.SGetLocalPlayerFaction())
			return;

		if (m_TaskSystem && !m_TaskSystem.IsTaskVisibleOnMap(task))
			return;

		super.OnTaskAdded(task);
	}
}
