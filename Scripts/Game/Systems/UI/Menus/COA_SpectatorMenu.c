class COA_SpectatorMenu: ChimeraMenuBase
{
	//=================================================================================================
	// PROPERTIES
	//=================================================================================================
	
	// UI Widgets
	Widget m_wRoot;                                // Root widget of the menu
	protected FrameWidget m_wMapFrame;                       // Frame widget for the map
	protected Widget m_wPlayerSlotWidget;                    // Widget for player slots
	protected Widget m_wBluforButton;                        // Button for Blufor faction
	protected Widget m_wOpforButton;                         // Button for Opfor faction
	protected Widget m_wIndforButton;                        // Button for Indfor faction
	protected Widget m_wCivButton;                           // Button for Civilian faction
	protected Widget m_wSlotSelector;                        // Widget for selecting slots
	protected FrameWidget m_wFrameSlots;                     // Frame for displaying slots
	protected FrameWidget m_wFrameChannels;                  // Frame for displaying VON channels
	protected FrameWidget m_wFrameGameInfo;                  // Frame for displaying Game Info
	protected FrameWidget m_wSlotWarning;                  	 // Frame for displaying the button to open slotting
	protected COA_ListboxComponent m_wPlayerSlots;           // Listbox component for player slots
	protected COA_ListboxComponent m_wVONChannels;           // Listbox component for VON channels
	protected SCR_ButtonComponent m_wBulletPathButton;       // Button component for Toggling bullet paths
	protected SCR_ButtonComponent m_wDismissSlottingButton;       // Button component for Dismissing Slotting Warning
	
	// Game-related components
	protected COA_Gamemode m_Gamemode;                       // Reference to the gamemode instance
	protected COA_MenuManager m_MenuManager;                 // Reference to the menu manager
	protected SCR_ChatPanel m_ChatPanel;                     // Reference to the chat panel
	protected SCR_MapEntity m_MapEntity;                     // Reference to the map entity
	protected Faction m_fSelectedFaction;                    // Currently selected faction
	protected IEntity m_eSpecEntity;                         // Entity being spectated
	
	// Spectator tracking
	protected ref array<RplId> m_aEntityIcons = {};          // Array of entity IDs with icons
	protected ref array<Widget> m_aSpectatorWidgets = {};    // Array of spectator UI widgets
	protected ref array<ref COA_SpectatorLabelIconCharacter> m_aSpectatorIcons = {}; // Array of spectator icons
	
	// Group icon tracking
	protected ref array<int> m_aGroupIconIds = {};           // Array of group IDs with icons
	protected ref array<Widget> m_aGroupIconWidgets = {};    // Array of group icon UI widgets
	protected ref array<ref COA_SpectatorLabelIconGroup> m_aGroupIcons = {}; // Array of group icon handlers

	// Row tracking for incremental UpdateSlots() (group-scoped patch instead of full Clear+rebuild)
	protected ref map<SCR_AIGroup, int> m_mGroupHeaderIndex = new map<SCR_AIGroup, int>();
	protected ref map<SCR_AIGroup, ref array<int>> m_mGroupSlotIndices = new map<SCR_AIGroup, ref array<int>>();

	// Row tracking for incremental UpdateChannel() (single-row patch instead of full Clear+rebuild)
	protected ref map<int, int> m_mChannelHeaderIndex = new map<int, int>();
	protected ref map<int, ref map<int, int>> m_mChannelPlayerRowIndex = new map<int, ref map<int, int>>();
	
	// Faction counters
	protected int m_iBluforSlots = 0;                        // Total Blufor slots
	protected int m_iOpforSlots = 0;                         // Total Opfor slots
	protected int m_iIndforSlots = 0;                        // Total Indfor slots
	protected int m_iCivSlots = 0;                           // Total Civilian slots
	protected int m_iAliveBluforSlots = 0;                   // Alive Blufor slots
	protected int m_iAliveOpforSlots = 0;                    // Alive Opfor slots
	protected int m_iAliveIndforSlots = 0;                   // Alive Indfor slots
	protected int m_iAliveCivSlots = 0;                      // Alive Civilian slots
	
	// State tracking
	protected bool m_bIsMapOpened = false;                   // Flag indicating if map is open
	protected int m_iLocalChannelUpdates = 0;                // Counter for local channel updates
	protected bool m_bHideUi = false;                        // Flag indicating if UI is hidden
	ref array<Widget> m_aRequest = {};            			  // Array of request widgets
	protected bool m_bFrameEventRegistered = false;          // Flag to track if frame event is registered
	protected bool m_bTPPMode = false;                       // True = third-person camera, false = first-person (helmet cam)
	protected int m_iCamCycle = 2;                           // 0 = helmet FPP, 1 = eye-cam, 2 = TPP orbit - see ToggleCameraMode
	
	// Last kill world position, updated by OnKillfeedNotification, used by Action_TeleportToKill
	protected vector m_vLastKillPosition = vector.Zero;

	bool m_bNVGActivated = false;             				  // NVG activation state for spectator
	
	// Main timer elements
	protected TextWidget m_wTimer;
	protected ImageWidget m_wBackground;

	// Game state references
	protected COA_SafestartManager m_SafestartManager;
	protected string m_sStoredServerWorldTime;
	protected string m_sServerWorldTime;
	protected SCR_PopUpNotification m_PopUpNotification = null;
	
	// Ticket elements
	protected COA_RespawnManager m_RespawnManager;
	protected Widget m_wBLUFORTickets;
	protected TextWidget m_wBLUFORTicketsText;
	protected bool m_bBLUFORTicketsActive;
	protected Widget m_wOPFORTickets;
	protected TextWidget m_wOPFORTicketsText;
	protected bool m_bOPFORTicketsActive;
	protected Widget m_wINDFORTickets;
	protected TextWidget m_wINDFORTicketsText;
	protected bool m_bINDFORTicketsActive;
	protected Widget m_wCIVTickets;
	protected TextWidget m_wCIVTicketsText;
	protected bool m_bCIVTicketsActive;
	
	protected bool m_bWarningDismissed = false;
	
	protected COA_EntityInfoDisplay m_EntityInfoDisplay;

	// Camera mode button row (helmet / eye / orbit) - see InitCameraModeButtons
	protected Widget m_wCameraModeButtons;
	protected ImageWidget m_wHelmetCamBG;
	protected ImageWidget m_wEyeCamBG;
	protected ImageWidget m_wOrbitCamBG;

	// Damage report overlay
	protected Widget m_wDamageReportPanel;
	protected ImageWidget m_wDamageReportBodyOutline;
	protected TextWidget m_wDamageReportSubject;
	protected TextWidget m_wDamageReportFatalRegion;
	protected RichTextWidget m_wDamageReportLog;
	protected ImageWidget m_wDamageRegionHead;
	protected ImageWidget m_wDamageRegionTorso;
	protected ImageWidget m_wDamageRegionLeftArm;
	protected ImageWidget m_wDamageRegionRightArm;
	protected ImageWidget m_wDamageRegionLeftLeg;
	protected ImageWidget m_wDamageRegionRightLeg;
	protected ref array<ref COA_SpectatorDamageReportEntry> m_aDamageReportEntries = {};
	
	//=================================================================================================
	// MENU LIFECYCLE METHODS
	//=================================================================================================
	
	/**
	 * Called when the menu is opened
	 * Sets up UI elements, registers action listeners, and initializes slots
	 */
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		// Reset the death low-pass audio filter (CharacterLifeState FMOD variable).
		// SCR_NoiseFilterEffect sets this to DEAD on death and relies on SCR_DeployMenuBase.SGetOnMenuOpen()
		// to reset it, which never fires when using a custom spectator menu.
		AudioSystem.SetVariableByName("CharacterLifeState", ECharacterLifeState.ALIVE, "{A60F08955792B575}Sounds/_SharedData/Variables/GlobalVariables.conf");

		// Initialize HUD visibility
		SCR_HUDManagerComponent hudManager = GetGame().GetHUDManager();
		if (hudManager) 
		{
			hudManager.SetVisible(true);
			hudManager.SetVisibleLayers(EHudLayers.ALWAYS_TOP);
		}
		
		// Initialize widget references
		m_wRoot = GetRootWidget();
		Widget wChatPanel = m_wRoot.FindAnyWidget("ChatPanel");
		if (wChatPanel)
			m_ChatPanel = SCR_ChatPanel.Cast(wChatPanel.FindHandler(SCR_ChatPanel));
		
		// Get game components
		m_Gamemode = COA_Gamemode.GetInstance();
		m_MenuManager = COA_MenuManager.GetInstance();
		
		// Initialize UI components
		m_wMapFrame = FrameWidget.Cast(m_wRoot.FindAnyWidget("MapFrame"));
		m_wSlotWarning = FrameWidget.Cast(m_wRoot.FindAnyWidget("SlotWarning"));
		m_wPlayerSlotWidget = m_wRoot.FindAnyWidget("PlayerSlots");
		m_wPlayerSlots = COA_ListboxComponent.Cast(m_wPlayerSlotWidget.FindHandler(COA_ListboxComponent));
		m_wVONChannels = COA_ListboxComponent.Cast(m_wRoot.FindAnyWidget("VONChannels").FindHandler(COA_ListboxComponent));
		
		m_RespawnManager = COA_RespawnManager.GetInstance();
		m_wBLUFORTicketsText = TextWidget.Cast(m_wRoot.FindAnyWidget("BLUFORTicketsText"));
		m_wOPFORTicketsText = TextWidget.Cast(m_wRoot.FindAnyWidget("OPFORTicketsText"));
		m_wINDFORTicketsText = TextWidget.Cast(m_wRoot.FindAnyWidget("INDFORTicketsText"));
		m_wCIVTicketsText = TextWidget.Cast(m_wRoot.FindAnyWidget("CIVTicketsText"));
		m_wBLUFORTickets = m_wRoot.FindAnyWidget("BLUFORTickets");
		m_wOPFORTickets = m_wRoot.FindAnyWidget("OPFORTickets");
		m_wINDFORTickets = m_wRoot.FindAnyWidget("INDFORTickets");
		m_wCIVTickets = m_wRoot.FindAnyWidget("CIVTickets");
		
		m_wDismissSlottingButton = SCR_ButtonComponent.Cast(m_wRoot.FindAnyWidget("DismissWarning").FindHandler(SCR_ButtonComponent));
		m_wDismissSlottingButton.m_OnClicked.Insert(DismissSlottingWarning);
		
		// Register input action listeners
		RegisterActionListeners();
		
		// Initialize slots
		InitSlots();
		
		// Select default faction based on availability
		SelectDefaultFaction();
		
		// Initialize faction buttons
		InitFactionButtons();
		
		// Get follow-mode HUD overlay
		Widget entityInfoDisplay = m_wRoot.FindAnyWidget("EntityInfoDisplay");
		m_EntityInfoDisplay = COA_EntityInfoDisplay.Cast(entityInfoDisplay.FindHandler(COA_EntityInfoDisplay));
		InitCameraModeButtons();
		InitDamageReportWidgets();
		
		// Initialize VON (Voice Over Network)
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			InitVON();
			// Surgical per-player channel-move updates patch only the affected rows instead of a
			// full VON channel list rebuild; the existing m_iChannelChanges poll in OnMenuUpdate
			// remains as a fallback for structural channel create/remove.
			m_MenuManager.GetOnPlayerChannelChanged().Insert(OnPlayerChannelChanged);
		}
		
		// Update slots and register for slot updates
		UpdateSlots();
		COA_SlottingManager.GetInstance().GetOnSlottingUpdate().Insert(UpdateSlots);
		// Surgical player-ID delta updates patch only the affected group instead of a full rebuild
		COA_SlottingManager.GetInstance().GetOnSlotChanged().Insert(OnSlotPlayerIdChanged);
		// Separate handler checks only whether the local player was just slotted during the game phase
		COA_SlottingManager.GetInstance().GetOnSlotChanged().Insert(OnSlotChangedCheckAutoInsert);
		
		// Get game system references
		m_SafestartManager = COA_SafestartManager.GetInstance();
		
		// Find and cast main timer widgets
		m_wTimer = TextWidget.Cast(m_wRoot.FindAnyWidget("timeLeftTimer"));
		m_wBackground = ImageWidget.Cast(m_wRoot.FindAnyWidget("timeLeftBackground"));

		// Get notification system reference
		m_PopUpNotification = SCR_PopUpNotification.GetInstance();

		// Subscribe to notification events to track kill locations for R-key teleport
		SCR_NotificationsComponent notifComp = SCR_NotificationsComponent.GetInstance();
		if (notifComp)
			notifComp.GetOnNotification().Insert(OnKillfeedNotification);
	}
	
	void DismissSlottingWarning()
	{
		m_bWarningDismissed = true;
	}
	
	/**
	 * Registers all action listeners for the menu
	 */
	protected void RegisterActionListeners()
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			inputManager.AddActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
			inputManager.AddActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		}
		inputManager.AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnCOA_ChatToggleAction);
		inputManager.AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		inputManager.AddActionListener("GadgetMap", EActionTrigger.DOWN, Action_ToggleMap);
		inputManager.AddActionListener("ManualCameraTeleport", EActionTrigger.DOWN, Action_ManualCameraTeleport);
		inputManager.AddActionListener("EditorToggleUI", EActionTrigger.DOWN, HideUI);
		inputManager.AddActionListener("COA_ShowDamageReport", EActionTrigger.DOWN, Action_ToggleDamageReport);
		inputManager.AddActionListener("COA_SpecNVG", EActionTrigger.DOWN, ToggleNVGs);
		inputManager.AddActionListener("COA_SpecToggleCamMode", EActionTrigger.DOWN, ToggleCameraMode);
		inputManager.AddActionListener("COA_SpecKillTeleport", EActionTrigger.DOWN, Action_TeleportToKill);
	}

	//------------------------------------------------------------------------------------------------
	protected void InitDamageReportWidgets()
	{
		m_wDamageReportPanel = m_wRoot.FindAnyWidget("DamageReportPanel");
		if (!m_wDamageReportPanel)
			return;

		m_wDamageReportSubject = TextWidget.Cast(m_wRoot.FindAnyWidget("DamageReportSubject"));
		m_wDamageReportFatalRegion = TextWidget.Cast(m_wRoot.FindAnyWidget("DamageReportFatalRegion"));
		m_wDamageReportLog = RichTextWidget.Cast(m_wRoot.FindAnyWidget("DamageReportLog"));
		m_wDamageReportBodyOutline = ImageWidget.Cast(m_wRoot.FindAnyWidget("DamageReportBodyOutline"));
		m_wDamageRegionHead = ImageWidget.Cast(m_wRoot.FindAnyWidget("DamageRegionHead"));
		m_wDamageRegionTorso = ImageWidget.Cast(m_wRoot.FindAnyWidget("DamageRegionTorso"));
		m_wDamageRegionLeftArm = ImageWidget.Cast(m_wRoot.FindAnyWidget("DamageRegionLeftArm"));
		m_wDamageRegionRightArm = ImageWidget.Cast(m_wRoot.FindAnyWidget("DamageRegionRightArm"));
		m_wDamageRegionLeftLeg = ImageWidget.Cast(m_wRoot.FindAnyWidget("DamageRegionLeftLeg"));
		m_wDamageRegionRightLeg = ImageWidget.Cast(m_wRoot.FindAnyWidget("DamageRegionRightLeg"));

		m_wDamageReportPanel.SetVisible(false);
		LoadDamageReportImages();
		ResetDamageRegionColors();
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadDamageReportImages()
	{
		if (m_wDamageReportBodyOutline)
		{
			m_wDamageReportBodyOutline.LoadImageTexture(0, "{FA3183D43A82BEFB}UI/Images/DamageReport/ace_player_model.edds");
			m_wDamageReportBodyOutline.SetImage(0);
			m_wDamageReportBodyOutline.SetColor(Color.FromRGBA(255, 255, 255, 255));
		}

		LoadDamageReportImage(m_wDamageRegionHead, "{DD56B2D615DF6168}UI/Images/DamageReport/damage_head.edds");
		LoadDamageReportImage(m_wDamageRegionTorso, "{624C15FE16D3E6D9}UI/Images/DamageReport/damage_torso.edds");
		LoadDamageReportImage(m_wDamageRegionLeftArm, "{635CFD77538EF5B5}UI/Images/DamageReport/damage_left_arm.edds");
		LoadDamageReportImage(m_wDamageRegionRightArm, "{96DD13FD6E5A7CAA}UI/Images/DamageReport/damage_right_arm.edds");
		LoadDamageReportImage(m_wDamageRegionLeftLeg, "{48EDAB246566DEB3}UI/Images/DamageReport/damage_left_leg.edds");
		LoadDamageReportImage(m_wDamageRegionRightLeg, "{6B1AFAA6450E91FA}UI/Images/DamageReport/damage_right_leg.edds");
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadDamageReportImage(ImageWidget widget, ResourceName texture)
	{
		if (!widget)
			return;

		widget.LoadImageTexture(0, texture);
		widget.SetImage(0);
	}

	//------------------------------------------------------------------------------------------------
	void Action_ToggleDamageReport()
	{
		if (!m_wDamageReportPanel)
			InitDamageReportWidgets();

		if (!m_wDamageReportPanel)
			return;

		bool visible = !m_wDamageReportPanel.IsVisible();
		m_wDamageReportPanel.SetVisible(visible);

		if (visible)
			RefreshDamageReport();
	}

	//------------------------------------------------------------------------------------------------
	void RefreshDamageReport()
	{
		if (!m_wDamageReportPanel || !m_wDamageReportPanel.IsVisible())
			return;

		int playerId = GetDamageReportPlayerId();
		COA_SpectatorDamageReportStore.GetEventsForPlayer(playerId, m_aDamageReportEntries);

		string subjectName = GetDamageReportPlayerName(playerId);
		if (m_wDamageReportSubject)
			m_wDamageReportSubject.SetText(subjectName);

		string fatalRegion = COA_SpectatorDamageReportStore.GetFatalBodyRegion(playerId);
		SetDamageReportFatalRegion(fatalRegion);
		SetDamageRegionColors(fatalRegion);

		if (!m_wDamageReportLog)
			return;

		if (m_aDamageReportEntries.IsEmpty())
		{
			m_wDamageReportLog.SetText("No damage recorded.");
			return;
		}

		string reportText;
		int startIndex = Math.Max(0, m_aDamageReportEntries.Count() - 18);
		for (int i = startIndex; i < m_aDamageReportEntries.Count(); i++)
		{
			COA_SpectatorDamageReportEntry entry = m_aDamageReportEntries[i];
			string sourceLabel;
			string otherName;

			if (entry.m_iVictimPlayerId == playerId)
			{
				sourceLabel = "from";
				otherName = entry.m_sAttackerName;
			}
			else
			{
				sourceLabel = "to";
				otherName = entry.m_sVictimName;
			}

			string fatalText;
			if (entry.m_bFatal)
				fatalText = " KILL";

			string rangeText = "?m";
			if (entry.m_fRangeMeters >= 0)
				rangeText = string.Format("%1m", Math.Round(entry.m_fRangeMeters));

			string line = string.Format("[%1] %2 (%3) | %4 dmg | %5 %6 | %7 | %8%9", FormatDamageReportTime(entry.m_iWorldTime), entry.m_sDamageType, rangeText, Math.Round(entry.m_fDamageValue), sourceLabel, otherName, entry.m_sBodyRegion, entry.m_sHitZone, fatalText);
			if (reportText.IsEmpty())
				reportText = line;
			else
				reportText = reportText + "\n" + line;
		}

		m_wDamageReportLog.SetText(reportText);
	}

	//------------------------------------------------------------------------------------------------
	protected int GetDamageReportPlayerId()
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (m_eSpecEntity && playerManager)
		{
			int controlledPlayerId = playerManager.GetPlayerIdFromControlledEntity(m_eSpecEntity);
			if (controlledPlayerId > 0)
				return controlledPlayerId;

			RplComponent rpl = RplComponent.Cast(m_eSpecEntity.FindComponent(RplComponent));
			if (rpl)
			{
				COA_SlotData slotData = COA_SlottingManager.GetInstance().GetSlotDataFromCharacter(rpl.Id());
				if (slotData && slotData.GetSlotCurrentPlayerId() > 0)
					return slotData.GetSlotCurrentPlayerId();
			}
		}

		return SCR_PlayerController.GetLocalPlayerId();
	}

	//------------------------------------------------------------------------------------------------
	protected string GetDamageReportPlayerName(int playerId)
	{
		if (playerId <= 0)
			return "No Player Selected";

		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		if (!playerName.IsEmpty())
			return playerName;

		return string.Format("Player %1", playerId);
	}

	//------------------------------------------------------------------------------------------------
	protected string FormatDamageReportTime(int worldTime)
	{
		int totalSeconds = worldTime / 1000;
		int minutes = totalSeconds / 60;
		int seconds = totalSeconds - (minutes * 60);
		string secondsText = seconds.ToString();

		if (seconds < 10)
			secondsText = "0" + secondsText;

		return string.Format("%1:%2", minutes, secondsText);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetDamageReportFatalRegion(string fatalRegion)
	{
		if (!m_wDamageReportFatalRegion)
			return;

		if (fatalRegion.IsEmpty())
			m_wDamageReportFatalRegion.SetText("Fatal Location: No fatal damage recorded");
		else
			m_wDamageReportFatalRegion.SetText("Fatal Location: " + fatalRegion);
	}

	//------------------------------------------------------------------------------------------------
	protected void ResetDamageRegionColors()
	{
		Color inactiveColor = Color.FromRGBA(255, 255, 255, 0);
		SetDamageRegionColor(m_wDamageRegionHead, inactiveColor);
		SetDamageRegionColor(m_wDamageRegionTorso, inactiveColor);
		SetDamageRegionColor(m_wDamageRegionLeftArm, inactiveColor);
		SetDamageRegionColor(m_wDamageRegionRightArm, inactiveColor);
		SetDamageRegionColor(m_wDamageRegionLeftLeg, inactiveColor);
		SetDamageRegionColor(m_wDamageRegionRightLeg, inactiveColor);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetDamageRegionColors(string fatalRegion)
	{
		ResetDamageRegionColors();

		if (fatalRegion.IsEmpty())
			return;

		Color activeColor = Color.FromRGBA(205, 24, 24, 145);
		switch (fatalRegion)
		{
			case "Head":
				SetDamageRegionColor(m_wDamageRegionHead, activeColor);
				break;
			case "Torso":
				SetDamageRegionColor(m_wDamageRegionTorso, activeColor);
				break;
			case "Left Arm":
				SetDamageRegionColor(m_wDamageRegionLeftArm, activeColor);
				break;
			case "Right Arm":
				SetDamageRegionColor(m_wDamageRegionRightArm, activeColor);
				break;
			case "Left Leg":
				SetDamageRegionColor(m_wDamageRegionLeftLeg, activeColor);
				break;
			case "Right Leg":
				SetDamageRegionColor(m_wDamageRegionRightLeg, activeColor);
				break;
			case "Arm":
				SetDamageRegionColor(m_wDamageRegionLeftArm, activeColor);
				SetDamageRegionColor(m_wDamageRegionRightArm, activeColor);
				break;
			case "Leg":
				SetDamageRegionColor(m_wDamageRegionLeftLeg, activeColor);
				SetDamageRegionColor(m_wDamageRegionRightLeg, activeColor);
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SetDamageRegionColor(Widget widget, Color color)
	{
		if (widget)
			widget.SetColor(color);
	}
	
	/**
	 * Cycles between first-person (helmet cam), true first-person (eye cam), and third-person
	 * spectator camera modes. Can be triggered by the HUD button or a key binding.
	 * If currently following an entity, the camera switches mode immediately.
	 */
	void ToggleCameraMode()
	{
		if (!m_eSpecEntity || !m_bFrameEventRegistered)
		{
			m_bTPPMode = !m_bTPPMode;
			return;
		}

		SetCameraMode((m_iCamCycle + 1) % 3);
	}

	/**
	 * Switches the camera to a specific mode (0 = helmet FPP, 1 = eye-cam, 2 = TPP orbit) on the
	 * currently spectated entity. Shared by ToggleCameraMode and the CameraModeButtons HUD row.
	 */
	protected void SetCameraMode(int mode)
	{
		if (!m_eSpecEntity || !m_bFrameEventRegistered)
			return;

		m_iCamCycle = mode;

		COA_PlayerCameraManager camManager = COA_PlayerCameraManager.GetInstance();

		switch (m_iCamCycle)
		{
			case 0:
				m_bTPPMode = false;
				camManager.SetCameraOnRailsEntity(m_eSpecEntity, false);
				break;

			case 1:
				m_bTPPMode = false;
				camManager.SetCameraOnRailsEntityEyeMode(m_eSpecEntity);
				break;

			case 2:
				m_bTPPMode = true;
				camManager.SetCameraOnRailsEntity(m_eSpecEntity, true);
				break;
		}

		UpdateCameraModeButtonsUI();
	}

	/**
	 * Finds the CameraModeButtons HUD row (see CameraModeButtons.layout) and wires its three
	 * buttons directly to SetCameraMode.
	 */
	protected void InitCameraModeButtons()
	{
		m_wCameraModeButtons = m_wRoot.FindAnyWidget("CameraModeButtons");
		if (!m_wCameraModeButtons)
			return;

		m_wHelmetCamBG = ImageWidget.Cast(m_wRoot.FindAnyWidget("HelmetCamBG"));
		m_wEyeCamBG = ImageWidget.Cast(m_wRoot.FindAnyWidget("EyeCamBG"));
		m_wOrbitCamBG = ImageWidget.Cast(m_wRoot.FindAnyWidget("OrbitCamBG"));

		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("HelmetCamSelectButton")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectCameraModeHelmet);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("EyeCamSelectButton")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectCameraModeEye);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("OrbitCamSelectButton")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectCameraModeOrbit);

		UpdateCameraModeButtonsUI();
	}

	//------------------------------------------------------------------------------------------------
	void SelectCameraModeHelmet()
	{
		SetCameraMode(0);
	}

	//------------------------------------------------------------------------------------------------
	void SelectCameraModeEye()
	{
		SetCameraMode(1);
	}

	//------------------------------------------------------------------------------------------------
	void SelectCameraModeOrbit()
	{
		SetCameraMode(2);
	}

	/**
	 * Highlights whichever CameraModeButtons entry matches m_iCamCycle so the row reflects the
	 * active camera mode.
	 */
	protected void UpdateCameraModeButtonsUI()
	{
		if (!m_wHelmetCamBG || !m_wEyeCamBG || !m_wOrbitCamBG)
			return;
		
		Color active = Color.FromRGBA(0, 149, 255, 153);
		SCR_ChimeraCharacter player = SCR_ChimeraCharacter.Cast(m_eSpecEntity);
		if (!player)
			return;
		
		if (player.m_pFactionComponent && player.m_pFactionComponent.GetAffiliatedFaction())
			active = player.m_pFactionComponent.GetAffiliatedFaction().GetFactionColor();

		Color inactive = Color.FromRGBA(37, 37, 37, 153);

		m_wHelmetCamBG.SetColor(inactive);
		m_wEyeCamBG.SetColor(inactive);
		m_wOrbitCamBG.SetColor(inactive);

		switch (m_iCamCycle)
		{
			case 0: m_wHelmetCamBG.SetColor(active); break;
			case 1: m_wEyeCamBG.SetColor(active); break;
			case 2: m_wOrbitCamBG.SetColor(active); break;
		}
	}

	/**
	 * Toggles night vision goggles in spectator mode
	 */
	void ToggleNVGs()
	{
		m_bNVGActivated = !m_bNVGActivated;
		
		const BaseWorld world = GetGame().GetWorld();
		if (!world)
		{
			return;
		}
		
		const int cameraId = world.GetCurrentCameraId();
		
		if (m_bNVGActivated)
		{
			world.SetCameraPostProcessEffect(cameraId, 16, PostProcessEffectType.HDR, "{511CD467ED159EA2}Assets/Items/Equipment/NVG/pvs14/data/NVG_Spectator_HDR.emat");
			world.SetCameraHDRBrightness(cameraId, 0.4);
		}
		else
		{
			world.SetCameraPostProcessEffect(cameraId, 16, PostProcessEffectType.HDR, "{9DEECCABE8357209}Common/Postprocess/HDR.emat");
			world.SetCameraHDRBrightness(cameraId, -1);
		}
	}
	
	/**
	 * forces night vision goggles off in spectator mode
	 */
	void ForceNVGsOff()
	{
		const BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		
		const int cameraId = world.GetCurrentCameraId();
		
		world.SetCameraPostProcessEffect(cameraId, 16, PostProcessEffectType.HDR, "{9DEECCABE8357209}Common/Postprocess/HDR.emat");
		world.SetCameraHDRBrightness(cameraId, -1);
	}
	
	/**
	 * Initializes faction buttons and their click handlers
	 */
	protected void InitFactionButtons()
	{
		m_wBluforButton = m_wRoot.FindAnyWidget("BLUSelectButton");
		m_wOpforButton = m_wRoot.FindAnyWidget("OPFSelectButton");
		m_wIndforButton = m_wRoot.FindAnyWidget("INDSelectButton");
		m_wCivButton = m_wRoot.FindAnyWidget("CIVSelectButton");
		m_wFrameSlots = FrameWidget.Cast(m_wRoot.FindAnyWidget("FrameSlots"));
		m_wSlotSelector = m_wRoot.FindAnyWidget("SlotSelector");
		m_wFrameChannels = FrameWidget.Cast(m_wRoot.FindAnyWidget("VONSlots"));
		m_wFrameGameInfo = FrameWidget.Cast(m_wRoot.FindAnyWidget("GameInfo"));
		
		// Register faction button click handlers
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wBluforButton).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionBlufor);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wOpforButton).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionOpfor);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wIndforButton).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionIndfor);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wCivButton).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionCiv);
		
		// Register create channel button click handler
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("CreateChannel")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(CreateChannel);
		
		// Apply faction restrictions if enabled
		ApplyFactionRestrictions();
	}
	
	/**
	 * Applies faction-based spectator restrictions to UI elements
	 * Disables faction buttons that the spectator is not allowed to view
	 */
	protected void ApplyFactionRestrictions()
	{
		// Only apply restrictions if the setting is enabled
		if (!m_Gamemode || !m_Gamemode.m_bHideOtherSpectatorFactions)
			return;
		
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		Faction localPlayerFaction = COA_SlottingManager.GetInstance().GetPlayerSlotFaction(localPlayerId);
		
		// If player has no faction, don't restrict anything
		if (!localPlayerFaction)
			return;
		
		string localFactionKey = localPlayerFaction.GetFactionKey();
		
		// Disable faction buttons that don't match the player's faction
		if (localFactionKey != "BLUFOR" && m_wBluforButton)
			m_wBluforButton.SetEnabled(false);
		
		if (localFactionKey != "OPFOR" && m_wOpforButton)
			m_wOpforButton.SetEnabled(false);
		
		if (localFactionKey != "INDFOR" && m_wIndforButton)
			m_wIndforButton.SetEnabled(false);
		
		if (localFactionKey != "CIV" && m_wCivButton)
			m_wCivButton.SetEnabled(false);
	}
	
	/**
	 * Select default faction based on availability
	 * When faction restrictions are enabled, automatically selects the player's own faction
	 */
	protected void SelectDefaultFaction()
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		
		// If faction restrictions are enabled, select the player's own faction
		if (m_Gamemode && m_Gamemode.m_bHideOtherSpectatorFactions)
		{
			int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
			Faction localPlayerFaction = slottingManager.GetPlayerSlotFaction(localPlayerId);
			
			if (localPlayerFaction)
			{
				string localFactionKey = localPlayerFaction.GetFactionKey();
				
				if (localFactionKey == "BLUFOR" && slottingManager.IsFactionValid("BLUFOR"))
					SelectFactionBlufor();
				else if (localFactionKey == "OPFOR" && slottingManager.IsFactionValid("OPFOR"))
					SelectFactionOpfor();
				else if (localFactionKey == "INDFOR" && slottingManager.IsFactionValid("INDFOR"))
					SelectFactionIndfor();
				else if (localFactionKey == "CIV" && slottingManager.IsFactionValid("CIV"))
					SelectFactionCiv();
				
				return;
			}
		}
		
		// Default behavior: select first valid faction
		if(slottingManager.IsFactionValid("BLUFOR"))
			SelectFactionBlufor();
		else if(slottingManager.IsFactionValid("OPFOR"))
			SelectFactionOpfor();
		else if(slottingManager.IsFactionValid("INDFOR"))
			SelectFactionIndfor();
		else if(slottingManager.IsFactionValid("CIV"))
			SelectFactionCiv();
	}
	
	protected void InitVON()
	{
		// Initialize VON with a slight delay to ensure proper setup
		GetGame().GetCallqueue().Call(Action_VONon);
		GetGame().GetCallqueue().Call(Action_VONOff);
	}
	
	/**
	 * Updates the compass UI based on camera orientation
	 */
	void UpdateCompass()
	{
		// Get camera yaw angle (double negation cancels — equivalent to raw [0] value)
		float yaw = COA_PlayerCameraManager.GetInstance().m_eCamera.GetYawPitchRoll()[0];
		
		// Normalise to 0-360
		yaw = yaw - 360 * Math.Floor(yaw / 360);
		if (yaw < 0)
			yaw += 360;
		
		FrameWidget compassMoveable = FrameWidget.Cast(m_wRoot.FindAnyWidget("CompassFrameMoveable"));
		if (!compassMoveable)
			return;
		
		// Only shift horizontally — vertical offsets are pinned to the exact layout values
		// so the bar cannot shift up/down regardless of yaw.
		float scroll = 1880 * (yaw / 360);
		FrameSlot.SetOffsets(compassMoveable, -1090 - scroll, -67.785, -2750 + scroll, -990.214);
	}
	
	/**
	 * Called every frame to update the menu
	 * @param tDelta - Time since last frame
	 */
	float m_fUpdateBuffer = 0;
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);

		// Update compass
		UpdateCompass();
		
		// Ensure map context is active when map is open
		if (m_MapEntity)
			GetGame().GetInputManager().ActivateContext("MapContext");
		
		// Update VON channels if needed and handle radio frequency updates
		if(m_iLocalChannelUpdates != m_MenuManager.m_iChannelChanges)
		{
			UpdateChannel();
			// Radio frequency is updated within UpdateChannel() method
		}
		
		// Handle spectator camera
		UpdateSpectatorCamera(tDelta);

		// When CVON is disabled, keep the spectator entity's world position in sync with the
		// camera every frame so vanilla VON proximity checks (server-side) route alive players'
		// direct speech to the spectator. The camera is a local-only entity with no RplComponent,
		// so we cannot parent the spectator character to it — we must replicate position explicitly.
		// BaseGameEntity.Teleport updates the physics body and flushes a position update through
		// the character replication path, matching the pattern used by PS_PlayableControllerComponent.
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			IEntity specEntity = SCR_PlayerController.GetLocalMainEntity();
			if (specEntity && COA_EntityHelper.IsSpectator(specEntity))
			{
				CameraBase camera = GetGame().GetCameraManager().CurrentCamera();
				if (camera)
				{
					// Preserve entity orientation; only update world-space position
					vector mat[4];
					specEntity.GetWorldTransform(mat);
					mat[3] = camera.GetOrigin();

					BaseGameEntity bgEntity = BaseGameEntity.Cast(specEntity);
					if (bgEntity)
						bgEntity.Teleport(mat);
					else
						specEntity.SetWorldTransform(mat);
				}
			}
		}

		// Update follow-mode HUD overlay
		m_EntityInfoDisplay.UpdateEntityInfoDisplay(m_eSpecEntity);
		if (m_wCameraModeButtons)
			m_wCameraModeButtons.SetVisible(m_eSpecEntity != null);
		RefreshDamageReport();
		
		// Process channel requests
		ProcessChannelRequests(tDelta);
		
		// Update UI panel visibility based on cursor position
		UpdateUIPanelVisibility(tDelta);
		
		// Update icons
		UpdateIcons();
		
		//Hmm I wonder if this updates tickets
		UpdateTickets();
		
		// Update chat if available
		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);
		
		// Set kill feed type to dead local
		SCR_NotificationSenderComponent sender = SCR_NotificationSenderComponent.Cast(GetGame().GetGameMode().FindComponent(SCR_NotificationSenderComponent));
		sender.SetKillFeedTypeDeadLocal();
		
		UpdateTimer();
		
		if (m_fUpdateBuffer >= 1)
		{
			UpdatePlayerIcons();
			m_fUpdateBuffer = 0;
		}
		m_fUpdateBuffer += tDelta;
		
		if (m_SafestartManager.GetSafestartStatus() && !m_bWarningDismissed)
			m_wSlotWarning.SetVisible(true);
		else
			m_wSlotWarning.SetVisible(false);
	}
	
	//Used to update tickets
	void UpdateTickets()
	{
		//BLUFOR LOGIC
		if (m_RespawnManager.m_iBLUFORTickets > 0 && !m_bBLUFORTicketsActive) //Lets tickets stay on screen even if they reach 0 as they are considered an active faction.
			m_bBLUFORTicketsActive = true;
		
		if (m_bBLUFORTicketsActive)
		{
			m_wBLUFORTickets.SetVisible(true);
			m_wBLUFORTicketsText.SetText("BLUFOR Tickets: " + m_RespawnManager.m_iBLUFORTickets.ToString());
		}
		else
			m_wBLUFORTickets.SetVisible(false);
		
		//OPFOR LOGIC
		if (m_RespawnManager.m_iOPFORTickets > 0 && !m_bOPFORTicketsActive)
			m_bOPFORTicketsActive = true;
		
		if (m_bOPFORTicketsActive)
		{
			m_wOPFORTickets.SetVisible(true);
			m_wOPFORTicketsText.SetText("OPFOR Tickets: " + m_RespawnManager.m_iOPFORTickets.ToString());
		}
		else
			m_wOPFORTickets.SetVisible(false);
		
		//INDFOR LOGIC
		if (m_RespawnManager.m_iINDFORTickets > 0 && !m_bINDFORTicketsActive)
			m_bINDFORTicketsActive = true;
		
		if (m_bINDFORTicketsActive)
		{
			m_wINDFORTickets.SetVisible(true);
			m_wINDFORTicketsText.SetText("INDFOR Tickets: " + m_RespawnManager.m_iINDFORTickets.ToString());
		}
		else
			m_wINDFORTickets.SetVisible(false);
		
		//CIV LOGIC
		if (m_RespawnManager.m_iCIVTickets > 0 && !m_bCIVTicketsActive)
			m_bCIVTicketsActive = true;
		
		if (m_bCIVTicketsActive)
		{
			m_wCIVTickets.SetVisible(true);
			m_wCIVTicketsText.SetText("CIV Tickets: " + m_RespawnManager.m_iCIVTickets.ToString());
		}
		else
			m_wCIVTickets.SetVisible(false);

	}

	/**
	 * Handle spectator camera updates
	 * @param tDelta - Time since last frame
	 */
	protected void UpdateSpectatorCamera(float tDelta)
	{
		COA_PlayerCameraManager cameraManager = COA_PlayerCameraManager.GetInstance();
		
		if (m_eSpecEntity)
		{
			InputManager im = GetGame().GetInputManager();
			
			// Check if user is trying to control camera manually or if entity is a spectator.
			// In TPP mode we ignore ManualCameraRotate — mouse movement doesn't mean the
			// user has taken control of the free camera, so we keep following the entity.
			bool isManualControl = 
				im.GetActionValue("ManualCameraMoveLateral") != 0 || 
				im.GetActionValue("ManualCameraMoveVertical") != 0 || 
				im.GetActionValue("ManualCameraMoveLongitudinal") != 0 || 
				(!m_bTPPMode && im.GetActionValue("ManualCameraRotate") != 0) || 
				COA_EntityHelper.IsSpectator(m_eSpecEntity);
				
			if (isManualControl)
			{
				// Reset spectator entity and unregister frame event
				m_eSpecEntity = null;
				UnregisterFrameEvent();
				
				// Reset camera angle after leaving FPP
				vector mat = cameraManager.m_eCamera.GetAngles();
				cameraManager.m_eCamera.SetAngles(Vector(mat[0], mat[1], 0));
			}
			else
			{
				// Register frame event for smooth camera tracking if not already registered
				if (!m_bFrameEventRegistered)
				{
					RegisterFrameEvent();
				}
			}
		} 
		else if(!m_eSpecEntity)
		{
			// Reset camera roll when not spectating and unregister frame event
			vector mat = cameraManager.m_eCamera.GetAngles();
			cameraManager.m_eCamera.SetAngles(Vector(mat[0], mat[1], 0));
			UnregisterFrameEvent();
		}
	}
	
	/**
	 * Registers the frame event for smooth spectator camera tracking
	 */
	protected void RegisterFrameEvent()
	{
		if (!m_bFrameEventRegistered)
		{
			IEntity specEntity = SCR_PlayerController.GetLocalMainEntity();
			
			if (!COA_EntityHelper.IsSpectator(specEntity))
				return;
			
			COA_PlayerCameraManager camManager = COA_PlayerCameraManager.GetInstance();
			camManager.SetCameraOnRailsEntity(m_eSpecEntity, m_bTPPMode);
			
			m_bFrameEventRegistered = true;
		}
	}
	
	/**
	 * Unregisters the frame event for spectator camera tracking
	 */
	protected void UnregisterFrameEvent()
	{
		if (m_bFrameEventRegistered)
		{
			IEntity specEntity = SCR_PlayerController.GetLocalMainEntity();
			
			if (!COA_EntityHelper.IsSpectator(specEntity))
				return;
			
			COA_PlayerCameraManager camManager = COA_PlayerCameraManager.GetInstance();
			camManager.SetCameraOnRailsEntity(null);
			
			m_bFrameEventRegistered = false;
		}
	}
	
	/**
	 * Process channel join requests
	 * @param tDelta - Time since last frame
	 */
	protected void ProcessChannelRequests(float tDelta)
	{
		// Process each request widget
		for (int i = m_aRequest.Count() - 1; i >= 0; i--)
		{
			Widget request = m_aRequest[i];
			COA_ListBoxElementComponent comp = COA_ListBoxElementComponent.Cast(request.FindHandler(COA_ListBoxElementComponent));
			
			// Check if player has joined the channel
			if (m_MenuManager.IsPlayerInChannel(comp.m_iPlayerId, comp.m_iChannelId))
			{
				request.RemoveFromHierarchy();
				m_aRequest.RemoveOrdered(i);
				continue;
			}
			
			// Handle request deletion animation
			if (comp.m_bDeleteRequest)
			{
				Widget buttonAnim = request.FindAnyWidget("ButtonAnim");
				float posX = FrameSlot.GetPosX(buttonAnim);
				
				if (posX > 500)
				{
					request.RemoveFromHierarchy();
					m_aRequest.RemoveOrdered(i);
					continue;
				}
				
				FrameSlot.SetPosX(buttonAnim, posX + tDelta * 2000);
				continue;
			}
			else if (FrameSlot.GetPosX(request.FindAnyWidget("ButtonAnim")) > 0)
			{
				// Handle request appearance animation
				Widget buttonAnim = request.FindAnyWidget("ButtonAnim");
				float posX = FrameSlot.GetPosX(buttonAnim);
				
				if (posX - tDelta * 2000 > 0)
					FrameSlot.SetPosX(buttonAnim, posX - tDelta * 2000);
				else 
					FrameSlot.SetPosX(buttonAnim, 0);
			}
			
			// Update request timer
			comp.GetProgress().SetCurrent(comp.GetProgress().GetCurrent() - tDelta);
			if (comp.GetProgress().GetCurrent() <= 0)
			{
				request.RemoveFromHierarchy();
				m_aRequest.RemoveOrdered(i);
			}
		}
	}
	
	/**
	 * Update player icons in the spectator UI
	 */
	protected void UpdatePlayerIcons()
	{
		array<RplId> comparisonRplIds = {};
		IEntity localMainEnt = SCR_PlayerController.GetLocalMainEntity();
		
		//------------------------------------------------------------------------------------------------
		// ALL LOBBY-BASED CHARACTERS
		//------------------------------------------------------------------------------------------------
		if(m_Gamemode)
		{
			foreach (COA_PlayerCharacter lobbyEntity : m_Gamemode.GetActiveCharacters())
			{	
				if (lobbyEntity && lobbyEntity != localMainEnt)
				{
					RplComponent playerRplComponent = RplComponent.Cast(lobbyEntity.FindComponent(RplComponent));
					if (!playerRplComponent)
						continue;

					RplId playerRplId = playerRplComponent.Id();
					comparisonRplIds.Insert(playerRplId);
					SetIconForEntity(lobbyEntity, playerRplId);
				};
			};
		};
		
		//------------------------------------------------------------------------------------------------
		// CLEAR ICONS THAT DONT EXIST
		//------------------------------------------------------------------------------------------------
		
		array<int> indexesToDelete = {};
		
		foreach (RplId rplId : m_aEntityIcons)
		{
			if(!rplId || !rplId.IsValid() || !comparisonRplIds.Contains(rplId))
			{
				int index = m_aEntityIcons.Find(rplId);
				
				if(index != -1)
					indexesToDelete.Insert(index);
			}
		};
		
		foreach (int index : indexesToDelete)
		{
			m_aEntityIcons.RemoveOrdered(index);
			delete m_aSpectatorWidgets.Get(index);
			m_aSpectatorWidgets.RemoveOrdered(index);
			m_aSpectatorIcons.RemoveOrdered(index);
		}
		
		//------------------------------------------------------------------------------------------------
		// GROUP NATO ICONS - Create/update floating NATO group icons for all groups
		//------------------------------------------------------------------------------------------------
		UpdateGroupIcons();
	}
	
	/**
	 * Checks if an entity's icon should be shown based on faction restrictions
	 * @param entity - The entity to check
	 * @return true if the icon should be shown, false otherwise
	 */
	protected bool ShouldShowEntityIcon(IEntity entity)
	{
		if (!entity)
			return false;
		
		// Get the local spectator's faction
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		Faction localPlayerFaction = COA_SlottingManager.GetInstance().GetPlayerSlotFaction(localPlayerId);
		
		if (!localPlayerFaction)
			return true; // If local player has no faction, show all icons
		
		// Get the entity's faction
		FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (!factionComp)
			return true; // If entity has no faction component, show it
		
		Faction entityFaction = factionComp.GetAffiliatedFaction();
		if (!entityFaction)
			return true; // If entity has no faction, show it
		
		// Only show entities from the same faction
		return (entityFaction == localPlayerFaction);
	}
	
	/**
	 * Set the icon for the provided entity
	 * @param entity - Entity to pass along to the icon
	 * @param entityId - EntityId to use to insert into the icon arrays
	 */
	protected void SetIconForEntity(IEntity entity, RplId entityId)
	{
		// Skip if icon already exists
		if (m_aEntityIcons.Contains(entityId))
			return;
		
		// Check faction restrictions - hide icons from other factions if enabled
		if (m_Gamemode && m_Gamemode.m_bHideOtherSpectatorFactions)
		{
			if (!ShouldShowEntityIcon(entity))
				return;
		}
		
		// Create new spectator icon
		Widget spectatorIconWidget = GetGame().GetWorkspace().CreateWidgets(
			"{68625BAD23CEE68F}UI/Spectator/SpectatorLabelIconCharacter.layout", 
			FrameWidget.Cast(GetRootWidget().FindAnyWidget("IconsFrame"))
		);
		
		COA_SpectatorLabelIconCharacter spectatorIcon = COA_SpectatorLabelIconCharacter.Cast(
			spectatorIconWidget.FindHandler(COA_SpectatorLabelIconCharacter)
		);
		
		// If the character is alive and not a spectator, let spectators spectate them
		if (!COA_EntityHelper.IsSpectator(entity))
		{
			// Give the icon a reference to this menu so its click callbacks can call SelectSpecCursorFPP/TPP directly
			spectatorIcon.SetSpectatorMenu(this);
			
			// LMB — follow in FPP (helmet cam) via direct entity reference, bypassing cursor hit-testing
			spectatorIcon.GetButton().m_OnClicked.Insert(spectatorIcon.OnLMBClicked);
		}
		
		spectatorIcon.SetEntity(entity, "Spine3");
		
		// Store references to the icon
		m_aEntityIcons.Insert(entityId);
		m_aSpectatorIcons.Insert(spectatorIcon);
		m_aSpectatorWidgets.Insert(spectatorIconWidget);
	};
	
	//=================================================================================================
	// GROUP NATO ICON METHODS
	//=================================================================================================
	
	/**
	 * Updates group NATO icons across all factions
	 * Creates new icons for groups that don't have them, removes stale ones
	 */
	protected void UpdateGroupIcons()
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return;
		
		// Collect all current group IDs across all factions
		array<int> currentGroupIds = {};
		array<SCR_AIGroup> allGroups = slottingManager.GetAllGroups();
		
		foreach (SCR_AIGroup group : allGroups)
		{
			if (!group || group.IsPrivate())
				continue;
			
			int groupId = group.GetGroupID();
			currentGroupIds.Insert(groupId);
			
			// Create icon if it doesn't exist yet
			SetIconForGroup(group, groupId);
		}
		
		// Remove group icons that no longer exist.
		// Collect IDs to remove first, then delete from highest index downward
		// so that RemoveOrdered doesn't invalidate subsequent indices.
		array<int> groupIdsToDelete = {};

		foreach (int storedGroupId : m_aGroupIconIds)
		{
			if (!currentGroupIds.Contains(storedGroupId))
				groupIdsToDelete.Insert(storedGroupId);
		}

		foreach (int idToDelete : groupIdsToDelete)
		{
			int index = m_aGroupIconIds.Find(idToDelete);
			if (index == -1)
				continue;

			m_aGroupIconIds.RemoveOrdered(index);
			delete m_aGroupIconWidgets.Get(index);
			m_aGroupIconWidgets.RemoveOrdered(index);
			m_aGroupIcons.RemoveOrdered(index);
		}
	}
	
	/**
	 * Checks if a group's icon should be shown based on faction restrictions
	 * @param group - The group to check
	 * @return true if the icon should be shown, false otherwise
	 */
	protected bool ShouldShowGroupIcon(SCR_AIGroup group)
	{
		if (!group)
			return false;
		
		// Get the local spectator's faction
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		Faction localPlayerFaction = COA_SlottingManager.GetInstance().GetPlayerSlotFaction(localPlayerId);
		
		if (!localPlayerFaction)
			return true; // If local player has no faction, show all group icons
		
		// Get the group's faction
		Faction groupFaction = group.GetFaction();
		if (!groupFaction)
			return true; // If group has no faction, show it
		
		// Only show groups from the same faction
		return (groupFaction == localPlayerFaction);
	}
	
	/**
	 * Creates a floating NATO group icon for the specified group
	 * @param group - The SCR_AIGroup to create an icon for
	 * @param groupId - The group's unique ID
	 */
	protected void SetIconForGroup(SCR_AIGroup group, int groupId)
	{
		// Skip if icon already exists for this group
		if (m_aGroupIconIds.Contains(groupId))
			return;
		
		// Check faction restrictions - hide group icons from other factions if enabled
		if (m_Gamemode && m_Gamemode.m_bHideOtherSpectatorFactions)
		{
			if (!ShouldShowGroupIcon(group))
				return;
		}
		
		// Create new group icon widget
		Widget groupIconWidget = GetGame().GetWorkspace().CreateWidgets(
			"{B19F5B7D61B4E1C1}UI/Spectator/SpectatorLabelIconGroup.layout",
			FrameWidget.Cast(GetRootWidget().FindAnyWidget("IconsFrame"))
		);
		
		if (!groupIconWidget)
			return;
		
		COA_SpectatorLabelIconGroup groupIcon = COA_SpectatorLabelIconGroup.Cast(
			groupIconWidget.FindHandler(COA_SpectatorLabelIconGroup)
		);
		
		if (!groupIcon)
		{
			groupIconWidget.RemoveFromHierarchy();
			return;
		}
		
		groupIcon.SetGroup(group);
		
		// Store references
		m_aGroupIconIds.Insert(groupId);
		m_aGroupIcons.Insert(groupIcon);
		m_aGroupIconWidgets.Insert(groupIconWidget);
	}
	
	/**
	 * Update UI panel visibility based on cursor position
	 * @param tDelta - Time since last frame
	 */
	protected void UpdateUIPanelVisibility(float tDelta)
	{
		// Get cursor position
		int x, y;
		WidgetManager.GetMousePos(x, y);
		y = GetGame().GetWorkspace().DPIUnscale(y);
		
		// Get screen size
		float sX, sY;
		m_wRoot.GetScreenSize(sX, sY);
		
		// Update slots panel visibility
		float leftSlotX = FrameSlot.GetPosX(m_wFrameSlots);
		float leftSlotY = FrameSlot.GetPosY(m_wFrameSlots);
		
		if (x <= leftSlotX + 220 && y >= leftSlotY && y <= leftSlotY + 450)
		{
			// Expand slots panel when cursor is over it
			leftSlotX += tDelta * 2400.0;
			if (leftSlotX > 0)
				leftSlotX = 0;
			
			FrameSlot.SetPosX(m_wFrameSlots, leftSlotX);
			m_wRoot.FindAnyWidget("SliderBGL").SetVisible(false);
			m_wRoot.FindAnyWidget("ArrowL").SetVisible(false);
		}
		else
		{
			// Collapse slots panel when cursor moves away
			leftSlotX -= tDelta * 2400.0;
			if (leftSlotX < -200)
				leftSlotX = -200;
			
			FrameSlot.SetPosX(m_wFrameSlots, leftSlotX);
			m_wRoot.FindAnyWidget("SliderBGL").SetVisible(true);
			m_wRoot.FindAnyWidget("ArrowL").SetVisible(true);
		}
		
		// Update VON channels panel visibility
		float leftVONX = FrameSlot.GetPosX(m_wFrameChannels);
		float leftVONY = FrameSlot.GetPosY(m_wFrameChannels);
		
		if (x >= leftVONX -20 + sX && y >= leftVONY && y <= leftVONY + 450)
		{
			// Expand VON panel when cursor is over it
			leftVONX -= tDelta * 2400.0;
			if (leftVONX < -220)
				leftVONX = -220;
			
			FrameSlot.SetPosX(m_wFrameChannels, leftVONX);
			m_wRoot.FindAnyWidget("SliderBGR").SetVisible(false);
			m_wRoot.FindAnyWidget("ArrowR").SetVisible(false);
		}
		else
		{
			// Collapse VON panel when cursor moves away
			leftVONX += tDelta * 2400.0;
			if (leftVONX > -20)
				leftVONX = -20;
			
			FrameSlot.SetPosX(m_wFrameChannels, leftVONX);
			m_wRoot.FindAnyWidget("SliderBGR").SetVisible(true);
			m_wRoot.FindAnyWidget("ArrowR").SetVisible(true);
		}
		
		// Update VON channels panel visibility
		float leftGameInfoX = FrameSlot.GetPosX(m_wFrameGameInfo);
		float leftGameInfoY = FrameSlot.GetPosY(m_wFrameGameInfo);
		
		if (x <= leftGameInfoX + 170 && y >= leftGameInfoY && y <= leftGameInfoY + 200)
		{
			// Expand slots panel when cursor is over it
			leftGameInfoX += tDelta * 2400.0;
			if (leftGameInfoX > 0)
				leftGameInfoX = 0;
			
			FrameSlot.SetPosX(m_wFrameGameInfo, leftGameInfoX);
			m_wRoot.FindAnyWidget("SliderBGLL").SetVisible(false);
			m_wRoot.FindAnyWidget("ArrowLL").SetVisible(false);
		}
		else
		{
			// Collapse slots panel when cursor moves away
			leftGameInfoX -= tDelta * 2400.0;
			if (leftGameInfoX < -150)
				leftGameInfoX = -150;
			
			FrameSlot.SetPosX(m_wFrameGameInfo, leftGameInfoX);
			m_wRoot.FindAnyWidget("SliderBGLL").SetVisible(true);
			m_wRoot.FindAnyWidget("ArrowLL").SetVisible(true);
		}
	}
	
	/**
	 * Create a new VON channel
	 */
	void CreateChannel()
	{
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		string expectedChannelName = GetGame().GetPlayerManager().GetPlayerName(localPlayerId) + "'s Channel (" + localPlayerId + ")";
		string localPlayerIdStr = localPlayerId.ToString();

		// Check if player already owns an active channel - i.e. a channel bearing their name that
		// they are still actually a member of, not just a name match. A name match alone isn't
		// enough: if this client's replicated copy of m_aVONChannels is a frame behind the server
		// (e.g. right after leaving the room, before the empty-channel cleanup has replicated
		// back down), blindly matching on name would silently refuse to create a new channel with
		// no feedback at all, making it look permanently stuck.
		foreach(string channel: m_MenuManager.m_aVONChannels)
		{
			ref array<string> channelSplit = {};
			channel.Split("|", channelSplit, true);
			if (channelSplit.Get(0) != expectedChannelName)
				continue;

			if (channelSplit.Count() > 1)
			{
				ref array<string> members = {};
				channelSplit.Get(1).Split(",", members, true);
				if (members.Contains(localPlayerIdStr))
				{
					if (m_PopUpNotification)
						m_PopUpNotification.PopupMsg("You already have an active voice channel.", 5);

					return;
				}
			}

			// Name matches but the player isn't actually a member (stale/orphaned entry) -
			// don't block creation on it, keep looking/fall through.
		}

		// Create a new channel
		COA_PlayerRplToAuthorityManager.GetInstance().CreateChannel(localPlayerId);

		// Schedule radio frequency update after channel creation
		// Use a longer delay to allow server replication and channel assignment to complete
		if (!CVON_VONGameModeComponent.GetInstance())
			GetGame().GetCallqueue().CallLater(UpdateRadioFrequency, 500, false);
	}
	
	/**
	 * Toggle UI visibility
	 */
	void HideUI()
	{
		SCR_HUDManagerComponent hudManager = SCR_HUDManagerComponent.Cast(
			SCR_PlayerController.Cast(GetGame().GetPlayerController()).GetHUDManagerComponent()
		);
		
		if (m_wRoot.IsVisible())
		{
			// Hide UI
			m_wRoot.SetVisible(false);
			hudManager.GetHUDRootWidget().SetVisible(false);
		}
		else
		{
			// Show UI
			m_wRoot.SetVisible(true);
			hudManager.GetHUDRootWidget().SetVisible(true);
		}
	}
	
	/**
	 * Updates the Voice Over Network (VON) channels display in the UI
	 * Shows available channels and their members
	 */
	void UpdateChannel()
	{
		// Clear existing channels
		m_wVONChannels.Clear();
		m_mChannelHeaderIndex.Clear();
		m_mChannelPlayerRowIndex.Clear();

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		// Iterate through all available channels
		foreach(int channelIndex, string channelData: m_MenuManager.m_aVONChannels)
		{
			BuildChannelRows(channelIndex, channelData);
		}

		// Update local channel counter to match server state
		m_iLocalChannelUpdates = m_MenuManager.m_iChannelChanges;

		UpdateLocalPlayerRadioState();
	}

	/**
	 * Builds the header + player rows for a single channel, appending them at the current end of
	 * m_wVONChannels and recording their indices so a later per-player move can patch just the
	 * affected rows without reparsing/rebuilding the whole channel list.
	 * @param channelIndex Index of this channel within m_MenuManager.m_aVONChannels
	 * @param channelData Raw "ChannelName|PlayerID1,PlayerID2,..." string for this channel
	 */
	protected void BuildChannelRows(int channelIndex, string channelData)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		// Parse channel data format: "ChannelName|PlayerID1,PlayerID2,..."
		ref array<string> channelParts = {};
		channelData.Split("|", channelParts, true);

		if (channelParts.Count() == 0)
			return;

		string channelName = channelParts.Get(0);

		// Add channel to the list
		int headerIndex = m_wVONChannels.AddItemChannel(null, channelName);
		COA_ListBoxElementComponent channelComponent = m_wVONChannels.GetCRFElementComponent(headerIndex);

		if (!channelComponent)
			return;

		// Store channel ID and register join button click handler
		channelComponent.m_iChannelId = channelIndex;
		channelComponent.GetChannelButton().m_OnClicked.Insert(JoinChannelDelay);

		m_mChannelHeaderIndex.Set(channelIndex, headerIndex);
		map<int, int> playerRowIndex = new map<int, int>();
		m_mChannelPlayerRowIndex.Set(channelIndex, playerRowIndex);

		// If channel has no players, nothing more to add
		if (channelParts.Count() <= 1)
			return;

		// Parse player IDs in the channel
		ref array<string> playerIds = {};
		channelParts.Get(1).Split(",", playerIds, true);

		// Add each player in the channel to the display
		foreach(string playerIdStr: playerIds)
		{
			// Skip invalid player IDs
			if (playerIdStr.IsEmpty())
				continue;

			int playerId = playerIdStr.ToInt();

			if (!IsPlayerVisibleInChannel(playerId, channelName))
				continue;

			AppendChannelPlayerRow(channelIndex, playerId, playerManager);
		}
	}

	/**
	 * Surgical handler for COA_MenuManager's per-player channel-move notification (see
	 * COA_MenuManager.GetOnPlayerChannelChanged()): patches only the affected rows instead of
	 * reparsing/rebuilding the whole VON channel list.
	 */
	void OnPlayerChannelChanged(int playerId, int newChannelIndex, int oldChannelIndex)
	{
		bool handledFully = true;

		if (oldChannelIndex >= 0 && !RemoveChannelPlayerRow(oldChannelIndex, playerId))
			handledFully = false;

		if (newChannelIndex >= 0 && IsPlayerVisibleInChannel(playerId, GetChannelName(newChannelIndex)))
		{
			if (!AddChannelPlayerRow(newChannelIndex, playerId))
				handledFully = false;
		}

		if (playerId == SCR_PlayerController.GetLocalPlayerId())
			UpdateLocalPlayerRadioState();

		// Only suppress the structural fallback poll (OnMenuUpdate's m_iChannelChanges check) when
		// the surgical patch actually covered this change in full - e.g. NOT a brand-new channel the
		// client hasn't built a header for yet, which still needs the full UpdateChannel() rebuild.
		if (handledFully)
			m_iLocalChannelUpdates = m_MenuManager.m_iChannelChanges;
	}

	/**
	 * Removes a single player's row from a channel, if one is currently tracked there.
	 * @return False if the channel itself was never built (caller should fall back to a full rebuild)
	 */
	protected bool RemoveChannelPlayerRow(int channelIndex, int playerId)
	{
		if (!m_mChannelHeaderIndex.Contains(channelIndex))
			return false;

		if (!m_mChannelPlayerRowIndex.Contains(channelIndex))
			return true;

		map<int, int> playerRowIndex = m_mChannelPlayerRowIndex.Get(channelIndex);
		if (!playerRowIndex.Contains(playerId))
			return true;

		int rowIndex = playerRowIndex.Get(playerId);
		playerRowIndex.Remove(playerId);

		m_wVONChannels.RemoveItemRange(rowIndex, rowIndex);
		ShiftChannelIndicesAfter(rowIndex, 1);
		return true;
	}

	/**
	 * Adds a single player's row to a channel. Since the listbox is append-only, this rebuilds the
	 * channel's whole block (header + its already-tracked players, reconstructed from
	 * m_mChannelPlayerRowIndex rather than by reparsing the replicated array - see
	 * COA_MenuManager.AddPlayerToChannel for why re-parsing here would be racy) at the end of the
	 * list, plus the new player - so the channel visibly moves to the bottom on membership changes.
	 * @return False if the channel itself was never built (caller should fall back to a full rebuild)
	 */
	protected bool AddChannelPlayerRow(int channelIndex, int playerId)
	{
		if (!m_mChannelHeaderIndex.Contains(channelIndex))
			return false;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return true;

		map<int, int> playerRowIndex = m_mChannelPlayerRowIndex.Get(channelIndex);
		if (playerRowIndex.Contains(playerId))
			return true; // already shown

		array<int> existingPlayerIds = {};
		foreach (int existingPlayerId, int existingRowIdx : playerRowIndex)
			existingPlayerIds.Insert(existingPlayerId);

		int headerIndex = m_mChannelHeaderIndex.Get(channelIndex);
		int lastIndex = headerIndex;
		foreach (int existingRowIdx : playerRowIndex)
		{
			if (existingRowIdx > lastIndex)
				lastIndex = existingRowIdx;
		}

		string channelName = GetChannelName(channelIndex);

		m_mChannelHeaderIndex.Remove(channelIndex);
		m_mChannelPlayerRowIndex.Remove(channelIndex);
		m_wVONChannels.RemoveItemRange(headerIndex, lastIndex);
		ShiftChannelIndicesAfter(headerIndex, lastIndex - headerIndex + 1);

		int newHeaderIndex = m_wVONChannels.AddItemChannel(null, channelName);
		COA_ListBoxElementComponent channelComponent = m_wVONChannels.GetCRFElementComponent(newHeaderIndex);
		if (channelComponent)
		{
			channelComponent.m_iChannelId = channelIndex;
			channelComponent.GetChannelButton().m_OnClicked.Insert(JoinChannelDelay);
		}

		m_mChannelHeaderIndex.Set(channelIndex, newHeaderIndex);
		map<int, int> newPlayerRowIndex = new map<int, int>();
		m_mChannelPlayerRowIndex.Set(channelIndex, newPlayerRowIndex);

		foreach (int existingPlayerId : existingPlayerIds)
			AppendChannelPlayerRow(channelIndex, existingPlayerId, playerManager);

		AppendChannelPlayerRow(channelIndex, playerId, playerManager);
		return true;
	}

	/**
	 * Appends one player row at the current end of m_wVONChannels and records its index.
	 * Caller is responsible for making sure this lands within the correct channel's block.
	 */
	protected void AppendChannelPlayerRow(int channelIndex, int playerId, PlayerManager playerManager)
	{
		int playerIndex = m_wVONChannels.AddItem(
			playerManager.GetPlayerName(playerId),
			null,
			"{68D74FF57296AFFB}UI/Listbox/PlayerListboxElementVON.layout"
		);

		COA_ListBoxElementComponent playerComponent = m_wVONChannels.GetCRFElementComponent(playerIndex);
		if (playerComponent)
		{
			playerComponent.m_iPlayerId = SCR_PlayerController.GetLocalPlayerId();
			playerComponent.m_bIsPlayer = true;
		}

		m_mChannelPlayerRowIndex.Get(channelIndex).Set(playerId, playerIndex);
	}

	/**
	 * After removing rows starting at removedFromIndex, every other tracked channel/player row index
	 * past that point needs to shift down by the removed count to stay accurate for future patches.
	 */
	protected void ShiftChannelIndicesAfter(int removedFromIndex, int delta)
	{
		foreach (int channelIdx, int headerIdx : m_mChannelHeaderIndex)
		{
			if (headerIdx > removedFromIndex)
				m_mChannelHeaderIndex.Set(channelIdx, headerIdx - delta);
		}

		foreach (int channelIdx, map<int, int> playerRowIndex : m_mChannelPlayerRowIndex)
		{
			array<int> playerIds = {};
			foreach (int pid, int rowIdx : playerRowIndex)
				playerIds.Insert(pid);

			foreach (int pid : playerIds)
			{
				int rowIdx = playerRowIndex.Get(pid);
				if (rowIdx > removedFromIndex)
					playerRowIndex.Set(pid, rowIdx - delta);
			}
		}
	}

	/**
	 * Resolves a channel's display name from its stable name portion of the replicated array entry.
	 * Only the name is read (never the player-list portion, which can be stale relative to a
	 * just-arrived per-player notification - see COA_MenuManager.AddPlayerToChannel).
	 */
	protected string GetChannelName(int channelIndex)
	{
		if (channelIndex < 0 || channelIndex >= m_MenuManager.m_aVONChannels.Count())
			return "";

		ref array<string> nameParts = {};
		m_MenuManager.m_aVONChannels[channelIndex].Split("|", nameParts, true);
		if (nameParts.IsEmpty())
			return "";

		return nameParts.Get(0);
	}

	/**
	 * Visibility rules for the spectator VON display: connected, "Deafen" shows only the local
	 * player, and spectator channels only show dead players. Deliberately takes the channel name
	 * (not an index) so callers control whether it comes from a stable read (see GetChannelName).
	 */
	protected bool IsPlayerVisibleInChannel(int playerId, string channelName)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager || !playerManager.IsPlayerConnected(playerId))
			return false;

		if (playerId != SCR_PlayerController.GetLocalPlayerId() && channelName == "Deafen")
			return false;

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (slottingManager)
		{
			COA_SlotData playerSlotData = slottingManager.GetPlayerSlotData(playerId);
			if (playerSlotData && !playerSlotData.GetIsDeadSlot())
				return false; // Skip alive players
		}

		return true;
	}

	/**
	 * Toggles radio power/frequency for the local player based on their current channel. Runs
	 * unconditionally after a full UpdateChannel() rebuild, and after any surgical channel change
	 * that affects the local player specifically (see OnPlayerChannelChanged).
	 */
	protected void UpdateLocalPlayerRadioState()
	{
		if (CVON_VONGameModeComponent.GetInstance())
			return;

		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		bool isInChannel = m_MenuManager.GetChannel(localPlayerId) != 0;
		SetRadioPower(isInChannel);

		// Update radio frequency to match current channel assignment
		// This ensures the radio frequency is correct after channel changes
		if (isInChannel)
		{
			// Schedule frequency update after a small delay to ensure replication is complete
			GetGame().GetCallqueue().CallLater(UpdateRadioFrequency, 100, false);
		}
	}
	
	/**
	 * Updates the radio frequency to match the current channel assignment
	 * Called after channel changes to ensure proper frequency synchronization
	 */
	protected void UpdateRadioFrequency()
	{
		// Get the current transceiver and update its frequency
		RadioTransceiver transceiver = GetVoNTransiver();
		// The GetVoNTransiver() call automatically sets the correct frequency
		// No additional work needed here as the frequency is set in that method
	}
	
	/**
	 * Schedules channel join with a short delay to prevent UI issues
	 */
	void JoinChannelDelay()
	{
		GetGame().GetCallqueue().Call(JoinChannel);
	}
	
	/**
	 * Joins the currently selected VON channel
	 */
	void JoinChannel()
	{
		// Get the selected channel component
		COA_ListBoxElementComponent selectedComponent = m_wVONChannels.GetCRFElementComponent(m_wVONChannels.GetSelectedItem());
		if (!selectedComponent)
			return;
		
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		int channelId = selectedComponent.m_iChannelId;
		
		// Handle channel join through appropriate manager based on channel type
		if (channelId > 1)
		{
			// Request to join non-default channel via RPC
			Print(string.Format("[VON] Client %1 requesting to join channel %2", localPlayerId, channelId), LogLevel.NORMAL);
			COA_PlayerRplToAuthorityManager.GetInstance().RequestToJoinChannel(channelId, localPlayerId);
		}
		else
		{
			// Join default channel directly
			COA_PlayerRplToAuthorityManager.GetInstance().JoinChannel(localPlayerId, channelId);
		}
		
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			// Schedule radio frequency update after channel join
			// Use a delay to allow server replication to complete
			GetGame().GetCallqueue().CallLater(UpdateRadioFrequency, 200, false);
		}
	}

	/**
	 * Selects a specific entity to spectate in true first-person (eye cam) mode - the default for
	 * left-click. Called directly from the icon's left-click handler with the known entity,
	 * bypassing cursor hit-testing which is unreliable inside button callbacks.
	 * @param entity - The entity to follow in eye-cam mode
	 */
	void SelectSpecCursor(IEntity entity)
	{
		if (!entity)
			return;

		IEntity specEntity = SCR_PlayerController.GetLocalMainEntity();
		if (!COA_EntityHelper.IsSpectator(specEntity))
			return;

		// Toggle off if already following this entity in FPP mode (helmet or eye cam)
		if (m_bFrameEventRegistered && m_eSpecEntity == entity)
		{
			m_eSpecEntity = null;
			UnregisterFrameEvent();
			return;
		};
			
		m_eSpecEntity = entity;

		m_bFrameEventRegistered = true;
		SetCameraMode(m_iCamCycle);
		UpdateCameraModeButtonsUI();
	}
	
	/**
	 * Called only on GetOnSlotChanged events. Closes the spectator menu and inserts the local player
	 * if the slot that just changed now belongs to them and is not a dead slot.
	 * Checking the specific changed slot (rather than IsPlayerInASlot) ensures only the slotted
	 * player's own client reacts, not every spectator who happens to be in a live slot.
	 */
	protected void OnSlotChangedCheckAutoInsert()
	{
		if (!m_Gamemode || m_Gamemode.m_GamemodeState != COA_EGamemodeState.GAME)
			return;

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		int changedSlotId = slottingManager.GetLastChangedSlotId();
		if (changedSlotId <= 0)
			return;

		COA_SlotData changedSlot = slottingManager.GetSlotData(changedSlotId);
		if (!changedSlot)
			return;

		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		if (changedSlot.GetSlotCurrentPlayerId() == localPlayerId && !changedSlot.GetIsDeadSlot())
		{
			GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.COA_SpectatorMenu);
			COA_PlayerRplToAuthorityManager.GetInstance().RequestInitilizePlayer(localPlayerId);
		}
	}

	/**
	 * Selects BLUFOR faction for display
	 */
	void SelectFactionBlufor()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("BLUFOR");
		UpdateSlots();
	}
	
	/**
	 * Selects OPFOR faction for display
	 */
	void SelectFactionOpfor()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("OPFOR");
		UpdateSlots();
	}
	
	/**
	 * Selects INDFOR faction for display
	 */
	void SelectFactionIndfor()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("INDFOR");
		UpdateSlots();
	}
	
	/**
	 * Selects Civilian faction for display
	 */
	void SelectFactionCiv()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("CIV");
		UpdateSlots();
	}
	
	/**
	 * Initializes slot counters for all factions
	 * Counts total and alive slots for each faction
	 */
	void InitSlots()
	{
		// Get all slots from the slotting manager
		map<int, ref COA_SlotData> slotMap = COA_SlottingManager.GetInstance().GetSlotMap();
		
		// Process each slot to count by faction
		foreach (int slotId, COA_SlotData slotData : slotMap)
		{
			// Skip locked or empty slots
			if(slotData.GetIsLockedSlot() || slotData.GetSlotCurrentPlayerId() == 0)
				continue;
			
			// Update counters based on faction
			string factionKey = slotData.GetSlotFactionKey();
			bool isAlive = !slotData.GetIsDeadSlot();
			
			if (factionKey == "BLUFOR")
			{
				m_iBluforSlots++;
				if(isAlive) 
					m_iAliveBluforSlots++;
			}
			else if (factionKey == "OPFOR")
			{
				m_iOpforSlots++;
				if(isAlive) 
					m_iAliveOpforSlots++;
			}
			else if (factionKey == "INDFOR")
			{
				m_iIndforSlots++;
				if(isAlive) 
					m_iAliveIndforSlots++;
			}
			else if (factionKey == "CIV")
			{
				m_iCivSlots++;
				if(isAlive) 
					m_iAliveCivSlots++;
			}
		}
	}
	
	/**
	 * Updates the UI to display slots for the selected faction
	 * Shows faction flags, player counts, and player lists by group
	 */
	void UpdateSlots()
	{
		// Reset faction counters
		m_iBluforSlots = 0;
		m_iOpforSlots = 0;
		m_iIndforSlots = 0;
		m_iCivSlots = 0;
		m_iAliveBluforSlots = 0;
		m_iAliveOpforSlots = 0;
		m_iAliveIndforSlots = 0;
		m_iAliveCivSlots = 0;

		// Clear player slots list
		m_wPlayerSlots.Clear();
		m_mGroupHeaderIndex.Clear();
		m_mGroupSlotIndices.Clear();

		// Initialize slot counters
		InitSlots();

		// Update faction UI elements
		UpdateFactionUI("BLUFOR", m_wRoot.FindAnyWidget("BLUButton"),
			m_wRoot.FindAnyWidget("BLUFlag"), m_wRoot.FindAnyWidget("BLURatio"),
			m_iAliveBluforSlots, m_iBluforSlots);

		UpdateFactionUI("OPFOR", m_wRoot.FindAnyWidget("OPFButton"),
			m_wRoot.FindAnyWidget("OPFFlag"), m_wRoot.FindAnyWidget("OPFRatio"),
			m_iAliveOpforSlots, m_iOpforSlots);

		UpdateFactionUI("INDFOR", m_wRoot.FindAnyWidget("INDButton"),
			m_wRoot.FindAnyWidget("INDFlag"), m_wRoot.FindAnyWidget("INDRatio"),
			m_iAliveIndforSlots, m_iIndforSlots);

		UpdateFactionUI("CIV", m_wRoot.FindAnyWidget("CIVButton"),
			m_wRoot.FindAnyWidget("CIVFlag"), m_wRoot.FindAnyWidget("CIVRatio"),
			m_iAliveCivSlots, m_iCivSlots);

		array<SCR_AIGroup> factionGroups = {};

		if (m_fSelectedFaction)
			factionGroups = COA_SlottingManager.GetInstance().GetAllGroups(m_fSelectedFaction.GetFactionKey());

		if (factionGroups.IsEmpty())
			return;

		// Process each group and its players
		foreach(SCR_AIGroup group : factionGroups)
		{
			// Skip private groups
			if(group.IsPrivate())
				continue;

			BuildGroupRows(group);
		}
	}

	/**
	 * Builds (or rebuilds, if called via RebuildGroupRows) the header + slot rows for a single group,
	 * appending them at the current end of m_wPlayerSlots and recording their indices in
	 * m_mGroupHeaderIndex/m_mGroupSlotIndices so a later per-slot change can patch just this group.
	 * @param group The group to build rows for
	 */
	protected void BuildGroupRows(SCR_AIGroup group)
	{
		map<int, ref COA_SlotData> slotMap = COA_SlottingManager.GetInstance().GetSlotMap();

		int playersInGroup = 0;
		int deadPlayersInGroup = 0;

		// Add group to the player slots UI
		int groupIndex = m_wPlayerSlots.AddItemSpecGroup(null, group);
		COA_ListBoxElementComponent groupComponent = m_wPlayerSlots.GetCRFElementComponent(groupIndex);

		if (groupComponent)
		{
			// Set group colors and icon
			Color factionColor = group.GetFaction().GetFactionColor();
			groupComponent.GetGroupUnderline().SetColor(factionColor);

			if(group.GetFaction().GetFactionKey() == "INDFOR")
				groupComponent.GetGroupIcon().SetColor(factionColor);

			groupComponent.GetGroupIcon().LoadImageFromSet(0, SCR_Faction.Cast(group.GetFaction()).GetGroupFlagImageSet(), group.GetGroupFlag());
		}

		// Get group ID
		RplId groupId = 0;
		RplComponent groupRplComp = RplComponent.Cast(group.FindComponent(RplComponent));
		if (groupRplComp)
		{
			groupId = groupRplComp.Id();
		}

		array<int> slotIndices = {};

		// Process all slots in this group
		foreach(int slotId, COA_SlotData slotData : slotMap)
		{
			// Skip slots that don't belong to this group/faction
			if (slotData.GetSlotCurrentGroup() != groupId ||
				slotData.GetIsLockedSlot() ||
				slotData.GetSlotCurrentPlayerId() == 0 ||
				GetGame().GetFactionManager().GetFactionByKey(slotData.GetSlotFactionKey()) != m_fSelectedFaction)
				continue;

			// Count dead players
			if (slotData.GetIsDeadSlot())
			{
				deadPlayersInGroup++;
				continue;
			}

			// Skip locked slots
			if(slotData.GetIsLockedSlot() && slotData.GetSlotCurrentPlayerId() <= 0)
				continue;

			// Add slot to the UI
			int slotIndex = m_wPlayerSlots.AddItemSpecSlot(null, slotId);
			COA_ListBoxElementComponent slotComponent = m_wPlayerSlots.GetCRFElementComponent(slotIndex);
			slotIndices.Insert(slotIndex);

			// Count occupied slots
			if (slotData.GetSlotCurrentPlayerId() > 0)
				playersInGroup++;

			// Add click handler for spectating
			if (slotComponent && slotComponent.GetSlotButton())
			{
				slotComponent.GetSlotButton().m_OnClicked.Insert(SelectSpecDelay);
			}

			// Set player name if slot is occupied
			if (slotData.GetSlotCurrentPlayerId() > 0 && slotComponent)
			{
				PlayerManager playerManager = GetGame().GetPlayerManager();
				if (playerManager)
				{
					slotComponent.SetPlayerText(playerManager.GetPlayerName(slotData.GetSlotCurrentPlayerId()));

					// Show disconnected indicator if player is no longer connected
					if (!playerManager.IsPlayerConnected(slotData.GetSlotCurrentPlayerId()))
					{
						slotComponent.GetDisconnectWidget().SetVisible(true);
					}
				}
			}
		}

		// Remove empty groups; only track groups that actually have visible rows, so a later
		// per-slot change knows there's nothing to remove before rebuilding.
		if (playersInGroup == 0)
		{
			m_wPlayerSlots.RemoveItem(groupIndex);
		}
		else
		{
			m_mGroupHeaderIndex.Set(group, groupIndex);
			m_mGroupSlotIndices.Set(group, slotIndices);
		}
	}

	/**
	 * Surgical handler for COA_SlottingManager's per-slot player-ID change invoker (see
	 * COA_SlottingManager.GetOnSlotChanged()): patches only the affected group's rows instead of
	 * rebuilding the entire slot list.
	 */
	void OnSlotPlayerIdChanged()
	{
		int slotId = COA_SlottingManager.GetInstance().GetLastChangedSlotId();
		COA_SlotData slotData = COA_SlottingManager.GetInstance().GetSlotData(slotId);
		if (!slotData)
			return;

		// Not the currently-displayed faction — nothing visible is affected.
		if (GetGame().GetFactionManager().GetFactionByKey(slotData.GetSlotFactionKey()) != m_fSelectedFaction)
			return;

		SCR_AIGroup group = COA_EntityHelper.GetGroupFromRplId(slotData.GetSlotCurrentGroup());
		if (!group || group.IsPrivate())
			return;

		RebuildGroupRows(group);
	}

	/**
	 * Removes a group's existing rows (if any are currently tracked/visible) and rebuilds them,
	 * freshly appended at the end of the list — the engine's listbox widgets are append-only, so a
	 * changed group's rows land at the end rather than back in their original position.
	 * @param group The group whose rows should be refreshed
	 */
	protected void RebuildGroupRows(SCR_AIGroup group)
	{
		if (m_mGroupHeaderIndex.Contains(group))
		{
			int headerIndex = m_mGroupHeaderIndex.Get(group);
			array<int> slotIndices = m_mGroupSlotIndices.Get(group);

			int lastIndex = headerIndex;
			if (slotIndices && !slotIndices.IsEmpty())
				lastIndex = slotIndices[slotIndices.Count() - 1];

			int removedCount = lastIndex - headerIndex + 1;

			m_mGroupHeaderIndex.Remove(group);
			m_mGroupSlotIndices.Remove(group);

			m_wPlayerSlots.RemoveItemRange(headerIndex, lastIndex);
			ShiftGroupIndicesAfter(headerIndex, removedCount);
		}

		BuildGroupRows(group);
	}

	/**
	 * After removing rows starting at removedFromIndex, every other tracked group's row indices
	 * past that point need to shift down by the removed count to stay accurate for future patches.
	 */
	protected void ShiftGroupIndicesAfter(int removedFromIndex, int delta)
	{
		foreach (SCR_AIGroup otherGroup, int headerIdx : m_mGroupHeaderIndex)
		{
			if (headerIdx > removedFromIndex)
				m_mGroupHeaderIndex.Set(otherGroup, headerIdx - delta);
		}

		foreach (SCR_AIGroup otherGroup, array<int> slotIndices : m_mGroupSlotIndices)
		{
			for (int i = 0; i < slotIndices.Count(); i++)
			{
				if (slotIndices[i] > removedFromIndex)
					slotIndices[i] = slotIndices[i] - delta;
			}
		}
	}

	/**
	 * Helper method to update a single faction's UI elements
	 * @param factionKey - The faction key (e.g., "BLUFOR", "OPFOR")
	 * @param buttonWidget - Button widget for this faction
	 * @param flagWidget - Flag image widget for this faction
	 * @param ratioWidget - Text widget showing player count ratio
	 * @param aliveCount - Number of alive players in faction
	 * @param totalCount - Total number of players in faction
	 */
	protected void UpdateFactionUI(string factionKey, 
		Widget buttonWidget, Widget flagWidget, Widget ratioWidget, int aliveCount, int totalCount)
	{
		// Skip if faction is not valid
		if (!COA_SlottingManager.GetInstance().IsFactionValid(factionKey))
			return;
			
		// Show faction button
		if (buttonWidget)
			buttonWidget.SetVisible(true);
		
		// Determine faction icon
		ResourceName iconPath;
		
		// Try to get icon from gearscript first
		ResourceName gearScriptResource = COA_Gamemode.GetInstance().GetGearScriptResource(factionKey);
		if (!gearScriptResource.IsEmpty())
		{
			COA_GearScriptConfig gearConfig = COA_GearScriptConfig.Cast(
				BaseContainerTools.CreateInstanceFromContainer(
					BaseContainerTools.LoadContainer(gearScriptResource).GetResource().ToBaseContainer()
				)
			);
			
			if (gearConfig && !gearConfig.m_FactionIcon.IsEmpty())
			{
				iconPath = gearConfig.m_FactionIcon;
			}
		}
		
		// Fallback to default faction flag
		if (iconPath.IsEmpty())
		{
			SCR_Faction faction = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey(factionKey));
			if (faction)
				iconPath = faction.GetFactionFlag();
		}
		
		// Set flag image and player count ratio
		ImageWidget flagImage = ImageWidget.Cast(flagWidget);
		if (flagImage && !iconPath.IsEmpty())
			flagImage.LoadImageTexture(0, iconPath);
			
		TextWidget ratioText = TextWidget.Cast(ratioWidget);
		if (ratioText)
			ratioText.SetText(aliveCount.ToString() + "/" + totalCount.ToString());
	}
	
	/**
	 * Schedules the spectator selection with a small delay
	 * This prevents potential UI issues from immediate execution
	 */
	void SelectSpecDelay()
	{
		// Use a short delay to allow UI to update before selection occurs
		GetGame().GetCallqueue().Call(SelectSpec);
	}
	
	/**
	 * Selects an entity to spectate based on the currently selected slot in the player list
	 * This allows the player to view the game from another player's perspective
	 */
	void SelectSpec()
	{
		// Get the component for the selected item in the player slots list
		COA_ListBoxElementComponent selectedComponent = COA_ListBoxElementComponent.Cast(
			m_wPlayerSlots.GetElementComponent(m_wPlayerSlots.GetSelectedItem())
		);
		
		// Return if no valid slot is selected
		if (!selectedComponent || selectedComponent.m_iSlotId == 0)
			return;
		
		// Get slot data from the slotting manager
		COA_SlotData slotData = COA_SlottingManager.GetInstance().GetSlotData(selectedComponent.m_iSlotId);
		if (!slotData)
			return;
		
		// Check if faction-based spectator restriction is enabled
		if (m_Gamemode && m_Gamemode.m_bHideOtherSpectatorFactions)
		{
			int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
			Faction localPlayerFaction = COA_SlottingManager.GetInstance().GetPlayerSlotFaction(localPlayerId);
			Faction targetFaction = GetGame().GetFactionManager().GetFactionByKey(slotData.GetSlotFactionKey());
			
			// Prevent spectating players from other factions
			if (localPlayerFaction && targetFaction && localPlayerFaction != targetFaction)
				return;
		}
		
		// Find the entity associated with the slot and set it as the spectator target
		RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(slotData.GetSlotCurrentCharacter()));
		if (rplComponent)
		{
			m_eSpecEntity = rplComponent.GetEntity();
		}
		else
		{
			int playerId = SCR_PlayerController.GetLocalPlayerId();
			COA_PlayerRplToAuthorityManager.GetInstance().MoveSpecCamToSlot(selectedComponent.m_iSlotId, playerId);
		}
	}
	
	/**
	 * Initializes the menu when it's first created
	 * Sets up root widget and map references
	 */
	override void OnMenuInit()
	{		
		// Call parent class initialization
		super.OnMenuInit();

		// Store reference to the root widget
		m_wRoot = GetRootWidget();
		
		// Initialize map entity if not already done
		if (!m_MapEntity)
		{
			m_MapEntity = SCR_MapEntity.GetMapInstance();
		}
		
		// Hide the map frame initially
		Widget mapFrame = m_wRoot.FindAnyWidget("MapFrame");
		if (mapFrame)
		{
			mapFrame.SetVisible(false);
		}
	}
	
	/**
	 * Performs cleanup when the menu is closed
	 * Unregisters event handlers and resets game state
	 */
	override void OnMenuClose()
	{
		// Call parent class cleanup
		super.OnMenuClose();
		
		// Unregister spectator camera frame event
		UnregisterFrameEvent();
		
		// Unregister from slotting updates
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (slottingManager)
		{
			slottingManager.GetOnSlottingUpdate().Remove(UpdateSlots);
			slottingManager.GetOnSlotChanged().Remove(OnSlotPlayerIdChanged);
			slottingManager.GetOnSlotChanged().Remove(OnSlotChangedCheckAutoInsert);
		}

		// Unregister from VON channel updates
		if (m_MenuManager)
			m_MenuManager.GetOnPlayerChannelChanged().Remove(OnPlayerChannelChanged);
		
		// Remove all action listeners
		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager)
		{
			if (!CVON_VONGameModeComponent.GetInstance())
			{
				inputManager.RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
				inputManager.RemoveActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
			}
			inputManager.RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnCOA_ChatToggleAction);
			inputManager.RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
			inputManager.RemoveActionListener("GadgetMap", EActionTrigger.DOWN, Action_ToggleMap);
			inputManager.RemoveActionListener("ManualCameraTeleport", EActionTrigger.DOWN, Action_ManualCameraTeleport);
			inputManager.RemoveActionListener("EditorToggleUI", EActionTrigger.DOWN, HideUI);
			inputManager.RemoveActionListener("COA_ShowDamageReport", EActionTrigger.DOWN, Action_ToggleDamageReport);
			inputManager.RemoveActionListener("COA_SpecNVG", EActionTrigger.DOWN, ToggleNVGs);
			inputManager.RemoveActionListener("COA_SpecToggleCamMode", EActionTrigger.DOWN, ToggleCameraMode);
			inputManager.RemoveActionListener("COA_SpecKillTeleport", EActionTrigger.DOWN, Action_TeleportToKill);
		}
		
		ForceNVGsOff();
		
		// Restore workspace opacity if it was set to 0
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace && workspace.GetOpacity() == 0)
		{
			workspace.SetOpacity(1);
		}
		
		// Reset kill feed type to default
		SCR_NotificationSenderComponent sender = SCR_NotificationSenderComponent.Cast(
			GetGame().GetGameMode().FindComponent(SCR_NotificationSenderComponent)
		);
		if (sender)
			sender.SetKillFeedTypeNoneLocal();
	}
	
	/**
	 * Opens the player list when the scoreboard key is pressed
	 */
	protected static void OnShowPlayerList()
	{
		ArmaReforgerScripted.OpenPlayerList();
	}
	
	/**
	 * Called when any notification is received locally.
	 * For killfeed notifications, resolves and stores the victim's world position
	 * so Action_TeleportToKill can jump to it.
	 */
	protected void OnKillfeedNotification(SCR_NotificationData data)
	{
		int id = data.GetID();
		if (id != ENotification.PLAYER_DIED &&
			id != ENotification.PLAYER_KILLED_PLAYER &&
			id != ENotification.AI_KILLED_PLAYER &&
			id != ENotification.POSSESSED_AI_DIED &&
			id != ENotification.POSSESSED_AI_KILLED_PLAYER &&
			id != ENotification.POSSESSED_AI_KILLED_POSSESSED_AI &&
			id != ENotification.AI_KILLED_POSSESSED_AI)
			return;

		// Force the display data to resolve the entity position into the notification data
		SCR_NotificationDisplayData displayData = data.GetDisplayData();
		if (displayData)
			displayData.SetPosition(data);

		vector pos;
		data.GetPosition(pos);
		if (pos != vector.Zero)
			m_vLastKillPosition = pos;
	}

	/**
	 * Teleports the spectator camera to the location of the most recent killfeed event.
	 * Mirrors the Zeus "R" shortcut for jumping to kill events.
	 * If currently following a player, detaches first so the free-cam teleport takes effect.
	 */
	void Action_TeleportToKill()
	{
		if (m_vLastKillPosition == vector.Zero)
			return;

		// Detach from any followed entity so the teleport takes effect
		if (m_eSpecEntity)
		{
			m_eSpecEntity = null;
			UnregisterFrameEvent();
		}

		MoveCamera(m_vLastKillPosition);
	}

	/**
	 * Teleports the camera to the position under the cursor
	 * Triggered by the manual camera teleport action
	 */
	void Action_ManualCameraTeleport()
	{
		vector worldPosition = GetCursorWorldPos();
		if (worldPosition != vector.Zero)
		{
			MoveCamera(worldPosition);
		}
	}
	
	/**
	 * Moves the camera to the specified world position
	 * 
	 * @param worldPosition - The target position in world coordinates
	 */
	void MoveCamera(vector worldPosition)
	{
		// Get the current camera
		SCR_ManualCamera camera = SCR_ManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (!camera)
			return;
		
		// Find the teleport component and use it to move the camera
		SCR_TeleportToCursorManualCameraComponent teleportComponent = SCR_TeleportToCursorManualCameraComponent.Cast(
			camera.FindCameraComponent(SCR_TeleportToCursorManualCameraComponent)
		);
		
		if (teleportComponent)
		{
			teleportComponent.TeleportCamera(worldPosition, true, false);
		}
	}
	
	/**
	 * Calculates the world position corresponding to the cursor position
	 * Handles both map cursor and 3D world cursor positions
	 * 
	 * @return The world position vector under the cursor
	 */
	vector GetCursorWorldPos()
	{
		ArmaReforgerScripted game = GetGame();
		WorkspaceWidget workspace = game.GetWorkspace();
		BaseWorld world = game.GetWorld();
		
		vector worldPos = vector.Zero;
		
		// If map is open, get position from map cursor
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (mapEntity && mapEntity.IsOpen())
		{
			float worldX, worldY;
			mapEntity.GetMapCursorWorldPosition(worldX, worldY);
			
			worldPos[0] = worldX;
			worldPos[2] = worldY;
			
			// Get the terrain height at this position
			if (world)
			{
				worldPos[1] = world.GetSurfaceY(worldPos[0], worldPos[2]);
			}
			
			return worldPos;
		}
		
		// If map is not open, perform a raycast from the camera through the cursor
		vector cursorPos = GetCursorPos();
		vector outDir;
		
		// Project from screen to world
		vector startPos = workspace.ProjScreenToWorld(cursorPos[0], cursorPos[1], outDir, world, -1);
		outDir *= 1000.0; // Extend ray to ensure it hits something
	
		// Set up trace parameters
		TraceParam trace = new TraceParam();
		trace.Start = startPos;
		trace.End = startPos + outDir;
		trace.Flags = TraceFlags.ANY_CONTACT | TraceFlags.WORLD | TraceFlags.ENTS | TraceFlags.OCEAN; 
		
		// Perform the trace
		float traceDis = 0;
		if (world)
		{
			traceDis = world.TraceMove(trace, null);
		}
		
		worldPos = startPos + outDir * traceDis;
		return worldPos;
	}
	
	/**
	 * Gets the current cursor position in screen coordinates
	 * 
	 * @return The cursor position vector (x, y, 0)
	 */
	vector GetCursorPos()
	{
		InputManager inputManager = GetGame().GetInputManager();
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		
		// For mouse and keyboard, return actual cursor position
		if (inputManager && inputManager.IsUsingMouseAndKeyboard())
		{
			int cursorX, cursorY;
			WidgetManager.GetMousePos(cursorX, cursorY);
			
			float scaledX = workspace.DPIUnscale(cursorX);
			float scaledY = workspace.DPIUnscale(cursorY);
			
			return Vector(scaledX, scaledY, 0);
		} 
		else 
		{
			// For gamepad or other input methods, use screen center
			float centerX = workspace.DPIUnscale(workspace.GetWidth() / 2.0);
			float centerY = workspace.DPIUnscale(workspace.GetHeight() / 2.0);
			
			return Vector(centerX, centerY, 0);
		}
	}
	
	//=================================================================================================
	// RADIO AND VOICE COMMUNICATION METHODS
	//=================================================================================================

	/**
	 * Retrieves the player's radio transceiver and configures it for voice communication
	 * @return The configured RadioTransceiver object
	 */
	RadioTransceiver GetVoNTransiver()
	{
		// Get local player entity
		IEntity playerEntity = SCR_PlayerController.GetLocalMainEntity();
		if (!playerEntity)
			return null;

		// Get all items in player's inventory
		ref array<IEntity> inventoryItems = {};
		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(
			playerEntity.FindComponent(SCR_InventoryStorageManagerComponent)
		);

		if (!inventoryManager)
			return null;

		inventoryManager.GetItems(inventoryItems);

		// Find the radio entity in inventory
		IEntity radioEntity = null;
		foreach (IEntity item : inventoryItems)
		{
			if (item && item.FindComponent(BaseRadioComponent))
			{
				radioEntity = item;
				break;
			}
		}

		if (!radioEntity)
			return null;

		// Get radio component and power it on
		BaseRadioComponent radioComponent = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		if (!radioComponent)
			return null;

		radioComponent.SetPower(true);

		// Get transceiver and set frequency based on channel
		RadioTransceiver transceiver = RadioTransceiver.Cast(radioComponent.GetTransceiver(0));
		if (!transceiver)
			return null;

		// Get the current player's channel with improved frequency calculation
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		int playerChannelId = COA_MenuManager.GetInstance().GetChannel(localPlayerId);

		// Calculate unique frequency for the channel to prevent conflicts
		// Use a base frequency of 10000 + (channelId * 1000) to ensure separation
		// This prevents frequency collisions between different channels
		float frequency = 10000.0 + (playerChannelId * 1000.0);

		// For custom channels (ID > 1), add additional offset based on channel name hash
		// This ensures each custom channel gets a truly unique frequency
		if (playerChannelId > 1 && m_MenuManager.m_aVONChannels.IsIndexValid(playerChannelId))
		{
			string channelData = m_MenuManager.m_aVONChannels[playerChannelId];
			ref array<string> channelParts = {};
			channelData.Split("|", channelParts, true);

			if (channelParts.Count() > 0)
			{
				string channelName = channelParts[0];
				// Use channel name hash to create unique frequency offset
				int nameHash = channelName.Hash();
				// Ensure positive hash and limit range to prevent frequency overlap
				int frequencyOffset = Math.AbsInt(nameHash) % 500;
				frequency += frequencyOffset;
			}
		}

		transceiver.SetFrequency(frequency);

		return transceiver;
	}

	/**
	 * Sets the power state of the player's radio
	 * @param input - true to power on, false to power off
	 */
	void SetRadioPower(bool input)
	{
		// Get local player entity
		IEntity playerEntity = SCR_PlayerController.GetLocalMainEntity();
		if (!playerEntity)
			return;

		// Get inventory manager
		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(
			playerEntity.FindComponent(SCR_InventoryStorageManagerComponent)
		);

		if (!inventoryManager)
			return;

		// Get all inventory items
		ref array<IEntity> inventoryItems = {};
		inventoryManager.GetItems(inventoryItems);

		// Find radio in inventory
		IEntity radioEntity = null;
		foreach (IEntity item : inventoryItems)
		{
			if (item && item.FindComponent(BaseRadioComponent))
			{
				radioEntity = item;
				break;
			}
		}

		if (!radioEntity)
			return;

		// Set radio power state
		BaseRadioComponent radioComponent = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		if (radioComponent)
		{
			radioComponent.SetPower(input);
		}
	}

	/**
	 * Activates voice transmission when PTT key is pressed
	 * Connects to the appropriate radio channel
	 */
	void Action_VONon()
	{
		// Check if player is in a valid channel
		int playerChannel = COA_MenuManager.GetInstance().GetChannel(SCR_PlayerController.GetLocalPlayerId());
		if (playerChannel == 0)
			return;

		// Cancel any pending VoN disable calls
		GetGame().GetCallqueue().Remove(LobbyVoNDisableDelayed);

		// Get VoN component from player entity
		IEntity playerEntity = SCR_PlayerController.GetLocalMainEntity();
		if (!playerEntity)
			return;

		SCR_VoNComponent vonComponent = SCR_VoNComponent.Cast(playerEntity.FindComponent(SCR_VoNComponent));
		if (!vonComponent)
			return;

		// Configure and activate voice transmission
		// Get fresh transceiver with updated frequency each time VON is activated
		RadioTransceiver transceiver = GetVoNTransiver();
		if (!transceiver)
			return;

		vonComponent.SetTransmitRadio(transceiver);
		vonComponent.SetCommMethod(ECommMethod.SQUAD_RADIO);
		vonComponent.SetCapture(true);
	}

	/**
	 * Deactivates voice transmission when PTT key is released
	 * Uses a delay to prevent audio cutoff
	 */
	void Action_VONOff()
	{
		// Check if player is in a valid channel
		int playerChannel = COA_MenuManager.GetInstance().GetChannel(SCR_PlayerController.GetLocalPlayerId());
		if (playerChannel == 0)
			return;

		// Schedule delayed VoN deactivation to prevent audio cutoff
		GetGame().GetCallqueue().Call(LobbyVoNDisableDelayed);
	}

	/**
	 * Delayed method to disable voice transmission
	 * Used to prevent audio cutoff when releasing PTT key
	 */
	void LobbyVoNDisableDelayed()
	{
		// Get VoN component from player entity
		IEntity playerEntity = SCR_PlayerController.GetLocalMainEntity();
		if (!playerEntity)
			return;

		SCR_VoNComponent vonComponent = SCR_VoNComponent.Cast(playerEntity.FindComponent(SCR_VoNComponent));
		if (!vonComponent)
			return;

		// Reset communication method and stop capturing
		vonComponent.SetCommMethod(ECommMethod.DIRECT);
		vonComponent.SetCapture(false);
	}
	
	//=================================================================================================
	// UI UPDATE METHODS
	//=================================================================================================
	
	/**
	 * Updates all spectator icons on the screen
	 * Refreshes position, visibility, and status of player markers
	 */
	void UpdateIcons()
	{
		if (!m_aSpectatorIcons)
			return;
			
		// Update each spectator icon
		foreach (COA_SpectatorLabelIconCharacter spectatorIcon : m_aSpectatorIcons)
		{
			if (spectatorIcon)
			{
				spectatorIcon.Update();
			}
		}
		
		// Update each group icon
		if (m_aGroupIcons)
		{
			foreach (COA_SpectatorLabelIconGroup groupIcon : m_aGroupIcons)
			{
				if (groupIcon)
				{
					groupIcon.Update();
				}
			}
		}
	}
	
	/**
	 * Handles the chat toggle action
	 * Opens the chat panel with a small delay to prevent UI issues
	 */
	void Action_OnCOA_ChatToggleAction()
	{
		if (!m_ChatPanel)
			return;
		
		// Use a small delay to ensure UI is ready
		GetGame().GetCallqueue().Call(OpenChatWrap);
	}
	
	/**
	 * Opens the chat panel if it's currently closed
	 */
	void OpenChatWrap()
	{
		if (!m_ChatPanel)
			return;
			
		if (!m_ChatPanel.IsOpen())
		{
			SCR_ChatPanelManager chatManager = SCR_ChatPanelManager.GetInstance();
			if (chatManager)
			{
				chatManager.OpenChatPanel(m_ChatPanel);
			}
		}
	}
	
	/**
	 * Handles the exit action
	 * Opens pause menu instead of directly exiting game to prevent accidental exits
	 */
	void Action_Exit()
	{
		// Note: Opening pause menu instead of directly exiting the game
		// because players often accidentally exit the game
		GetGame().GetCallqueue().Call(OpenPauseMenuWrap);
	}
	
	/**
	 * Opens the pause menu
	 */
	void OpenPauseMenuWrap()
	{
		ArmaReforgerScripted.OpenPauseMenu();
	}
	
	//=================================================================================================
	// MAP CONTROL METHODS
	//=================================================================================================
	
	/**
	 * Toggles the map display on/off
	 */
	void Action_ToggleMap()
	{
		if (!m_MapEntity)
			return;
			
		if (!m_MapEntity.IsOpen())
		{
			OpenMap();
		}
		else
		{
			CloseMap();
		}
	}
	
	/**
	 * Opens the map and configures it for display
	 * Disables camera input while the map is open
	 */
	void OpenMap()
	{
		// Disable camera input while map is open
		SCR_ManualCamera camera = SCR_ManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
		{
			camera.SetInputEnabled(false);
		}
		
		// Show map frame
		if (m_wMapFrame)
		{
			m_wMapFrame.SetVisible(true);
		}
		
		// Clear any tool menu widgets
		Widget toolMenu = GetRootWidget().FindAnyWidget("ToolMenuHoriz");
		if (toolMenu)
		{
			SCR_WidgetHelper.RemoveAllChildren(toolMenu);
		}
		
		// Get map configuration from game mode
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;
		
		SCR_MapConfigComponent configComponent = SCR_MapConfigComponent.Cast(
			gameMode.FindComponent(SCR_MapConfigComponent)
		);
		
		if (!configComponent || !m_MapEntity)
			return;
		
		// Configure and open the map
		MapConfiguration mapConfig = m_MapEntity.SetupMapConfig(
			EMapEntityMode.FULLSCREEN, 
			configComponent.GetEditorMapConfig(), 
			m_wMapFrame
		);
		
		m_MapEntity.OpenMap(mapConfig);
	}
	
	/**
	 * Closes the map and re-enables camera input
	 */
	void CloseMap()
	{
		// Re-enable camera input
		SCR_ManualCamera camera = SCR_ManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
		{
			camera.SetInputEnabled(true);
		}
		
		// Hide map frame
		if (m_wMapFrame)
		{
			m_wMapFrame.SetVisible(false);
		}
		
		// Close the map
		if (m_MapEntity)
		{
			m_MapEntity.CloseMap();
		}
	}
	
	//-------------------------------------------------------------------------
	// Timer Update - Called every second
	//-------------------------------------------------------------------------
	void UpdateTimer()
	{	
		// Get current mission time
		m_sServerWorldTime = COA_GameTimerManager.GetInstance().GetServerWorldTime();
		
		// Skip update if in safestart, time is empty, or hasn't changed
		if (m_sServerWorldTime == "N/A" ||
			m_SafestartManager.GetSafestartStatus() || 
			m_sServerWorldTime.IsEmpty() || 
			m_sStoredServerWorldTime == m_sServerWorldTime) 
		{
			return;
		}
		
		// Store time for comparison in next update
		m_sStoredServerWorldTime = m_sServerWorldTime;
		
		// Handle time warnings (15min, 5min, end)
		HandleTimeWarnings();
		
		// Format and display time remaining
		UpdateTimeDisplay();
	}
	
	//-------------------------------------------------------------------------
	// Helper Methods
	//-------------------------------------------------------------------------
	
	/**
	* Handles time warnings at specific thresholds
	*/
	protected void HandleTimeWarnings()
	{
		// Play sound and show notification at specific time thresholds
		if (m_sServerWorldTime == "00:15:00" || 
			m_sServerWorldTime == "00:05:00" || 
			m_sServerWorldTime == "Mission Time Expired!") 
		{
			// Play warning sound
			AudioSystem.PlaySound("{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav");
			
			// Show appropriate message based on time
			if (m_sServerWorldTime == "00:15:00") 
			{
				m_PopUpNotification.PopupMsg("Mission Ends In 15 Minutes!", 10);
			}
			else if (m_sServerWorldTime == "00:05:00") 
			{
				m_PopUpNotification.PopupMsg("Mission Ends In 5 Minutes!", 10);
			}
			else if (m_sServerWorldTime == "Mission Time Expired!") 
			{
				GetGame().GetCallqueue().Remove(UpdateTimer);
				m_PopUpNotification.PopupMsg(m_sServerWorldTime, 10);
				m_wTimer.SetText(m_sServerWorldTime);
				return;
			}
		}
	}
	
	/**
	* Updates the time display including formatting and visibility
	*/
	protected void UpdateTimeDisplay()
	{
		// Split time string into components
		array<string> timeParts = {};
		m_sServerWorldTime.Split(":", timeParts, false);
		if (timeParts.Count() < 3 || !m_wTimer)
			return;
		
		// Format time display (drop the hour part if it's 00)
		string displayTime = m_sServerWorldTime;
		if (timeParts[0] == "00")
		{
			displayTime = string.Format("%1:%2", timeParts[1], timeParts[2]);
		}
		
		m_wTimer.SetText("Mission End: " + displayTime);
		
		// Set color based on time remaining
		if (timeParts[0] == "00" && timeParts[1].ToInt() < 5)
		{
			// Less than 5 minutes - red
			m_wTimer.SetColorInt(ARGB(255, 200, 65, 65));
		}
		else if (timeParts[0] == "00" && timeParts[1].ToInt() < 15)
		{
			// Less than 15 minutes - yellow
			m_wTimer.SetColorInt(ARGB(255, 230, 230, 0));
		}
		else
		{
			// Normal - light gray
			m_wTimer.SetColorInt(ARGB(255, 215, 215, 215));
		}
	}
}
