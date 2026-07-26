class COA_PlayerChatCommandManagerClass : ScriptComponentClass {}

class COA_PlayerChatCommandManager : ScriptComponent
{	
	protected COA_PlayerRplToAuthorityManager m_PlayerRplToAuthorityManager;
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 MANAGER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{	
		super.OnPostInit(owner);
		m_PlayerRplToAuthorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
		
		GetGame().GetCallqueue().Call(AddMsgAction);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Registers chat commands for admin messaging
	void AddMsgAction()
	{
		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		
		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("admin");
		invoker.Insert(SendAdminMessage);
		
		ChatCommandInvoker invoker2 = chatPanelManager.GetCommandInvoker("a");
		invoker2.Insert(SendAdminMessage);
		
		ChatCommandInvoker invoker3 = chatPanelManager.GetCommandInvoker("r");
		invoker3.Insert(ReplyAdminMessage);
		
		ChatCommandInvoker invoker4 = chatPanelManager.GetCommandInvoker("reply");
		invoker4.Insert(ReplyAdminMessage);
		
		ChatCommandInvoker invoker5 = chatPanelManager.GetCommandInvoker("aar");
		invoker5.Insert(Advance_Callback);
		
		// CURRENTLY BROKEN IN 1.7 - NEEDS UPDATING COA_PersistanceManager
		// ChatCommandInvoker invoker6 = chatPanelManager.GetCommandInvoker("save");
		// invoker6.Insert(SaveMission_Callback);		
		
#ifdef COALITION_REFORGER_FRAMEWORK
		ChatCommandInvoker invoker7 = chatPanelManager.GetCommandInvoker("bug");
		invoker7.Insert(ReportBug);
#endif
		
		ChatCommandInvoker invoker8 = chatPanelManager.GetCommandInvoker("adminmenu");
		invoker8.Insert(OpenAdminMenu);

#ifdef COALITION_REFORGER_FRAMEWORK
		ChatCommandInvoker invoker9 = chatPanelManager.GetCommandInvoker("forwarddeploy");
		invoker9.Insert(ReopenForwardDeployMenu);

		ChatCommandInvoker invoker10 = chatPanelManager.GetCommandInvoker("fd");
		invoker10.Insert(ReopenForwardDeployMenu);
#endif
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 ADMIN MENU METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//!Opens admin menu :O
	void OpenAdminMenu(SCR_ChatPanel panel, string data)
	{
		// Check if the current player has admin permissions
		bool hasAdminAccess = false;
		COA_PermissionManager permissionManager = COA_PermissionManager.GetInstance();
		
		// Check admin/moderator permissions and required game instances
		if (SCR_Global.IsAdmin() || (permissionManager && permissionManager.IsModerator() && COA_Gamemode.GetInstance()))
			hasAdminAccess = true;
		
		// Only register click handler if the admin menu is visible
		if (hasAdminAccess)
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.COA_AdminMenu);
	}
	
#ifdef COALITION_REFORGER_FRAMEWORK
//=============================================================================================================================================================================================================================================================================================================================================================
//	 FORWARD DEPLOY METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Reopens the JIP forward deploy menu (e.g. if the player closed it earlier via the back action).
	//! Usage: /forwarddeploy or /fd
	void ReopenForwardDeployMenu(SCR_ChatPanel panel, string data)
	{
		SCR_ChatComponent chatComponent;
		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
			chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		COA_SafestartManager safestartManager = COA_SafestartManager.GetInstance();
		if (!gamemode || !safestartManager || gamemode.m_bLockUnusedSlots || safestartManager.GetSafestartStatus())
		{
			if (chatComponent)
				chatComponent.ShowMessage("Forward deploy is not available right now.");
			return;
		}

		IEntity controlled = SCR_PlayerController.GetLocalControlledEntity();
		if (!controlled || COA_EntityHelper.IsSpectator(controlled))
		{
			if (chatComponent)
				chatComponent.ShowMessage("You need to be playing a character to forward deploy.");
			return;
		}

		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.COA_JIPForwardDeployMenu);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 BUG REPORT METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Reports bugs to github
	//! \param[in] panel - Chat panel
	//! \param[in] data - Message content
	void ReportBug(SCR_ChatPanel panel, string data)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;
		
		int playerID = GetGame().GetPlayerController().GetPlayerId();
		
		if (!data.Length() > 0)
		{
			chatComponent.ShowMessage("You need to include your bug report after /bug");
			return;
		}	
		
		chatComponent.ShowMessage(string.Format("Bug Report Sent: \"%1\"", data));
		m_PlayerRplToAuthorityManager.ReportBug(data, playerID);
	}
#endif
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 ADMIN MESSAGING METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Sends an admin message from the player to server
	//! \param[in] panel - Chat panel
	//! \param[in] data - Message content
	void SendAdminMessage(SCR_ChatPanel panel, string data)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;
		
		int playerID = GetGame().GetPlayerController().GetPlayerId();
		
		if (!data.Length() > 0)
		{
			chatComponent.ShowMessage("You need to include your message after /a");
			return;
		}	
		
		chatComponent.ShowMessage(string.Format("Message Sent: \"%1\"", data));
		m_PlayerRplToAuthorityManager.SendAdminMessage(data, playerID);
	}

	//------------------------------------------------------------------------------------------------
	//! Allows admins to reply to specific players
	//! \param[in] panel - Chat panel
	//! \param[in] data - Message including player ID and content
	void ReplyAdminMessage(SCR_ChatPanel panel, string data)
	{
		// Verify sender is admin or moderator
		if (!SCR_Global.IsAdmin() && !COA_PermissionManager.GetInstance().IsModerator())
			return;
		
		// Parse player ID and message from input
		array<string> dataSplit = {};
		data.Split(" ", dataSplit, false);
		string name;
		string toSend;
		
		name = dataSplit[0];
		dataSplit.RemoveOrdered(0);
		
		toSend = SCR_StringHelper.Join(" ", dataSplit);
		
		array<int> players = {};
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		pm.GetPlayers(players);
		
		int playerId = 0;
		foreach (int player: players)
		{
			string playerName = pm.GetPlayerName(player);
			
			if (playerName.ToLower() == name.ToLower())
			{
				playerId = player;
				break;
			}
		}
		
		// Get chat component
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;
		
		// Validate player ID
		if (!playerId)
		{
			chatComponent.ShowMessage("INVALID PLAYER NAME");
			return;
		}
		
		if (playerId == 0)
		{
			chatComponent.ShowMessage("INVALID PLAYER NAME");
			return;
		}
		
		// Get the ID of the admin replying to the ticket
		int adminID = SCR_PlayerController.GetLocalPlayerId();

		// Send message
		chatComponent.ShowMessage(string.Format("Message Sent to %2: \"%1\"", 
			toSend, 
			GetGame().GetPlayerManager().GetPlayerName(playerId)));
		
		toSend = string.Format("\"%1\"", toSend);
		m_PlayerRplToAuthorityManager.ReplyAdminMessage(toSend, playerId, adminID, true);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 GAMEMODE STATE METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Callback for advancing gamemode state with optional faction winner parameter
	//! Usage: /aar [faction]
	//! Examples: /aar, /aar blufor, /aar blu, /aar opfor, /aar opf, /aar indfor, /aar ind, /aar civ
	void Advance_Callback(SCR_ChatPanel panel, string data)
	{
		// Check if admin privileges are required
		if (!SCR_Global.IsAdmin())
		{
			if (panel)
			{
				SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
				if (chatComponent)
					chatComponent.ShowMessage("You need admin privileges to use the /aar command.");
			}
			return;
		}
		
		// Parse faction parameter if provided
		if (data && data.Length() > 0)
		{
			// Clean up the input - remove extra spaces and convert to uppercase
			data.Trim();
			data.ToUpper();
			
			// Map short faction names to full names
			FactionKey winningFaction = "";
			switch (data)
			{
				case "BLUFOR":
				case "BLU":
				case "B":
				case "BLUE":
					winningFaction = "BLUFOR";
					break;
					
				case "OPFOR":
				case "OPF":
				case "O":
				case "RED":
					winningFaction = "OPFOR";
					break;
					
				case "INDFOR":
				case "IND":
				case "I":
				case "INDEPENDENT":
				case "GREEN":
					winningFaction = "INDFOR";
					break;
					
				case "CIV":
				case "C":
				case "CIVILIAN":
					winningFaction = "CIV";
					break;
					
				default:
					// Invalid faction specified
					if (panel)
					{
						SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
						if (chatComponent)
						{
							string validOptions = "Valid faction options: blufor (blu), opfor (opf), indfor (ind), civ";
							chatComponent.ShowMessage(string.Format("Invalid faction '%1'. %2", data, validOptions));
						}
					}
					return;
			}
			
#ifdef COALITION_REFORGER_FRAMEWORK
			// Set the winning faction in the logging manager
			COA_LoggingManager loggingManager = COA_LoggingManager.GetInstance();
			if (loggingManager)
			{
				// Show confirmation message
				if (panel)
				{
					SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
					if (chatComponent)
						chatComponent.ShowMessage(string.Format("Winner set to %1. Advancing to AAR...", winningFaction));
				}
			}
#endif
			
			// Advance to AAR state — winning faction is passed to the server RPC so
			// SetWinningFaction is called on the authority where the log file handle exists.
			m_PlayerRplToAuthorityManager.RequestAdvanceGamemodeState(true, winningFaction);
		}
		else
		{
			// No faction specified - show current usage
			if (panel)
			{
				SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
				if (chatComponent)
					chatComponent.ShowMessage("Usage: /aar [faction] - Examples: /aar blufor, /aar opfor, /aar indfor, /aar civ");
					
			}
			return;
		}
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 SAVE MISSION METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Callback for triggering a manual mission save
	//! Usage: /save [save name]
	//! Examples: /save, /save After Attack, /save Checkpoint 1
	// NOTE: CURRENTLY BROKEN. REFERENCE COA_PersistanceManager FOR UPDATE
	/*
	void SaveMission_Callback(SCR_ChatPanel panel, string data)
	{
		// Check if admin privileges are required
		if (!SCR_Global.IsAdmin())
		{
			if (panel)
			{
				SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
				if (chatComponent)
					chatComponent.ShowMessage("You need admin privileges to use the /save command.");
			}
			return;
		}
		
		// Use provided name or default
		string saveName = "Manual Save";
		if (data && data.Length() > 0)
		{
			data.Trim();
			saveName = data;
		}
		
		// Show confirmation to admin
		if (panel)
		{
			SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
			if (chatComponent)
				chatComponent.ShowMessage(string.Format("Requesting mission save: %1...", saveName));
		}
		
		// Request save from server via RPC
		m_PlayerRplToAuthorityManager.RequestMissionSave(saveName);
	}
	*/
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected static COA_PlayerChatCommandManager m_sInstance;
	void COA_PlayerChatCommandManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_PlayerChatCommandManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static COA_PlayerChatCommandManager GetInstance()
	{
		return m_sInstance;
	}
}