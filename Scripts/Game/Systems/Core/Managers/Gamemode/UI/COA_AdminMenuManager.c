class COA_AdminMenuManagerClass : ScriptComponentClass {}

class COA_TicketMessageData
{
	string sender;
	string msg;
	string timestamp;
	
	//------------------------------------------------------------------------------------------------
	// REPLICATION STUFF
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	static bool Extract(COA_TicketMessageData instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{	
		snapshot.SerializeString(instance.sender);
		snapshot.SerializeString(instance.msg);
		snapshot.SerializeString(instance.timestamp);
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, COA_TicketMessageData instance)
	{
		snapshot.SerializeString(instance.sender);
		snapshot.SerializeString(instance.msg);
		snapshot.SerializeString(instance.timestamp);
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.EncodeString(packet); // sender
		snapshot.EncodeString(packet); // msg
		snapshot.EncodeString(packet); // timestamp
	}
	
	//------------------------------------------------------------------------------------------------
	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.DecodeString(packet); // sender
		snapshot.DecodeString(packet); // msg
		snapshot.DecodeString(packet); // timestamp
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx)
	{
		if (!lhs.CompareStringSnapshots(rhs)) return false;  // sender
	    if (!lhs.CompareStringSnapshots(rhs)) return false;  // msg
	    if (!lhs.CompareStringSnapshots(rhs)) return false;  // timestamp
	    return true;
	}
	
	//------------------------------------------------------------------------------------------------
	static bool PropCompare(COA_TicketMessageData instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		return snapshot.CompareString(instance.sender)
		&& snapshot.CompareString(instance.msg)
		&& snapshot.CompareString(instance.timestamp);
	}
}

class COA_AdminActionLog
{
	string timestamp;
	string action; // Description of the action
	COA_EAdminLogLevel level;
}

class COA_Ticket
{
	int ticketID; // ID of the player who requested help
	int adminID; // Admin assigned to the ticket
	ref array<ref COA_TicketMessageData> messages = new array<ref COA_TicketMessageData>(); // Array of messages from the player since they asked for help
	
	//------------------------------------------------------------------------------------------------
	void AddMessage(string sender, string msg)
	{	
		COA_TicketMessageData message = new COA_TicketMessageData;
		message.sender = sender;
		message.msg = msg;
		message.timestamp = COA_AdminMenuManager.GetFormattedTimestamp();
		messages.Insert(message);
	}
}

class COA_AdminMenuManager : ScriptComponent
{	
	// Map of player tickets
	private ref map<int, ref COA_Ticket> m_mTickets = new map<int, ref COA_Ticket>();
	
	// Array of admin actions
	private ref array<ref COA_AdminActionLog> m_mAdminActions = new array<ref COA_AdminActionLog>();
	
	//------------------------------------------------------------------------------------------------
	//! Creates a formatted timestamp string from current system time
	//! \return Formatted timestamp in HH:MM:SS format
	static string GetFormattedTimestamp()
	{
		int hour = 0;
		int minute = 0;
		int second = 0;
		
		// Get local time
		System.GetHourMinuteSecond(hour, minute, second);
		
		// Format with leading zeros
		return string.Format("%1:%2:%3", hour.ToString(2), minute.ToString(2), second.ToString(2));
	}
	
	//------------------------------------------------------------------------------------------------
	//! Stores admin logs for UI
	//! \param[in] data Description of the action
	void StoreAdminLogs(string data, COA_EAdminLogLevel level)
	{
		// Create new log
		COA_AdminActionLog log = new COA_AdminActionLog();
		log.action = data;
		log.timestamp = GetFormattedTimestamp();
		log.level = level;
		m_mAdminActions.Insert(log);
		
		// Refresh Lists if admin menu is open
		COA_RplBroadcastManager.GetInstance().RefreshAdminMenuLists();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Returns list of actions taken by admins this mission
	//! \return Array of admin action logs
	array<ref COA_AdminActionLog> GetAdminActionLogs()
	{
		return m_mAdminActions;
	}

	//------------------------------------------------------------------------------------------------
	//! Creates new ticket or adds new message to existing ticket
	//! \param[in] ticketID ID of the ticket (ID of the player that initially opened the ticket)
	//! \param[in] senderID ID of the player sending the message
	//! \param[in] data Text in the message
	void NewTicketMessage(int ticketID, int senderID, string data)
	{
		// Get name of player sending the admin message
		string sender = GetGame().GetPlayerManager().GetPlayerName(senderID);
		if (!sender)
			return;
		
		COA_Ticket ticket;
		
		// Check if player already has ticket open
		if (!m_mTickets.Contains(ticketID))
		{
			// Creates new ticket
			ticket = new COA_Ticket();
			ticket.ticketID = ticketID;
			ticket.messages = {};
			m_mTickets.Set(ticketID, ticket);
		}
		else 
		{
			// Grab the existing ticket
			m_mTickets.Find(ticketID, ticket);
		}
		
		// Add the new message to the ticket
		ticket.AddMessage(sender, data);
		
		// Refresh Lists if admin menu is open
		COA_RplBroadcastManager.GetInstance().RefreshAdminMenuLists();
	}
		
	//------------------------------------------------------------------------------------------------
	//! Keeps track of the admin that helping with this ticket
	//! \param[in] ticketID ID of the ticket (ID of the player that initially opened the ticket)
	//! \param[in] admin ID of the admin that will be helping the player
	void AssignAdminTicket(int ticketID, int adminID)
	{
		COA_Ticket ticket;
		
		// Check if player already has ticket open
		if (!m_mTickets.Contains(ticketID))
			return;

		m_mTickets.Find(ticketID, ticket);
		
		// Check if admin is already assigned
		if (ticket.adminID)
			return;
		
		// Assign the admin to the ticket
		ticket.adminID = adminID;
		
		// Refresh Lists if admin menu is open
		COA_RplBroadcastManager.GetInstance().NotifiyTicketAssigned(ticketID, adminID, true);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Returns a list of current open tickets
	//! \return Array of player IDs with open tickets
	array<int> GetOpenTickets()
	{
		array<int> ticketIDs = {};
		
		// Gather list of playerIDs for tickets that are open
		foreach (int id, ref COA_Ticket ticket : m_mTickets)
		{
			ticketIDs.Insert(ticket.ticketID);
		}
		
		return ticketIDs;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Returns list of messages from a ticket
	//! \param[in] playerID ID of the player whose ticket messages to retrieve
	//! \return Array of ticket messages or null if no ticket found
	array<ref COA_TicketMessageData> GetTicketMessages(int playerID)
	{
		COA_Ticket ticket;
		
		// Get the messages from the player's ID
		if (m_mTickets.Find(playerID, ticket))
			return ticket.messages;
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Close a ticket
	//! \param[in] ticketID ID of the ticket to close
	void CloseTicket(int ticketID)
	{		
		// Remove the ticket from the map
		m_mTickets.Remove(ticketID);
		
		// Refresh Lists if admin menu is open
		COA_RplBroadcastManager.GetInstance().RefreshAdminMenuLists();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Refreshes the lists on new message and new log being added
	void RefreshLists()
	{		
		// Check if the top menu is the admin menu
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (!topMenu)
			return;
			
		if (!topMenu.IsInherited(COA_AdminMenu))
			return;
			
		// Repopulate menu components
		COA_AdminMenu adminMenu = COA_AdminMenu.Cast(topMenu);
		if (adminMenu.GetCurrentOpenTab() == "Tickets")
		{
			adminMenu.GetTicketMessages();
			adminMenu.GetOpenTickets();
		}

		adminMenu.PopulateAdminActionsList();
	}

	//------------------------------------------------------------------------------------------------
	//! Check if a ticket exists for the given ID
	//! \param[in] ticketID ID of the ticket to check
	//! \return True if ticket exists, false otherwise
	bool TicketExists(int ticketID)
	{		
		return m_mTickets.Contains(ticketID);
	}

	//------------------------------------------------------------------------------------------------
	// ADMIN COUNTDOWN (admin menu > Hints > Countdown)
	// Replicated once per start/cancel: the label, who it's for and the server time it ends. Each
	// client counts down locally against the replicated server clock (ChimeraWorld.GetServerTimestamp),
	// so nothing is sent per second. Shown by COA_AdminCountdownDisplay.
	//------------------------------------------------------------------------------------------------

	static const int COUNTDOWN_MAX_SECONDS = 7200;
	static const int COUNTDOWN_MAX_LABEL_LENGTH = 32;
	// After it ends: clients show TIME for 5 s (COA_AdminCountdownDisplay), then the server clears it
	protected static const int COUNTDOWN_EXPIRE_DELAY_MS = 6000;

	// Bumped on every start/cancel so clients notice a restart with the same label
	[RplProp(onRplName: "OnCountdownReplicated")]
	protected int m_iCountdownSerial;

	[RplProp()]
	protected bool m_bCountdownActive;

	[RplProp()]
	protected string m_sCountdownLabel;

	// Empty = everyone, else the faction key it's shown to
	[RplProp()]
	protected string m_sCountdownFaction;

	[RplProp()]
	protected WorldTimestamp m_CountdownEnd;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Clients: a countdown already running when they join arrives with the initial snapshot;
		// a slow check picks it up whether or not the replication callback fired for it
		if (GetGame().InPlayMode() && !System.IsConsoleApp())
			GetGame().GetCallqueue().CallLater(CheckCountdownDisplay, 1000, true);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(CheckCountdownDisplay);
		GetGame().GetCallqueue().Remove(ExpireCountdown);
		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: start (or restart) the countdown
	//! \param[in] label Shown on the HUD, e.g. "Mission ending"
	//! \param[in] seconds Duration, clamped to 1..COUNTDOWN_MAX_SECONDS
	//! \param[in] factionKey Empty for everyone, else only that faction sees it
	void StartCountdown(string label, int seconds, string factionKey)
	{
		if (!Replication.IsServer())
			return;

		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return;

		seconds = Math.ClampInt(seconds, 1, COUNTDOWN_MAX_SECONDS);

		m_sCountdownLabel = SanitizeCountdownLabel(label);
		m_sCountdownFaction = factionKey;
		m_CountdownEnd = world.GetServerTimestamp().PlusSeconds(seconds);
		m_bCountdownActive = true;
		m_iCountdownSerial++;
		Replication.BumpMe();

		// Clear it once every client has shown TIME, so late joiners and the admin menu see it as over
		GetGame().GetCallqueue().Remove(ExpireCountdown);
		GetGame().GetCallqueue().CallLater(ExpireCountdown, seconds * 1000 + COUNTDOWN_EXPIRE_DELAY_MS, false);

		// The authority doesn't get its own RplProp callback (listen server / Workbench host)
		OnCountdownReplicated();
	}

	//------------------------------------------------------------------------------------------------
	//! Server: the countdown ran out a little while ago
	protected void ExpireCountdown()
	{
		if (!m_bCountdownActive)
			return;

		m_bCountdownActive = false;
		Replication.BumpMe();
		OnCountdownReplicated();
	}

	//------------------------------------------------------------------------------------------------
	//! Server: stop the countdown early
	//! \return false if none was running
	bool CancelCountdown()
	{
		if (!Replication.IsServer() || !m_bCountdownActive)
			return false;

		GetGame().GetCallqueue().Remove(ExpireCountdown);

		m_bCountdownActive = false;
		m_iCountdownSerial++;
		Replication.BumpMe();

		OnCountdownReplicated();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! One line, capped, never empty
	static string SanitizeCountdownLabel(string label)
	{
		label.Replace("\r", " ");
		label.Replace("\n", " ");
		label = label.Trim();

		if (label.Length() > COUNTDOWN_MAX_LABEL_LENGTH)
			label = label.Substring(0, COUNTDOWN_MAX_LABEL_LENGTH).Trim();

		if (label.IsEmpty())
			label = "Countdown";

		return label;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCountdownReplicated()
	{
		COA_AdminCountdownDisplay.Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckCountdownDisplay()
	{
		if (m_bCountdownActive && !COA_AdminCountdownDisplay.IsRunning())
			COA_AdminCountdownDisplay.Refresh();
	}

	//------------------------------------------------------------------------------------------------
	//! \return true while a countdown is started and not cancelled (it may already have run out - see GetCountdownRemainingMs)
	bool IsCountdownActive()
	{
		return m_bCountdownActive;
	}

	//------------------------------------------------------------------------------------------------
	int GetCountdownSerial()
	{
		return m_iCountdownSerial;
	}

	//------------------------------------------------------------------------------------------------
	string GetCountdownLabel()
	{
		return m_sCountdownLabel;
	}

	//------------------------------------------------------------------------------------------------
	//! \return empty for everyone, else the faction key
	string GetCountdownFaction()
	{
		return m_sCountdownFaction;
	}

	//------------------------------------------------------------------------------------------------
	//! \return milliseconds until the countdown ends on the server clock (negative once it has run out)
	float GetCountdownRemainingMs()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return 0;

		return m_CountdownEnd.DiffMilliseconds(world.GetServerTimestamp());
	}
	
	//------------------------------------------------------------------------------------------------
	protected static COA_AdminMenuManager m_sInstance;
	void COA_AdminMenuManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_AdminMenuManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static COA_AdminMenuManager GetInstance()
	{
		return m_sInstance;
	}
}
