//------------------------------------------------------------------------------------------------
//! Server-side notifications for the website's staff session report. The Lobby only raises them;
//! CRF_LoggingManager subscribes and writes them to COAServerLog.txt, which the Discord bot stores.
//! All invokers fire on the server only.
class COA_SessionLog
{
	//! (string text, COA_EAdminLogLevel level) - every entry that goes to the in-game admin log
	protected static ref ScriptInvoker s_OnAdminAction;

	//! (string eventName, string playerName, string adminName) - eventName is "open", "assigned" or "closed"
	protected static ref ScriptInvoker s_OnTicket;

	//! (string adminName, string note) - staff marked an incident with /incident
	protected static ref ScriptInvoker s_OnIncident;

	//------------------------------------------------------------------------------------------------
	static ScriptInvoker GetOnAdminAction()
	{
		if (!s_OnAdminAction)
			s_OnAdminAction = new ScriptInvoker();
		return s_OnAdminAction;
	}

	//------------------------------------------------------------------------------------------------
	static ScriptInvoker GetOnTicket()
	{
		if (!s_OnTicket)
			s_OnTicket = new ScriptInvoker();
		return s_OnTicket;
	}

	//------------------------------------------------------------------------------------------------
	static ScriptInvoker GetOnIncident()
	{
		if (!s_OnIncident)
			s_OnIncident = new ScriptInvoker();
		return s_OnIncident;
	}

	//------------------------------------------------------------------------------------------------
	static void AdminAction(string text, COA_EAdminLogLevel level)
	{
		if (Replication.IsServer() && s_OnAdminAction)
			s_OnAdminAction.Invoke(text, level);
	}

	//------------------------------------------------------------------------------------------------
	static void Ticket(string eventName, string playerName, string adminName)
	{
		if (Replication.IsServer() && s_OnTicket)
			s_OnTicket.Invoke(eventName, playerName, adminName);
	}

	//------------------------------------------------------------------------------------------------
	static void Incident(string adminName, string note)
	{
		if (Replication.IsServer() && s_OnIncident)
			s_OnIncident.Invoke(adminName, note);
	}
}
