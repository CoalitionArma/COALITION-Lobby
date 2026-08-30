class COA_PlayerRedialMenuManagerClass : ScriptComponentClass
{

}

class COA_PlayerRedialMenuAction
{
    string catagoryID;
    SCR_SelectionMenuEntry entry;
}

class COA_PlayerRedialMenuManager : ScriptComponent
{
    [Attribute()]
    protected ref SCR_RadialMenuController m_RadialMenuController;

    protected SCR_RadialMenu m_RadialMenu;

    protected ref array<ref COA_PlayerRedialMenuAction> m_aRegisteredActions = {};

    protected ref ScriptInvoker<SCR_SelectionMenuEntry> m_OnActionPerformed = new ScriptInvoker<SCR_SelectionMenuEntry>();
    protected ref ScriptInvoker m_OnBeforeOpen = new ScriptInvoker();

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        if (!m_RadialMenuController)
            return;

        m_RadialMenuController.GetOnTakeControl().Insert(OnTakeControl);
        m_RadialMenuController.GetOnControllerChanged().Insert(OnControllerChanged);

        GetGame().GetInputManager().AddActionListener("COA_OpenPlayerRedialMenu", EActionTrigger.DOWN, OpenRadialMenu);
    }

    //------------------------------------------------------------------------------------------------
    ScriptInvoker<SCR_SelectionMenuEntry> GetOnActionPerformed()
    {
        return m_OnActionPerformed;
    }

    //------------------------------------------------------------------------------------------------
    ScriptInvoker GetOnBeforeOpen()
    {
        return m_OnBeforeOpen;
    }

    //------------------------------------------------------------------------------------------------
    protected void OnTakeControl(SCR_RadialMenuController controller)
    {
        if (GetGame().GetPlayerController() != GetOwner())
            return;

        m_RadialMenu = controller.GetRadialMenu();
        if (!m_RadialMenu)
            return;

        m_RadialMenu.GetOnPerform().Insert(OnActionPerformed);
        m_RadialMenu.GetOnBeforeOpen().Insert(OnBeforeOpen);
    }

    //------------------------------------------------------------------------------------------------
    protected void OnControllerChanged(SCR_RadialMenuController controller, bool hasControl)
    {
        if (hasControl || !m_RadialMenu)
            return;

        m_RadialMenu.GetOnPerform().Remove(OnActionPerformed);
        m_RadialMenu.GetOnBeforeOpen().Remove(OnBeforeOpen);
        m_RadialMenu = null;
    }

    //------------------------------------------------------------------------------------------------
    protected void OpenRadialMenu()
    {
        if (!m_RadialMenuController)
            return;

        if (m_RadialMenuController.IsMenuOpen())
        {
            m_RadialMenuController.CloseMenu();
            return;
        }

        m_RadialMenuController.OnInputOpen();
    }

    //------------------------------------------------------------------------------------------------
    protected void OnActionPerformed(SCR_SelectionMenu menu, SCR_SelectionMenuEntry action)
    {
        if (!action)
            return;
        
        m_OnActionPerformed.Invoke(action);
         
        m_RadialMenu.Close();
    }

    //------------------------------------------------------------------------------------------------
    protected void OnBeforeOpen()
    {
        if (!m_RadialMenu)
            return;

        m_OnBeforeOpen.Invoke();

        m_RadialMenu.ClearEntries();
        BuildMenu();
    }

    //------------------------------------------------------------------------------------------------
    protected void BuildMenu()
    {
        if (!m_RadialMenu)
            return;

        array<ref SCR_SelectionMenuCategoryEntry> categories = {};

        foreach (COA_PlayerRedialMenuAction action : m_aRegisteredActions)
        {
            if (!action.entry || !action.entry.IsEnabled())
                continue;

            SCR_SelectionMenuCategoryEntry catagory;
            
            foreach (SCR_SelectionMenuCategoryEntry existingCategory : categories)
            {
                if (existingCategory.GetId() == action.catagoryID)
                {
                    catagory = existingCategory;
                    break;
                }
            }

            if (!catagory)
            {
                catagory = new SCR_SelectionMenuCategoryEntry();
                catagory.SetId(action.catagoryID);
                catagory.SetName(action.catagoryID);
                
                categories.Insert(catagory);
                
                m_RadialMenu.AddCategoryEntry(catagory);
            }
            
            catagory.AddEntry(action.entry);
        }
    }

    //------------------------------------------------------------------------------------------------
    void RegisterEntry(string catagoryName, SCR_SelectionMenuEntry entry)
    {
        COA_PlayerRedialMenuAction action = new COA_PlayerRedialMenuAction();
        action.catagoryID = catagoryName;
        action.entry = entry;

        m_aRegisteredActions.Insert(action);
    }
    
    //------------------------------------------------------------------------------------------------
    void ~COA_PlayerRedialMenuManager()
    {
        GetGame().GetInputManager().RemoveActionListener("COA_OpenPlayerRedialMenu", EActionTrigger.DOWN, OpenRadialMenu);

        if (m_RadialMenuController)
        {
            m_RadialMenuController.GetOnTakeControl().Remove(OnTakeControl);
            m_RadialMenuController.GetOnControllerChanged().Remove(OnControllerChanged);
        }
    }

}