//------------------------------------------------------------------------------------------------
//! Administrative menu for server management
class COA_AdminMenu : ChimeraMenuBase
{
	// Hover/press feedback and entrance animation - see COA_UIPolish
	protected ref COA_MenuPolish m_UIPolish;

	//-----------------------------------------------------------------------------
	// UI Components
	//-----------------------------------------------------------------------------
	
	// Core components
	protected COA_PlayerControllerManager m_clientComponent;
	protected InputManager m_InputManager;
	protected SCR_ChatPanel m_ChatPanel;
	protected bool m_bFocused = true;
	
	// Main widgets
	protected Widget m_wRoot;
	protected Widget m_wMenuContent;
	protected Widget m_wConfirmationMenu;
	protected FrameWidget m_gearResetMenuRoot;
	
	// Game managers
	protected PlayerManager m_playerManager;
	protected SCR_GroupsManagerComponent m_groupManagerComponent;
	protected COA_AdminMenuManager m_AdminMenuManager;
	
	// List containers
	protected OverlayWidget m_list1Root;
	protected OverlayWidget m_list2Root;
	protected OverlayWidget m_list3Root;
	protected OverlayWidget m_list4Root;
	protected OverlayWidget m_list5Root;
	
	// List components
	protected SCR_ListBoxComponent m_list1;
	protected SCR_ListBoxComponent m_list2;
	protected SCR_ListBoxComponent m_list3;
	protected SCR_ListBoxComponent m_list4;
	protected SCR_ListBoxComponent m_list5;
	
	// Text input widgets
	protected MultilineEditBoxWidget m_editBox1;
	protected EditBoxWidget m_editbox2;
	protected EditBoxWidget m_editbox3;
	protected WindowWidget m_windowBox1;
	
	// Menu navigation buttons
	protected SCR_ButtonTextComponent m_respawnMenuButton;
	protected SCR_ButtonTextComponent m_resetGearMenuButton;
	protected SCR_ButtonTextComponent m_teleportMenuButton;
	protected SCR_ButtonTextComponent m_hintMenuButton;
	protected SCR_ButtonTextComponent m_healMenuButton;
	protected SCR_ButtonTextComponent m_ticketMenuButton;
	protected SCR_ButtonTextComponent m_GamemodeMenuButton;
	protected SCR_ButtonTextComponent m_EnvironmentMenuButton;
	protected SCR_ButtonTextComponent m_GearFixMenuButton;
	
	// Action buttons
	protected SCR_ButtonTextComponent m_actionButton;
	protected SCR_ButtonTextComponent m_searchButton1;
	protected SCR_ButtonTextComponent m_searchButton2;
	protected SCR_ButtonTextComponent m_menuButton1;
	protected SCR_ButtonTextComponent m_menuButton2;
	protected SCR_ButtonTextComponent m_menuButton3;
	protected SCR_ButtonTextComponent m_menuButton4;
	
	// Data collections
	protected ref array<int> m_groupIDList = {};
	protected ref array<int> m_allPlayers = {};
	protected ref array<SCR_AIGroup> m_outGroups = {};
	protected ref array<int> m_spawnPoints = {};
	protected ref array<Faction> m_factions = {};
	protected ref array<string> m_selectableFactions = {};
	
	// Ticket State
	protected int m_iSelectedTicket = -1;
	
	// Gear Script List
	protected ref COA_GearScriptConfigStruct m_gearsetlist;
	
	protected bool m_bGameModeMenuOpen = false;
	protected bool m_bHintMenuOpen = false;
	protected bool m_bEnvironmentMenuOpen = false;
	protected ref COA_GearFixPanel m_GearFixPanel;
	protected ref array<int> m_CachedTickets = {};
	
	//-----------------------------------------------------------------------------
	// General UI Methods
	//-----------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Initialize the menu when it opens & sets up all UI elements and displays initial respawn menu
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		// Get manager instances
		m_InputManager = GetGame().GetInputManager();
		m_playerManager = GetGame().GetPlayerManager();
		m_groupManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		m_clientComponent = COA_PlayerControllerManager.GetInstance();
		m_AdminMenuManager= COA_AdminMenuManager.GetInstance();

		// Setup menu roots
		m_wRoot = GetRootWidget();

		// Set up menu navigation buttons
		InitializeMenuButtons();

		// Initialize chat panel
		InitializeChat();

		// Populate Admin Logs
		PopulateAdminActionsList();
		
		// Delay opening of initial menu
		GetGame().GetCallqueue().Call(DelayedMenuInitialization);

		// Hover feedback on buttons without their own, and a quick staggered fade-in (COA_UIPolish)
		m_UIPolish = new COA_MenuPolish(GetRootWidget(), true);
	}

	//------------------------------------------------------------------------------------------------
	//! Set up the initial menu (Tickets)
	protected void DelayedMenuInitialization()
	{
		// Setup log filters
		SCR_ButtonTextComponent filterLogHigh = GetMenuButton("HighFilterButton", m_wRoot);
		SCR_ButtonTextComponent filterLogMedium = GetMenuButton("MediumFilterButton", m_wRoot);
		SCR_ButtonTextComponent filterLogLow = GetMenuButton("LowFilterButton", m_wRoot);
		if (!filterLogHigh || !filterLogMedium || !filterLogLow)
			return;
		
		filterLogHigh.m_OnToggled.Insert(ToggleLogLevelFilter);
		filterLogMedium.m_OnToggled.Insert(ToggleLogLevelFilter);
		filterLogLow.m_OnToggled.Insert(ToggleLogLevelFilter);

		// The gear check's warning card opens the menu straight on the Gear Fix tab
		if (COA_GearFixClient.HasPendingOpen())
		{
			GearFixButton();
			return;
		}

		InitializeTicketMenu();
		UpdateMenuButtonColors(m_ticketMenuButton);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get a list box from the current loadded menu
	//! \param[in] listbox name of the list box located in the provded widget
	//! \param[in] widget name of widget to search (Default current open menu)
	//! \return list box component
	protected SCR_ListBoxComponent GetListBox(string listbox, Widget widget = null)
	{
		if (!widget)
			widget = m_wMenuContent;
		
		Widget listRoot = OverlayWidget.Cast(widget.FindAnyWidget(listbox));
		return SCR_ListBoxComponent.Cast(listRoot.FindHandler(SCR_ListBoxComponent));
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get a button from the current loadded menu
	//! \param[in] button name of the button located in the provded widget
	//! \param[in] widget name of widget to search (Default current open menu)
	//! \return list box component
	protected SCR_ButtonTextComponent GetMenuButton(string button, Widget widget = null)
	{
		if (!widget)
			widget = m_wMenuContent;
		
		return SCR_ButtonTextComponent.GetButtonText(button, widget);
	}

	//------------------------------------------------------------------------------------------------
	//! Get a multiline edit box from the current loaded menu
	//! \param[in]  name of the root widget of the edit box
	protected MultilineEditBoxWidget GetMultilineEditBox(string multiEditBox, Widget widget = null)
	{
		if (!widget)
			widget = m_wMenuContent;
		
		return MultilineEditBoxWidget.Cast(widget.FindAnyWidget(multiEditBox));
	}

	//------------------------------------------------------------------------------------------------
	//! Get a image widget from the current loaded menu
	//! \param[in]  name of the image widget
	protected ImageWidget GetImageWidget(string imagewidget, Widget widget = null)
	{
		if (!widget)
			widget = m_wMenuContent;
		
		return ImageWidget.Cast(widget.FindAnyWidget(imagewidget));
	}

	//------------------------------------------------------------------------------------------------
	//! Get a edit box from the current loaded menu
	//! \param[in]  name of the root widget of the edit box
	protected EditBoxWidget GetEditBox(string EditBox, Widget widget = null)
	{
		if (!widget)
			widget = m_wMenuContent;
		
		return EditBoxWidget.Cast(widget.FindAnyWidget(EditBox));
	}

	//------------------------------------------------------------------------------------------------
	//! Like GetListBox, but returns null instead of failing when the widget isn't in the layout
	protected SCR_ListBoxComponent FindListBox(string listbox, Widget widget = null)
	{
		if (!widget)
			widget = m_wMenuContent;

		if (!widget)
			return null;

		Widget listRoot = widget.FindAnyWidget(listbox);
		if (!listRoot)
			return null;

		return SCR_ListBoxComponent.Cast(listRoot.FindHandler(SCR_ListBoxComponent));
	}

	//------------------------------------------------------------------------------------------------
	//! Text widget lookup that tolerates a missing widget
	protected TextWidget FindText(string name, Widget widget = null)
	{
		if (!widget)
			widget = m_wMenuContent;

		if (!widget)
			return null;

		return TextWidget.Cast(widget.FindAnyWidget(name));
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initialize menu navigation buttons
	protected void InitializeMenuButtons()
	{
		// Respawn menu button
		m_ticketMenuButton = SCR_ButtonTextComponent.GetButtonText("TicketButton", m_wRoot);
		m_ticketMenuButton.m_OnClicked.Insert(TicketButton);
		
		// Respawn menu button
		m_respawnMenuButton = SCR_ButtonTextComponent.GetButtonText("RespawnButton", m_wRoot);
		m_respawnMenuButton.m_OnClicked.Insert(RespawnButton);

		// Reset gear menu button
		m_resetGearMenuButton = SCR_ButtonTextComponent.GetButtonText("ResetGearButton", m_wRoot);
		m_resetGearMenuButton.m_OnClicked.Insert(ResetGearButton);

		// Teleport menu button
		m_teleportMenuButton = SCR_ButtonTextComponent.GetButtonText("TeleportButton", m_wRoot);
		m_teleportMenuButton.m_OnClicked.Insert(TeleportButton);

		// Hint menu button
		m_hintMenuButton = SCR_ButtonTextComponent.GetButtonText("HintButton", m_wRoot);
		m_hintMenuButton.m_OnClicked.Insert(HintButton);

		// Heal menu button
		m_healMenuButton = SCR_ButtonTextComponent.GetButtonText("HealButton", m_wRoot);
		m_healMenuButton.m_OnClicked.Insert(HealButton);
		
		// Heal menu button
		m_GamemodeMenuButton = SCR_ButtonTextComponent.GetButtonText("GamemodeButton", m_wRoot);
		m_GamemodeMenuButton.m_OnClicked.Insert(GamemodeButton);

		// Weather & time menu button
		m_EnvironmentMenuButton = SCR_ButtonTextComponent.GetButtonText("EnvironmentButton", m_wRoot);
		if (m_EnvironmentMenuButton)
			m_EnvironmentMenuButton.m_OnClicked.Insert(EnvironmentButton);

		// Gear Fix menu button
		m_GearFixMenuButton = SCR_ButtonTextComponent.GetButtonText("GearFixButton", m_wRoot);
		if (m_GearFixMenuButton)
			m_GearFixMenuButton.m_OnClicked.Insert(GearFixButton);

		// Nav rail hover: the item's own rounded fill fades in (it's transparent until selected), rather than
		// the generic square overlay COA_MenuPolish would give the transparent button
		array<SCR_ButtonTextComponent> navButtons = {m_ticketMenuButton, m_respawnMenuButton, m_teleportMenuButton, m_healMenuButton, m_resetGearMenuButton, m_hintMenuButton, m_GamemodeMenuButton, m_EnvironmentMenuButton, m_GearFixMenuButton};
		foreach (SCR_ButtonTextComponent navButton : navButtons)
		{
			if (!navButton)
				continue;

			Widget navRoot = navButton.GetRootWidget();
			COA_UIHoverEffect.Attach(navRoot, navRoot.FindAnyWidget("NavBG"), COA_EHoverStyle.RAISE);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initialize the chat panel
	protected void InitializeChat()
	{
		Widget wChatPanel = GetRootWidget().FindAnyWidget("ChatPanel");
		if (wChatPanel)
			m_ChatPanel = SCR_ChatPanel.Cast(wChatPanel.FindHandler(SCR_ChatPanel));

		GetGame().GetInputManager().AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnCOA_ChatToggleAction);

		m_ChatPanel.SetAlwaysVisible(true);
		m_ChatPanel.ExpandMessageLines(20); // Increase the amount of message lines
		m_ChatPanel.ForceShowFullHistory(); // Load full history
	}

	//------------------------------------------------------------------------------------------------
	//! Clean up when menu is closed
	override void OnMenuClose()
	{
		if (m_UIPolish)
		{
			m_UIPolish.Cleanup();
			m_UIPolish = null;
		}

		super.OnMenuClose();

		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_FE_HUD_PAUSE_MENU_CLOSE);
		GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnCOA_ChatToggleAction);
		
		CleanupGearFixPanel();

		if (m_wMenuContent)
			delete m_wMenuContent;

		CloseConfirmAction();
		CloseEndMissionDialog();
		CloseContextMenu();

		if (m_ChatPanel)
			m_ChatPanel.SetAlwaysVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	//! Activates the Respawn menu
	void TicketButton()
	{
		UpdateMenuButtonColors(m_ticketMenuButton);
		ClearMenu();
		InitializeTicketMenu();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Activates the Respawn menu
	void RespawnButton()
	{
		UpdateMenuButtonColors(m_respawnMenuButton);
		ClearMenu();
		InitializeRespawnMenu();
	}

	//------------------------------------------------------------------------------------------------
	//! Activates the Reset Gear menu
	void ResetGearButton()
	{
		UpdateMenuButtonColors(m_resetGearMenuButton);
		ClearMenu();
		InitializeGearMenu();
	}

	//------------------------------------------------------------------------------------------------
	//! Activates the Teleport menu
	void TeleportButton()
	{
		UpdateMenuButtonColors(m_teleportMenuButton);
		ClearMenu();
		InitializeTeleportMenu();
	}

	//------------------------------------------------------------------------------------------------
	//! Activates the Hint menu
	void HintButton()
	{
		UpdateMenuButtonColors(m_hintMenuButton);
		ClearMenu();
		InitializeHintMenu();
	}

	//------------------------------------------------------------------------------------------------
	//! Activates the Heal menu
	void HealButton()
	{
		UpdateMenuButtonColors(m_healMenuButton);
		ClearMenu();
		InitializeHealMenu();
	}	
	
	//------------------------------------------------------------------------------------------------
	//! Activates the Gamemode Settings menu
	void GamemodeButton()
	{
		UpdateMenuButtonColors(m_GamemodeMenuButton);
		ClearMenu();
		InitializeGamemodeMenu();
	}

	//------------------------------------------------------------------------------------------------
	//! Activates the Weather & Time menu
	void EnvironmentButton()
	{
		UpdateMenuButtonColors(m_EnvironmentMenuButton);
		ClearMenu();
		InitializeEnvironmentMenu();
	}

	//------------------------------------------------------------------------------------------------
	//! Activates the Gear Fix menu
	void GearFixButton()
	{
		UpdateMenuButtonColors(m_GearFixMenuButton);
		ClearMenu();
		InitializeGearFixMenu();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Updates menu button colors to highlight the active menu
	//! \param[in]  activeButton The button widget of the active menu button
	protected void UpdateMenuButtonColors(SCR_ButtonTextComponent activeButton)
	{
		array<SCR_ButtonTextComponent> navButtons = {m_ticketMenuButton, m_respawnMenuButton, m_teleportMenuButton, m_healMenuButton, m_resetGearMenuButton, m_hintMenuButton, m_GamemodeMenuButton, m_EnvironmentMenuButton, m_GearFixMenuButton};
		foreach (SCR_ButtonTextComponent navButton : navButtons)
		{
			if (navButton)
				SetNavButtonActive(navButton.GetRootWidget(), navButton == activeButton);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Nav rail item look: active = raised fill, accent bar, bright icon and label
	protected void SetNavButtonActive(Widget button, bool active)
	{
		if (!button)
			return;

		// Through the hover effect: the item was just clicked, so it's hovered and its animation would paint over a plain SetColor
		Widget navBG = button.FindAnyWidget("NavBG");
		if (navBG)
		{
			if (active)
				COA_UIHoverEffect.SetTargetColor(button, navBG, Color.FromSRGBA(28, 31, 40, 255));
			else
				COA_UIHoverEffect.SetTargetColor(button, navBG, Color.FromSRGBA(28, 31, 40, 0));
		}

		Widget accent = button.FindAnyWidget("NavAccent");
		if (accent)
		{
			if (active)
				accent.SetColor(Color.FromSRGBA(201, 54, 54, 255));
			else
				accent.SetColor(Color.FromSRGBA(201, 54, 54, 0));
		}

		Color foreground = Color.FromSRGBA(169, 180, 204, 255);
		if (active)
			foreground = Color.FromSRGBA(239, 242, 247, 255);

		Widget icon = button.FindAnyWidget("NavIcon");
		if (icon)
			icon.SetColor(foreground);

		Widget label = button.FindAnyWidget("NavLabel");
		if (label)
			label.SetColor(foreground);
	}
	
	//------------------------------------------------------------------------------------------------ 	
	protected void UpdateMenuTitle(string title)
	{
		TextWidget.Cast(m_wRoot.FindAnyWidget("MenuSubTitle")).SetText(title);
	}

	//------------------------------------------------------------------------------------------------
	//! Clears all menu elements and data
	//! Resets visibility and clears event handlers
	void ClearMenu()
	{
		CloseContextMenu();
		CleanupGearFixPanel();

		// Remove menu widget
		if (m_wMenuContent)
			delete m_wMenuContent;

		// Clear data collections
		m_outGroups.Clear();
		m_spawnPoints.Clear();
		m_groupIDList.Clear();
		m_allPlayers.Clear();
		m_factions.Clear();
		m_selectableFactions.Clear();
		m_bGameModeMenuOpen = false;
		m_bHintMenuOpen = false;
		m_bEnvironmentMenuOpen = false;
		m_aWeatherStateNames.Clear();
		m_aAnnouncementTemplates.Clear();
		CloseEndMissionDialog();
	}

	//------------------------------------------------------------------------------------------------
	//! Populates a list box with available roles
	//! \param[in]  list The list box to populate with roles
	void AddRoles(SCR_ListBoxComponent list)
	{
		array<string> roleNames = {};
		SCR_Enum.GetEnumNames(COA_EGearRole, roleNames);
		
		foreach (string role : roleNames)
		{
			list.AddItem(role);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Gets player ID from player name
	//! \param[in]  name The player name to search for
	//! @return The matching player ID or 0 if not found
	protected int GetplayerIdFromName(string name)
	{
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		
		foreach (int pid : playerIds)
		{
			if (GetGame().GetPlayerManager().GetPlayerName(pid) == name)
				return pid;
		}

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Gets the resource prefab for a given group and role
	//! \param[in]  groupID The group ID
	//! \param[in]  index The role index
	//! @return The resource name for the corresponding prefab
	ResourceName GetPrefab(int index)
	{
		ResourceName prefab = COA_RoleHelper.RoleToResource(index);
		return prefab;
	}

	//------------------------------------------------------------------------------------------------
	//! Handle loss of menu focus
	override void OnMenuFocusLost()
	{
		super.OnMenuFocusLost();

		m_bFocused = false;
		m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_OPEN, EActionTrigger.DOWN, Close);
		m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, Close);
		#ifdef WORKBENCH
			m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_OPEN_WB, EActionTrigger.DOWN, Close);
			m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_BACK_WB, EActionTrigger.DOWN, Close);
		#endif
	}

	//------------------------------------------------------------------------------------------------
	//! Populates a list with active players
	//! \param[in]  list The list to populate with player names
	//! \param[in]  selectOpenTicket automatically select player that ticket is selected in the ticket menu
	//! \param[in]  filterByOpenTickets filter the list by players by open tickets
	//! \param[in]  filterByGroup filter the list by players by this group
	//! \param[in]  filterByFaction filter the list by players by this faction
	//! \param[in]  filterDeadOnly filter the list by dead players
	protected void PopulatePlayerList(SCR_ListBoxComponent list, bool selectOpenTicket = true, bool filterByOpenTickets = false, SCR_AIGroup filterByGroup = null, Faction filterByFaction = null, bool filterDeadOnly = false)
	{
		// Clear the list
		list.Clear();

		// Get all players
		m_playerManager.GetPlayers(m_allPlayers);
		TStringArray playerNames = {};

		// Get and sort player names
		foreach (int playerId : m_allPlayers)
			playerNames.Insert(m_playerManager.GetPlayerName(playerId));

		playerNames.Sort(false);

		// Add players to list if they're in a group and not spectating
		foreach (string name : playerNames)
		{
			int playerId = GetplayerIdFromName(name);
			Faction playerFaction = COA_SlottingManager.GetInstance().GetPlayerSlotFaction(playerId);
			SCR_AIGroup playerGroup = COA_SlottingManager.GetInstance().GetPlayerSlotGroup(playerId);

			if (!playerGroup)
				continue;

			bool isSpectating = COA_EntityHelper.IsSpectator(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId));

			// Respawn-style lists should only ever offer players who are actually eligible to be respawned
			if (filterDeadOnly && !COA_SlottingManager.GetInstance().IsPlayerConsideredDead(playerId) && !isSpectating)
				continue;

			if (!filterDeadOnly && isSpectating)
				continue;

			if (filterByOpenTickets && !m_CachedTickets.Contains(playerId))
				continue;

			if (filterByGroup && filterByGroup != playerGroup)
				continue;

			if (filterByFaction && filterByFaction != playerFaction)
				continue;

			list.AddItemWithColor(string.Format("%1", name), playerFaction.GetFactionColor());
		}

		AttachContextMenu(list);

		if (selectOpenTicket)
			SelectOpenTicketOnwer(list);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Populates the list of available groups
	//! \param[in]  list The list to populate with group names 
	protected void PopulateGroupsList(SCR_ListBoxComponent list)
	{
		foreach (SCR_AIGroup group : m_outGroups)
		{
			// Get faction info
			Faction groupFaction = group.GetFaction();
			if (!groupFaction)
				continue;
				
			string factionKey = groupFaction.GetFactionKey();
			if (factionKey.IsEmpty() || factionKey == "SPEC")
				continue;
				
			string factionTag = factionKey.Substring(0, 3);
			
			// Add group to list
			list.AddItem(string.Format("%1 | %2", factionTag, group.GetCustomNameWithOriginal()));
			m_groupIDList.Insert(group.GetGroupID());
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Populates the list of active factions
	//! \param[in]  list The list to populate with faction names 
	protected void PopulateFactionList(SCR_ListBoxComponent list)
	{
		// Get all factions
		GetGame().GetFactionManager().GetFactionsList(m_factions);
		
		// Add factions with active players
		foreach (Faction faction : m_factions)
		{
			if (SCR_FactionManager.SGetFactionPlayerCount(faction) > 0)
			{
				list.AddItem(faction.GetFactionName());
				m_selectableFactions.Insert(faction.GetFactionKey());
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Populates the list with players from a selected group
	//! \param[in]  groupList The list box to filter the player list by
	//! \param[in]  playerList The list to populate with player names 
	void SelectGroupByPlayer(SCR_ListBoxComponent groupList, SCR_ListBoxComponent playerList)
	{
		if (playerList.GetSelectedItem() < 0)
			return;
			
		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
		
		SCR_AIGroup selectedGroup = COA_SlottingManager.GetInstance().GetPlayerSlotGroup(playerId);
		
		foreach (int i, SCR_AIGroup group : m_outGroups)
		{
			if (selectedGroup == group)
			{
				// Adjust index for client mode
				int itemIndex = i;
				groupList.SetItemSelected(itemIndex, true);
				return;
			}
		};
	}

	//------------------------------------------------------------------------------------------------
	//! Select the current open ticket player in a list
	//! \param[in]  list The list to select the player on based on the ticket selected in the ticket menu
	void SelectOpenTicketOnwer(SCR_ListBoxComponent list)
	{
		// Find the open ticket selected in the list
		if (m_iSelectedTicket != -1)
		{			
			for (int i = 0; i < list.GetItemCount(); i++)
			{
				string ticketName = m_playerManager.GetPlayerName(m_iSelectedTicket);
				string listName = TextWidget.Cast(list.GetElementComponent(i).GetRootWidget().FindAnyWidget("Text")).GetText();
				if (ticketName == listName)
				{
					list.SetItemSelected(i, true, true);
					return;
				}
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Search the first player list
	void SearchList0()
	{
		// Load List Box
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		// Load Search Box
		EditBoxWidget searchBox = GetEditBox("SearchBox0");
		if (!searchBox)
			return;
		
		SearchPlayerList(playerList, searchBox.GetText());
	}
	
	//------------------------------------------------------------------------------------------------
	//! Search the second player list
	void SearchList1()
	{
		// Load List Box
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox1");
		if (!playerList)
			return;
		
		// Load Search Box
		EditBoxWidget searchBox = GetEditBox("SearchBox1");
		if (!searchBox)
			return;
		
		SearchPlayerList(playerList, searchBox.GetText());
	}
	
	//------------------------------------------------------------------------------------------------
	//! Filters a player list based on search text
	//! \param[in]  list The list box to filter
	//! \param[in]  searchData The search text to filter by
	void SearchPlayerList(SCR_ListBoxComponent list, string searchData)
	{
		TStringArray playerNames = {};
		m_playerManager.GetPlayers(m_allPlayers);
		list.Clear();

		// If search is empty, show all players
		if (searchData == "")
		{
			foreach (int playerId : m_allPlayers)
				playerNames.Insert(m_playerManager.GetPlayerName(playerId));
		} 
		else 
		{
			// Otherwise filter by search text
			string searchLower = searchData;
			searchLower.ToLower();
			
			foreach (int playerId : m_allPlayers)
			{
				string playerName = m_playerManager.GetPlayerName(playerId);
				string playerNameLower = playerName;
				playerNameLower.ToLower();

				if (playerNameLower.Contains(searchLower))
					playerNames.Insert(playerName);
			}
		}

		// Respawn's search box operates on the same dead/spectating-only list as its initial population
		bool filterDeadOnly = GetCurrentOpenTab() == "Respawn";

		// Sort and add filtered players
		playerNames.Sort(false);
		foreach (string name : playerNames)
		{
			int playerId = GetplayerIdFromName(name);
			Faction playerFaction = COA_SlottingManager.GetInstance().GetPlayerSlotFaction(playerId);

			if (!m_groupManagerComponent.GetPlayerGroup(playerId))
				continue;

			bool isSpectating = COA_EntityHelper.IsSpectator(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId));

			if (filterDeadOnly && !COA_SlottingManager.GetInstance().IsPlayerConsideredDead(playerId) && !isSpectating)
				continue;

			if (!filterDeadOnly && isSpectating)
				continue;

			list.AddItemWithColor(string.Format("%1", name), playerFaction.GetFactionColor());
		}

		AttachContextMenu(list);
	}

	//------------------------------------------------------------------------------------------------
	//! Toggle the filter for only players with open tickets in the first player list
	void ToggleOpenTicketFilter()
	{

		SCR_ButtonTextComponent menuButtonTicketOpenToggle = GetMenuButton("OpenTicketFilterButton");
		if (!menuButtonTicketOpenToggle)
			return;

		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;

		ImageWidget check = GetImageWidget("OpenTicketFilterButtonImage");
		if (!check)
			return;
		
		check.SetVisible(menuButtonTicketOpenToggle.IsToggled());

		PopulatePlayerList(playerList, false, menuButtonTicketOpenToggle.IsToggled(), filterDeadOnly: GetCurrentOpenTab() == "Respawn");
	}

	//------------------------------------------------------------------------------------------------
	//! Toggle filters for logs
	 void ToggleLogLevelFilter()
	 {
		// Setup log filters
		SCR_ButtonTextComponent filterLogHigh = GetMenuButton("HighFilterButton", m_wRoot);
		SCR_ButtonTextComponent filterLogMedium = GetMenuButton("MediumFilterButton", m_wRoot);
		SCR_ButtonTextComponent filterLogLow = GetMenuButton("LowFilterButton", m_wRoot);
		if (!filterLogHigh || !filterLogMedium || !filterLogLow)
			return;

		ImageWidget filterLogHighCheck = GetImageWidget("HighFilterButtonImage", m_wRoot);
		ImageWidget filterLogMediumCheck = GetImageWidget("MediumFilterButtonImage", m_wRoot);
		ImageWidget filterLogLowCheck = GetImageWidget("LowFilterButtonImage", m_wRoot);
		if (!filterLogHighCheck || !filterLogMediumCheck || !filterLogLowCheck)
			return;

		filterLogHighCheck.SetVisible(filterLogHigh.IsToggled());
		filterLogMediumCheck.SetVisible(filterLogMedium.IsToggled());
		filterLogLowCheck.SetVisible(filterLogLow.IsToggled());

		PopulateAdminActionsList();
	 }
	
	//------------------------------------------------------------------------------------------------
	//! Update chat while menu is active
	float m_fUpdateBuffer = 0;
	override void OnMenuUpdate(float tDelta)
	{
		if (m_UIPolish)
			m_UIPolish.Update(tDelta);

		super.OnMenuUpdate(tDelta);

		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);

		// The right-click menu's player left (ticket rows stay - a ticket can be closed after they leave)
		if (m_wContextMenu && m_iContextTicket == -1 && !m_playerManager.IsPlayerConnected(m_iContextPlayerId))
			CloseContextMenu();

		if (m_fUpdateBuffer >= 1)
		{
			if (m_bGameModeMenuOpen)
				GamemodeMenuUpdate();
			if (m_bHintMenuOpen)
				UpdateCountdownStatus();
			if (m_bEnvironmentMenuOpen)
				EnvironmentMenuUpdate();
			if (m_GearFixPanel)
				m_GearFixPanel.Update();
			m_fUpdateBuffer = 0;
		}
		m_fUpdateBuffer += tDelta;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Handle regaining menu focus
	override void OnMenuFocusGained()
	{
		super.OnMenuFocusGained();
		
		m_bFocused = true;
		m_InputManager.AddActionListener(UIConstants.MENU_ACTION_OPEN, EActionTrigger.DOWN, Close);
		m_InputManager.AddActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, Close);
		#ifdef WORKBENCH
			m_InputManager.AddActionListener(UIConstants.MENU_ACTION_OPEN_WB, EActionTrigger.DOWN, Close);
			m_InputManager.AddActionListener(UIConstants.MENU_ACTION_BACK_WB, EActionTrigger.DOWN, Close);
		#endif
	}

	//-----------------------------------------------------------------------------
	// Gear Menu Methods
	//-----------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Initialize the Gear Reset menu & Allows admins to reset player gear to role defaults
	void InitializeGearMenu()
	{
		// Load menu content widget
		m_wMenuContent = GetGame().GetWorkspace().CreateWidgets("{5C7EC9AAE498F6B6}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/GearMenu.layout");
		COA_UIPolish.AttachHoverEffects(m_wMenuContent); // hover feedback - sub-menus are separate workspace roots
		if (!m_wMenuContent)
			return;
		
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		SCR_ListBoxComponent roleList = GetListBox("RoleListBox0");
		if (!playerList || !roleList)
			return;
		
		// Load Buttons
		SCR_ButtonTextComponent searchButton0 = GetMenuButton("SearchButton0");
		SCR_ButtonTextComponent menuButton0 = GetMenuButton("MenuButton0");
		SCR_ButtonTextComponent menuButton1 = GetMenuButton("MenuButton1");
		SCR_ButtonTextComponent menuButton2 = GetMenuButton("MenuButton2");
		SCR_ButtonTextComponent menuButton3 = GetMenuButton("MenuButton3");
		SCR_ButtonTextComponent menuButton4 = GetMenuButton("MenuButton4");
		SCR_ButtonTextComponent menuButton5 = GetMenuButton("MenuButton5");
		SCR_ButtonTextComponent menuButton6 = GetMenuButton("MenuButton6");
		SCR_ButtonTextComponent menuButton7 = GetMenuButton("MenuButton7");
		if (!searchButton0 || !menuButton0 || !menuButton1 || !menuButton2 || !menuButton3 || !menuButton4 || !menuButton5 || !menuButton6 || !menuButton7)
			return;
		
		// Setup button event handlers
		searchButton0.m_OnClicked.Insert(SearchList0);
		menuButton0.m_OnClicked.Insert(ResetGear);
		menuButton1.m_OnClicked.Insert(AddLeaderRadio);
		menuButton2.m_OnClicked.Insert(AddGIRadio);
		menuButton3.m_OnClicked.Insert(AddBinos);
		menuButton4.m_OnClicked.Insert(AddMap);
		menuButton5.m_OnClicked.Insert(AddWrench);
		menuButton6.m_OnClicked.Insert(AddMedicKit);
		menuButton7.m_OnClicked.Insert(AddPrimaryAmmo);

		// Setup List Filters
		SCR_ButtonTextComponent menuButtonTicketOpenToggle = GetMenuButton("OpenTicketFilterButton");
		if (!menuButtonTicketOpenToggle)
			return;
		
		menuButtonTicketOpenToggle.m_OnToggled.Insert(ToggleOpenTicketFilter);
		
		// Setup selection change handler
		playerList.m_OnChanged.Insert(UpdateDefaultGear);
		
		// Change menu title
		UpdateMenuTitle("Gear Reset");

		// Add available roles
		AddRoles(roleList);

		// Populate player list
		PopulatePlayerList(playerList, true, menuButtonTicketOpenToggle.IsToggled());
	}

	//------------------------------------------------------------------------------------------------
	//! Updates default gear selection based on player selection
	void UpdateDefaultGear()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		SCR_ListBoxComponent roleList = GetListBox("RoleListBox0");
		if (!playerList || !roleList)
			return;
		
		// If no player selected, return
		if (playerList.GetSelectedItem() < 0)
			return;
			
		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;

		// Find the player's role in the list and select it
		for (int i = 0; i < roleList.GetItemCount(); i++)
		{
			if (i == COA_RoleHelper.ResourceToRole(COA_SlottingManager.GetInstance().GetPlayerSlotResource(playerId)))
			{
				roleList.SetItemSelected(i, true);
				return;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Adds leadership radio to selected player
	void AddLeaderRadio()
	{
		// Load List Box
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;

		if (playerList.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		// Get the player's group and faction
		SCR_AIGroup playerGroup = m_groupManagerComponent.GetPlayerGroup(playerId);
		if (!playerGroup)
			return;
			
		// Get radio prefab and add item
		string factionKey = playerGroup.GetFaction().GetFactionKey();
		ResourceName radioPrefab = COA_Gamemode.GetInstance().GetGearScriptSettings(factionKey).m_rLongRangeRadioPrefab;
		COA_PlayerRplToAuthorityManager.GetInstance().AddItem(playerId, radioPrefab, true);
	}

	//------------------------------------------------------------------------------------------------
	//! Adds GI radio to selected player
	void AddGIRadio()
	{
		// Load List Box
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;

		if (playerList.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		// Get the player's group and faction
		SCR_AIGroup playerGroup = m_groupManagerComponent.GetPlayerGroup(playerId);
		if (!playerGroup)
			return;
			
		// Get radio prefab and add item
		string factionKey = playerGroup.GetFaction().GetFactionKey();
		ResourceName radioPrefab = COA_Gamemode.GetInstance().GetGearScriptSettings(factionKey).m_rShortRangeRadioPrefab;
		COA_PlayerRplToAuthorityManager.GetInstance().AddItem(playerId, radioPrefab, true);
	}

	//------------------------------------------------------------------------------------------------
	//! Adds binoculars to selected player
	void AddBinos()
	{
		// Load List Box
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		if (playerList.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		// Get binoculars prefab and add item
		string binosPrefab = GetBinos(playerId);
		COA_PlayerRplToAuthorityManager.GetInstance().AddItem(playerId, binosPrefab, true);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Adds map to selected player
	void AddMap()
	{
		// Load List Box
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		if (playerList.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		//Add map
		const string mapPrefab = "{13772C903CB5E4F7}Prefabs/Items/Equipment/Maps/PaperMap_01_folded.et";
		COA_PlayerRplToAuthorityManager.GetInstance().AddItem(playerId, mapPrefab, true);
	}

	//------------------------------------------------------------------------------------------------
	//! Adds wrench to selected player
	void AddWrench()
	{
		// Load List Box
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		if (playerList.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		// Add wrench
		const string wrenchPrefab = "{33B2DFDCD0EBA3DB}Prefabs/Items/Equipment/Kits/RepairKit_01/RepairKit_01_wrench.et";
		COA_PlayerRplToAuthorityManager.GetInstance().AddItem(playerId, wrenchPrefab, true);
	}

	//------------------------------------------------------------------------------------------------
	//! Adds medic medical items to selected player from their faction's gearscript
	void AddMedicKit()
	{
		// Load List Box
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		if (playerList.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		// Get the player's group and faction
		SCR_AIGroup playerGroup = m_groupManagerComponent.GetPlayerGroup(playerId);
		if (!playerGroup)
			return;
			
		string factionKey = playerGroup.GetFaction().GetFactionKey();

		// Load the gear config for the faction
		COA_GearScriptConfig gearConfig = COA_GearscriptManager.GetInstance().LoadGearScriptConfig(COA_Gamemode.GetInstance().GetGearScriptResource(factionKey));
		if (!gearConfig || !gearConfig.m_MedicMedicalItems)
			return;

		// Add each medic medical item the configured number of times
		foreach (COA_Inventory_Item item : gearConfig.m_MedicMedicalItems)
		{
			for (int i = 0; i < item.m_iItemCount; i++)
				COA_PlayerRplToAuthorityManager.GetInstance().AddItem(playerId, item.m_sItemPrefab, true);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Adds primary weapon ammo to selected player based on their role's gearscript
	void AddPrimaryAmmo()
	{
		// Load List Box
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		if (playerList.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;

		// Get player entity and determine role from their prefab
		IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!playerEntity)
			return;

		COA_EGearRole role = COA_RoleHelper.ResourceToRole(playerEntity.GetPrefabData().GetPrefabName());

		// Get role config to find which magazine types this role uses
		COA_RolesConfig rolesConfig = COA_GearscriptManager.GetRolesConfig();
		if (!rolesConfig)
			return;

		COA_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
		if (!roleConfig || !roleConfig.m_aMagazines)
			return;

		bool isAssistant = (roleConfig.m_SlottingType == COA_ESlotType.ASSISTANT || roleConfig.m_SlottingType == COA_ESlotType.SPECIALTY_ASSISTANT);

		// Get player's faction and load gearscript config
		SCR_AIGroup playerGroup = m_groupManagerComponent.GetPlayerGroup(playerId);
		if (!playerGroup)
			return;

		string factionKey = playerGroup.GetFaction().GetFactionKey();
		COA_GearScriptConfig gearConfig = COA_GearscriptManager.GetInstance().LoadGearScriptConfig(COA_Gamemode.GetInstance().GetGearScriptResource(factionKey));
		if (!gearConfig)
			return;

		// Iterate through the role's magazine types, skip pistol (secondary)
		foreach (COA_EGearscriptMagazines magType : roleConfig.m_aMagazines)
		{
			if (magType == COA_EGearscriptMagazines.PISTOL_MAG)
				continue;

			array<ref COA_Magazine_Class> magArray;

			switch (magType)
			{
				case COA_EGearscriptMagazines.RIFLE_MAG:
					if (gearConfig.m_Rifles && !gearConfig.m_Rifles.IsEmpty())
						magArray = gearConfig.m_Rifles[0].m_MagazineArray;
					break;

				case COA_EGearscriptMagazines.RIFLEUGL_MAG:
					if (gearConfig.m_RifleUGLs && !gearConfig.m_RifleUGLs.IsEmpty())
						magArray = gearConfig.m_RifleUGLs[0].m_MagazineArray;
					break;

				case COA_EGearscriptMagazines.CARBINE_MAG:
					if (gearConfig.m_Carbines && !gearConfig.m_Carbines.IsEmpty())
						magArray = gearConfig.m_Carbines[0].m_MagazineArray;
					break;

				case COA_EGearscriptMagazines.SNIPER_MAG:
					if (gearConfig.m_SNIPER && gearConfig.m_SNIPER.m_MagazineArray)
						magArray = gearConfig.m_SNIPER.m_MagazineArray;
					break;

				case COA_EGearscriptMagazines.AR_MAG:
					if (gearConfig.m_AR && gearConfig.m_AR.m_MagazineArray)
						magArray = COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_AR.m_MagazineArray, isAssistant);
					break;

				case COA_EGearscriptMagazines.MMG_MAG:
					if (gearConfig.m_MMG && gearConfig.m_MMG.m_MagazineArray)
						magArray = COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_MMG.m_MagazineArray, isAssistant);
					break;

				case COA_EGearscriptMagazines.AT_MAG:
					if (gearConfig.m_AT && gearConfig.m_AT.m_MagazineArray)
						magArray = COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_AT.m_MagazineArray, isAssistant);
					break;

				case COA_EGearscriptMagazines.MAT_MAG:
					if (gearConfig.m_MAT && gearConfig.m_MAT.m_MagazineArray)
						magArray = COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_MAT.m_MagazineArray, isAssistant);
					break;

				case COA_EGearscriptMagazines.HAT_MAG:
					if (gearConfig.m_HAT && gearConfig.m_HAT.m_MagazineArray)
						magArray = COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_HAT.m_MagazineArray, isAssistant);
					break;

				case COA_EGearscriptMagazines.AA_MAG:
					if (gearConfig.m_AA && gearConfig.m_AA.m_MagazineArray)
						magArray = COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_AA.m_MagazineArray, isAssistant);
					break;

				case COA_EGearscriptMagazines.HMG_MAG:
					if (gearConfig.m_HMG && gearConfig.m_HMG.m_MagazineArray)
						magArray = COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_HMG.m_MagazineArray, isAssistant);
					break;
			}

			if (!magArray)
				continue;

			foreach (COA_Magazine_Class mag : magArray)
			{
				for (int i = 0; i < mag.m_MagazineCount; i++)
					COA_PlayerRplToAuthorityManager.GetInstance().AddItem(playerId, mag.m_Magazine, true);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Gets binoculars prefab for player's faction
	//! \param[in]  playerId The player ID to get binoculars for
	//! \return The binoculars prefab resource name
	string GetBinos(int playerId)
	{
		// Get player's group and faction
		SCR_AIGroup playerGroup = m_groupManagerComponent.GetPlayerGroup(playerId);
		if (!playerGroup)
			return "";
			
		string factionKey = playerGroup.GetFaction().GetFactionKey();

		// Load the gear config for the faction
		COA_GearScriptConfig gearConfig = COA_GearscriptManager.GetInstance().LoadGearScriptConfig(COA_Gamemode.GetInstance().GetGearScriptResource(factionKey));
		if (!gearConfig)
			return "";

		return gearConfig.m_sLeadershipBinocularsPrefab;
	}

	//------------------------------------------------------------------------------------------------
	//! Reset a player's gear to their role default
	void ResetGear()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		SCR_ListBoxComponent roleList = GetListBox("RoleListBox0");
		if (!playerList || !roleList)
			return;
		
		if (playerList.GetSelectedItem() < 0)
			return;

		if (roleList.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
		
		// Get the prefab for the selected role
		ResourceName prefab = GetPrefab(roleList.GetSelectedItem());
		if (prefab.IsEmpty())
			return;

		// Reset player's gear
		COA_PlayerRplToAuthorityManager.GetInstance().ResetGear(playerId, prefab, true);
	}
	
	//-----------------------------------------------------------------------------
	// Ticket Menu Methods
	//-----------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Initialize the ticket menu
	//! Allows admins to see admin messages
	void InitializeTicketMenu()
	{
		// Load menu content widget
		m_wMenuContent = GetGame().GetWorkspace().CreateWidgets("{FD7582ED92D34192}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/TicketMenu.layout");
		COA_UIPolish.AttachHoverEffects(m_wMenuContent); // hover feedback - sub-menus are separate workspace roots
		if (!m_wMenuContent)
			return;
		
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		// Load List Boxes
		SCR_ButtonTextComponent replyButton = GetMenuButton("MenuButton0");
		SCR_ButtonTextComponent assignButton = GetMenuButton("MenuButton1");
		SCR_ButtonTextComponent closeButton = GetMenuButton("MenuButton2");
		SCR_ButtonTextComponent searchButton = GetMenuButton("SearchButton0");
		if (!replyButton || !assignButton || !closeButton || !searchButton)
			return;
		
		// Setup button event handlers
		replyButton.m_OnClicked.Insert(ReplyToTicket);
		assignButton.m_OnClicked.Insert(AssignAdminToTicket);
		closeButton.m_OnClicked.Insert(CloseAdminTicket);
		searchButton.m_OnClicked.Insert(SearchList0);
		
		// Setup selection change handlers
		playerList.m_OnChanged.Insert(GetTicketMessages);
		
		// Change title of the menu
		UpdateMenuTitle("Tickets");

		// Populate list of players that need help
		GetOpenTickets();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Assign a admin to a ticket
	void AssignAdminToTicket()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		// Check if a ticket is selected
		if (playerList.GetSelectedItem() == -1 && m_iSelectedTicket == -1)
			return;
		
		// Get ID of the admin
		int adminID = SCR_PlayerController.GetLocalPlayerId();
		
		// Add the reply to ticket
		COA_PlayerRplToAuthorityManager.GetInstance().AssignAdminTicket(m_iSelectedTicket, adminID, false);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Assign a admin to a ticket
	void CloseAdminTicket()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		// Get ID of the admin
		int adminID = SCR_PlayerController.GetLocalPlayerId();
		
		// Reinitilaize the ticket menu
		TicketButton();
		
		// Check if a ticket is selected
		if (playerList.GetSelectedItem() == -1 && m_iSelectedTicket == -1)
			return;
		
		// Broadcast the removal of ticket
		COA_PlayerRplToAuthorityManager.GetInstance().CloseAdminTicket(m_iSelectedTicket, adminID, true);
		
		// Deselect ticket
		m_iSelectedTicket = -1;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Reply to a ticket selected in list1
	void ReplyToTicket()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		// Load Reply Box
		MultilineEditBoxWidget replyBox = GetMultilineEditBox("ReplyBox0");
		if (!replyBox)
			return;
		
		// If the reply box is empty
		if (replyBox.GetText() == "")
			return;
		
		// If no player is selected or if one was selected before a refresh
		if (playerList.GetSelectedItem() < 0 && m_iSelectedTicket < 1)
			return;
		
		// Get player ID of the admin replying to the message
		int adminID = SCR_PlayerController.GetLocalPlayerId();
		
		// Get the text for the reply box
		string reply = replyBox.GetText();
		
		// Add reply to tickets array
		COA_PlayerRplToAuthorityManager.GetInstance().ReplyAdminMessage(reply, m_iSelectedTicket, adminID, false);
		
		// Clear Text in reply box
		replyBox.SetText("");
	}
	
	//------------------------------------------------------------------------------------------------
	//! Request the list of messages from the server
	void GetTicketMessages()
	{
		int ticketID = -1;
		int adminID = SCR_PlayerController.GetLocalPlayerId();
		
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		// Check if a ticket was selected either via the list or pre ui refresh
		if (playerList.GetSelectedItem() != -1)
		{
			// Get selected player ID
			string ticketName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
			
			// Extract ID from ticketName in list
			array<string> data = {};
		
			ticketName.Split(":", data, true);
			if (data.IsEmpty())
				return;

			ticketID = data[0].ToInt();
			if (ticketID == 0)
				return;

			// Store selected ticket for use after ui refresh
			m_iSelectedTicket = ticketID;
		}
		else if (m_iSelectedTicket != -1)
			// Get the stored ticket selected before refresh
			ticketID = m_iSelectedTicket;
		else
			return;
		
		COA_PlayerRplToAuthorityManager.GetInstance().GetTicketMessages(adminID, ticketID);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Populates the list of messages selected in list 1
	//! \param[out]  messages Array of messages in a ticket
	void PopulateTicketMessages(array<ref COA_TicketMessageData> messages)
	{
		if (!messages)
			return;
		
		SCR_ListBoxComponent ticketMessagesList = GetListBox("TicketMessagesListBox0");
		if (!ticketMessagesList)
			return;
		
		// Clear old Messages 
		ticketMessagesList.Clear();
		
		// Format and add the messages to the list
		foreach (int i, ref COA_TicketMessageData message : messages)
		{
			ticketMessagesList.AddItem(string.Format("%1 - %2: %3", message.timestamp, message.sender, message.msg));
		}

	}
	
	//------------------------------------------------------------------------------------------------
	//! Request the list of open tickets from the server
	void GetOpenTickets()
	{	
		COA_PlayerRplToAuthorityManager.GetInstance().GetOpenTickets(SCR_PlayerController.GetLocalPlayerId());
	}
	
	//------------------------------------------------------------------------------------------------
	//! Populates the list of players that need help
	void PopulateOpenTicketList(array<int> tickets)
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		// Clear old ticket list
		playerList.Clear();
		
		m_CachedTickets = tickets;

		// Add the list of tickets with their IDs
		foreach (int playerId : tickets)
		{
			string name = m_playerManager.GetPlayerName(playerId);
			playerList.AddItem(string.Format("%1:%2", playerId ,name));
		}

		AttachContextMenu(playerList);

		// "Open ticket" from another tab's right-click menu - rows are in the order of the tickets array
		if (m_iTicketToSelect > 0)
		{
			int index = tickets.Find(m_iTicketToSelect);
			m_iTicketToSelect = 0;
			if (index >= 0 && index < playerList.GetItemCount())
				playerList.SetItemSelected(index, true, true);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Populates the list of messages selected in list 1
	void PopulateAdminActionsList()
	{
		array<ref COA_AdminActionLog> reversed = {};
		
		// Setup selection change handlers
		OverlayWidget list5root = OverlayWidget.Cast(m_wRoot.FindAnyWidget("List5Box"));
		SCR_ListBoxComponent list5 = SCR_ListBoxComponent.Cast(list5root.FindHandler(SCR_ListBoxComponent));
		
		// Get list of logs of admin aciton in the current mission
		array<ref COA_AdminActionLog> actions = m_AdminMenuManager.GetAdminActionLogs();
		if (!actions)
			return;
		
		// Reverse the order of logs so latest is at the top
		for (int i = actions.Count() - 1; i >= 0; i--)
		{
			reversed.Insert(actions[i]);
		}
		
		// Clear old logs 
		list5.Clear();
		
		// Grab current log filters
		SCR_ButtonTextComponent filterLogHigh = GetMenuButton("HighFilterButton", m_wRoot);
		SCR_ButtonTextComponent filterLogMedium = GetMenuButton("MediumFilterButton", m_wRoot);
		SCR_ButtonTextComponent filterLogLow = GetMenuButton("LowFilterButton", m_wRoot);
		if (!filterLogHigh || !filterLogMedium || !filterLogLow)
			return;

		// Format and add the messages to the list
		foreach (int i, ref COA_AdminActionLog action : reversed)
		{
			Color textColor = Color.FromSRGBA(239, 242, 247, 255);

			if (action.level == COA_EAdminLogLevel.High)
				if (!filterLogHigh.IsToggled())
					continue;
				else
					textColor = Color.FromSRGBA(232, 96, 96, 255);

			if (action.level == COA_EAdminLogLevel.Medium)
				if (!filterLogMedium.IsToggled())
					continue;
				else
					textColor = Color.FromSRGBA(232, 170, 72, 255);

			if (action.level == COA_EAdminLogLevel.Low && !filterLogLow.IsToggled())
				continue;

			list5.AddItemWithColor(string.Format("%1 - %2", action.timestamp, action.action), textColor);
		}
	}

	//-----------------------------------------------------------------------------
	// Respawn Menu Methods
	//-----------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Initialize the Respawn menu
	//! Allows admins to respawn dead players
	void InitializeRespawnMenu()
	{
		// Load menu content widget
		m_wMenuContent = GetGame().GetWorkspace().CreateWidgets("{0F4AF70DE5AA8A96}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/RespawnMenu.layout");
		COA_UIPolish.AttachHoverEffects(m_wMenuContent); // hover feedback - sub-menus are separate workspace roots
		if (!m_wMenuContent)
			return;
		
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		SCR_ListBoxComponent groupList = GetListBox("GroupListBox0");
		if (!playerList || !groupList)
			return;
		
		// Load Menu Buttons
		SCR_ButtonTextComponent searchButton0 = GetMenuButton("SearchButton0");
		SCR_ButtonTextComponent menuButton0 = GetMenuButton("MenuButton0");
		SCR_ButtonTextComponent menuButton1 = GetMenuButton("BLUFOR");
		SCR_ButtonTextComponent menuButton2 = GetMenuButton("OPFOR");
		SCR_ButtonTextComponent menuButton3 = GetMenuButton("INDFOR");
		SCR_ButtonTextComponent menuButton4 = GetMenuButton("CIV");
		if (!searchButton0 || !menuButton0 || !menuButton1 || !menuButton2 || !menuButton3 || !menuButton4)
			return;
			
		// Setup button event handlers
		searchButton0.m_OnClicked.Insert(SearchList0);
		menuButton0.m_OnClicked.Insert(RespawnPlayer);
		menuButton1.m_OnClicked.Insert(RespawnSide);
		menuButton2.m_OnClicked.Insert(RespawnSide);
		menuButton3.m_OnClicked.Insert(RespawnSide);
		menuButton4.m_OnClicked.Insert(RespawnSide);
		
		// Setup selection change handlers
		playerList.m_OnChanged.Insert(UpdateRespawnSelectedGroup);
		groupList.m_OnChanged.Insert(UpdateRespawnSpawnpoint);

		// Setup List Filters
		SCR_ButtonTextComponent menuButtonTicketOpenToggle = GetMenuButton("OpenTicketFilterButton");
		if (!menuButtonTicketOpenToggle)
			return;
		
		menuButtonTicketOpenToggle.m_OnToggled.Insert(ToggleOpenTicketFilter);
		
		// Change title of the menu
		UpdateMenuTitle("Respawn");

		// Get all players and groups
		m_playerManager.GetPlayers(m_allPlayers);
		m_outGroups = COA_SlottingManager.GetInstance().GetAllGroups();

		// Populate Dead Players list
		PopulatePlayerList(playerList, true, menuButtonTicketOpenToggle.IsToggled(), filterDeadOnly: true);
		
		// Populate Groups list
		PopulateGroupsList(groupList);
	}

	//------------------------------------------------------------------------------------------------
	//! Requests server to provide group ID for selected player
	void UpdateRespawnSelectedGroup()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		SCR_ListBoxComponent groupList = GetListBox("GroupListBox0");
		if (!groupList)
			return;
		
		SelectGroupByPlayer(groupList, playerList);
	}

	//------------------------------------------------------------------------------------------------
	//! Updates spawnpoint list based on selected group
	void UpdateRespawnSpawnpoint()
	{
		// Load List Boxes
		SCR_ListBoxComponent respawnPoints = GetListBox("SpawnpointListBox0");
		SCR_ListBoxComponent groupList = GetListBox("GroupListBox0");
		if (!respawnPoints || !groupList)
			return;
		
		if (groupList.GetSelectedItem() < 0)
			return;
			
		// Clear previous data
		respawnPoints.Clear();
		m_spawnPoints.Clear();
		
		// Get selected group ID
		int groupID = m_groupIDList.Get(groupList.GetSelectedItem());

		// Get player names
		TStringArray playerNames = {};
		foreach (int playerId : m_allPlayers)
			playerNames.Insert(m_playerManager.GetPlayerName(playerId));
		playerNames.Sort(false);

		// Add players from the selected group as spawnpoints
		foreach (string name : playerNames)
		{
			int playerId = GetplayerIdFromName(name);
			SCR_AIGroup playerGroup = COA_SlottingManager.GetInstance().GetPlayerSlotGroup(playerId);
			
			if (!playerGroup)
				continue;
				
			if (playerGroup.GetGroupID() == groupID)
			{
				respawnPoints.AddItem(string.Format("%1", m_playerManager.GetPlayerName(playerId)));
				m_spawnPoints.Insert(playerId);
			}
		}

		AttachContextMenu(respawnPoints);
	}

	//------------------------------------------------------------------------------------------------
	//! Respawns the selected player at the selected spawnpoint and group
	void RespawnPlayer()
	{
		// Load List Boxes
		SCR_ListBoxComponent respawnPoints = GetListBox("SpawnpointListBox0");
		SCR_ListBoxComponent groupList = GetListBox("GroupListBox0");
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!respawnPoints || !groupList)
			return;
		
		if (playerList.GetSelectedItem() < 0)
			return;

		if (groupList.GetSelectedItem() < 0)
			return;

		if (respawnPoints.GetSelectedItem() < 0)
			return;
		
		// Get selected player
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		// Get selected group and spawnpoint
		int groupID = m_groupIDList.Get(groupList.GetSelectedItem());
		int spawnpoint = m_spawnPoints.Get(respawnPoints.GetSelectedItem());
		
		// Spawn player on group
		COA_PlayerRplToAuthorityManager.GetInstance().SpawnOnGroup(playerId, spawnpoint, groupID, true);

		// Refresh the menu after a short delay
		GetGame().GetCallqueue().CallLater(ClearMenu, 1250, false);
		GetGame().GetCallqueue().CallLater(InitializeRespawnMenu, 1825, false);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Respawns blufor
	void RespawnSide()
	{
		// Find the button currently focused
		Widget button = GetGame().GetWorkspace().GetFocusedWidget();
		if (!button)
			return;	
		
		COA_PlayerRplToAuthorityManager.GetInstance().RespawnFaction(button.GetName(), true);
	}

	//-----------------------------------------------------------------------------
	// Teleport Menu Methods
	//-----------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Initialize the Teleport menu
	//! Allows admins to teleport players
	void InitializeTeleportMenu()
	{
		// Load menu content widget
		m_wMenuContent = GetGame().GetWorkspace().CreateWidgets("{681BEBC7B2B45D4E}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/TeleportMenu.layout");
		COA_UIPolish.AttachHoverEffects(m_wMenuContent); // hover feedback - sub-menus are separate workspace roots
		if (!m_wMenuContent)
			return;
		
		// Load List Boxes
		SCR_ListBoxComponent playerList0 = GetListBox("PlayerListBox0");
		SCR_ListBoxComponent playerList1 = GetListBox("PlayerListBox1");
		SCR_ListBoxComponent groupList = GetListBox("GroupListBox0");
		if (!playerList0 || !playerList1 || !groupList)
			return;
		
		// Load Menu Buttons
		SCR_ButtonTextComponent searchButton0 = GetMenuButton("SearchButton0");
		SCR_ButtonTextComponent searchButton1 = GetMenuButton("SearchButton1");
		SCR_ButtonTextComponent menuButton0 = GetMenuButton("MenuButton0");
		SCR_ButtonTextComponent menuButton1 = GetMenuButton("MenuButton1");
		SCR_ButtonTextComponent menuButton2 = GetMenuButton("MenuButton2");
		if (!searchButton0 || !searchButton1 || !menuButton0 || !menuButton1 || !menuButton2)
			return;
		
		// Setup button event handlers
		searchButton0.m_OnClicked.Insert(SearchList0);
		searchButton1.m_OnClicked.Insert(SearchList1);
		menuButton0.m_OnClicked.Insert(TeleportLocalToSelected);
		menuButton1.m_OnClicked.Insert(TeleportPlayers);
		menuButton2.m_OnClicked.Insert(TeleportSelectedToLocal);

		// Setup List Filters
		SCR_ButtonTextComponent menuButtonTicketOpenToggle = GetMenuButton("OpenTicketFilterButton");
		if (!menuButtonTicketOpenToggle)
			return;
		
		menuButtonTicketOpenToggle.m_OnToggled.Insert(ToggleOpenTicketFilter);

		// Setup selection change handlers
		playerList0.m_OnChanged.Insert(UpdateTeleportGroupList);
		groupList.m_OnChanged.Insert(UpdateTeleportToList);

		// Change title of the menu
		UpdateMenuTitle("Teleport");

		// Get list of groups
		m_outGroups = COA_SlottingManager.GetInstance().GetAllGroups();

		// Populate player lists
		PopulatePlayerList(playerList0, true, menuButtonTicketOpenToggle.IsToggled());
		PopulatePlayerList(playerList1, false);

		// Populate group list
		PopulateGroupsList(groupList);
	}

	//------------------------------------------------------------------------------------------------
	//! Teleports local player to selected player
	void TeleportLocalToSelected()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList0 = GetListBox("PlayerListBox0");
		if (!playerList0)
			return;
		
		if (playerList0.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList0.GetElementComponent(playerList0.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId2 = GetplayerIdFromName(playerName);
		if (playerId2 == 0)
			return;

		// Teleport local player to target
		TeleportLocalPlayer(SCR_PlayerController.GetLocalPlayerId(), playerId2);
	}
	
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//! Teleports a player to another player's location
	//! \param[in]  playerId1 - Player to teleport
	//! \param[in]  playerId2 - Destination player 
	void TeleportLocalPlayer(int playerId1, int playerId2)
	{
		IEntity entity2 = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId2);
		if (!entity2)
		{
			Print("[CRF] COA_AdminMenuUI: TeleportLocalPlayer - destination player has no controlled entity (may still be spawning)", LogLevel.WARNING);
			return;
		}

		vector teleportLocation = vector.Zero;
		SCR_WorldTools.FindEmptyTerrainPosition(teleportLocation, entity2.GetOrigin(), 10);
		SCR_Global.TeleportPlayer(playerId1, teleportLocation);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Teleports selected player to local player
	void TeleportSelectedToLocal()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList0 = GetListBox("PlayerListBox0");
		if (!playerList0)
			return;
		
		if (playerList0.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList0.GetElementComponent(playerList0.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId2 = GetplayerIdFromName(playerName);
		if (playerId2 == 0)
			return;

		// Teleport local player to target
		COA_PlayerRplToAuthorityManager.GetInstance().TeleportPlayers(playerId2, SCR_PlayerController.GetLocalPlayerId(), true);
	}

	//------------------------------------------------------------------------------------------------
	//! Teleports player 1 to player 2's position
	void TeleportPlayers()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList0 = GetListBox("PlayerListBox0");
		SCR_ListBoxComponent playerList1 = GetListBox("PlayerListBox1");
		if (!playerList0 || !playerList1)
			return;
		
		if (playerList0.GetSelectedItem() < 0)
			return;

		if (playerList1.GetSelectedItem() < 0)
			return;

		// Get selected player IDs
		string playerName1 = TextWidget.Cast(playerList0.GetElementComponent(playerList0.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		string playerName2 = TextWidget.Cast(playerList1.GetElementComponent(playerList1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId1 = GetplayerIdFromName(playerName1);
		int playerId2 = GetplayerIdFromName(playerName2);

		if (playerId1 == 0 || playerId2 == 0)
			return;

		// Teleport player 1 to player 2
		COA_PlayerRplToAuthorityManager.GetInstance().TeleportPlayers(playerId1, playerId2, true);
	}

	//------------------------------------------------------------------------------------------------
	//! Update group selected based on player selected in the first player list
	void UpdateTeleportGroupList()
	{
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;

		SCR_ListBoxComponent groupList = GetListBox("GroupListBox0");
		if (!groupList)
			return;

		SelectGroupByPlayer(groupList, playerList)
	}
	
	//------------------------------------------------------------------------------------------------
	//! Update teleport target from the selected group
	void UpdateTeleportToList()
	{
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox1");
		if (!playerList)
			return;

		SCR_ListBoxComponent groupList = GetListBox("GroupListBox0");
		if (!groupList)
			return;

		// Get the ID of the group selected
		int groupID = m_groupIDList.Get(groupList.GetSelectedItem());
		if (groupID == -1)
			return;

		PopulatePlayerList(playerList, false, false, m_groupManagerComponent.FindGroup(groupID));
	}

	//-----------------------------------------------------------------------------
	// Hint Menu Methods
	//-----------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Initialize the Hint menu
	//! Allows admins to send hint messages to players
	void InitializeHintMenu()
	{
		// Load menu content widget
		m_wMenuContent = GetGame().GetWorkspace().CreateWidgets("{10F6DA929AEE2069}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/HintMenu.layout");
		COA_UIPolish.AttachHoverEffects(m_wMenuContent); // hover feedback - sub-menus are separate workspace roots
		if (!m_wMenuContent)
			return;
		
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		SCR_ListBoxComponent factionList = GetListBox("FactionListBox0");
		if (!playerList || !factionList)
			return;
		
		// Load Reply Box
		MultilineEditBoxWidget hintBox = GetMultilineEditBox("HintBox0");
		if (!hintBox)
			return;
		
		// Load Menu Buttons
		SCR_ButtonTextComponent searchButton0 = GetMenuButton("SearchButton0");
		SCR_ButtonTextComponent menuButton0 = GetMenuButton("MenuButton0");
		SCR_ButtonTextComponent menuButton1 = GetMenuButton("MenuButton1");
		SCR_ButtonTextComponent menuButton2 = GetMenuButton("MenuButton2");
		if (!searchButton0 || !menuButton0 || !menuButton1 || !menuButton2)
			return;
		
		// Setup existing hint text if available
		hintBox.SetText(m_clientComponent.m_sHintText);
		
		// Setup button event handlers
		searchButton0.m_OnClicked.Insert(SearchList0);
		menuButton0.m_OnClicked.Insert(SendHintAll);
		menuButton1.m_OnClicked.Insert(SendHintFaction);
		menuButton2.m_OnClicked.Insert(SendHintPlayer);

		// Setup List Filters
		SCR_ButtonTextComponent menuButtonTicketOpenToggle = GetMenuButton("OpenTicketFilterButton");
		if (!menuButtonTicketOpenToggle)
			return;
		
		menuButtonTicketOpenToggle.m_OnToggled.Insert(ToggleOpenTicketFilter);
		
		// Change title of the menu
		UpdateMenuTitle("Hint");

		// Populate player list
		PopulatePlayerList(playerList, true, menuButtonTicketOpenToggle.IsToggled());
		
		// Populate faction list
		PopulateFactionList(factionList);

		// One-click templates and the HUD countdown (optional parts of the layout)
		InitializeAnnouncementTemplates();
		InitializeCountdownControls();
		m_bHintMenuOpen = true;
		UpdateCountdownStatus();
	}

	//------------------------------------------------------------------------------------------------
	//! Sends hint message to all players
	void SendHintAll()
	{
		// Load Reply Box
		MultilineEditBoxWidget hintBox = GetMultilineEditBox("HintBox0");
		if (!hintBox)
			return;
		
		string data = hintBox.GetText();
		m_clientComponent.m_sHintText = data;
		COA_PlayerRplToAuthorityManager.GetInstance().SendHint(data);
	}

	//------------------------------------------------------------------------------------------------
	//! Sends hint message to players in selected faction
	void SendHintFaction()
	{
		// Load List Boxes
		SCR_ListBoxComponent factionList = GetListBox("FactionListBox0");
		if (!factionList)
			return;
	
		// Load Reply Box
		MultilineEditBoxWidget hintBox = GetMultilineEditBox("HintBox0");
		if (!hintBox)
			return;
		
		if (factionList.GetSelectedItem() == -1)
			return;

		string data = hintBox.GetText();
		m_clientComponent.m_sHintText = data;
		string factionKey = m_selectableFactions.Get(factionList.GetSelectedItem());
		COA_PlayerRplToAuthorityManager.GetInstance().SendHint(data, -1, factionKey);
	}

	//------------------------------------------------------------------------------------------------
	//! Sends hint message to selected player
	void SendHintPlayer()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		// Load Reply Box
		MultilineEditBoxWidget hintBox = GetMultilineEditBox("HintBox0");
		if (!hintBox)
			return;
		
		if (playerList.GetSelectedItem() == -1)
			return;

		string data = hintBox.GetText();
		m_clientComponent.m_sHintText = data;
		
		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		COA_PlayerRplToAuthorityManager.GetInstance().SendHint(data, playerId);
	}

	//-----------------------------------------------------------------------------
	// Hint Menu: Announcement Templates & Countdown
	//-----------------------------------------------------------------------------

	protected ref array<string> m_aAnnouncementTemplates = {};
	protected string m_sCountdownMessage;		// feedback shown in place of the countdown status for a moment
	protected float m_fCountdownMessageUntil;	// world time (ms)

	//------------------------------------------------------------------------------------------------
	//! Fill the TEMPLATES list (COA_AdminAnnouncementTemplates); clicking one fills the message box,
	//! which the usual Send to player / faction / everyone buttons then send
	protected void InitializeAnnouncementTemplates()
	{
		SCR_ListBoxComponent templateList = FindListBox("TemplateListBox0");
		if (!templateList)
			return;

		COA_AdminAnnouncementTemplates.GetTemplates(m_aAnnouncementTemplates);
		foreach (string announcement : m_aAnnouncementTemplates)
		{
			templateList.AddItem(announcement);
		}

		templateList.m_OnChanged.Insert(ApplySelectedTemplate);
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplySelectedTemplate()
	{
		SCR_ListBoxComponent templateList = FindListBox("TemplateListBox0");
		MultilineEditBoxWidget hintBox = GetMultilineEditBox("HintBox0");
		if (!templateList || !hintBox)
			return;

		int index = templateList.GetSelectedItem();
		if (!m_aAnnouncementTemplates.IsIndexValid(index))
			return;

		hintBox.SetText(m_aAnnouncementTemplates[index]);
		m_clientComponent.m_sHintText = m_aAnnouncementTemplates[index];
	}

	//------------------------------------------------------------------------------------------------
	protected void InitializeCountdownControls()
	{
		SCR_ButtonTextComponent startAllButton = GetMenuButton("CountdownAllButton");
		if (startAllButton)
			startAllButton.m_OnClicked.Insert(StartCountdownAll);

		SCR_ButtonTextComponent startFactionButton = GetMenuButton("CountdownFactionButton");
		if (startFactionButton)
			startFactionButton.m_OnClicked.Insert(StartCountdownFaction);

		SCR_ButtonTextComponent cancelButton = GetMenuButton("CountdownCancelButton");
		if (cancelButton)
			cancelButton.m_OnClicked.Insert(CancelAdminCountdown);
	}

	//------------------------------------------------------------------------------------------------
	//! Read the countdown label and duration fields
	//! \return false (with feedback shown) if the duration isn't usable
	protected bool ReadCountdownInput(out string label, out int seconds)
	{
		EditBoxWidget labelBox = GetEditBox("CountdownLabelBox");
		EditBoxWidget minutesBox = GetEditBox("CountdownMinutesBox");
		EditBoxWidget secondsBox = GetEditBox("CountdownSecondsBox");
		if (!labelBox || !minutesBox || !secondsBox)
			return false;

		string labelText = labelBox.GetText();
		string minutesText = minutesBox.GetText();
		string secondsText = secondsBox.GetText();

		label = labelText.Trim();
		int minutes = minutesText.Trim().ToInt();
		int extraSeconds = secondsText.Trim().ToInt();
		seconds = minutes * 60 + extraSeconds;

		if (minutes < 0 || extraSeconds < 0 || seconds <= 0)
		{
			ShowCountdownMessage("Enter a duration in minutes and/or seconds.");
			return false;
		}

		if (seconds > COA_AdminMenuManager.COUNTDOWN_MAX_SECONDS)
		{
			ShowCountdownMessage("A countdown can run for 2 hours at most.");
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	void StartCountdownAll()
	{
		string label;
		int seconds;
		if (!ReadCountdownInput(label, seconds))
			return;

		COA_PlayerRplToAuthorityManager.GetInstance().StartAdminCountdown(label, seconds, string.Empty);
		ShowCountdownMessage("Countdown started for everyone.");
	}

	//------------------------------------------------------------------------------------------------
	//! For the faction selected in the FACTIONS list
	void StartCountdownFaction()
	{
		SCR_ListBoxComponent factionList = FindListBox("FactionListBox0");
		int factionIndex = -1;
		if (factionList)
			factionIndex = factionList.GetSelectedItem();

		if (!m_selectableFactions.IsIndexValid(factionIndex))
		{
			ShowCountdownMessage("Select a faction in the Factions list first.");
			return;
		}

		string label;
		int seconds;
		if (!ReadCountdownInput(label, seconds))
			return;

		string factionKey = m_selectableFactions[factionIndex];
		COA_PlayerRplToAuthorityManager.GetInstance().StartAdminCountdown(label, seconds, factionKey);
		ShowCountdownMessage(string.Format("Countdown started for %1.", factionKey));
	}

	//------------------------------------------------------------------------------------------------
	void CancelAdminCountdown()
	{
		if (!m_AdminMenuManager || !m_AdminMenuManager.IsCountdownActive())
		{
			ShowCountdownMessage("No countdown is running.");
			return;
		}

		COA_PlayerRplToAuthorityManager.GetInstance().CancelAdminCountdown();
		ShowCountdownMessage("Countdown cancelled.");
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowCountdownMessage(string message)
	{
		m_sCountdownMessage = message;
		m_fCountdownMessageUntil = GetGame().GetWorld().GetWorldTime() + 4000;
		UpdateCountdownStatus();
	}

	//------------------------------------------------------------------------------------------------
	//! Status line under the countdown buttons - refreshed every second while the Hints tab is open
	protected void UpdateCountdownStatus()
	{
		TextWidget statusText = FindText("CountdownStatusText");
		if (!statusText)
			return;

		if (!m_sCountdownMessage.IsEmpty() && GetGame().GetWorld().GetWorldTime() < m_fCountdownMessageUntil)
		{
			statusText.SetText(m_sCountdownMessage);
			return;
		}

		m_sCountdownMessage = string.Empty;

		if (!m_AdminMenuManager || !m_AdminMenuManager.IsCountdownActive())
		{
			statusText.SetText("No countdown running.");
			return;
		}

		float remainingMs = m_AdminMenuManager.GetCountdownRemainingMs();
		if (remainingMs <= 0)
		{
			statusText.SetText(string.Format("\"%1\" has finished.", m_AdminMenuManager.GetCountdownLabel()));
			return;
		}

		string target = "everyone";
		if (!m_AdminMenuManager.GetCountdownFaction().IsEmpty())
			target = m_AdminMenuManager.GetCountdownFaction();

		statusText.SetText(string.Format("Running for %1: \"%2\", %3 left.", target, m_AdminMenuManager.GetCountdownLabel(), COA_AdminCountdownDisplay.FormatRemaining(remainingMs)));
	}

	//-----------------------------------------------------------------------------
	// Heal Menu Methods
	//-----------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	//! Initialize the Heal menu
	//! Allows admins to heal players and repair vehicles
	void InitializeHealMenu()
	{
		// Load menu content widget
		m_wMenuContent = GetGame().GetWorkspace().CreateWidgets("{CCFF9CCE4508B294}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/HealMenu.layout");
		COA_UIPolish.AttachHoverEffects(m_wMenuContent); // hover feedback - sub-menus are separate workspace roots
		if (!m_wMenuContent)
			return;
		
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		// Load Menu Buttons
		SCR_ButtonTextComponent searchButton0 = GetMenuButton("SearchButton0");
		SCR_ButtonTextComponent menuButton0 = GetMenuButton("MenuButton0");
		SCR_ButtonTextComponent menuButton1 = GetMenuButton("MenuButton1");
		if (!searchButton0 || !menuButton0 || !menuButton1)
			return;
		
		// Setup button event handlers
		menuButton0.m_OnClicked.Insert(HealPlayer);
		menuButton1.m_OnClicked.Insert(HealPlayerVehicle);
		searchButton0.m_OnClicked.Insert(SearchList0);

		// Setup List Filters
		SCR_ButtonTextComponent menuButtonTicketOpenToggle = GetMenuButton("OpenTicketFilterButton");
		if (!menuButtonTicketOpenToggle)
			return;
		
		menuButtonTicketOpenToggle.m_OnToggled.Insert(ToggleOpenTicketFilter);
		
		// Change title of the menu
		UpdateMenuTitle("Heal");
		

		// Populate player list
		PopulatePlayerList(playerList, true, menuButtonTicketOpenToggle.IsToggled());
	}
	
	//------------------------------------------------------------------------------------------------
	//! Heals the selected player
	void HealPlayer()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		if (playerList.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;

		// Heal player only (not vehicle)
		COA_PlayerRplToAuthorityManager.GetInstance().Heal(playerId, true, false);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Repairs the vehicle of the selected player
	void HealPlayerVehicle()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		if (playerList.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;

		// Heal player and vehicle
		COA_PlayerRplToAuthorityManager.GetInstance().Heal(playerId, true, true);
	}
	
	//-----------------------------------------------------------------------------
	// Gamemode Settings Menu
	//-----------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Initialize the Respawn menu
	//! Allows admins to respawn dead players
	void InitializeGamemodeMenu()
	{
		// Load menu content widget
		m_wMenuContent = GetGame().GetWorkspace().CreateWidgets("{36D941F5D1C10513}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/GameModeMenu.layout");
		COA_UIPolish.AttachHoverEffects(m_wMenuContent); // hover feedback - sub-menus are separate workspace roots
		if (!m_wMenuContent)
			return;
		
		m_bGameModeMenuOpen = true;
		// Load Menu Sections
		Widget gamerTimer = m_wMenuContent.FindAnyWidget("GameTimer");
		Widget ticketCounters = m_wMenuContent.FindAnyWidget("Tickets");
		Widget gearSets = m_wMenuContent.FindAnyWidget("GearSets");
		
		// Load Buttons
		SCR_ButtonTextComponent resetGearButton = GetMenuButton("ApplyGearSets", gearSets);
		SCR_ButtonTextComponent aarGearButton = GetMenuButton("EnterAAR");
		if (!resetGearButton || !aarGearButton)
			return;
		
		// Setup invokers
		resetGearButton.m_OnClicked.Insert(ConfirmAction);
		aarGearButton.m_OnClicked.Insert(ConfirmEnterAAR);

		// End with result: a winner or a draw plus a result line (EndMissionDialog.layout)
		SCR_ButtonTextComponent endMissionButton = GetMenuButton("EndMissionButton");
		if (endMissionButton)
			endMissionButton.m_OnClicked.Insert(OpenEndMissionDialog);
		
		//	!!!!! Changing the time delta is done below and in the menu layout !!!!!
		
		// Load Menu Buttons for Game Timer
		
		// Time Values
		ref array<int> timeValues = {10, 5, -5, -10};	
		
		foreach (int time : timeValues)
		{
			string buttonName = string.Format("%1", time);
			SCR_ButtonTextComponent button = GetMenuButton(buttonName, gamerTimer);
			if (!button)
				return;
				
			button.m_OnClicked.Insert(UpdateTime);
		}
		
		// !!!!! Changing the ticket delta is done below and in the menu layout !!!!!
		
		// Load Menu Buttons for Tickets
		
		// Faction names
		ref array<string> factions = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
		
		// Actions
		ref array<string> actions = {"Add", "Subtract"};
		
		// Ticket values
		ref array<int> values = {10, 5, 1};
		
		foreach (string faction : factions)
		{
			foreach (string action : actions)
			{
				foreach (int value : values)
				{
					string buttonName = string.Format("%1_%2_%3", faction, action, value);
					SCR_ButtonTextComponent button = GetMenuButton(buttonName, ticketCounters);
					if (!button)
						return;
						
					button.m_OnClicked.Insert(UpdateTicket);
				}
			}
		}
		
		// Load config files into listboxs and array
		LoadGearConfigList();

		array<string> gearArray = {};
		foreach (string name, string path : m_gearsetlist.gearset)
			gearArray.Insert(name);
		
		gearArray.Sort();
		
		foreach (string name : gearArray)
		{
			foreach (string faction : factions)
			{
				SCR_ListBoxComponent listBox = GetListBox(string.Format("%1ListBox", faction) ,gearSets);
				listBox.AddItem(name);
			}
		}
		
		
		// Change title of the menu
		UpdateMenuTitle("Gamemode Settings");
		
		// Update menu data
		GamemodeMenuUpdate();
		
		//Toggle Respawn Wave Button
		SCR_ButtonTextComponent toggleWaveRespawn = SCR_ButtonTextComponent.Cast(m_wMenuContent.FindAnyWidget("RespawnWaveButton").FindHandler(SCR_ButtonTextComponent));
		toggleWaveRespawn.m_OnClicked.Insert(ToggleWaveRespawn);
		
		//Toggle Respawn Enabled Button
		SCR_ButtonTextComponent toggleRespawn = SCR_ButtonTextComponent.Cast(m_wMenuContent.FindAnyWidget("EnableRespawnButton").FindHandler(SCR_ButtonTextComponent));
		toggleRespawn.m_OnClicked.Insert(ToggleRespawn);
		
		//Setting Respawn Time Button
		EditBoxWidget.Cast(m_wMenuContent.FindAnyWidget("TicketsInput")).SetText(COA_RespawnManager.GetInstance().m_iCurrentTimeToRespawn.ToString());
		SCR_ButtonTextComponent setRespawnTime = SCR_ButtonTextComponent.Cast(m_wMenuContent.FindAnyWidget("SetRespawnTimeButton").FindHandler(SCR_ButtonTextComponent));
		setRespawnTime.m_OnClicked.Insert(SetRespawnTime);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Toggle wave respawns in the gamemode
	void ToggleWaveRespawn()
	{
		COA_PlayerRplToAuthorityManager.GetInstance().ToggleWaveRespawn();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Toggle resapwns in the gamemode
	void ToggleRespawn()
	{
		COA_PlayerRplToAuthorityManager.GetInstance().ToggleRespawn();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Set respawn timer in the gamemode
	void SetRespawnTime()
	{
		int respawnTime = EditBoxWidget.Cast(m_wMenuContent.FindAnyWidget("TicketsInput")).GetText().ToInt();
		COA_PlayerRplToAuthorityManager.GetInstance().SetRespawnTime(respawnTime);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Loads list of gearscipt configs for hot swapping (Pre generated in tools)
	void LoadGearConfigList()
	{
		JsonLoadContext ctx = new JsonLoadContext();
		m_gearsetlist = new COA_GearScriptConfigStruct();
	
		if (!ctx.LoadFromFile("configs/GearScripts/GearScriptsConfigList.json"))
			return;
		
		ctx.ReadValue("", m_gearsetlist);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Add time delta based on the button name that was clicked
	//! !!!!! Changing the time delta is done above and in the menu layout !!!!!
	void UpdateTime()
	{
		// Find the button currently focused
		Widget button = GetGame().GetWorkspace().GetFocusedWidget();
		if (!button)
			return;	
		
		// Get the delta from the button name
		int delta = button.GetName().ToInt() * 60000;
		
		COA_PlayerRplToAuthorityManager.GetInstance().UpdateTimer(delta);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Updates the amount respawn tickets in the gamemode based on the faction selected
	void UpdateTicket()
	{
		// Find the button currently focused
		Widget button = GetGame().GetWorkspace().GetFocusedWidget();
		if (!button)
			return;	
		
		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		
		array<string> requestParts = {};
		button.GetName().Split("_", requestParts, true);
		if (requestParts.Count() < 3)
			return;
		
		string action = requestParts[1];
		int delta = requestParts[2].ToInt();
		FactionKey faction = requestParts[0];
		
		COA_PlayerRplToAuthorityManager rplManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (rplManager)
			rplManager.UpdateTicket(action, faction, delta);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Updates the gearscript of each active faction
	void UpdateGearSets()
	{
		// Load gearsets section
		Widget gearSets = m_wMenuContent.FindAnyWidget("GearSets");
		
		// Check if faction gearset needs updating
		ref array<string> factions = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
		foreach (string faction : factions)
		{
			SCR_ListBoxComponent listBox = GetListBox(string.Format("%1ListBox", faction) ,gearSets);
			int selectedItem = listBox.GetSelectedItem();
			if (selectedItem < 0)
				continue;
			
			// Get the gearset key from selected item in the list box
			string key = TextWidget.Cast(listBox.GetElementComponent(listBox.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
			string gearSetPath;
			m_gearsetlist.gearset.Find(key, gearSetPath);
			
			// Ask the server to update factions gear
			COA_PlayerRplToAuthorityManager.GetInstance().UpdateGearSet(faction, gearSetPath);
		}

		CloseConfirmAction();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Conformation popup for enterAAR, ApplyGearSets
	void ConfirmAction()
	{
		// Find the button currently focused
		Widget button = GetGame().GetWorkspace().GetFocusedWidget();
		if (!button)
			return;	
		
		// Load menu content widget
		m_wConfirmationMenu = GetGame().GetWorkspace().CreateWidgets("{905BF1B70A9A44AC}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/ConfirmationMenu.layout");
		COA_UIPolish.AttachHoverEffects(m_wConfirmationMenu); // hover feedback - sub-menus are separate workspace roots
		if (!m_wMenuContent)
			return;

		// Get menu buttons
		SCR_ButtonTextComponent runButton = GetMenuButton("ExcuteButton", m_wConfirmationMenu);
		SCR_ButtonTextComponent cancelButton = GetMenuButton("CancelButton", m_wConfirmationMenu);
		
		// Get the function that needs confirming from the button name in the layout
		string confirmActionFunc = button.GetName();

		// Setup script invokers
		cancelButton.m_OnClicked.Insert(CloseConfirmAction);
		switch (confirmActionFunc)
		{
			case "EnterAAR" : runButton.m_OnClicked.Insert(EnterAAR); break;
			case "ApplyGearSets" : runButton.m_OnClicked.Insert(UpdateGearSets); break;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Close confirmation popup
	void CloseConfirmAction()
	{
		if (m_wConfirmationMenu)
		 delete m_wConfirmationMenu;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Advanced the game state to AAR
	void EnterAAR()
	{
		if (!COA_EGamemodeState.AAR)
			return;

		COA_PlayerRplToAuthorityManager.GetInstance().RequestAdvanceGamemodeState(true);
		CloseConfirmAction();
	}

	//-----------------------------------------------------------------------------
	// End Mission With Result (Game Mode tab)
	//-----------------------------------------------------------------------------

	protected static const ResourceName CONFIRMATION_LAYOUT = "{905BF1B70A9A44AC}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/ConfirmationMenu.layout";
	protected static const ResourceName END_MISSION_DIALOG_LAYOUT = "{3CBAACA6BFBB04F8}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/EndMissionDialog.layout";

	protected Widget m_wEndMissionDialog;
	protected ref array<string> m_aEndMissionOutcomes = {};	// outcome key per row of OutcomeListBox0
	protected string m_sPendingEndOutcome;
	protected string m_sPendingEndNote;

	//------------------------------------------------------------------------------------------------
	//! The confirmation popup (ConfirmationMenu.layout) with a custom question
	//! \return its Execute button for the caller to hook, or null
	protected SCR_ButtonTextComponent OpenConfirmation(string message)
	{
		CloseConfirmAction();

		m_wConfirmationMenu = GetGame().GetWorkspace().CreateWidgets(CONFIRMATION_LAYOUT);
		if (!m_wConfirmationMenu)
			return null;

		COA_UIPolish.AttachHoverEffects(m_wConfirmationMenu); // hover feedback - sub-menus are separate workspace roots

		TextWidget messageText = TextWidget.Cast(m_wConfirmationMenu.FindAnyWidget("ConfirmMessage"));
		if (messageText != null && !message.IsEmpty())
			messageText.SetText(message);

		SCR_ButtonTextComponent cancelButton = GetMenuButton("CancelButton", m_wConfirmationMenu);
		if (cancelButton)
			cancelButton.m_OnClicked.Insert(CloseConfirmAction);

		return GetMenuButton("ExcuteButton", m_wConfirmationMenu);
	}

	//------------------------------------------------------------------------------------------------
	//! "Enter AAR, no result" - it ends the mission, so it asks first
	void ConfirmEnterAAR()
	{
		SCR_ButtonTextComponent runButton = OpenConfirmation("End the mission without a result?");
		if (runButton)
			runButton.m_OnClicked.Insert(EnterAAR);
	}

	//------------------------------------------------------------------------------------------------
	//! End with result: pick the winning side (the mission's playable factions) or a draw, plus an
	//! optional result line. Confirmed, then sent to the server (RequestEndMission).
	void OpenEndMissionDialog()
	{
		CloseEndMissionDialog();

		m_wEndMissionDialog = GetGame().GetWorkspace().CreateWidgets(END_MISSION_DIALOG_LAYOUT);
		if (!m_wEndMissionDialog)
			return;

		COA_UIPolish.AttachHoverEffects(m_wEndMissionDialog); // hover feedback - sub-menus are separate workspace roots

		SCR_ButtonTextComponent cancelButton = GetMenuButton("EndMissionCancelButton", m_wEndMissionDialog);
		if (cancelButton)
			cancelButton.m_OnClicked.Insert(CloseEndMissionDialog);

		SCR_ButtonTextComponent submitButton = GetMenuButton("EndMissionSubmitButton", m_wEndMissionDialog);
		if (submitButton)
			submitButton.m_OnClicked.Insert(SubmitEndMission);

		m_aEndMissionOutcomes.Clear();
		SCR_ListBoxComponent outcomeList = FindListBox("OutcomeListBox0", m_wEndMissionDialog);
		if (outcomeList)
		{
			array<string> factionKeys = {};
			GetPlayableFactionKeys(factionKeys);

			FactionManager factionManager = GetGame().GetFactionManager();
			foreach (string factionKey : factionKeys)
			{
				Faction faction;
				if (factionManager)
					faction = factionManager.GetFactionByKey(factionKey);

				string rowText = COA_MissionResultHelper.GetOutcomeText(factionKey);
				if (faction)
					outcomeList.AddItemWithColor(string.Format("%1  (%2)", rowText, faction.GetFactionName()), faction.GetFactionColor());
				else
					outcomeList.AddItem(rowText);

				m_aEndMissionOutcomes.Insert(factionKey);
			}

			outcomeList.AddItem("DRAW  (no winner)");
			m_aEndMissionOutcomes.Insert(COA_MissionResultHelper.DRAW);
		}

		// The server checks both as well - saying so here beats a button that silently does nothing
		if (!SCR_Global.IsAdmin())
			SetEndMissionStatus("Only admins can end the mission.");
		else if (!IsMissionRunning())
			SetEndMissionStatus("The mission isn't running, so there's nothing to end.");
	}

	//------------------------------------------------------------------------------------------------
	//! The mission's playable sides, in BLUFOR/OPFOR/INDFOR/CIV order: those with slots, or - if
	//! there's no slot data - those the faction manager knows
	protected void GetPlayableFactionKeys(notnull array<string> outKeys)
	{
		array<string> candidates = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
		array<string> slottedKeys = {};

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (slottingManager != null && slottingManager.GetSlotMap() != null)
		{
			foreach (int slotId, COA_SlotData slotData : slottingManager.GetSlotMap())
			{
				if (!slotData)
					continue;

				string slotFactionKey = slotData.GetSlotFactionKey();
				if (!slottedKeys.Contains(slotFactionKey))
					slottedKeys.Insert(slotFactionKey);
			}
		}

		FactionManager factionManager = GetGame().GetFactionManager();
		foreach (string candidate : candidates)
		{
			if (!slottedKeys.IsEmpty())
			{
				if (slottedKeys.Contains(candidate))
					outKeys.Insert(candidate);
			}
			else if (factionManager != null && factionManager.GetFactionByKey(candidate) != null)
			{
				outKeys.Insert(candidate);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsMissionRunning()
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		return gamemode != null && gamemode.m_GamemodeState == COA_EGamemodeState.GAME;
	}

	//------------------------------------------------------------------------------------------------
	//! Dialog's End mission button: validate, then ask for confirmation
	void SubmitEndMission()
	{
		if (!m_wEndMissionDialog)
			return;

		if (!SCR_Global.IsAdmin())
		{
			SetEndMissionStatus("Only admins can end the mission.");
			return;
		}

		if (!IsMissionRunning())
		{
			SetEndMissionStatus("The mission isn't running, so there's nothing to end.");
			return;
		}

		SCR_ListBoxComponent outcomeList = FindListBox("OutcomeListBox0", m_wEndMissionDialog);
		int index = -1;
		if (outcomeList)
			index = outcomeList.GetSelectedItem();

		if (!m_aEndMissionOutcomes.IsIndexValid(index))
		{
			SetEndMissionStatus("Pick the winner, or Draw, first.");
			return;
		}

		m_sPendingEndOutcome = m_aEndMissionOutcomes[index];
		m_sPendingEndNote = string.Empty;

		EditBoxWidget noteBox = GetEditBox("ResultNoteBox", m_wEndMissionDialog);
		if (noteBox)
			m_sPendingEndNote = COA_MissionResultHelper.SanitizeNote(noteBox.GetText());

		SetEndMissionStatus(string.Empty);

		SCR_ButtonTextComponent runButton = OpenConfirmation(string.Format("End the mission: %1?", COA_MissionResultHelper.GetOutcomeText(m_sPendingEndOutcome)));
		if (runButton)
			runButton.m_OnClicked.Insert(ExecuteEndMission);
	}

	//------------------------------------------------------------------------------------------------
	//! Confirmed: the server records the result, announces it and advances to the AAR
	void ExecuteEndMission()
	{
		CloseConfirmAction();

		if (m_sPendingEndOutcome.IsEmpty())
			return;

		COA_PlayerRplToAuthorityManager.GetInstance().RequestEndMission(m_sPendingEndOutcome, m_sPendingEndNote);

		m_sPendingEndOutcome = string.Empty;
		m_sPendingEndNote = string.Empty;
		CloseEndMissionDialog();
	}

	//------------------------------------------------------------------------------------------------
	void CloseEndMissionDialog()
	{
		if (m_wEndMissionDialog)
			delete m_wEndMissionDialog;
	}

	//------------------------------------------------------------------------------------------------
	protected void SetEndMissionStatus(string text)
	{
		if (!m_wEndMissionDialog)
			return;

		TextWidget statusText = TextWidget.Cast(m_wEndMissionDialog.FindAnyWidget("EndMissionStatusText"));
		if (statusText)
			statusText.SetText(text);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Updates the UI Elements
	void GamemodeMenuUpdate()
	{	
		// Get current mission time
		string m_sServerWorldTime = COA_GameTimerManager.GetInstance().GetServerWorldTime();
		
		// Grab timer
		Widget gamerTimer = m_wMenuContent.FindAnyWidget("GameTimer");
		TextWidget gameTimerText = TextWidget.Cast(gamerTimer.FindWidget("CurrentGameTime0"));
		
		// Update Timer
		gameTimerText.SetText(m_sServerWorldTime);
		
		COA_Gamemode gm = COA_Gamemode.GetInstance();

		ref array<string> factions = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
		
		// Update Gearset titles to current gearsets
		Widget gearSets = m_wMenuContent.FindAnyWidget("GearSets");
		foreach (string faction : factions)
		{
			string resourceName;
			switch (faction)
			{
				case "BLUFOR" : resourceName = gm.m_rBLUFORCurrentGearScript; break;
				case "OPFOR" : resourceName = gm.m_rOPFORCurrentGearScript; break;
				case "INDFOR" : resourceName = gm.m_rINDFORCurrentGearScript; break;
				case "CIV" : resourceName = gm.m_rCIVILIANCurrentGearScript; break;
			}
			
			string gearSetName =  resourceName.Substring(resourceName.LastIndexOf("/") + 1, resourceName.LastIndexOf(".") - resourceName.LastIndexOf("/") - 1);
			gearSetName.Replace("COA_GS_", "");
			TextWidget.Cast(gearSets.FindAnyWidget(string.Format("%1ListTitle", faction))).SetText(gearSetName);
		}

		// Grab Ticket Counters
		Widget ticketCounters = m_wMenuContent.FindAnyWidget("Tickets");
		TextWidget bluforTicketText = TextWidget.Cast(ticketCounters.FindWidget("BluforTicketCount"));
		TextWidget opforTicketText = TextWidget.Cast(ticketCounters.FindWidget("OpforTicketCount"));
		TextWidget indforTicketText = TextWidget.Cast(ticketCounters.FindWidget("IndforTicketCount"));
		TextWidget civTicketText = TextWidget.Cast(ticketCounters.FindWidget("CivTicketCount"));
		
		// Update Ticket Counters
		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		if (respawnManager)
		{
			bluforTicketText.SetText(respawnManager.GetFactionTickets("BLUFOR").ToString());
			opforTicketText.SetText(respawnManager.GetFactionTickets("OPFOR").ToString());
			indforTicketText.SetText(respawnManager.GetFactionTickets("INDFOR").ToString());
			civTicketText.SetText(respawnManager.GetFactionTickets("CIV").ToString());
		}
		
		Widget respawnWaveButton = m_wMenuContent.FindAnyWidget("RespawnWaveButton");
		TextWidget respawnWaveButtonText = TextWidget.Cast(respawnWaveButton.FindWidget("ActionButtonText"));
		bool respawnWave = COA_RespawnManager.GetInstance().m_bCurrentWaveRespawn;
		if (respawnWave)
		{
			respawnWaveButtonText.SetText("Respawn Wave Enabled");
			respawnWaveButtonText.SetColor(Color.FromSRGBA(92, 196, 128, 255));
		}
		else
		{
			respawnWaveButtonText.SetText("Respawn Wave Disabled");
			respawnWaveButtonText.SetColor(Color.FromSRGBA(232, 96, 96, 255));
		}
		
		bool m_bRespawnEnabled = COA_RespawnManager.GetInstance().m_bCurrentRespawnEnabled;
		Widget respawnEnabledButton = m_wMenuContent.FindAnyWidget("EnableRespawnButton");
		TextWidget respawnEnabledText = TextWidget.Cast(respawnEnabledButton.FindWidget("ActionButtonText"));
		if (m_bRespawnEnabled)
		{
			respawnEnabledText.SetText("Respawns Enabled");
			respawnEnabledText.SetColor(Color.FromSRGBA(92, 196, 128, 255));
		}
		else
		{
			respawnEnabledText.SetText("Respawns Disabled");
			respawnEnabledText.SetColor(Color.FromSRGBA(232, 96, 96, 255));
		}
	}
	
	//-----------------------------------------------------------------------------
	// Gear Fix Menu
	//-----------------------------------------------------------------------------

	protected static const ResourceName GEAR_FIX_MENU_LAYOUT = "{A3D95E17C2B8460D}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/GearFixMenu.layout";

	//------------------------------------------------------------------------------------------------
	//! Session-wide gear overrides and the gear check. All of it lives in COA_GearFixPanel.
	void InitializeGearFixMenu()
	{
		// Load menu content widget
		m_wMenuContent = GetGame().GetWorkspace().CreateWidgets(GEAR_FIX_MENU_LAYOUT);
		if (!m_wMenuContent)
			return;

		COA_UIPolish.AttachHoverEffects(m_wMenuContent); // hover feedback - sub-menus are separate workspace roots

		// Change title of the menu
		UpdateMenuTitle("Gear Fix");

		m_GearFixPanel = new COA_GearFixPanel();
		m_GearFixPanel.Init(m_wMenuContent);
	}

	//------------------------------------------------------------------------------------------------
	protected void CleanupGearFixPanel()
	{
		if (m_GearFixPanel)
			m_GearFixPanel.Cleanup();

		m_GearFixPanel = null;
	}

	//-----------------------------------------------------------------------------
	// Weather & Time Menu
	//-----------------------------------------------------------------------------

	protected static const ResourceName ENVIRONMENT_MENU_LAYOUT = "{B997328C23673A96}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/EnvironmentMenu.layout";

	//! In-game hours the Blend button's weather change takes (15 in-game minutes)
	protected static const float WEATHER_BLEND_HOURS = 0.25;

	protected ref array<string> m_aWeatherStateNames = {};	// state name per row of WeatherListBox0

	//------------------------------------------------------------------------------------------------
	//! Initialize the Weather & Time menu: weather states read from the world's own weather state
	//! machine (so a mod's WeatherStates.conf override shows its states, not vanilla's), and the time of day
	void InitializeEnvironmentMenu()
	{
		// Load menu content widget
		m_wMenuContent = GetGame().GetWorkspace().CreateWidgets(ENVIRONMENT_MENU_LAYOUT);
		if (!m_wMenuContent)
			return;

		COA_UIPolish.AttachHoverEffects(m_wMenuContent); // hover feedback - sub-menus are separate workspace roots

		// Change title of the menu
		UpdateMenuTitle("Weather & Time");

		PopulateWeatherList();

		SCR_ButtonTextComponent instantButton = GetMenuButton("WeatherInstantButton");
		if (instantButton)
			instantButton.m_OnClicked.Insert(ApplyWeatherInstant);

		SCR_ButtonTextComponent blendButton = GetMenuButton("WeatherBlendButton");
		if (blendButton)
			blendButton.m_OnClicked.Insert(ApplyWeatherBlend);

		SCR_ButtonTextComponent setTimeButton = GetMenuButton("SetTimeButton");
		if (setTimeButton)
			setTimeButton.m_OnClicked.Insert(SetTimeOfDayFromInput);

		// Quick presets - the time is in the button's name, "Preset_HHMM" (see EnvironmentMenu.layout)
		array<string> presetNames = {"Preset_0530", "Preset_0800", "Preset_1200", "Preset_1830", "Preset_2300"};
		foreach (string presetName : presetNames)
		{
			SCR_ButtonTextComponent presetButton = GetMenuButton(presetName);
			if (presetButton)
				presetButton.m_OnClicked.Insert(SetTimeOfDayPreset);
		}

		// Start the time fields at the current in-game time
		TimeAndWeatherManagerEntity weatherManager = FindTimeAndWeatherManager();
		EditBoxWidget hourInput = GetEditBox("HourInput");
		EditBoxWidget minuteInput = GetEditBox("MinuteInput");
		if (weatherManager != null && hourInput != null && minuteInput != null)
		{
			int hours;
			int minutes;
			SplitTimeOfDay(weatherManager.GetTimeOfTheDay(), hours, minutes);
			hourInput.SetText(hours.ToString(2));
			minuteInput.SetText(minutes.ToString(2));
		}

		m_bEnvironmentMenuOpen = true;
		EnvironmentMenuUpdate();
	}

	//------------------------------------------------------------------------------------------------
	protected static TimeAndWeatherManagerEntity FindTimeAndWeatherManager()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return null;

		return world.GetTimeAndWeatherManager();
	}

	//------------------------------------------------------------------------------------------------
	//! \param[in] timeOfDay24 Hours <0, 24) as from GetTimeOfTheDay
	protected static void SplitTimeOfDay(float timeOfDay24, out int hours, out int minutes)
	{
		int totalMinutes = Math.Floor(timeOfDay24 * 60);
		hours = (totalMinutes / 60) % 24;
		minutes = totalMinutes % 60;
	}

	//------------------------------------------------------------------------------------------------
	//! Every state in the world's weather state machine, current one selected
	protected void PopulateWeatherList()
	{
		SCR_ListBoxComponent weatherList = FindListBox("WeatherListBox0");
		TimeAndWeatherManagerEntity weatherManager = FindTimeAndWeatherManager();
		if (!weatherList || !weatherManager)
			return;

		weatherList.Clear();
		m_aWeatherStateNames.Clear();

		array<ref WeatherState> weatherStates = {};
		weatherManager.GetWeatherStatesList(weatherStates);

		foreach (WeatherState weatherState : weatherStates)
		{
			if (!weatherState)
				continue;

			string stateName = weatherState.GetStateName();
			if (stateName.IsEmpty())
				continue;

			m_aWeatherStateNames.Insert(stateName);
			weatherList.AddItem(stateName);
		}

		WeatherState currentState = weatherManager.GetCurrentWeatherState();
		if (!currentState)
			return;

		int currentIndex = m_aWeatherStateNames.Find(currentState.GetStateName());
		if (currentIndex >= 0)
			weatherList.SetItemSelected(currentIndex, true, false);
	}

	//------------------------------------------------------------------------------------------------
	void ApplyWeatherInstant()
	{
		ApplySelectedWeather(0);
	}

	//------------------------------------------------------------------------------------------------
	void ApplyWeatherBlend()
	{
		ApplySelectedWeather(WEATHER_BLEND_HOURS);
	}

	//------------------------------------------------------------------------------------------------
	//! \param[in] transitionHours 0 = instant
	protected void ApplySelectedWeather(float transitionHours)
	{
		SCR_ListBoxComponent weatherList = FindListBox("WeatherListBox0");
		if (!weatherList)
			return;

		int index = weatherList.GetSelectedItem();
		if (!m_aWeatherStateNames.IsIndexValid(index))
		{
			SetEnvironmentStatus("Select a weather state first.");
			return;
		}

		COA_PlayerRplToAuthorityManager.GetInstance().RequestSetWeather(m_aWeatherStateNames[index], transitionHours);
		SetEnvironmentStatus(string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	void SetTimeOfDayFromInput()
	{
		EditBoxWidget hourInput = GetEditBox("HourInput");
		EditBoxWidget minuteInput = GetEditBox("MinuteInput");
		if (!hourInput || !minuteInput)
			return;

		string hourText = hourInput.GetText();
		string minuteText = minuteInput.GetText();
		hourText = hourText.Trim();
		minuteText = minuteText.Trim();

		int hours = hourText.ToInt();
		int minutes = minuteText.ToInt();
		if (hourText.IsEmpty() || hours < 0 || hours > 23 || minutes < 0 || minutes > 59)
		{
			SetEnvironmentStatus("Enter a time from 00:00 to 23:59.");
			return;
		}

		COA_PlayerRplToAuthorityManager.GetInstance().RequestSetTimeOfDay(hours, minutes);
		SetEnvironmentStatus(string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	//! Preset buttons are named "Preset_HHMM"
	void SetTimeOfDayPreset(SCR_ButtonBaseComponent button)
	{
		if (!button || !button.GetRootWidget())
			return;

		array<string> nameParts = {};
		button.GetRootWidget().GetName().Split("_", nameParts, true);
		if (nameParts.Count() < 2)
			return;

		int hhmm = nameParts[1].ToInt();
		COA_PlayerRplToAuthorityManager.GetInstance().RequestSetTimeOfDay(hhmm / 100, hhmm % 100);
		SetEnvironmentStatus(string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetEnvironmentStatus(string text)
	{
		TextWidget statusText = FindText("TimeStatusText");
		if (statusText)
			statusText.SetText(text);
	}

	//------------------------------------------------------------------------------------------------
	//! Current weather and in-game time - every second while the tab is open
	void EnvironmentMenuUpdate()
	{
		TimeAndWeatherManagerEntity weatherManager = FindTimeAndWeatherManager();
		if (!weatherManager)
			return;

		TextWidget weatherText = FindText("CurrentWeatherText");
		WeatherState currentState = weatherManager.GetCurrentWeatherState();
		if (weatherText != null && currentState != null)
			weatherText.SetText(currentState.GetStateName());

		TextWidget timeText = FindText("CurrentTimeText");
		if (timeText)
		{
			int hours;
			int minutes;
			SplitTimeOfDay(weatherManager.GetTimeOfTheDay(), hours, minutes);
			timeText.SetText(string.Format("%1:%2", hours.ToString(2), minutes.ToString(2)));
		}
	}

	//-----------------------------------------------------------------------------
	// Chat Methods
	//-----------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	//! Handles chat toggle action
	void Action_OnCOA_ChatToggleAction()
	{
		if (!m_ChatPanel)
			return;

		GetGame().GetCallqueue().Call(OpenChatWrap);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Opens the chat panel if not already open
	void OpenChatWrap()
	{
		if (!m_ChatPanel.IsOpen())
		{
			SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_ChatPanel);
		}
	}
	
	//-----------------------------------------------------------------------------
	// Right-Click Menu Methods (see COA_AdminContextMenu.c)
	//-----------------------------------------------------------------------------

	protected static const ResourceName CONTEXT_MENU_LAYOUT = "{7C1A4E2B9D3F6A01}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/AdminContextMenu.layout";
	protected static const ResourceName CONTEXT_MENU_ROW_LAYOUT = "{7C1A4E2B9D3F6A02}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/AdminContextMenuRow.layout";
	protected static const float CONTEXT_MENU_WIDTH = 240;
	protected static const float CONTEXT_MENU_HEADER = 36;
	protected static const float CONTEXT_MENU_ROW_HEIGHT = 36;
	protected static const float CONTEXT_MENU_PADDING = 6;

	protected Widget m_wContextMenu;
	protected int m_iContextPlayerId;
	protected int m_iContextTicket = -1; // the ticket right-clicked in the Tickets tab, else -1
	protected int m_iTicketToSelect; // player whose ticket "Open ticket" selects once the Tickets tab loads

	//------------------------------------------------------------------------------------------------
	//! Hook right click on every row of a player (or ticket) list - called after each repopulation
	protected void AttachContextMenu(SCR_ListBoxComponent list)
	{
		if (!list)
			return;

		int count = list.GetItemCount();
		for (int i = 0; i < count; i++)
		{
			Widget row = list.GetElementComponent(i).GetRootWidget();
			if (!row || row.FindHandler(COA_AdminListRowContextHandler))
				continue;

			COA_AdminListRowContextHandler handler = new COA_AdminListRowContextHandler();
			handler.m_Menu = this;
			handler.m_List = list;
			handler.m_wRow = row;
			row.AddHandler(handler);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Right-click menu for a list row, at the cursor
	//! \param[in] list the list the row is in
	//! \param[in] row root widget of the row right-clicked
	//! \param[in] screenX, screenY cursor position in screen pixels
	void OpenContextMenu(SCR_ListBoxComponent list, Widget row, int screenX, int screenY)
	{
		CloseContextMenu();

		int index = -1;
		int count = list.GetItemCount();
		for (int i = 0; i < count; i++)
		{
			if (list.GetElementComponent(i).GetRootWidget() == row)
			{
				index = i;
				break;
			}
		}

		if (index < 0)
			return;

		TextWidget rowText = TextWidget.Cast(row.FindAnyWidget("Text"));
		if (!rowText)
			return;

		// Ticket rows read "playerId:name" (PopulateOpenTicketList), the other lists just the name
		string rowName = rowText.GetText();
		bool isTicketRow = GetCurrentOpenTab() == "Tickets" && list == GetListBox("PlayerListBox0");
		int playerId;
		if (isTicketRow)
		{
			array<string> data = {};
			rowName.Split(":", data, true);
			if (!data.IsEmpty())
				playerId = data[0].ToInt();
		}
		else
		{
			playerId = GetplayerIdFromName(rowName);
		}

		bool connected = playerId > 0 && m_playerManager.IsPlayerConnected(playerId);
		if (!connected && !isTicketRow)
			return;

		// The menu acts on the row, so select it like a left click would (this also loads a ticket's messages)
		if (list.GetSelectedItem() != index)
			list.SetItemSelected(index, true);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		m_wContextMenu = workspace.CreateWidgets(CONTEXT_MENU_LAYOUT);
		if (!m_wContextMenu)
		{
			Print("[COA_AdminMenu] Failed to create " + CONTEXT_MENU_LAYOUT, LogLevel.WARNING);
			return;
		}

		m_wContextMenu.SetZOrder(300);
		m_iContextPlayerId = playerId;
		if (isTicketRow)
			m_iContextTicket = playerId;

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();

		TextWidget title = TextWidget.Cast(m_wContextMenu.FindAnyWidget("MenuTitle"));
		if (title)
		{
			string name = string.Format("Ticket %1", playerId);
			if (connected)
				name = m_playerManager.GetPlayerName(playerId);

			name.ToUpper();
			title.SetText(name);
		}

		// Accent in the player's faction colour, like their name in the list
		Faction faction = slottingManager.GetPlayerSlotFaction(playerId);
		Widget accent = m_wContextMenu.FindAnyWidget("MenuAccent");
		if (accent && faction)
			accent.SetColor(faction.GetFactionColor());

		COA_AdminContextActionButton backdrop = COA_AdminContextActionButton.Cast(m_wContextMenu.FindAnyWidget("Backdrop").FindHandler(COA_AdminContextActionButton));
		if (backdrop)
			backdrop.m_OnClicked.Insert(OnContextActionClicked);

		// Which actions apply to this player right now
		IEntity target = m_playerManager.GetPlayerControlledEntity(playerId);
		bool hasBody = connected && target && !COA_EntityHelper.IsSpectator(target) && !slottingManager.IsPlayerConsideredDead(playerId);
		bool canRespawn = connected && !hasBody && slottingManager.GetPlayerSlotGroup(playerId) != null;

		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		IEntity localEntity = m_playerManager.GetPlayerControlledEntity(localPlayerId);
		bool localHasBody = localEntity && !COA_EntityHelper.IsSpectator(localEntity);
		bool isSelf = playerId == localPlayerId;

		int rows;
		if (hasBody)
		{
			rows += AddContextAction("Heal", COA_EAdminContextAction.HEAL);
			if (SCR_CompartmentAccessComponent.GetVehicleIn(target))
				rows += AddContextAction("Repair vehicle", COA_EAdminContextAction.REPAIR_VEHICLE);

			rows += AddContextAction("Unstuck", COA_EAdminContextAction.UNSTUCK);
			if (localHasBody && !isSelf)
			{
				rows += AddContextAction("Bring to me", COA_EAdminContextAction.BRING_TO_ME);
				rows += AddContextAction("Go to player", COA_EAdminContextAction.GO_TO_PLAYER);
			}

			if (!GetPlayerRolePrefab(playerId).IsEmpty())
				rows += AddContextAction("Reset gear", COA_EAdminContextAction.RESET_GEAR);
		}

		if (canRespawn)
			rows += AddContextAction("Respawn on group", COA_EAdminContextAction.RESPAWN_ON_GROUP);

		if (isTicketRow)
		{
			rows += AddContextAction("Assign ticket to me", COA_EAdminContextAction.ASSIGN_TICKET);
			rows += AddContextAction("Close ticket", COA_EAdminContextAction.CLOSE_TICKET);
		}
		else if (m_CachedTickets.Contains(playerId))
		{
			rows += AddContextAction("Open ticket", COA_EAdminContextAction.OPEN_TICKET);
		}

		if (connected)
			rows += AddContextAction("Copy name", COA_EAdminContextAction.COPY_NAME);

		// Size to the rows and keep the menu on screen
		Widget menu = m_wContextMenu.FindAnyWidget("Menu");
		if (!menu)
			return;

		float height = CONTEXT_MENU_HEADER + rows * CONTEXT_MENU_ROW_HEIGHT + CONTEXT_MENU_PADDING;
		float screenWidth = workspace.DPIUnscale(workspace.GetWidth());
		float screenHeight = workspace.DPIUnscale(workspace.GetHeight());
		float x = Math.Clamp(workspace.DPIUnscale(screenX), 0, Math.Max(0, screenWidth - CONTEXT_MENU_WIDTH));
		float y = Math.Clamp(workspace.DPIUnscale(screenY), 0, Math.Max(0, screenHeight - height));

		FrameSlot.SetSize(menu, CONTEXT_MENU_WIDTH, height);
		FrameSlot.SetPos(menu, x, y);
	}

	//------------------------------------------------------------------------------------------------
	//! \return 1 if the row was added (for sizing), else 0
	protected int AddContextAction(string label, COA_EAdminContextAction action)
	{
		Widget list = m_wContextMenu.FindAnyWidget("Actions");
		if (!list)
			return 0;

		Widget row = GetGame().GetWorkspace().CreateWidgets(CONTEXT_MENU_ROW_LAYOUT, list);
		if (!row)
			return 0;

		TextWidget text = TextWidget.Cast(row.FindAnyWidget("ActionText"));
		if (text)
			text.SetText(label);

		COA_AdminContextActionButton button = COA_AdminContextActionButton.Cast(row.FindHandler(COA_AdminContextActionButton));
		if (button)
		{
			button.m_iAction = action;
			button.m_OnClicked.Insert(OnContextActionClicked);
		}

		// ActionBG starts fully transparent, so brighten by overlaying rather than lightening it
		COA_UIHoverEffect.Attach(row, row.FindAnyWidget("ActionBG"), COA_EHoverStyle.OVERLAY);
		return 1;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnContextActionClicked(int action)
	{
		int playerId = m_iContextPlayerId;
		int ticket = m_iContextTicket;
		CloseContextMenu();

		if (action == COA_EAdminContextAction.CLOSE || playerId <= 0)
			return;

		COA_PlayerRplToAuthorityManager authorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (!authorityManager)
			return;

		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();

		switch (action)
		{
			case COA_EAdminContextAction.HEAL:
			{
				authorityManager.Heal(playerId, true, false);
				break;
			}
			case COA_EAdminContextAction.REPAIR_VEHICLE:
			{
				authorityManager.Heal(playerId, true, true);
				break;
			}
			case COA_EAdminContextAction.UNSTUCK:
			{
				// Teleporting a player to themselves moves them to the nearest free spot (RpcDo_TeleportPlayers)
				authorityManager.TeleportPlayers(playerId, playerId, true);
				break;
			}
			case COA_EAdminContextAction.BRING_TO_ME:
			{
				authorityManager.TeleportPlayers(playerId, localPlayerId, true);
				break;
			}
			case COA_EAdminContextAction.GO_TO_PLAYER:
			{
				TeleportLocalPlayer(localPlayerId, playerId);
				break;
			}
			case COA_EAdminContextAction.RESET_GEAR:
			{
				ResourceName prefab = GetPlayerRolePrefab(playerId);
				if (!prefab.IsEmpty())
					authorityManager.ResetGear(playerId, prefab, true);
				break;
			}
			case COA_EAdminContextAction.RESPAWN_ON_GROUP:
			{
				RespawnOnOwnGroup(playerId);
				break;
			}
			case COA_EAdminContextAction.OPEN_TICKET:
			{
				m_iTicketToSelect = playerId;
				TicketButton();
				break;
			}
			case COA_EAdminContextAction.ASSIGN_TICKET:
			{
				m_iSelectedTicket = ticket;
				authorityManager.AssignAdminTicket(ticket, localPlayerId, false);
				break;
			}
			case COA_EAdminContextAction.CLOSE_TICKET:
			{
				authorityManager.CloseAdminTicket(ticket, localPlayerId, true);
				m_iSelectedTicket = -1;
				TicketButton();
				break;
			}
			case COA_EAdminContextAction.COPY_NAME:
			{
				System.ExportToClipboard(m_playerManager.GetPlayerName(playerId));
				break;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void CloseContextMenu()
	{
		if (m_wContextMenu)
			m_wContextMenu.RemoveFromHierarchy();

		m_wContextMenu = null;
		m_iContextPlayerId = 0;
		m_iContextTicket = -1;
	}

	//------------------------------------------------------------------------------------------------
	//! \return the prefab of the role in the player's slot, for resetting their gear to it
	protected ResourceName GetPlayerRolePrefab(int playerId)
	{
		COA_SlotData slotData = COA_SlottingManager.GetInstance().GetPlayerSlotData(playerId);
		if (!slotData)
			return ResourceName.Empty;

		COA_RolesConfig rolesConfig = COA_GearscriptManager.GetRolesConfig();
		if (!rolesConfig)
			return ResourceName.Empty;

		COA_RoleConfig roleConfig = rolesConfig.FindRoleConfig(slotData.GetSlotRole());
		if (!roleConfig)
			return ResourceName.Empty;

		return roleConfig.m_RoleResource;
	}

	//------------------------------------------------------------------------------------------------
	//! Respawn a dead or spectating player on a living member of their own group (the server falls
	//! back to the group leader when nobody is alive)
	protected void RespawnOnOwnGroup(int playerId)
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		SCR_AIGroup group = slottingManager.GetPlayerSlotGroup(playerId);
		if (!group)
			return;

		int spawnOnPlayerId = -1;
		array<int> players = {};
		m_playerManager.GetPlayers(players);
		foreach (int memberId : players)
		{
			if (memberId == playerId || slottingManager.GetPlayerSlotGroup(memberId) != group)
				continue;

			IEntity member = m_playerManager.GetPlayerControlledEntity(memberId);
			if (!member || COA_EntityHelper.IsSpectator(member) || slottingManager.IsPlayerConsideredDead(memberId))
				continue;

			spawnOnPlayerId = memberId;
			break;
		}

		COA_PlayerRplToAuthorityManager.GetInstance().SpawnOnGroup(playerId, spawnOnPlayerId, group.GetGroupID(), true);

		// Refresh the dead players list, as RespawnPlayer does
		if (GetCurrentOpenTab() == "Respawn")
		{
			GetGame().GetCallqueue().CallLater(ClearMenu, 1250, false);
			GetGame().GetCallqueue().CallLater(InitializeRespawnMenu, 1825, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	// GETTERS
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	//! Gets the title of the current open page
	string GetCurrentOpenTab()
	{
		// Get Sub menu title text
		TextWidget menuSubTitle = TextWidget.Cast(m_wRoot.FindAnyWidget("MenuSubTitle"));
		if (!menuSubTitle)
			return "";
		
		return menuSubTitle.GetText();
	}
}
class COA_GearScriptConfigStruct
{
	ref map<string, string> gearset;
}
