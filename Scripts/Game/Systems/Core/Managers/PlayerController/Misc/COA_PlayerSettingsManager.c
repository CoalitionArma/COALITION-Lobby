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
	//! Client: start holding the mute as soon as this player controller exists. Waiting for
	//! COA_PlayerController.UpdateLocalPlayerController isn't enough - it skips the audio lock when the
	//! gamemode entity hasn't reached the client yet, and then nothing muted the game world at all.
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode() || System.IsConsoleApp())
			return;

		StartHoldingAudioLock();
	}

	//------------------------------------------------------------------------------------------------
	//! Initializes audio lock by storing current volume and setting to 0. The game world stays silent
	//! through briefing and slotting (HoldAudioLock) and gets its sound back when the game starts.
	void InitAudioLock()
	{
		m_iAudioSetting = AudioSystem.GetMasterVolume(AudioSystem.SFX);
		SetSFXVolume(0);
		StartHoldingAudioLock();
	}

	//------------------------------------------------------------------------------------------------
	protected void StartHoldingAudioLock()
	{
		if (m_bHoldingAudioLock)
			return;

		m_bHoldingAudioLock = true;
		GetGame().GetCallqueue().CallLater(HoldAudioLock, 0, true);
		Print("[Coalition Lobby] Game sound muted until the game starts");
	}

	//------------------------------------------------------------------------------------------------
	protected void StopHoldingAudioLock()
	{
		if (!m_bHoldingAudioLock)
			return;

		GetGame().GetCallqueue().Remove(HoldAudioLock);
		m_bHoldingAudioLock = false;
		Print("[Coalition Lobby] Game sound mute released");
	}

	//------------------------------------------------------------------------------------------------
	//! Every frame until the game starts: keep the game world's sound (SFX) muted. The wind, rain and
	//! other 2D ambients go out through the SFX master (Sounds/FinalMix.afm: ENV_AMB_2D -> Output SFX).
	//! Muting once isn't enough - the vanilla loading screen turns every master volume back on when it
	//! closes (LoadingScreen.EnableSounds), and a user settings refresh can too. The AAR and outro hold
	//! their mute the same way. While the gamemode hasn't replicated yet the state is unknown, so the
	//! game world stays muted until it has.
	protected void HoldAudioLock()
	{
		// Only the local player's own controller (a listen server holds everyone's)
		PlayerController localController = GetGame().GetPlayerController();
		if (localController && localController != GetOwner())
		{
			StopHoldingAudioLock();
			return;
		}

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (gamemode && gamemode.m_GamemodeState != COA_EGamemodeState.BRIEFING && gamemode.m_GamemodeState != COA_EGamemodeState.SLOTTING)
		{
			StopHoldingAudioLock();

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