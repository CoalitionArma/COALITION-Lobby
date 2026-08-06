class COA_InitializationHelper
{
	//------------------------------------------------------------------------------------------------
	//! Remove player from their current group if any
	//! \param[in] playerId ID of the player to remove from group
	static void RemovePlayerFromCurrentGroup(int playerId)
	{
		SCR_AIGroup currentGroup = SCR_GroupsManagerComponent.GetInstance().GetPlayerGroup(playerId);
		if (currentGroup)
			currentGroup.RemovePlayer(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Assign character entity to player controller.
	//! Lobby pre-spawns all characters before calling InitilizePlayer, so SetInitialMainEntity is
	//! always used directly. The RequestSpawn path was previously attempted here but caused
	//! intermittent stats-tracking failures: the vanilla AssignEntity_S guard cancels the entire
	//! spawn finalization (and therefore never fires OnPlayerSpawnFinalize_S) whenever the player
	//! already controls the target entity — which happens on reconnects and re-initializations.
	//! Callers are always responsible for calling NotifyPlayerSpawned after this method returns.
	//! \param[in] playerController Player Controller to assign character to
	//! \param[in] character Character to assign to player
	static void AssignCharacterToPlayer(SCR_PlayerController playerController, COA_PlayerCharacter character)
	{	
		playerController.SetInitialMainEntity(character);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Assign faction to the player controller. AUTHORITY ONLY.
	//! SCR_PlayerFactionAffiliationComponent.RequestFaction() RPCs to the server either way, but it
	//! can be refused by the spawn lock, and on a client the refusal is silent. Running it on the
	//! authority (where the lock state is real) keeps the player-controller faction deterministic
	//! rather than dependent on a request the client can never confirm.
	//! \param[in] playerController Player controller to assign faction to
	//! \param[in] faction Faction to assign
	static void AssignFactionToPlayer(SCR_PlayerController playerController, Faction faction)
	{
		if (!faction || !playerController)
			return;

		SCR_PlayerFactionAffiliationComponent affiliationComponent = SCR_PlayerFactionAffiliationComponent.Cast(
			playerController.FindComponent(SCR_PlayerFactionAffiliationComponent)
		);

		if (affiliationComponent)
			affiliationComponent.RequestFaction(faction);
	}

	//------------------------------------------------------------------------------------------------
	//! Build the player's radio list and tune it. AUTHORITY ONLY — every step here is
	//! server-authoritative and silently does nothing when run on a client:
	//!  - InitializeRadios() fills the server-side copy of m_aRadios, which is what
	//!    TuneFreqDelayWithPresets() iterates and what the VON backend reads.
	//!  - TuneFreqDelayWithPresets() ends in BaseTransceiver.SetFrequency(), which only
	//!    replicates from the authority.
	//!  - InitializeRadioFromServer() sends an RplRcver.Owner RPC, which is a no-op unless
	//!    it originates on the server. It is what tells the owning client to rebuild its own
	//!    radio list, so the client never needs to call any of this itself.
	static void SetupPlayerRadios(int playerId, IEntity entity)
	{
		if (playerId <= 0)
			return;

		// Initialize radios for player
		COA_PlayerController pc = COA_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		COA_PlayerRplToOwnerManager rplToOwnerManager = COA_PlayerRplToOwnerManager.GetInstance();
		// Cache groups manager reference - PERFORMANCE OPTIMIZATION
		SCR_GroupsManagerComponent groupsMan = SCR_GroupsManagerComponent.GetInstance();
		
		// Rebuild the radio list after replacing the player's gear. Tuning before
		// this can access stale entities left behind by ClearEntityGear().
		if (pc)
			pc.InitializeRadios(entity);

		if (groupsMan)
			groupsMan.TuneFreqDelayWithPresets(playerId, entity);
		
		if (rplToOwnerManager && pc)
		{
			rplToOwnerManager.InitializeRadioFromServer();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Sets up radio frequencies based on player group
	//! Configures both group and platoon frequencies
	static void SetupRadioFrequency()
	{
		// Get player's entity
		IEntity entity = SCR_PlayerController.GetLocalMainEntity();
		if (!entity || COA_EntityHelper.IsSpectator(entity))
			return;

		// Find radio in inventory
		array<IEntity> items = {};
		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventoryManager)
			return;

		inventoryManager.GetItems(items);
		IEntity radioEntity;
		foreach (IEntity item : items)
		{
			if (item && item.FindComponent(BaseRadioComponent))
			{
				radioEntity = item;
				break;
			}
		}

		if (!radioEntity)
			return;

		// Get radio components
		BaseRadioComponent radio = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		if (!radio)
			return;

		BaseTransceiver grpTsv = radio.GetTransceiver(0);
		if (!grpTsv)
			return;

		// Get player's group
		SCR_GroupsManagerComponent m_GroupManager = SCR_GroupsManagerComponent.GetInstance();
		if (!m_GroupManager)
			return;

		SCR_AIGroup group = m_GroupManager.GetPlayerGroup(SCR_PlayerController.GetLocalPlayerId());
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		// Set frequency based on group
		if (group)
		{
			grpTsv.SetFrequency(group.GetRadioFrequency());
		}

		// Set up Voice over Network component
		SCR_VONController vc = SCR_VONController.Cast(pc.FindComponent(SCR_VONController));
		SCR_VoNComponent von = SCR_VoNComponent.Cast(entity.FindComponent(SCR_VoNComponent));
		if (!vc || !von)
			return;

		von.SetTransmitRadio(grpTsv);

		// Set up platoon radio if available
		BaseTransceiver pltTsv = radio.GetTransceiver(1);
		if (pltTsv)
			von.SetTransmitRadio(pltTsv);

		vc.PublicResetVON();
		vc.SetVONComponent(von);
	}
}
