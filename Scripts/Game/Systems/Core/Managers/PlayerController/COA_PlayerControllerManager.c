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
	
	// Game Systems
	protected COA_Gamemode m_Gamemode;                      // Reference to the active gamemode
	protected COA_PlayerRplToAuthorityManager m_PlayerRplToAuthorityManager;  // Network authority manager
	protected COA_PlayerCameraManager m_CameraManager;                  // Reference to the local camera manager
	protected const int PLAYER_CLIENT_INIT_RETRY_DELAY_MS = 200;
	protected const int MAX_PLAYER_CLIENT_INIT_RETRIES = 20; // ~4 seconds at 200ms interval

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
		InitializeManagers();
		
		// Only mute audio during briefing/slotting/AAR — explicit state check prevents muting JIP players
		// joining mid-game, and avoids muting on null gamemode (timing edge case)
		if (m_Gamemode && m_Gamemode.m_GamemodeState != COA_EGamemodeState.GAME)
		{
			//GetGame().GetCallqueue().Call(COA_PlayerSettingsManager.GetInstance().InitFPSLock);
			GetGame().GetCallqueue().Call(COA_PlayerSettingsManager.GetInstance().InitAudioLock);
		}
		
		GetGame().GetCallqueue().Call(COA_PlayerMenuManager.GetInstance().OpenCurrentStateMenu);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void InitializeManagers()
	{
		m_Gamemode = COA_Gamemode.GetInstance();
		m_PlayerRplToAuthorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
		m_CameraManager = COA_PlayerCameraManager.GetInstance();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 PLAYER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	void VerifyClientCharacter(RplId playerCharID, int retryCount = 0)
	{
		// Get player character
		IEntity playerCharacter = COA_EntityHelper.GetCharacterFromRplId(playerCharID);
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetOwner());
		SCR_PlayerFactionAffiliationComponent playerFactionAffiliation = SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
		
		// If references are not initialized
		if (!playerCharacter 
			|| !playerController 
			|| !m_PlayerRplToAuthorityManager
			|| playerCharacter != playerController.GetLocalMainEntity()
			|| playerController.GetLocalMainEntityFaction() != playerFactionAffiliation.GetAffiliatedFaction()
		) { // retry briefly and then stop
			retryCount++;
			if (retryCount >= MAX_PLAYER_CLIENT_INIT_RETRIES)
			{
				Print(string.Format("[Coalition Lobby] WARNING: Failed to initialize player client for character %1 after %2 attempts", playerCharID, retryCount), LogLevel.WARNING);
				return;
			}

			GetGame().GetCallqueue().CallLater(VerifyClientCharacter, PLAYER_CLIENT_INIT_RETRY_DELAY_MS, false, playerCharID, retryCount);
			return;
		};
		
		// We passed all checks and now we call up to the server to get our gear and finilized client Initialization with InitilizePlayerClient;
		m_PlayerRplToAuthorityManager.FinalizeInitilizePlayer(playerController.GetPlayerId(), playerCharID);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initializes the player client
	//! Cleans up previous camera, closes menus, and sets up player-specific settings
	//! \param[in] playerCharacter - The spectator entity the server created and set to this player
	void InitilizePlayerClient(RplId playerCharID)
	{
		// Get player character
		IEntity playerCharacter = COA_EntityHelper.GetCharacterFromRplId(playerCharID);
		
		// Should never happen due to VerifyClientCharacter, but just in case
		if (!playerCharacter)
			return;
		
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
			InitilizeLocalSpectator();
		else
			InitilizeLocalCharacter();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initilizes players if they have a valid spectator entity
	protected void InitilizeLocalSpectator()
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
	protected void InitilizeLocalCharacter()
	{
		// Clean up previous camera if exists
		if (m_CameraManager.m_eCamera)
			delete m_CameraManager.m_eCamera;
		
		// NOTE: this used to fire m_Gamemode.GetOnPlayerSpawned().Invoke(...) here, on the CLIENT,
		// as a stand-in for the server event that never ran because possession bypassed vanilla's
		// spawn pipeline. That is an authority-side invoker: every vanilla listener is registered
		// from server context and expects a server-resolved playerId/entity, so firing it locally
		// satisfied the data collector and misled everything else.
		// COA_PlayerHelper.AssignCharacterToPlayer now routes through the possess-spawn pipeline, so
		// SCR_BaseGameMode raises the real event on the authority and this fake is not needed.

		// Reset Stored Pos
		GetGame().GetCallqueue().CallLater(m_CameraManager.UpdateStoredCameraPos, 200, false, vector.Zero, vector.Zero, vector.Zero, vector.Zero);
		
		// Reset kill feed type to default
		SCR_NotificationSenderComponent sender = SCR_NotificationSenderComponent.Cast(
			GetGame().GetGameMode().FindComponent(SCR_NotificationSenderComponent)
		);
		if (sender)
			sender.SetKillFeedTypeNoneLocal();
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
