class COA_PlayerSettingsManagerClass : ScriptComponentClass {}

class COA_PlayerSettingsManager : ScriptComponent
{	
	// Game Performance Settings
	protected int m_iFPS = -1;              // Stored user FPS setting (-1 means uninitialized)
	protected int m_iAudioSetting = -1;     // Stored audio volume (-1 means uninitialized)
	protected bool m_bHoldingAudioLock;     // HoldAudioLock is running (briefing / slotting)

//=============================================================================================================================================================================================================================================================================================================================================================
//	 SETTINGS METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Initializes audio lock by storing current volume and setting to 0. The game world stays silent
	//! through briefing and slotting (HoldAudioLock) and gets its sound back when the game starts.
	void InitAudioLock()
	{
		m_iAudioSetting = AudioSystem.GetMasterVolume(AudioSystem.SFX);
		SetSFXVolume(0);

		if (!m_bHoldingAudioLock)
		{
			m_bHoldingAudioLock = true;
			GetGame().GetCallqueue().CallLater(HoldAudioLock, 0, true);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Every frame until the game starts: keep the game world's sound (SFX) muted. Muting once isn't
	//! enough - the vanilla loading screen turns every master volume back on when it closes
	//! (LoadingScreen.EnableSounds), and a user settings refresh can too, so whichever ran last won and
	//! ambient sound played through briefing and slotting. The AAR and outro hold their mute the same way.
	protected void HoldAudioLock()
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (gamemode && gamemode.m_GamemodeState != COA_EGamemodeState.BRIEFING && gamemode.m_GamemodeState != COA_EGamemodeState.SLOTTING)
		{
			GetGame().GetCallqueue().Remove(HoldAudioLock);
			m_bHoldingAudioLock = false;

			// In game: sound back on (COA_PlayerControllerManager.InitilizePlayerClient does the same when
			// the player's character arrives - whichever comes first)
			if (gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
				ResetSettingsToStoredValues();

			return;
		}

		if (AudioSystem.GetMasterVolume(AudioSystem.SFX) > 0)
			SetSFXVolume(0);
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
		if (m_bHoldingAudioLock)
		{
			GetGame().GetCallqueue().Remove(HoldAudioLock);
			m_bHoldingAudioLock = false;
		}

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
		if (m_bHoldingAudioLock && GetGame())
			GetGame().GetCallqueue().Remove(HoldAudioLock);

		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static COA_PlayerSettingsManager GetInstance()
	{
		return m_sInstance;
	}
}