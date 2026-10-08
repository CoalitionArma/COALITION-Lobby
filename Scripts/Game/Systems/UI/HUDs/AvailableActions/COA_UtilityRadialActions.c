class COA_UtilityRadialActionsClass : ScriptComponentClass
{

}

//! Player radial menu entries for everyday questions: call a medic, which frequencies am I on,
//! am I missing any of my kit. Added next to COA_PlaceRallyPointAction under COA_PlayerRadialMenuManager.
class COA_UtilityRadialActions : ScriptComponent
{
	protected ref SCR_SelectionMenuEntry m_RequestMedic;
	protected ref SCR_SelectionMenuEntry m_ShowFrequencies;
	protected ref SCR_SelectionMenuEntry m_CheckGear;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		COA_PlayerRadialMenuManager radialMenu = COA_PlayerRadialMenuManager.Cast(owner.FindComponent(COA_PlayerRadialMenuManager));
		if (!radialMenu)
			return;

		m_RequestMedic = new SCR_SelectionMenuEntry();
		m_RequestMedic.SetId("support_request_medic");
		m_RequestMedic.SetName("Request medic");

		m_ShowFrequencies = new SCR_SelectionMenuEntry();
		m_ShowFrequencies.SetId("info_frequencies");
		m_ShowFrequencies.SetName("My frequencies");

		m_CheckGear = new SCR_SelectionMenuEntry();
		m_CheckGear.SetId("info_check_gear");
		m_CheckGear.SetName("Check my gear");

		radialMenu.RegisterEntry("Support", m_RequestMedic);
		radialMenu.RegisterEntry("Info", m_ShowFrequencies);
		radialMenu.RegisterEntry("Info", m_CheckGear);
		radialMenu.GetOnBeforeOpen().Insert(UpdateAvailability);
		radialMenu.GetOnActionPerformed().Insert(OnPerformAction);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateAvailability()
	{
		IEntity character = SCR_PlayerController.GetLocalControlledEntity();
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		bool playing = character && gamemode && gamemode.m_GamemodeState == COA_EGamemodeState.GAME
			&& !COA_EntityHelper.IsSpectator(character) && COA_DamageHelper.CheckIfEntityAlive(character);

		m_RequestMedic.Enable(playing);
		m_ShowFrequencies.Enable(playing && CVON_VONGameModeComponent.GetInstance() != null);
		m_CheckGear.Enable(playing);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPerformAction(SCR_SelectionMenuEntry entry)
	{
		if (entry == m_RequestMedic)
		{
			COA_PlayerRplToAuthorityManager.GetInstance().RequestMedic();
			return;
		}

		if (entry == m_ShowFrequencies)
		{
			COA_PlayerChatCommandManager chatCommands = COA_PlayerChatCommandManager.GetInstance();
			if (chatCommands)
				chatCommands.ShowFrequencies(null, string.Empty);
			return;
		}

		if (entry == m_CheckGear)
			CheckGear();
	}

	//------------------------------------------------------------------------------------------------
	//! Popup with the result, and the list of missing items in chat
	protected void CheckGear()
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		IEntity character = SCR_PlayerController.GetLocalControlledEntity();
		SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
		if (!slottingManager || !character || !popup)
			return;

		COA_SlotData slotData = slottingManager.GetPlayerSlotData(SCR_PlayerController.GetLocalPlayerId());
		array<string> missing = {};
		if (!slotData || !COA_GearCheckHelper.FindMissing(character, slotData.GetSlotFactionKey(), slotData.GetSlotRole(), missing))
		{
			popup.PopupMsg("Gear check unavailable", 4);
			return;
		}

		if (missing.IsEmpty())
		{
			popup.PopupMsg("Gear check: nothing missing", 4);
			return;
		}

		if (missing.Count() == 1)
			popup.PopupMsg("Gear check: missing 1 item", 5, missing[0]);
		else
			popup.PopupMsg(string.Format("Gear check: missing %1 items", missing.Count()), 5, "See chat for the list");

		PlayerController pc = GetGame().GetPlayerController();
		SCR_ChatComponent chatComponent;
		if (pc)
			chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));

		if (!chatComponent)
			return;

		chatComponent.ShowMessage(string.Format("Missing from your %1 loadout:", slotData.GetSlotName()));
		foreach (string item : missing)
			chatComponent.ShowMessage("  " + item);
	}
}
