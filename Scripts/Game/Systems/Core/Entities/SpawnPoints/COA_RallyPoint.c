class COA_RallyPointClass : COA_StaticSpawnPointClass
{
}

class COA_RallyPoint: COA_StaticSpawnPoint
{	
	protected SCR_AIGroup m_group;
	protected Faction m_faction;

	//------------------------------------------------------------------------------------------------
	void SetupRallyPoint()
	{
		array<string> factions = new array<string>();
		SCR_Enum.GetEnumNames(COA_EFactions, factions);
		
		SCR_CampaignBuildingCompositionComponent compositionComponent = SCR_CampaignBuildingCompositionComponent.Cast(this.FindComponent(SCR_CampaignBuildingCompositionComponent));
		if (!compositionComponent)
			return;
		
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return;
		
		int playerId = compositionComponent.GetBuilderId();
		if (playerId == -1)
			return;

		m_group = slottingManager.GetPlayerSlotGroup(playerId);
		if (!m_group)
			return;
		
		m_faction = slottingManager.GetPlayerSlotFaction(playerId);
		if (!m_faction)
			return;
		
		string groupCustomName = m_group.GetCustomNameWithOriginal();
		
		RemovePreviousRallyPoint();
		
		m_SpawnPointSettings.SetSpawnPointName(groupCustomName + " RP");
		m_SpawnPointSettings.SetSpawnPointFaction(factions.Find(m_faction.GetFactionKey()));
		m_SpawnPointSettings.SetRestrictedToGroup(groupCustomName);
		m_SpawnPointSettings.SetSpawnPointActive(true);
		
		if (COA_RespawnManager.GetInstance())
			COA_RespawnManager.GetInstance().RegisterRespawnPoint(m_SpawnPointSettings, this);

		if (m_SpawnPointSettings.m_bSpawnBlockEnabled)
			GetGame().GetCallqueue().CallLater(UpdateFlagProximity, (int)(m_SpawnPointSettings.GetSpawnPointBlockedFrequency() * 1000), true);
	}
	
	//------------------------------------------------------------------------------------------------
	private void RemovePreviousRallyPoint()
	{
		array<COA_SpawnPointData> factionRespawnPoints = COA_RespawnManager.GetInstance().GetFactionSpawnpoints(m_faction.GetFactionKey(), m_group);
		
		foreach(COA_SpawnPointData spawnPointData : factionRespawnPoints)
		{ 
			IEntity entity = COA_EntityHelper.GetEntityFromRplId(spawnPointData.GetSpawnPointEntity());
			if (!entity)
				continue;
			
			COA_RallyPoint rally = COA_RallyPoint.Cast(entity);
			if (!rally)
				continue;
			
			if (rally.m_group == m_group)			
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
		}
	}
};