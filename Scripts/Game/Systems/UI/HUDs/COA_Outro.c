class COA_Outro: ChimeraMenuBase
{
	protected static const ResourceName STATS_LAYOUT = "{7CDA3F81B4920E56}UI/layouts/HUD/Intro/COA_AARStats.layout";
	protected Widget              m_wAARStatsRoot;
	
	protected float               m_fStatsFadeElapsed;
	protected bool                m_bFadingStats;

	string SanitizeMissionName(string fullName)
	{
	    array<string> parts = {};
		
	    fullName.Split(" ", parts, true);
	
	    // Remove the first two tokens like "CRF" and "CO50"/"COTVT55"
	    if (parts.Count() > 2)
	    {
	        string cleanName;
	        for (int i = 2; i < parts.Count(); i++)
	        {
	            if (i > 2)
	                cleanName += " ";
	            cleanName += parts[i];
	        }
			cleanName.ToUpper();
	        return cleanName;
	    }
	
		fullName.ToUpper();
	    return fullName; // fallback if unexpected format
	}
	
	override void OnMenuOpen()
	{
		TextWidget.Cast(GetRootWidget().FindAnyWidget("TitleText")).SetText(SanitizeMissionName(GetGame().GetMissionName()));
		AudioSystem.SetMasterVolume(AudioSystem.SFX, 0);
		GetGame().GetCallqueue().CallLater(SubTitle, 3000, false);
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		COA_Gamemode.GetInstance().m_bIsInEndCredits = true;
	}
	
	override void OnMenuClose()
	{
		AudioSystem.SetMasterVolume(AudioSystem.SFX, 100);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.COA_Outro);
	}
	
	void SubTitle()
	{
		// Show outcome text if a winning faction was set
		COA_RplBroadcastManager rplBroadcast = COA_RplBroadcastManager.GetInstance();
		if (rplBroadcast && rplBroadcast.m_sOutroWinningFaction != "")
		{
			TextWidget outcomeTxt = TextWidget.Cast(GetRootWidget().FindAnyWidget("TitleText3"));
			if (outcomeTxt)
			{
				outcomeTxt.SetText(GetOutcomeText(rplBroadcast.m_sOutroWinningFaction));
				outcomeTxt.SetVisible(true);
			}
		}

		GetRootWidget().FindAnyWidget("TitleText1").SetVisible(true);
		GetRootWidget().FindAnyWidget("TitleText2").SetVisible(true);
	}

	protected string GetOutcomeText(string faction)
	{
		if (faction == "BLUFOR")  return "BLUFOR VICTORY";
		if (faction == "OPFOR")   return "OPFOR VICTORY";
		if (faction == "INDFOR")  return "INDFOR VICTORY";
		if (faction == "CIV")     return "CIVILIAN VICTORY";
		return "";
	}

	void Action_Exit()
	{
		// Note: Opening pause menu instead of directly exiting the game
		// because players often accidentally exit the game
		GetGame().GetCallqueue().Call(OpenPauseMenuWrap);
	}
	
	void OpenPauseMenuWrap()
	{
		ArmaReforgerScripted.OpenPauseMenu();
	}
}