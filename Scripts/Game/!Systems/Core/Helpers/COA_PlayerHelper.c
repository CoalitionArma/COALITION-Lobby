class COA_PlayerHelper
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
	//! Assign faction to player controller
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
	//! Assign character entity to player controller.
	//!
	//! Routes through vanilla's possess-spawn pipeline where possible, because that is the only path
	//! that reaches SCR_BaseGameMode.OnPlayerSpawnFinalize_S - which is in turn the only place
	//! m_OnPlayerSpawned is ever invoked, and the only place every registered SCR_BaseGameModeComponent
	//! gets its OnPlayerSpawnFinalize_S callback. Calling SetInitialMainEntity() directly possesses
	//! the entity correctly but silently skips all of that, so ~20 vanilla listeners (callsigns,
	//! commanding, SCR_AIGroup's gadget-loaded hook, seizing) and 9 vanilla components (spawn
	//! protection, XP, data collector, wave respawn timer, perceived faction, notifications) never
	//! fire - along with any third-party mod hooking either.
	//!
	//! The earlier attempt at RequestSpawn failed because of the guard in
	//! SCR_SpawnHandlerComponent.AssignEntity_S:
	//!     IEntity previous = playerController.GetControlledEntity();
	//!     if (previous == entity) return false;   // "Already controlling!"
	//! On a reconnect or re-initialization the player already controls the target entity, so the
	//! request aborted and finalization never ran - which is the stats-tracking failure that was
	//! observed. That case is now detected up front and skipped rather than requested, so the guard
	//! is never hit.
	//!
	//! Callers are always responsible for calling NotifyPlayerSpawned after this method returns.
	//! \param[in] playerController Player Controller to assign character to
	//! \param[in] character Character to assign to player
	static void AssignCharacterToPlayer(SCR_PlayerController playerController, COA_PlayerCharacter character)
	{
		if (!playerController || !character)
			return;

		// Already controlling this exact entity: there is nothing to assign, and requesting a spawn
		// would trip the AssignEntity_S guard above. Fall through to the direct call, which is
		// effectively idempotent here (re-transfers ownership, re-invokes m_OnPossessed) and matches
		// the behaviour this path has always had on re-init.
		if (playerController.GetControlledEntity() != character && RequestPossessSpawn(playerController, character))
			return;

		playerController.SetInitialMainEntity(character);
	}

	//------------------------------------------------------------------------------------------------
	//! Hand the character over via SCR_PossessSpawnData, the vanilla path for "this entity already
	//! exists, give it to the player". This is the same call SCR_SpawnLogic and the Game Master make
	//! server-side (SCR_SpawnLogic.c:433, SCR_PlacingEditorComponent.c:916), and RequestRespawn is
	//! sealed and authority-aware, so it is safe to call from the server.
	//! \return true if the request was accepted; false if the caller should fall back to a direct
	//!         SetInitialMainEntity (respawn component missing, spawn lock held, handler unresolved)
	protected static bool RequestPossessSpawn(notnull SCR_PlayerController playerController, notnull COA_PlayerCharacter character)
	{
		SCR_RespawnComponent respawnComponent = SCR_RespawnComponent.Cast(playerController.FindComponent(SCR_RespawnComponent));
		if (!respawnComponent)
			return false;

		SCR_PossessSpawnData spawnData = SCR_PossessSpawnData.FromEntity(character);
		if (!spawnData || !spawnData.IsValid())
			return false;

		// The entity is already spawned and streamed, so there is nothing to preload - the Game
		// Master sets this for the same reason when possessing an entity it just placed.
		spawnData.SetSkipPreload(true);

		return respawnComponent.RequestSpawn(spawnData);
	}
}