class COA_PlayerRadialMenuManagerClass : ScriptComponentClass
{

}

class COA_PlayerRadialMenuAction
{
    string catagoryID;
    SCR_SelectionMenuEntry entry;
}

//! see COA_PlaceRallyPointAction for example of usage
class COA_PlayerRadialMenuManager : ScriptComponent
{
    [Attribute()]
    protected ref SCR_RadialMenuController m_RadialMenuController;

    protected SCR_RadialMenu m_RadialMenu;

    protected ref array<ref COA_PlayerRadialMenuAction> m_aRegisteredActions = {};

    protected ref ScriptInvoker<SCR_SelectionMenuEntry> m_OnActionPerformed = new ScriptInvoker<SCR_SelectionMenuEntry>();
    protected ref ScriptInvoker m_OnBeforeOpen = new ScriptInvoker();

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        if (!m_RadialMenuController)
            return;

        m_RadialMenuController.GetOnTakeControl().Insert(OnTakeControl);
        m_RadialMenuController.GetOnControllerChanged().Insert(OnControllerChanged);

        GetGame().GetInputManager().AddActionListener("COA_OpenPlayerRadialMenu", EActionTrigger.DOWN, OpenRadialMenu);
    }

    //------------------------------------------------------------------------------------------------
    //! Returns the callback invoker triggered when a radial menu option is selected.
    //! Used by external systems to react to a player choosing an item from the redial menu.
    //! \return ScriptInvoker for the selected menu entry.
    ScriptInvoker<SCR_SelectionMenuEntry> GetOnActionPerformed()
    {
        return m_OnActionPerformed;
    }

    //------------------------------------------------------------------------------------------------
    //! Returns the callback invoker triggered immediately before the radial menu is populated.
    //! Used to prepare or refresh menu entries before the redial menu is opened.
    //! \return ScriptInvoker invoked before the menu is built.
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

        foreach (COA_PlayerRadialMenuAction action : m_aRegisteredActions)
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
    //! Adds a menu option to the player radial menu.
    //! This registers a single entry for a category and makes it available when the redial menu is built.
    //! Use this to add quick actions, toggles, or utility entries to the player's radial menu.
    //! \param[in] catagoryName Name of the category/group the entry should belong to. Entries with the same category name are grouped together under one radial menu section.
    //! \param[in] entry The SCR_SelectionMenuEntry to register. It will be displayed and invoked when the user selects it from the radial menu.
    void RegisterEntry(string catagoryName, SCR_SelectionMenuEntry entry)
    {
        COA_PlayerRadialMenuAction action = new COA_PlayerRadialMenuAction();
        action.catagoryID = catagoryName;
        action.entry = entry;

        m_aRegisteredActions.Insert(action);
    }
    
    //------------------------------------------------------------------------------------------------
    void ~COA_PlayerRadialMenuManager()
    {
        GetGame().GetInputManager().RemoveActionListener("COA_OpenPlayerRadialMenu", EActionTrigger.DOWN, OpenRadialMenu);

        if (m_RadialMenuController)
        {
            m_RadialMenuController.GetOnTakeControl().Remove(OnTakeControl);
            m_RadialMenuController.GetOnControllerChanged().Remove(OnControllerChanged);
        }
    }

}