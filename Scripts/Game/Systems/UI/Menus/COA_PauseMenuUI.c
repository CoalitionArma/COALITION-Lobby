class COA_PauseMenuUI: PauseMenuUI
{
	// Mission info card on the right (mission, objectives, time left, admins online, rules)
	protected ref COA_MissionInfoPanel m_MissionInfoPanel;

	//--------------------------------------------------------------------------------------------------
	// Overridden method that's called when the pause menu is opened
	//--------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		// Call the parent implementation first
		super.OnMenuOpen();

		m_MissionInfoPanel = new COA_MissionInfoPanel();
		if (!m_MissionInfoPanel.Init(GetRootWidget()))
			m_MissionInfoPanel = null;

		// Get the admin menu button and frame
		SCR_ButtonTextComponent adminButton = SCR_ButtonTextComponent.GetButtonText("CoalAdminMenuButton", GetRootWidget());
		FrameWidget adminFrame = FrameWidget.Cast(GetRootWidget().FindAnyWidget("CoalAdminMenu"));
		
		// Validate widget references
		if (!adminButton || !adminFrame)
		{
			Print("COA_PauseMenuUI: Could not find admin menu UI elements", LogLevel.WARNING);
			return;
		}
		
		// Check if the current player has admin permissions
		bool hasAdminAccess = false;
		COA_PermissionManager permissionManager = COA_PermissionManager.GetInstance();
		
		// Check admin/moderator permissions and required game instances
		if (SCR_Global.IsAdmin() || (permissionManager && permissionManager.IsModerator() && COA_Gamemode.GetInstance()))
			hasAdminAccess = true;
		
		// Set visibility based on permissions
		adminFrame.SetVisible(hasAdminAccess);
		
		// Only register click handler if the admin menu is visible
		if (hasAdminAccess)
			adminButton.m_OnClicked.Insert(OpenAdminMenu);
	}
	
	//--------------------------------------------------------------------------------------------------
	// Keeps the mission info card's timer and admin list current
	//--------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);

		if (m_MissionInfoPanel)
			m_MissionInfoPanel.Update(tDelta);
	}

	//--------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		if (m_MissionInfoPanel)
		{
			m_MissionInfoPanel.Clear();
			m_MissionInfoPanel = null;
		}

		super.OnMenuClose();
	}

	//--------------------------------------------------------------------------------------------------
	// Opens the admin menu when the admin button is clicked
	//--------------------------------------------------------------------------------------------------
	void OpenAdminMenu()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.COA_AdminMenu);
	}
}