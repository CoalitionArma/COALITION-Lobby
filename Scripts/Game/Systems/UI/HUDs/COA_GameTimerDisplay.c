
class COA_GameTimerDisplay : SCR_InfoDisplayExtended
{
	//-------------------------------------------------------------------------
	// Widget References
	//-------------------------------------------------------------------------
	// Main timer elements
	protected SCR_MapEntity m_MapEntity;
	protected TextWidget m_wTimer;
	protected ImageWidget m_wBackground;
	protected TextWidget m_wTimerLabel;		// "MISSION END" caption left of the time
	protected ImageWidget m_wTimerAccent;	// state bar: brand red, amber under 15 min, bright red under 5
	
	// Ticket display for faction one (BLUFOR)
	protected ImageWidget m_wTicketOneImage; // Missing semicolon fixed
	protected TextWidget m_wTicketOneText; // Missing semicolon fixed
	protected TextWidget m_wTicketOneNumber; // Missing semicolon fixed
	protected ImageWidget m_wTicketOneBackground; // Missing semicolon fixed
	
	// Ticket display for faction two (OPFOR)
	protected ImageWidget m_wTicketTwoImage; // Missing semicolon fixed
	protected TextWidget m_wTicketTwoText; // Missing semicolon fixed
	protected TextWidget m_wTicketTwoNumber; // Missing semicolon fixed
	protected ImageWidget m_wTicketTwoBackground; // Missing semicolon fixed
	
	// Ticket display for faction three (INDFOR)
	protected ImageWidget m_wTicketThreeImage; // Missing semicolon fixed
	protected TextWidget m_wTicketThreeText; // Missing semicolon fixed
	protected TextWidget m_wTicketThreeNumber; // Missing semicolon fixed
	protected ImageWidget m_wTicketThreeBackground; // Missing semicolon fixed
	
	// Ticket display for faction four (CIV)
	protected ImageWidget m_wTicketFourImage; // Missing semicolon fixed
	protected TextWidget m_wTicketFourText; // Missing semicolon fixed
	protected TextWidget m_wTicketFourNumber; // Missing semicolon fixed
	protected ImageWidget m_wTicketFourBackground; // Missing semicolon fixed

	// Game state references
	protected COA_SafestartManager m_SafestartManager;
	protected string m_sStoredServerWorldTime;
	protected string m_sServerWorldTime;
	protected SCR_PopUpNotification m_PopUpNotification = null;
	
	protected bool m_bUpdateTimer = false;

	// Ticket rows live in a drawer tucked into the right edge behind a "TICKETS" tab; hovering it
	// slides it out (COA_HoverDrawer, like the briefing player list). See COA_GameTimerDisplay.layout.
	protected static const float TICKET_DRAWER_OPEN_X = -280;		// -(screen margin 12 + drawer width 268)
	protected static const float TICKET_DRAWER_CLOSED_X = -44;		// only the 44 px tab on screen
	protected static const float TICKET_DRAWER_BOTTOM = -50;		// just above the mission timer row
	protected static const float TICKET_ROW_PITCH = 38;
	protected static const float TICKET_TAB_MIN_HEIGHT = 72;
	protected Widget m_wTicketDrawer;
	protected TextWidget m_wTicketTabArrow;
	protected ref COA_HoverDrawer m_TicketDrawer;
	
	//-------------------------------------------------------------------------
	// Initialization
	//-------------------------------------------------------------------------
	override protected void DisplayInit(IEntity owner)
	{
		super.DisplayInit(owner);
		// We really dont want this running on the server
		if (RplSession.Mode() == RplMode.Dedicated)
			return;

		// Set up periodic timer update every second
		m_bUpdateTimer = true;

		// Get notification system reference
		m_PopUpNotification = SCR_PopUpNotification.GetInstance();
	}
	
	/**
	 * Updates the game timer display UI elements.
	 * Called on each frame update to refresh the timer visualization.
	 * 
	 * @param owner The entity that owns this display component
	 * @param timeSlice The time elapsed since the last update in seconds
	 * @override Overrides the base class implementation to provide game timer specific display logic
	 */
	float m_fUpdateBuffer = 0;
	override protected void DisplayUpdate(IEntity owner, float timeSlice)
	{
		super.DisplayUpdate(owner, timeSlice);

		// Slide the ticket drawer every frame while it's shown
		if (m_TicketDrawer && m_wTicketDrawer && m_wTicketDrawer.IsVisible() && m_wTicketDrawer.GetOpacity() > 0)
			m_TicketDrawer.Update(timeSlice);
		
		// Only fire if in-game
		if (!GetGame().GetWorld().GetWorldTime() || !SCR_PlayerController.GetLocalControlledEntity())
			return;
		
		if (m_fUpdateBuffer >= 1)
		{
			if (m_bUpdateTimer)
				UpdateTimer();
			m_fUpdateBuffer = 0;
		}
		m_fUpdateBuffer += timeSlice;
		
		// Initialize references if they don't exist
		// This handles respawn support and first-time initialization
		// Check if we're in-game and already initialized
		if (!m_SafestartManager || !m_wTimer || !m_wBackground || !m_MapEntity) 
		{
			// Get game system references
			m_SafestartManager = COA_SafestartManager.GetInstance();
			m_MapEntity = SCR_MapEntity.GetMapInstance();
			
			// Find and cast main timer widgets
			m_wTimer = TextWidget.Cast(m_wRoot.FindAnyWidget("timeLeftTimer"));
			m_wBackground = ImageWidget.Cast(m_wRoot.FindAnyWidget("timeLeftBackground"));
			m_wTimerLabel = TextWidget.Cast(m_wRoot.FindAnyWidget("timeLeftLabel"));
			m_wTimerAccent = ImageWidget.Cast(m_wRoot.FindAnyWidget("timeLeftAccent"));
			
			// Find and cast faction one ticket widgets
			m_wTicketOneImage = ImageWidget.Cast(m_wRoot.FindAnyWidget("TicketOneImage"));
			m_wTicketOneText = TextWidget.Cast(m_wRoot.FindAnyWidget("TicketOneText"));
			m_wTicketOneNumber = TextWidget.Cast(m_wRoot.FindAnyWidget("TicketOneNumber"));
			m_wTicketOneBackground = ImageWidget.Cast(m_wRoot.FindAnyWidget("TicketOneBackground"));
			
			// Find and cast faction two ticket widgets
			m_wTicketTwoImage = ImageWidget.Cast(m_wRoot.FindAnyWidget("TicketTwoImage"));
			m_wTicketTwoText = TextWidget.Cast(m_wRoot.FindAnyWidget("TicketTwoText"));
			m_wTicketTwoNumber = TextWidget.Cast(m_wRoot.FindAnyWidget("TicketTwoNumber"));
			m_wTicketTwoBackground = ImageWidget.Cast(m_wRoot.FindAnyWidget("TicketTwoBackground"));
			
			// Find and cast faction three ticket widgets
			m_wTicketThreeImage = ImageWidget.Cast(m_wRoot.FindAnyWidget("TicketThreeImage"));
			m_wTicketThreeText = TextWidget.Cast(m_wRoot.FindAnyWidget("TicketThreeText"));
			m_wTicketThreeNumber = TextWidget.Cast(m_wRoot.FindAnyWidget("TicketThreeNumber"));
			m_wTicketThreeBackground = ImageWidget.Cast(m_wRoot.FindAnyWidget("TicketThreeBackground"));
			
			// Find and cast faction four ticket widgets
			m_wTicketFourImage = ImageWidget.Cast(m_wRoot.FindAnyWidget("TicketFourImage"));
			m_wTicketFourText = TextWidget.Cast(m_wRoot.FindAnyWidget("TicketFourText"));
			m_wTicketFourNumber = TextWidget.Cast(m_wRoot.FindAnyWidget("TicketFourNumber"));
			m_wTicketFourBackground = ImageWidget.Cast(m_wRoot.FindAnyWidget("TicketFourBackground"));

			// Hover drawer holding the ticket rows
			m_wTicketDrawer = m_wRoot.FindAnyWidget("TicketDrawer");
			m_wTicketTabArrow = TextWidget.Cast(m_wRoot.FindAnyWidget("TicketTabArrow"));
			if (m_wTicketDrawer && !m_TicketDrawer)
			{
				m_TicketDrawer = new COA_HoverDrawer(m_wTicketDrawer, TICKET_DRAWER_CLOSED_X, TICKET_DRAWER_OPEN_X);
				m_TicketDrawer.m_OnOpenChanged.Insert(OnTicketDrawerChanged);
			}
			
			return;
		}
	}
	
	//-------------------------------------------------------------------------
	// Timer Update - Called every second
	//-------------------------------------------------------------------------
	void UpdateTimer()
	{	
		// Skip update if essential components are missing or player is spectating
		if (!m_SafestartManager || !m_wTimer || !m_wBackground || !m_MapEntity || 
			!SCR_PlayerController.GetLocalControlledEntity() || 
			COA_EntityHelper.IsSpectator()) 
		{
			return;
		}
		
		// Handle HUD visibility toggle
		if(!COA_PlayerControllerManager.GetInstance().m_bHUDVisible)
		{
			// Hide all timer elements when HUD is disabled
			SetTimerVisibility(false);
			return;
		} 
		else 
		{
			// Show timer elements when HUD is enabled
			SetTimerVisibility(true);
		}
		
		// Handle ticket display when game is active and safestart is disabled
		if (COA_Gamemode.GetInstance().m_GamemodeState == COA_EGamemodeState.GAME && 
			!COA_SafestartManager.GetInstance().GetSafestartStatus())
		{
			UpdateTicketDisplay();
		}
		
		// Get current mission time
		m_sServerWorldTime = COA_GameTimerManager.GetInstance().GetServerWorldTime();
		
		// Handle invalid time or end of mission
		if (m_sServerWorldTime == "N/A") 
		{
			m_bUpdateTimer = false;
			return;
		}
		
		// Skip update if in safestart, time is empty, or hasn't changed
		if (m_SafestartManager.GetSafestartStatus() || 
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
	* Sets visibility of timer and ticket UI elements
	* @param isVisible - whether elements should be visible
	*/
	protected void SetTimerVisibility(bool isVisible)
	{
		// Set main timer visibility
		m_wTimer.SetVisible(isVisible);
		m_wBackground.SetVisible(isVisible);
		if (m_wTicketDrawer)
			m_wTicketDrawer.SetVisible(isVisible);
		if (m_wTimerLabel)
			m_wTimerLabel.SetVisible(isVisible);
		if (m_wTimerAccent)
			m_wTimerAccent.SetVisible(isVisible);
		
		// Set ticket one visibility
		m_wTicketOneImage.SetVisible(isVisible);
		m_wTicketOneText.SetVisible(isVisible);
		m_wTicketOneNumber.SetVisible(isVisible);
		m_wTicketOneBackground.SetVisible(isVisible);
	}
	
	/**
	* Show/hide the ticket drawer and fit it to the rows it holds (its rows sit at its bottom)
	*/
	protected void ShowTicketDrawer(bool show, bool allFactions)
	{
		if (!m_wTicketDrawer)
			return;

		if (!show)
		{
			m_wTicketDrawer.SetOpacity(0);
			return;
		}

		int rows = 1;
		if (allFactions)
			rows = 4;

		float height = Math.Max(rows * TICKET_ROW_PITCH - 4, TICKET_TAB_MIN_HEIGHT);
		FrameSlot.SetSizeY(m_wTicketDrawer, height);
		FrameSlot.SetPosY(m_wTicketDrawer, TICKET_DRAWER_BOTTOM - height);
		m_wTicketDrawer.SetOpacity(1);
	}

	/**
	* Tab arrow points the way the drawer will move
	*/
	protected void OnTicketDrawerChanged(bool open)
	{
		if (!m_wTicketTabArrow)
			return;

		if (open)
			m_wTicketTabArrow.SetText("›");
		else
			m_wTicketTabArrow.SetText("‹");
	}

	/**
	* Updates the ticket display based on player faction or admin status
	*/
	protected void UpdateTicketDisplay()
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return;
		
		// Different display for admins vs regular players
		if (!SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId()))
		{
			// For regular players - only show their faction's tickets
			string faction = SCR_FactionManager.SGetLocalPlayerFaction().GetFactionKey();
			
			// Skip ticket display for spectators
			if (faction != "SPEC") 
			{
				// Faction colour on the accent bar and the number; the label stays neutral
				StyleTicketRow(m_wTicketOneImage, m_wTicketOneText, m_wTicketOneNumber, factionManager.GetFactionByKey(faction), "TICKETS LEFT");
				
				// Display appropriate ticket count based on faction
				UpdateFactionTickets(faction, m_wTicketOneNumber);
			}
		} else {
			// For admins - show all factions' tickets
			
			// BLUFOR tickets (position one)
			StyleTicketRow(m_wTicketOneImage, m_wTicketOneText, m_wTicketOneNumber, factionManager.GetFactionByKey("BLUFOR"), "BLUFOR TICKETS");
			UpdateFactionTickets("BLUFOR", m_wTicketOneNumber);
			
			// OPFOR tickets (position two)
			StyleTicketRow(m_wTicketTwoImage, m_wTicketTwoText, m_wTicketTwoNumber, factionManager.GetFactionByKey("OPFOR"), "OPFOR TICKETS");
			UpdateFactionTickets("OPFOR", m_wTicketTwoNumber);
			
			// INDFOR tickets (position three)
			StyleTicketRow(m_wTicketThreeImage, m_wTicketThreeText, m_wTicketThreeNumber, factionManager.GetFactionByKey("INDFOR"), "INDFOR TICKETS");
			UpdateFactionTickets("INDFOR", m_wTicketThreeNumber);
			
			// CIV tickets (position four)
			StyleTicketRow(m_wTicketFourImage, m_wTicketFourText, m_wTicketFourNumber, factionManager.GetFactionByKey("CIV"), "CIV TICKETS");
			UpdateFactionTickets("CIV", m_wTicketFourNumber);
		}
	}
	
	/**
	* Colours a ticket row: faction colour on the accent bar and number, neutral caption
	*/
	protected void StyleTicketRow(ImageWidget accent, TextWidget label, TextWidget number, Faction faction, string caption)
	{
		if (!faction)
			return;

		Color factionColor = faction.GetFactionColor();
		if (accent)
			accent.SetColor(factionColor);
		if (number)
			number.SetColor(factionColor);
		if (label)
			label.SetText(caption);
	}

	/**
	* Updates ticket display for a specific faction
	* @param faction - faction key ("BLUFOR", "OPFOR", "INDFOR", "CIV")
	* @param ticketWidget - the text widget to update
	*/
	protected void UpdateFactionTickets(string faction, TextWidget ticketWidget)
	{
		int tickets = -1;
		
		// Get ticket count from respawn manager
		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		if (respawnManager)
			tickets = respawnManager.GetFactionTickets(faction);
		
		// Display "INF" for infinite tickets (-1) or the actual count
		if (tickets == -1)
			ticketWidget.SetText("INF");
		else
			ticketWidget.SetText(tickets.ToString());
	}
	
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
				m_bUpdateTimer = false;
				m_PopUpNotification.PopupMsg(m_sServerWorldTime, 10);
				m_wTimer.SetText("EXPIRED");
				m_wTimer.SetColor(Color.FromSRGBA(232, 96, 96, 255));
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
		
		// Determine visibility based on time remaining and map status
		bool shouldHideDisplay = (m_SafestartManager.GetSafestartStatus() || 
								((timeParts[0] != "00" || timeParts[1].ToInt() >= 5) && 
								(!m_MapEntity || !m_MapEntity.IsOpen())));
		
		// Set opacity based on visibility requirement
		if (shouldHideDisplay) 
		{
			SetWidgetOpacity(0);
			return;
		}
		else 
		{
			// Show timer
			m_wTimer.SetOpacity(1);
			m_wBackground.SetOpacity(1);
			if (m_wTimerLabel)
				m_wTimerLabel.SetOpacity(1);
			if (m_wTimerAccent)
				m_wTimerAccent.SetOpacity(1);
			
			// Determine if tickets should be shown
			COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
			bool hasAnyTickets = false;
			if (respawnManager)
			{
				hasAnyTickets = (respawnManager.GetFactionTickets("BLUFOR") > -1 || 
								respawnManager.GetFactionTickets("OPFOR") > -1 || 
								respawnManager.GetFactionTickets("INDFOR") > -1 || 
								respawnManager.GetFactionTickets("CIV") > -1);
			}
			
			ShowTicketDrawer(hasAnyTickets, SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId()));

			if (hasAnyTickets)
			{
				// Show player faction tickets
				m_wTicketOneImage.SetOpacity(1);
				m_wTicketOneText.SetOpacity(1);
				m_wTicketOneNumber.SetOpacity(1);
				m_wTicketOneBackground.SetOpacity(1);
				
				// For admins, show all faction tickets
				if (SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId()))
				{
					m_wTicketTwoImage.SetOpacity(1);
					m_wTicketTwoText.SetOpacity(1);
					m_wTicketTwoNumber.SetOpacity(1);
					m_wTicketTwoBackground.SetOpacity(1);
					
					m_wTicketThreeImage.SetOpacity(1);
					m_wTicketThreeText.SetOpacity(1);
					m_wTicketThreeNumber.SetOpacity(1);
					m_wTicketThreeBackground.SetOpacity(1);				
					
					m_wTicketFourImage.SetOpacity(1);
					m_wTicketFourText.SetOpacity(1);
					m_wTicketFourNumber.SetOpacity(1);
					m_wTicketFourBackground.SetOpacity(1);
				}
			}
		}
		
		// Format time display (drop the hour part if it's 00)
		string displayTime = m_sServerWorldTime;
		if (timeParts[0] == "00")
		{
			displayTime = string.Format("%1:%2", timeParts[1], timeParts[2]);
		}
		
		m_wTimer.SetText(displayTime);
		
		// Value and accent bar follow the time remaining
		Color valueColor = Color.FromSRGBA(239, 242, 247, 255);	// normal
		Color accentColor = Color.FromSRGBA(201, 54, 54, 255);
		if (timeParts[0] == "00" && timeParts[1].ToInt() < 5)
		{
			valueColor = Color.FromSRGBA(232, 96, 96, 255);		// under 5 minutes
			accentColor = Color.FromSRGBA(232, 96, 96, 255);
		}
		else if (timeParts[0] == "00" && timeParts[1].ToInt() < 15)
		{
			valueColor = Color.FromSRGBA(232, 170, 72, 255);	// under 15 minutes
			accentColor = Color.FromSRGBA(232, 170, 72, 255);
		}

		m_wTimer.SetColor(valueColor);
		if (m_wTimerAccent)
			m_wTimerAccent.SetColor(accentColor);
	}
	
	/**
	* Sets opacity of all widgets to the specified value
	* @param opacity - opacity value (0-1)
	*/
	protected void SetWidgetOpacity(float opacity)
	{
		// Main timer elements
		m_wTimer.SetOpacity(opacity);
		m_wBackground.SetOpacity(opacity);
		if (m_wTimerLabel)
			m_wTimerLabel.SetOpacity(opacity);
		if (m_wTimerAccent)
			m_wTimerAccent.SetOpacity(opacity);
		
		if (m_wTicketDrawer)
			m_wTicketDrawer.SetOpacity(opacity);

		// Faction one ticket elements
		m_wTicketOneImage.SetOpacity(opacity);
		m_wTicketOneText.SetOpacity(opacity);
		m_wTicketOneNumber.SetOpacity(opacity);
		m_wTicketOneBackground.SetOpacity(opacity);
		
		// Faction two ticket elements
		m_wTicketTwoImage.SetOpacity(opacity);
		m_wTicketTwoText.SetOpacity(opacity);
		m_wTicketTwoNumber.SetOpacity(opacity);
		m_wTicketTwoBackground.SetOpacity(opacity);
		
		// Faction three ticket elements
		m_wTicketThreeImage.SetOpacity(opacity);
		m_wTicketThreeText.SetOpacity(opacity);
		m_wTicketThreeNumber.SetOpacity(opacity);
		m_wTicketThreeBackground.SetOpacity(opacity);
		
		// Faction four ticket elements
		m_wTicketFourImage.SetOpacity(opacity);
		m_wTicketFourText.SetOpacity(opacity);
		m_wTicketFourNumber.SetOpacity(opacity);
		m_wTicketFourBackground.SetOpacity(opacity);
	}
}

