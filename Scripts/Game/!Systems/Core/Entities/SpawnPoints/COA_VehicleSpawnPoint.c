class COA_VehicleSpawnPointClass : VehicleClass
{
}

class COA_VehicleSpawnPoint: Vehicle
{
	[Attribute("0", "auto", "Is this the default respawn point to be selected", category: "CRF Spawn Point Settings")]
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
	void ~COA_VehicleSpawnPoint()
	{
		// Only server should unregister respawn points
		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;
		
		if (COA_RespawnManager.GetInstance())
			COA_RespawnManager.GetInstance().UnRegisterRespawnPoint(m_iLocallyStoredId);
	}
};