//------------------------------------------------------------------------------------------------
//! Generic-entity counterpart to COA_PlayerHelper.AssignCharacterToPlayer, which cannot be reused
//! directly for GM-placed AI because its parameter is typed COA_PlayerCharacter - a GM-placed
//! character comes from an arbitrary editor-catalog faction prefab, not this mod's own
//! player-character prefab, so that cast would always fail. This is a type-generalized copy of the
//! same proven logic (see COA_PlayerHelper.c's own doc comment for why SCR_PossessSpawnData +
//! SCR_RespawnComponent.RequestSpawn is used instead of calling SetPossessedEntity/
//! SetInitialMainEntity directly: the direct call skips SCR_BaseGameMode.OnPlayerSpawnFinalize_S
//! and therefore every vanilla listener/component that hooks it).
class COA_PossessionHelper
{
	//------------------------------------------------------------------------------------------------
	//! \param[in] playerController Player controller to hand the character over to
	//! \param[in] character Pre-existing character entity to possess (not spawned by this call)
	static void PossessExistingEntity(notnull SCR_PlayerController playerController, notnull SCR_ChimeraCharacter character)
	{
		if (playerController.GetControlledEntity() != character && RequestPossessSpawn(playerController, character))
			return;

		playerController.SetInitialMainEntity(character);
	}

	//------------------------------------------------------------------------------------------------
	//! \return true if the request was accepted; false if the caller should fall back to a direct
	//!         SetInitialMainEntity (respawn component missing, spawn data invalid, request rejected)
	protected static bool RequestPossessSpawn(notnull SCR_PlayerController playerController, notnull SCR_ChimeraCharacter character)
	{
		SCR_RespawnComponent respawnComponent = SCR_RespawnComponent.Cast(playerController.FindComponent(SCR_RespawnComponent));
		if (!respawnComponent)
			return false;

		SCR_PossessSpawnData spawnData = SCR_PossessSpawnData.FromEntity(character);
		if (!spawnData || !spawnData.IsValid())
			return false;

		// The entity already exists and is fully spawned/streamed - nothing to preload.
		spawnData.SetSkipPreload(true);

		return respawnComponent.RequestSpawn(spawnData);
	}
}
