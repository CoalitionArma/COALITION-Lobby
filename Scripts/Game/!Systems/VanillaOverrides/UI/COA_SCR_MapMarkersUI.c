modded class SCR_PlayerControllerCommandingComponent
{
	override bool AddElementsFromCategoryToMap(notnull SCR_PlayerCommandingMenuCategoryElement category, SCR_SelectionMenuCategoryEntry parentCategory = null)
	{
		if (!COA_Gamemode.GetInstance())
		{
			return super.AddElementsFromCategoryToMap(category, parentCategory);
		}
		
		//Buh...
		COA_GamemodeManager gamemodeManager = COA_GamemodeManager.GetInstance();
		if (!gamemodeManager)
			return super.AddElementsFromCategoryToMap(category, parentCategory);
		
		int playerId = SCR_PlayerController.GetLocalPlayerId();
		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsManager)
			return super.AddElementsFromCategoryToMap(category, parentCategory);

		SCR_AIGroup playerGroup = groupsManager.GetPlayerGroup(playerId);
		if (!playerGroup)
		    return super.AddElementsFromCategoryToMap(category, parentCategory);
		
		COA_SafestartManager safestartManager = COA_SafestartManager.GetInstance();
		if (!safestartManager)
		    return super.AddElementsFromCategoryToMap(category, parentCategory);
		
		if (!playerGroup.IsPlayerLeader(playerId) || safestartManager.GetSafestartStatus())
		    return super.AddElementsFromCategoryToMap(category, parentCategory);
		
		COA_ForwardDeployManager forwardDeployManager = COA_ForwardDeployManager.GetInstance();
		Faction playerFaction = playerGroup.GetFaction();
		if (!forwardDeployManager || !playerFaction || !forwardDeployManager.IsForwardDeployActive(playerFaction.GetFactionKey()))
			return super.AddElementsFromCategoryToMap(category, parentCategory);
		
		SCR_MapMarkerMenuEntry menuEntry = new SCR_MapMarkerMenuEntry();
		menuEntry.SetName("Forward Deploy Element");
		menuEntry.GetOnPerform().Insert(CheckIfValidSpawn);
		menuEntry.SetIcon("{F7E8D4834A3AFF2F}UI/Imagesets/Conflict/conflict-icons-bw.imageset", "RespawnSmall");
		
		m_MapContextualMenu.InsertCustomRadialEntry(menuEntry, parentCategory);
		return super.AddElementsFromCategoryToMap(category, parentCategory);
	}
	
	void CheckIfValidSpawn()
	{
	    int playerId = SCR_PlayerController.GetLocalPlayerId();
	    Faction faction = SCR_FactionManager.SGetPlayerFaction(playerId);
	    if (!faction)
	        return;
	    
	    string factionKey = faction.GetFactionKey();
	    COA_PlayerRplToAuthorityManager rplManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (rplManager && m_MapContextualMenu)
			rplManager.RequestForwardDeploy(m_MapContextualMenu.GetMenuWorldPosition(), factionKey, playerId);
	}
}
