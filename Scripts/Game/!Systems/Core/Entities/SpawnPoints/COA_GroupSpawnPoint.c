class COA_GroupSpawnPointClass : GenericEntityClass
{
}

class COA_GroupSpawnPoint: GenericEntity
{
	[Attribute("1-1", "auto", "Callsign for the group to spawn at this point", category: "CRF Spawn Point Settings")]
	string m_sCallsignOfGroupToSpawn;
	
	[Attribute(category: "CRF Spawn Point Settings")]
	ref COA_SpawnPointData m_SpawnPointSettings;
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		// Only server should register respawn points
		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;
		
		m_SpawnPointSettings.SetSpawnPointToTemp();
		
		if (COA_RespawnManager.GetInstance())
			COA_RespawnManager.GetInstance().RegisterRespawnPoint(m_SpawnPointSettings, this);
	};
};