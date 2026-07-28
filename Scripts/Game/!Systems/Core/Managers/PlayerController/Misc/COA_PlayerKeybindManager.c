class COA_PlayerKeybindManagerClass : ScriptComponentClass {}

class COA_PlayerKeybindManager : ScriptComponent
{		
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 MANAGER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{	
		super.OnPostInit(owner);
		
		// Register input action handlers
		GetGame().GetInputManager().AddActionListener("COA_ToggleSideReady", EActionTrigger.DOWN, ToggleSideReady);
		GetGame().GetInputManager().AddActionListener("COA_AdminForceReady", EActionTrigger.DOWN, AdminForceReady);
		GetGame().GetInputManager().AddActionListener("COA_OpenLobby", EActionTrigger.PRESSED, OpenSlottingMenu);
		GetGame().GetInputManager().AddActionListener("SwitchSpectatorUI", EActionTrigger.DOWN, UpdateHUDVisible);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 HUD/MENU METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateHUDVisible()
	{
		COA_PlayerControllerManager playerControllerManager = COA_PlayerControllerManager.GetInstance();
		
		playerControllerManager.m_bHUDVisible = !playerControllerManager.m_bHUDVisible
	}
	
	//------------------------------------------------------------------------------------------------
	//! Opens the slotting menu for player assignment
	//! \param[in] value - Input value (1.0 for pressed)
	//! \param[in] reason - Trigger reason
	protected void OpenSlottingMenu(float value = 0.0, EActionTrigger reason = 0)
	{
		if (value != 1)
			return;
		
		COA_PlayerMenuManager.GetInstance().OpenSlottingMenu()
	}	
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 READY-UP METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Toggles ready state for player's side/faction
	//! Only faction leaders can toggle ready state
	protected void ToggleSideReady()
	{
		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupManager)
			return;

		SCR_AIGroup playersGroup = groupManager.GetPlayerGroup(SCR_PlayerController.GetLocalPlayerId());
		if (!playersGroup)
			return;

		string playerName = GetGame().GetPlayerManager().GetPlayerName(SCR_PlayerController.GetLocalPlayerId());
		if (!playerName || playerName == "")
			return;

		// Only group leaders can toggle ready state
		if (playersGroup.IsPlayerLeader(SCR_PlayerController.GetLocalPlayerId()))
		{
			SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			Faction faction = factionManager.GetPlayerFaction(SCR_PlayerController.GetLocalPlayerId());
		
			if (faction.GetFactionKey() == "")
				return;
			
			COA_PlayerRplToAuthorityManager.GetInstance().ToggleSideReady(faction.GetFactionKey(), playerName, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Admin command to force ready state for all sides
	protected void AdminForceReady()
	{
		if (!SCR_Global.IsAdmin())
			return;
		
		COA_PlayerRplToAuthorityManager.GetInstance().ToggleSideReady("", 
			GetGame().GetPlayerManager().GetPlayerName(SCR_PlayerController.GetLocalPlayerId()), 
			true);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected static COA_PlayerKeybindManager m_sInstance;
	void COA_PlayerKeybindManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_PlayerKeybindManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static COA_PlayerKeybindManager GetInstance()
	{
		return m_sInstance;
	}
}