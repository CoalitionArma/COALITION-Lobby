modded class SCR_EditorManagerEntity
{
	//----------------------------------------------------------------
	// True when the local player is allowed unlimited editor access
	// (spectators, moderators and admins). Used both to decide the
	// limited state on open and to unlock the ArmaVision camera, see
	// COA_SCR_CameraLimitedEditorComponent.
	//----------------------------------------------------------------
	static bool COA_HasUnlimitedEditorAccess()
	{
		if (COA_EntityHelper.IsSpectator())
			return true;

		if (COA_PermissionManager.GetInstance() && COA_PermissionManager.GetInstance().IsModerator())
			return true;

		return SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId());
	}

	//----------------------------------------------------------------
	// Determines if the editor can be opened based on current mode and permissions
	//----------------------------------------------------------------
	override bool CanOpen()
	{
		if (!COA_Gamemode.GetInstance())
		{
			return super.CanOpen();;
		}

		// Spectators and moderators always get unlimited editor access
		if (COA_HasUnlimitedEditorAccess())
		{
			SetIsLimited(false);
		}
		else
		{
			// Regular players get limited editor
			SetIsLimited(true);
		}

		// If in building mode, always limit editor capabilities
		if (GetCurrentMode() == EEditorMode.BUILDING)
		{
			SetIsLimited(true);
			return true;
		}
		
		// No modes available
		if (m_Modes.IsEmpty())
			return false;
		
		// Determine if editor can be opened based on permissions
		if (IsLimited())
			return m_CanOpen == m_CanOpenSum;
		else
			return m_CanOpen | EEditorCanOpen.ALIVE == m_CanOpenSum;
	}
	
	//----------------------------------------------------------------
	// Sets whether the editor functionality should be limited
	// @param input - true to limit functionality, false for full access
	//----------------------------------------------------------------
	void SetIsLimited(bool input)
	{
		m_bIsLimited = input;
	}

	//----------------------------------------------------------------
	// Controls server-side toggling of the editor
	//----------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	override protected void ToggleServer(bool open)
	{
		// Use default behavior if no COA_Gamemode is active
		if (!COA_Gamemode.GetInstance())
		{
			super.ToggleServer(open);
			return;
		}

		// Return if no state change or if in client mode
		if (m_bIsOpened == open || RplSession.Mode() == RplMode.Client) 
			return;
			
		// Check if editor can be closed
		if (!open && !CanClose())
		{
			SCR_NotificationsComponent.SendToPlayer(GetPlayerID(), ENotification.EDITOR_CANNOT_CLOSE);
			return;
		}
		
		// Set editor state
		m_bIsOpened = open;
		Rpc(ToggleOwner, m_bIsOpened);
		
		// Handle editor being opened
		if (m_bIsOpened)
		{
			if (m_CurrentModeEntity)
				m_CurrentModeEntity.ActivateModeServer();
			
			Event_OnOpenedServer.Invoke();
		}
		// Handle editor being closed
		else
		{
			if (m_CurrentModeEntity)
				m_CurrentModeEntity.DeactivateModeServer();
			
			Event_OnClosedServer.Invoke();
		}
	}
	
	//----------------------------------------------------------------
	// Handles events related to editor state changes
	//----------------------------------------------------------------
	override void StartEvents(EEditorEventOperation type = EEditorEventOperation.NONE)
	{
		// Call base implementation
		super.StartEvents(type);
		
		// Skip custom behavior if no COA_Gamemode is active
		if (!COA_Gamemode.GetInstance())
			return;
		
		// Handle editor opening - close all Lobby menus
		if (type == EEditorEventOperation.OPEN)
		{	
			GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.COA_PreviewMenu);
			GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.COA_SlottingMenu);
			GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.COA_SpectatorMenu);
			GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.COA_AARMenu);
		}
		// Handle editor closing - reopen appropriate UI
		else if (type == EEditorEventOperation.CLOSE)
		{	
			if (COA_Gamemode.GetInstance())
			{
				// Schedule UI reopening after a short delay
				GetGame().GetCallqueue().Call(OpenUI);
			}
		}
	}
	
	//----------------------------------------------------------------
	// Opens the appropriate UI menu based on the current gamemode state
	//----------------------------------------------------------------
	void OpenUI()
	{	
		// Get the active gamemode instance
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		
		// Verify gamemode exists and check player permissions
		if (gamemode)
		{
			 if (!COA_EntityHelper.IsSpectator())
				return;
		}

		// Ensure local controlled entity exists
		if (SCR_PlayerController.GetLocalControlledEntity() == null)
			return;
			
		// Open appropriate menu based on gamemode state
		switch (gamemode.m_GamemodeState)
		{
			case COA_EGamemodeState.BRIEFING: 
			{
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.COA_PreviewMenu);
				break;
			}
			case COA_EGamemodeState.SLOTTING: 
			{
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.COA_SlottingMenu);
				break;
			}
			case COA_EGamemodeState.GAME: 
			{
				// Verify entities exist
				if (!SCR_PlayerController.GetLocalMainEntity() || !SCR_PlayerController.GetLocalControlledEntity())
					return;
				
				// Initialize spectator camera if player is spectating
				bool isSpectator = COA_EntityHelper.IsSpectator(SCR_PlayerController.GetLocalMainEntity());
				bool isSameEntity = SCR_PlayerController.GetLocalControlledEntity() == SCR_PlayerController.GetLocalMainEntity();
				
				if (isSpectator && isSameEntity)
					GetGame().GetCallqueue().CallLater(COA_PlayerRplToAuthorityManager.GetInstance().RequestInitilizePlayer, 200, false, SCR_PlayerController.GetLocalPlayerId());
				
				break;
			}
			case COA_EGamemodeState.AAR: 
			{
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.COA_AARMenu);
				break;
			}
		}
	}
}