class COA_RallyPointClass : COA_StaticSpawnPointClass
{
}

class COA_RallyPoint: COA_StaticSpawnPoint
{	
	protected SCR_AIGroup m_group;
	protected Faction m_faction;
	protected AudioHandle m_AudioSystem;

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		vector transform[4];
		owner.GetWorldTransform(transform);

		AudioSystem.PlayEvent("{EFFB3D8065A486E3}", "SOUND_DEPLOY", transform);
		m_AudioSystem = AudioSystem.PlayEvent("{EFFB3D8065A486E3}", "SOUND_DEPLOYED_RADIO_CHATTER", transform);

		if (m_SpawnPointSettings)
			return;

		m_SpawnPointSettings = new COA_SpawnPointData();
	};

	//------------------------------------------------------------------------------------------------
	void SetupRallyPoint(int playerId)
	{
		array<string> factions = new array<string>();
		SCR_Enum.GetEnumNames(COA_EFactions, factions);
		
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return;

		m_group = slottingManager.GetPlayerSlotGroup(playerId);
		if (!m_group)
			return;
		
		m_faction = slottingManager.GetPlayerSlotFaction(playerId);
		if (!m_faction)
			return;

		RemovePreviousRallyPoint();
		
		string groupCustomName = m_group.GetCustomNameWithOriginal();
		
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
			
			COA_RallyPoint rallyPoint = COA_RallyPoint.Cast(entity);
			if (!rallyPoint)
				continue;

			if (rallyPoint.m_group != m_group)
				return;

			rallyPoint.DestroyRallPoint();
		}
	}

	//------------------------------------------------------------------------------------------------
	void DestroyRallPoint()
	{
		AudioSystem.TerminateSound(this.m_AudioSystem);

		SCR_EntityHelper.DeleteEntityAndChildren(this);
	}
};