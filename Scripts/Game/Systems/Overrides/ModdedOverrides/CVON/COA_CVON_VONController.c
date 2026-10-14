modded class SCR_VONController
{
	ref map<int, bool> m_SpectatorChecks = new map<int, bool>;
    int m_iLastChannelChanges = -1; // Track last known channel change count
	COA_MenuManager m_CRFMenuManager;
	COA_RespawnManager m_RespawnManager;
	COA_SlottingManager m_SlottingManager;
	COA_Gamemode m_Gamemode;
	
	//Stores the local player controller for frame checks, saves frame time having a pointer to it.
	SCR_PlayerController m_PlayerController;
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		m_CRFMenuManager = COA_MenuManager.GetInstance();
		m_RespawnManager = COA_RespawnManager.GetInstance();
		m_SlottingManager = COA_SlottingManager.GetInstance();
		m_Gamemode = COA_Gamemode.GetInstance();
		
		if (!Replication.IsServer())
			m_PlayerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
	}
    
	// Last seen value of IsAARVoiceActive(), so the spectator cache is rebuilt when the AAR starts
	protected bool m_bLastAARVoiceActive;

	//------------------------------------------------------------------------------------------------
	//! During the AAR (once the outro has finished) voice is channel-based for everyone: every player
	//! is treated like a spectator, so only players in the same COA_MenuManager channel hear each
	//! other - at full volume and in the same language - and in-world proximity voice is off.
	bool IsAARVoiceActive()
	{
		if (!m_Gamemode)
			m_Gamemode = COA_Gamemode.GetInstance();

		return m_Gamemode && m_Gamemode.m_GamemodeState == COA_EGamemodeState.AAR && !m_Gamemode.m_bIsInEndCredits;
	}

	//------------------------------------------------------------------------------------------------
    //! Only update when something actually changes
    void UpdateSpectatorChecksIfNeeded()
    {
        if (!m_CRFMenuManager)
            return;

        // Check if channels have changed, or the AAR voice mode switched on/off
        bool aarVoiceActive = IsAARVoiceActive();
        if (m_iLastChannelChanges != m_CRFMenuManager.m_iChannelChanges || aarVoiceActive != m_bLastAARVoiceActive)
        {
            m_iLastChannelChanges = m_CRFMenuManager.m_iChannelChanges;
            m_bLastAARVoiceActive = aarVoiceActive;
            UpdateSpectatorChecks(); // Only update when channels change
        }
    }
    
	//------------------------------------------------------------------------------------------------
    //! Cache individual player spectator status - update on demand
    bool SpectatorCheck(int playerId)
    {
        // Check if we have cached data
        if (m_SpectatorChecks.Contains(playerId))
            return m_SpectatorChecks.Get(playerId);
        
        // Not cached - calculate and cache it
        bool result = CalculateSpectatorStatus(playerId);
        m_SpectatorChecks.Insert(playerId, result);
        return result;
    }
    
	//------------------------------------------------------------------------------------------------
    bool CalculateSpectatorStatus(int playerId)
    {
        bool isPlayerAndClientSpec = IsOtherPlayerSpectator(playerId);
        bool inSameChannel = InSameChannel(playerId);
        return isPlayerAndClientSpec && inSameChannel;
    }
	
	//------------------------------------------------------------------------------------------------
	void UpdateSpectatorChecks()
	{
		m_SpectatorChecks.Clear();
		array<int> playerIds = {};
		m_PlayerManager.GetPlayers(playerIds);
		foreach (int playerId: playerIds)
		{
			bool isPlayerAndClientSpec = IsOtherPlayerSpectator(playerId);
			bool inSameChannel = InSameChannel(playerId);

			if (isPlayerAndClientSpec && inSameChannel)
				m_SpectatorChecks.Insert(playerId, true);
			else
				m_SpectatorChecks.Insert(playerId, false);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsOtherPlayerSpectator(int playerId)
	{
		if (playerId == 0)
			return false;

		// Everyone talks through channels during the AAR (see IsAARVoiceActive)
		if (IsAARVoiceActive())
			return true;

		if (!m_FactionManager || !m_FactionManager.GetPlayerFaction(playerId))
			return false;
		
		string otherFactionKey = m_FactionManager.GetPlayerFaction(playerId).GetFactionKey();
		bool isOtherPlayerSpec = otherFactionKey == "SPEC" || m_VONGameModeComponent.IsPlayerListening(playerId);
		
		return isOtherPlayerSpec;
	}
	
	//------------------------------------------------------------------------------------------------
	bool InSameChannel(int playerId)
	{
		if (!m_CRFMenuManager)
			return false;
		
		int frequency = m_CRFMenuManager.GetChannel(m_PlayerController.GetPlayerId());
		int senderFrequency = m_CRFMenuManager.GetChannel(playerId);
		
		//Are we deafened
		if (frequency == 0)
			return false;
		
		if (frequency == senderFrequency)
			return true;
		else
			return false;
	}
	
		
	//------------------------------------------------------------------------------------------------
	override bool IsSameLanguage(int localPlayerId, int transmissionPlayerId)
	{
		// The mission is over - everyone understands everyone in the AAR
		if (IsAARVoiceActive())
			return true;

		Faction localFaction = m_FactionManager.GetPlayerFaction(localPlayerId);
		Faction transmissionFaction = m_FactionManager.GetPlayerFaction(transmissionPlayerId);

		// When faction separation is active, players who are spectating or waiting in the respawn
		// screen must not understand players from other factions, regardless of spectator status.
		// This mirrors the volume silencing already done in ComputeSpectatorLR.
		if (m_Gamemode && m_Gamemode.m_bSeperateSpectatorsByFaction)
		{
			bool localIsSpec = localFaction && localFaction.GetFactionKey() == "SPEC";
			bool localIsListening = m_VONGameModeComponent.IsPlayerListening(localPlayerId);

			if (localIsSpec || localIsListening)
			{
				if (!m_SlottingManager)
					m_SlottingManager = COA_SlottingManager.GetInstance();

				Faction localSlotFaction = m_SlottingManager.GetPlayerSlotFaction(localPlayerId);
				Faction transSlotFaction = m_SlottingManager.GetPlayerSlotFaction(transmissionPlayerId);

				if (localSlotFaction != transSlotFaction)
					return false;
			}
		}

		//If Local player is a spectator it will always be in the same language
		if (localFaction.GetFactionKey() == "SPEC")
			return true;
		
		//If the player or transmitting player is a zeus all should understand
		IEntity player = m_PlayerController.GetControlledEntity();
		IEntity otherPlayer = m_PlayerManager.GetPlayerControlledEntity(transmissionPlayerId);
		if (player)
			if (player.GetPrefabData().GetPrefabName()  == "{15992AA89FF4475A}Prefabs/Characters/!GS_Characters/Special/COA_Zeus.et")
				return true;
		
		if (otherPlayer)
			if (otherPlayer.GetPrefabData().GetPrefabName()  == "{15992AA89FF4475A}Prefabs/Characters/!GS_Characters/Special/COA_Zeus.et")
				return true;
		
		return super.IsSameLanguage(localPlayerId, transmissionPlayerId);
	}

	//------------------------------------------------------------------------------------------------
	override void ActivateCVON(CVON_EVONTransmitType transmitType = CVON_EVONTransmitType.NONE)
	{
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu)
			if(topMenu.IsInherited(COA_Outro))
				return;
		
		super.ActivateCVON(transmitType);
	}
	
	//------------------------------------------------------------------------------------------------
	override bool ShouldMuffleAudio(IEntity senderEntity, int playerId = 0, out int loweredDecibles = 0)
	{
		if (SpectatorCheck(playerId))
			return false;

		// Spectator or Game Master camera: it isn't inside any building or vehicle
		if (IsListeningFromCamera())
			return false;
		
		IEntity player = m_PlayerController.GetLocalControlledEntity();
		if (!player)
			return false;
		
		if (!senderEntity)
			return false;
		
		if (CanPlayerSeeSender(senderEntity, player))
			return false;
		
		IEntity receiverBuilding;
		IEntity senderBuilding;
		bool isSenderInBuilding = IsInBuildingOrVehicle(senderEntity, senderBuilding);
		bool isPlayerInBuilding = IsInBuildingOrVehicle(player, receiverBuilding);
		if (CheckIfInSameVehicle(senderEntity, player))
			return false;
		
		if (!isSenderInBuilding && !isPlayerInBuilding)
			return false;
		
		if (isPlayerInBuilding != isSenderInBuilding)
		{
			loweredDecibles = CVON_DB_ATTEN_BUILDING;
			return true;
		}
		
		if (senderBuilding != receiverBuilding)
		{
			loweredDecibles = CVON_DB_ATTEN_BUILDING * 2;
			return true;
		}
		float top;
		float bottom;
		
		DetermineHearingWindow(player, top, bottom);
		vector senderOrigin = GetHeadHeight(senderEntity);
		if (senderOrigin[1] > top || senderOrigin[1] < bottom)
		{
			loweredDecibles = CVON_DB_ATTEN_BUILDING;
			return true;
		}
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	override void ComputeSpectatorLR(int playerId, out float outLeft = 1, out float outRight = 1, out int silencedDecibels = 0)
	{
		if (COA_Gamemode.GetInstance() && COA_Gamemode.GetInstance().m_bIsInEndCredits)
		{
			outLeft = 0;
			outRight = 0;
			return;
		}
		
		if (SpectatorCheck(playerId))
		{
			if (!m_RespawnManager)
				m_RespawnManager = COA_RespawnManager.GetInstance();
			
			if (!m_Gamemode)
				m_Gamemode = COA_Gamemode.GetInstance();
			
			if (!m_Gamemode)
				return;
			
			//Mutes spectator audio coming in if its from another player not in our faction
			if (m_RespawnManager && m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
			{
				if (m_RespawnManager.m_bCurrentRespawnEnabled && m_Gamemode.m_bSeperateSpectatorsByFaction)
				{
					Faction otherFaction = m_SlottingManager.GetPlayerSlotFaction(playerId);
					Faction localFaction = m_SlottingManager.GetPlayerSlotFaction(m_PlayerController.GetPlayerId());
					
					if (otherFaction != localFaction)
					{
						outLeft = 0;
						outRight = 0;
						return;
					}
				}
			}
			outLeft = 1;
			outRight = 1;
			silencedDecibels = 0;
			return;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! CVON runs the voice update (membership, gains, plugin files, heartbeat); the Lobby only says who
	//! is on a spectator channel and when in-world voice is off, through CVON's hooks below.
	override void UpdateMembership()
	{
		UpdateSpectatorChecksIfNeeded();
		super.UpdateMembership();
	}

	//------------------------------------------------------------------------------------------------
	//! Spectating: hear the world around the spectator camera (free camera or a followed player's view),
	//! not around the hidden spectator character
	override bool IsListeningFromCamera()
	{
		return super.IsListeningFromCamera() || COA_EntityHelper.IsSpectator();
	}

	//------------------------------------------------------------------------------------------------
	//! Spectators in our channel: gains come from ComputeSpectatorLR
	override bool IsSpectatorTalker(int playerId)
	{
		return SpectatorCheck(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Both of us spectating in the same channel: heard at any distance, even when not streamed in
	override bool HearOverChannel(int playerId)
	{
		if (!m_PlayerController)
			return false;

		return SpectatorCheck(m_PlayerController.GetPlayerId()) && SpectatorCheck(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! End credits: silence. AAR: no in-world voice, only players in your channel are heard.
	override bool OverrideTalkerGains(int playerId, out float left, out float right)
	{
		if (COA_Gamemode.GetInstance() && COA_Gamemode.GetInstance().m_bIsInEndCredits)
		{
			left = 0;
			right = 0;
			return true;
		}

		if (SpectatorCheck(playerId))
		{
			left = 1;
			right = 1;
			return true;
		}

		if (IsAARVoiceActive())
		{
			left = 0;
			right = 0;
			return true;
		}
		return false;
	}
}
