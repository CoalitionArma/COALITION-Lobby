class COA_RallyPointClass : GenericEntityClass
{
}

class COA_RallyPoint: GenericEntity
{	
	ref COA_SpawnPointData m_SpawnPointSettings = new COA_SpawnPointData();
	
	protected int m_iLocallyStoredId;
	
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

		if (COA_Gamemode.GetInstance().m_bSpawnBlockEnabled)
			GetGame().GetCallqueue().CallLater(UpdateFlagProximity, (int)(COA_Gamemode.GetInstance().m_iSpawnBlockFrequancy * 1000), true);
	}
	
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
	
	//------------------------------------------------------------------------------------------------
	void ~COA_RallyPoint()
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