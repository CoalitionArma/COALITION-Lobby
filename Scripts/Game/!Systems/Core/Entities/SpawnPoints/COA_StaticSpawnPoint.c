class COA_StaticSpawnPointClass : GenericEntityClass
{
}

class COA_StaticSpawnPoint: GenericEntity
{
	[Attribute("0", "auto", "Is this a default respawn point for its faction. Flag several to build a spawn pool: normally the first one is used, but if the faction's 'Randomize Spawnpoints' setting is enabled, one is picked at random from the flagged points at each initial/automatic spawn", category: "CRF Spawn Point Settings")]
	bool m_bIsDefaultSpawn;
	
	[Attribute(category: "CRF Spawn Point Settings")]
	ref COA_SpawnPointData m_SpawnPointSettings;
	
	protected int m_iLocallyStoredId;
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		// Only server should register respawn points
		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;
		
		if (COA_RespawnManager.GetInstance())
			COA_RespawnManager.GetInstance().RegisterRespawnPoint(m_SpawnPointSettings, this);
	};
	
	//------------------------------------------------------------------------------------------------
	int GetLocalSpawnPointId()
	{
		return m_iLocallyStoredId;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetLocalSpawnPointId(int spawnPointId)
	{
		m_iLocallyStoredId = spawnPointId;
	}
	
	//------------------------------------------------------------------------------------------------
	void ~COA_StaticSpawnPoint()
	{
		// Only server should unregister respawn points
		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;
		
		if (COA_RespawnManager.GetInstance())
			COA_RespawnManager.GetInstance().UnRegisterRespawnPoint(m_iLocallyStoredId);
	}
};