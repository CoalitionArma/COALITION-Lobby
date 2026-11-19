class COA_PlayerSettingsManagerClass : ScriptComponentClass {}

class COA_PlayerSettingsManager : ScriptComponent
{	
	// Game Performance Settings
	protected int m_iFPS = -1;              // Stored user FPS setting (-1 means uninitialized)
	protected int m_iAudioSetting = -1;     // Stored audio volume (-1 means uninitialized)
	protected static bool s_bHoldingAudioLock;	// HoldAudioLock is running (any state but GAME)
	protected static bool s_bGamemodeSeen;		// the gamemode entity has existed since the hold started

//=============================================================================================================================================================================================================================================================================================================================================================
//	 SETTINGS METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Client: hold the mute from this player controller too, in case it exists before the gamemode
	//! entity (COA_Gamemode.EOnInit starts it as soon as that loads, which is earlier)
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		StartHoldingAudioLock();
	}

	//------------------------------------------------------------------------------------------------
	//! Initializes audio lock by storing current volume and setting to 0. The game world stays silent
	//! until the game starts (HoldAudioLock).
	void InitAudioLock()
	{
		m_iAudioSetting = AudioSystem.GetMasterVolume(AudioSystem.SFX);
		SetSFXVolume(0);
		StartHoldingAudioLock();
	}

	//------------------------------------------------------------------------------------------------
	//! Client: mute the game world (SFX) until the gamemode is in GAME. Per machine, not per player
	//! controller, so it can start before this player's controller arrives: started by COA_Gamemode
	//! when its entity loads (the world's sound starts a few seconds before the controller exists) and
	//! on every state change away from GAME (the next round's slotting after an AAR).
	static void StartHoldingAudioLock()
	{
		if (s_bHoldingAudioLock || !GetGame() || !GetGame().InPlayMode() || System.IsConsoleApp() || RplSession.Mode() == RplMode.Dedicated)
			return;

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (gamemode && gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
			return;

		s_bHoldingAudioLock = true;
		s_bGamemodeSeen = gamemode != null;
		AudioSystem.SetMasterVolume(AudioSystem.SFX, 0);
		GetGame().GetCallqueue().CallLater(HoldAudioLock, 0, true);
		Print("[Coalition Lobby] Game sound muted until the game starts");
	}

	//------------------------------------------------------------------------------------------------
	protected static void StopHoldingAudioLock()
	{
		if (!s_bHoldingAudioLock)
			return;

		s_bHoldingAudioLock = false;
		if (GetGame())
			GetGame().GetCallqueue().Remove(HoldAudioLock);

		Print("[Coalition Lobby] Game sound mute released");
	}

	//------------------------------------------------------------------------------------------------
	//! Every frame until the game starts: keep the game world's sound (SFX) muted. The wind, rain and
	//! other 2D ambients go out through the SFX master (Sounds/FinalMix.afm: ENV_AMB_2D -> Output SFX).
	//! Muting once isn't enough - the vanilla loading screen turns every master volume back on when it
	//! closes (LoadingScreen.EnableSounds), and a user settings refresh or the AAR closing can too.
	//! While the gamemode hasn't replicated yet the state is unknown, so the game world stays muted
	//! until it has. A single check per frame, no loops.
	protected static void HoldAudioLock()
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (gamemode)
			s_bGamemodeSeen = true;

		// Left the server (the gamemode was here and is gone): the call queue outlives the world, so
		// stop here or the main menu stays muted
		if ((!gamemode && s_bGamemodeSeen) || (gamemode && gamemode.m_GamemodeState == COA_EGamemodeState.GAME))
		{
			// In game: sound back on (COA_PlayerControllerManager.InitilizePlayerClient does the same when
			// the player's character arrives - whichever comes first)
			StopHoldingAudioLock();
			AudioSystem.SetMasterVolume(AudioSystem.SFX, 100);
			return;
		}

		if (AudioSystem.GetMasterVolume(AudioSystem.SFX) > 0)
			AudioSystem.SetMasterVolume(AudioSystem.SFX, 0);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Sets FPS limit to specified value
	//! \param[in] video - Video settings container
	//! \param[in] fps - FPS limit to set
	void SetFPS(BaseContainer video, int fps)
	{
		video.Set("MaxFps", fps);
		GetGame().UserSettingsChanged();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Retrieves and stores initial user FPS setting
	//! \param[in] video - Video settings container
	void GetInitialUserFPSValue(BaseContainer video)
	{
		video.Get("MaxFps", m_iFPS);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initializes FPS lock by storing current value and setting to 30
	void InitFPSLock()
	{
		//BaseContainer video = GetGame().GetEngineUserSettings().GetModule("VideoUserSettings");
		//GetInitialUserFPSValue(video);
		//SetFPS(video, 30);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Sets SFX volume to specified level
	//! \param[in] volume - Volume level to set
	void SetSFXVolume(int volume)
	{
		AudioSystem.SetMasterVolume(AudioSystem.SFX, volume);
	}
	
	
	//------------------------------------------------------------------------------------------------
	//! Restores user settings to original values
	void ResetSettingsToStoredValues()
	{
		//BaseContainer video = GetGame().GetEngineUserSettings().GetModule("VideoUserSettings");
		//SetFPS(video, 0);

		// Stop holding the mute too (disconnecting from slotting, or the game starting)
		StopHoldingAudioLock();

		SetSFXVolume(100);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static COA_PlayerSettingsManager m_sInstance;
	void COA_PlayerSettingsManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_PlayerSettingsManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static COA_PlayerSettingsManager GetInstance()
	{
		return m_sInstance;
	}
}