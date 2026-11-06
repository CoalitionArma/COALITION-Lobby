modded class SCR_PossessSpawnHandlerComponent
{
	//------------------------------------------------------------------------------------------------
	//! 1.9 added a terrain-bounds check on data.GetPosition() to SCR_SpawnHandlerComponent.ValidateData_S,
	//! but SCR_PossessSpawnData doesn't override GetPosition(), so it always checks <0 0 0>. That passes
	//! on maps whose corner is at or below sea level (Everon, Chernarus) and fails everywhere else
	//! (Denali): the request is dropped as CANNOT_VALIDATE with nothing logged, and since
	//! COA_PlayerHelper.RequestPossessSpawn already returned true, the player is never given their
	//! character.
	//! The check guards a requested spawn position. Possession takes an entity that already exists
	//! wherever it is, so this keeps vanilla's possess checks and leaves the position one out. Enforce
	//! can't call past the immediate super, so the base data check is repeated here instead.
	protected override bool ValidateData_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnData data)
	{
		SCR_PossessSpawnData possessData = SCR_PossessSpawnData.Cast(data);
		if (!possessData || !possessData.IsValid())
			return false;

		return Replication.FindItem(possessData.GetRplId()) != null;
	}

	//------------------------------------------------------------------------------------------------
	override void OnFinalizeDone_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnData data, IEntity entity)
	{
		// Assign possessed entity's faction to the player
		PlayerController pc = requestComponent.GetPlayerController();
		SCR_PlayerFactionAffiliationComponent playerFactionComp = SCR_PlayerFactionAffiliationComponent.Cast(pc.FindComponent(SCR_PlayerFactionAffiliationComponent));
		Faction targetFaction = GetFaction(entity);
		if (!targetFaction || targetFaction == playerFactionComp.GetAffiliatedFaction())
			return;

		playerFactionComp.SetAffiliatedFaction(targetFaction);

		// Notify faction manager
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		factionManager.UpdatePlayerFaction_S(playerFactionComp);

		// Notify game mode
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		gameMode.OnPlayerFactionSet_S(playerFactionComp, targetFaction);
	}

	//------------------------------------------------------------------------------------------------
	protected Faction GetFaction(IEntity entity)
	{
		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (!facComp)
			return null;
		
		return facComp.GetAffiliatedFaction();
	}
}