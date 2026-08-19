class COA_StaticSpawnPointClass : GenericEntityClass
{
}

class COA_StaticSpawnPoint: GenericEntity
{
	[Attribute("0", "auto", "Is this a default respawn point for its faction. Flag several to build a spawn pool: normally the first one is used, but if the faction's 'Randomize Spawnpoints' setting is enabled, one is picked at random from the flagged points at each initial/automatic spawn", category: "Spawn Point Settings")]
	bool m_bIsDefaultSpawn;
	
	[Attribute(category: "Spawn Point Settings")]
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

		if (COA_Gamemode.GetInstance().m_bSpawnBlockEnabled)
			GetGame().GetCallqueue().CallLater(UpdateFlagProximity, (int)(COA_Gamemode.GetInstance().m_iSpawnBlockFrequancy * 1000), true);
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

		if (COA_Gamemode.GetInstance().m_bSpawnBlockEnabled)
			GetGame().GetCallqueue().Remove(UpdateFlagProximity);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateFlagProximity()
	{
		array<vector> enemyPositions = {};
		CollectFactionCharacterPositions(m_SpawnPointSettings.GetSpawnPointFaction(), enemyPositions);

		int blockRadius = COA_Gamemode.GetInstance().m_iSpawnBlockRadius;

		float radiusSq = blockRadius * blockRadius;

		vector flagOrigin = this.GetOrigin();

		bool block = false;

		foreach (vector enemyPosition : enemyPositions)
		{
			if (vector.DistanceSq(flagOrigin, enemyPosition) <= radiusSq)
			{
				block = true;
				break;
			}
		}

		if (m_SpawnPointSettings.GetIsSpawnPointBlocked() == block)
			return;

		m_SpawnPointSettings.SetSpawnPointBlocked(block);

		COA_RplBroadcastManager.GetInstance().SpawnPointBlockChanged(m_SpawnPointSettings.GetSpawnPointId(), block);
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectFactionCharacterPositions(int faction, notnull array<vector> positions)
	{
		FactionKey factionKey = SCR_Enum.GetEnumName(COA_EFactions, faction);
		array<COA_PlayerCharacter> characters = COA_Gamemode.GetInstance().GetActiveCharacters();

		foreach (COA_PlayerCharacter character : characters)
		{
			if (COA_EntityHelper.IsSpectator(character))
				continue;

			// Skip the dead so corpses do not keep a flag locked down.
			SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(character.FindComponent(SCR_DamageManagerComponent));
			if (damageManager && damageManager.GetState() == EDamageState.DESTROYED)
				continue;

			FactionKey characterFactionKey = GetCharacterFactionKey(character);
			if (characterFactionKey == factionKey || characterFactionKey == "")
				continue;

			positions.Insert(character.GetOrigin());
		}
	}

	//------------------------------------------------------------------------------------------------
	static FactionKey GetCharacterFactionKey(IEntity entity)
	{
		if (!entity)
			return "";

		FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (!affiliation)
			return "";

		Faction faction = affiliation.GetAffiliatedFaction();
		if (!faction)
			return "";

		return faction.GetFactionKey();
	}
};