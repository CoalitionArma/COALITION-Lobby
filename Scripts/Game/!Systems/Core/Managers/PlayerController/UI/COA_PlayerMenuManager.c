class COA_PlayerMenuManagerClass : ScriptComponentClass {}

class COA_PlayerMenuManager : ScriptComponent
{		
//=============================================================================================================================================================================================================================================================================================================================================================
//	 MENU ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Opens appropriate menu based on current gamemode state
	void OpenCurrentStateMenu()
	{	
		// Initialize references first
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		
		// Check if we should skip AAR
		if (gamemode && gamemode.m_GamemodeState == COA_EGamemodeState.AAR)
			return;
		
		// Close any existing menus
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu)
			topMenu.Close();
		GetGame().GetMenuManager().CloseAllMenus();
		
		// Open appropriate menu based on gamemode state
		switch (gamemode.m_GamemodeState)
		{
			case COA_EGamemodeState.BRIEFING: 
			{
				COA_PlayerCameraManager.GetInstance().SetCameraOnRailsOrbit(gamemode.GetGenericSpawn(), 850, 225, 1);
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.COA_PreviewMenu);
				break;
			}
			case COA_EGamemodeState.SLOTTING:
			{
				COA_PlayerCameraManager.GetInstance().SetCameraOnRailsOrbit(gamemode.GetGenericSpawn(), 850, 225, 1);
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.COA_SlottingMenu);
				break;
			}
			case COA_EGamemodeState.GAME: 
			{
				COA_PlayerRplToAuthorityManager.GetInstance().RequestInitilizePlayer(SCR_PlayerController.GetLocalPlayerId());
				break;
			}
			case COA_EGamemodeState.AAR: 
			{
				break;
			}
		}
	}	

	//------------------------------------------------------------------------------------------------
	//! Opens the slotting menu for player assignment
	void OpenSlottingMenu()
	{
		// Check if appropriate menu is already open
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu)
		{
			if (topMenu.IsInherited(COA_PreviewMenu) || topMenu.IsInherited(COA_SlottingMenu))
				return;
			else if (topMenu.IsInherited(COA_SpectatorMenu))
				GetGame().GetMenuManager().CloseMenu(topMenu);
		}

		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.COA_SlottingMenu);
	}	
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 TITLE CARD
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	void DisplayTitleCard()
	{
		Widget titleCard = GetGame().GetWorkspace().CreateWidgets("{4D2AE199F111C14A}UI/layouts/HUD/Intro/COA_Intro.layout");
		TextWidget.Cast(titleCard.FindAnyWidget("TitleText")).SetText(COA_MissionHelper.SanitizeMissionName(GetGame().GetMissionName()));
		AudioSystem.PlaySound("{932C08A5A988F96A}Sounds/Intro/cinematicBoom.wav");
		GetGame().GetCallqueue().CallLater(RemoveTitleCardWidget, 4000, false, titleCard);
	}
	
	//------------------------------------------------------------------------------------------------
	static void RemoveTitleCardWidget(Widget widget)
	{
		if (widget)
			widget.RemoveFromHierarchy();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected static COA_PlayerMenuManager m_sInstance;
	void COA_PlayerMenuManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_PlayerMenuManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static COA_PlayerMenuManager GetInstance()
	{
		return m_sInstance;
	}
}