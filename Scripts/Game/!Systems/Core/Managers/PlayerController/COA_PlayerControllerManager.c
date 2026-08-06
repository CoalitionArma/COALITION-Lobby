class COA_PlayerControllerManagerClass : ScriptComponentClass
{
}

class COA_PlayerControllerManager : ScriptComponent
{
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	// UI and Display
	string m_sHintText = "Type Here";      // Text displayed for hints to player
	bool m_bHUDVisible = true;             // Controls visibility of HUD elements
	Widget m_wSavedHintWidget;             // Reference to hint widget for reuse
	bool m_bIsListeningToSpec = false;
	vector m_vPlayersLastDeath[4];
	
	protected const int CLIENT_ASSIGN_RETRY_DELAY_MS = 100;
	protected const int CLIENT_ASSIGN_MAX_RETRIES = 100;
	
	// Game Systems
	protected COA_Gamemode m_Gamemode;                      // Reference to the active gamemode
	protected COA_PlayerRplToAuthorityManager m_PlayerRplToAuthorityManager;  // Network authority manager
	protected COA_PlayerCameraManager m_CameraManager;                  // Reference to the local camera manager

//=============================================================================================================================================================================================================================================================================================================================================================
//	 COMPONENT INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Initializes the player controller component
	//! Sets up input listeners and schedules initial setup calls
	void InitilizePlayerControllerComp()
	{
		// Skip initialization on dedicated servers or in editor
		if (!GetGame().InPlayMode() || RplSession.Mode() == RplMode.Dedicated)
			return;
		
		// Get references to required systems
		InitializeReferences();
		
		// Only mute audio during briefing/slotting/AAR — explicit state check prevents muting JIP players
		// joining mid-game, and avoids muting on null gamemode (timing edge case)
		if (m_Gamemode && m_Gamemode.m_GamemodeState != COA_EGamemodeState.GAME)
		{
			//GetGame().GetCallqueue().Call(COA_PlayerSettingsManager.GetInstance().InitFPSLock);
			GetGame().GetCallqueue().Call(COA_PlayerSettingsManager.GetInstance().InitAudioLock);
		}
		
		GetGame().GetCallqueue().Call(COA_PlayerMenuManager.GetInstance().OpenCurrentStateMenu);
	}
	
	protected void InitializeReferences()
	{
		// Get references to required managers/systems
		m_Gamemode = COA_Gamemode.GetInstance();
		m_PlayerRplToAuthorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
		m_CameraManager = COA_PlayerCameraManager.GetInstance();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 PLAYER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Initializes the player client: cleans up the previous camera, closes menus and applies
	//! player-specific settings.
	//! Purely client-local presentation work. Faction, group membership and radio tuning are all
	//! settled on the authority before this is broadcast — see
	//! COA_GamemodeManager.ScheduleAssignPlayerToCharacter — so nothing here asks the server for
	//! state, it only waits until possession has actually replicated down.
	//! \param[in] playerCharID RplId of the character the server assigned to this player
	//! \param[in] attempt current retry count
	void InitilizePlayerClient(RplId playerCharID, int attempt)
	{
		// Just In Case
		InitializeReferences();

		// Get player character
		IEntity playerCharacter = COA_EntityHelper.GetCharacterFromRplId(playerCharID);
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(SCR_PlayerController.GetLocalPlayerId()));

		//--------------------------------------------------------------------------- CHECK ---------------------------------------------------------------------------
		if (!playerCharacter || !playerController || !COA_PlayerCharacter.Cast(playerCharacter) || (SCR_PlayerController.GetLocalMainEntity() != playerCharacter)
			|| !m_Gamemode || !m_PlayerRplToAuthorityManager || !m_CameraManager)
		{
			if (attempt + 1 >= CLIENT_ASSIGN_MAX_RETRIES)
			{
				Print(string.Format("[COA_PlayerControllerManager] ERROR: Failed to initialize Client %1 after %2 attempts (this is VERY bad)", SCR_PlayerController.GetLocalPlayerId(), CLIENT_ASSIGN_MAX_RETRIES), LogLevel.ERROR);
				return;
			}

			GetGame().GetCallqueue().CallLater(InitilizePlayerClient, CLIENT_ASSIGN_RETRY_DELAY_MS, false, playerCharID, attempt + 1);
			return;
		}
		//--------------------------------------------------------------------------- CHECK PASS ---------------------------------------------------------------------------

		// Close all menus
		if (m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
		{
			GetGame().GetMenuManager().CloseAllMenus();
			COA_PlayerSettingsManager.GetInstance().ResetSettingsToStoredValues();
			if (!CVON_VONGameModeComponent.GetInstance())
				COA_InitializationHelper.SetupRadioFrequency();
		};
		
		m_CameraManager.DisableCameraOnRails();
		
		if (COA_EntityHelper.IsSpectator(playerCharacter))
			InitilizeLocalSpectator(playerCharacter);
		else
			InitilizeLocalCharacter(playerCharacter);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initilizes players if they have a valid spectator entity
	//! \param[in] playerCharacter - The spectator entity the server created and set to this player
	protected void InitilizeLocalSpectator(IEntity playerCharacter)
	{
		m_CameraManager.InitilizeSpecCamera();
		
		// Register for VON (voice chat)
		m_PlayerRplToAuthorityManager.CheckVONRegister(SCR_PlayerController.GetLocalPlayerId());
		
		// Open spectator menu if in game state
		if (m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.COA_SpectatorMenu);
		
		// Turn on killfeed for specs
		SCR_NotificationSenderComponent sender = SCR_NotificationSenderComponent.Cast(
			GetGame().GetGameMode().FindComponent(SCR_NotificationSenderComponent)
		);
		if (sender)
			sender.SetKillFeedTypeDeadLocal();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initilizes players if they have a valid slotted character
	//! \param[in] playerCharacter - The player entity the server created and set to this player
	protected void InitilizeLocalCharacter(IEntity playerCharacter)
	{
		// Clean up previous camera if exists
		if (m_CameraManager.m_eCamera)
			delete m_CameraManager.m_eCamera;
		
		// Originally added for data collector
		m_Gamemode.GetOnPlayerSpawned().Invoke(SCR_PlayerController.GetLocalPlayerId(), playerCharacter);
		
		// Reset Stored Pos
		GetGame().GetCallqueue().CallLater(m_CameraManager.UpdateStoredCameraPos, 200, false, vector.Zero, vector.Zero, vector.Zero, vector.Zero);
		
		// Reset kill feed type to default
		SCR_NotificationSenderComponent sender = SCR_NotificationSenderComponent.Cast(
			GetGame().GetGameMode().FindComponent(SCR_NotificationSenderComponent)
		);
		if (sender)
			sender.SetKillFeedTypeNoneLocal();

		// Radios are built and tuned on the authority, which then pushes the rebuild down to this
		// client via COA_PlayerRplToOwnerManager.InitializeRadioFromServer(). Doing it here as well
		// would either no-op or fight the authoritative state, so it is deliberately absent.
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static COA_PlayerControllerManager m_sInstance;
	void COA_PlayerControllerManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_PlayerControllerManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static COA_PlayerControllerManager GetInstance()
	{
		return m_sInstance;
	}
}
