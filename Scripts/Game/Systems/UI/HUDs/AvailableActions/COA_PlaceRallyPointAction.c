class COA_PlaceRallyPointActionClass : ScriptComponentClass
{

}

class COA_PlaceRallyPointAction : ScriptComponent
{
    protected ref SCR_SelectionMenuEntry m_Action;

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        COA_PlayerRadialMenuManager radialMenu = COA_PlayerRadialMenuManager.Cast(owner.FindComponent(COA_PlayerRadialMenuManager));
        if (!radialMenu)
            return;

        m_Action = new SCR_SelectionMenuEntry();
        m_Action.SetId("placeables_rallypoint");
        m_Action.SetName("Rallypoint");

        radialMenu.RegisterEntry("Placeables", m_Action);
        radialMenu.GetOnBeforeOpen().Insert(UpdateAvailability);
        radialMenu.GetOnActionPerformed().Insert(OnPerformAction)
    }

    //------------------------------------------------------------------------------------------------
    protected void UpdateAvailability()
    {
        COA_Gamemode gamemode = COA_Gamemode.GetInstance();
        if (!gamemode || !m_Action)
            return;

        IEntity character = SCR_PlayerController.GetLocalControlledEntity();
        if (!character)
            return;

        m_Action.Enable(gamemode.m_bRallyPointsEnabled && COA_RoleHelper.IsSquadLeaderRole(character));
    }

    //------------------------------------------------------------------------------------------------
    protected void OnPerformAction()
    {
        
        COA_Gamemode gamemode = COA_Gamemode.GetInstance();
        if (!gamemode || !gamemode.m_bRallyPointsEnabled)
            return;

        int playerId = SCR_PlayerController.GetLocalPlayerId();
        if (playerId == -1)
            return; 
        
       COA_PlayerRplToAuthorityManager.GetInstance().RequestPlaceRallypoint(playerId);
    }
}