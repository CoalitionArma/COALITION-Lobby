class COA_ForwardDeployRequest
{
	int m_iPlayerId;
	vector m_vTransform;
}

class COA_ForwardDeployManagerClass : ScriptComponentClass {}

class COA_ForwardDeployManager : ScriptComponent
{
	protected ref array<IEntity> m_aForwardDeployZones = {};
	protected ref array<ref COA_ForwardDeployRequest> m_aForwardDeployRequests = {};

	//Scratch state for the enemy-proximity query in IsPositionNearEnemy() - QueryEntitiesBySphere callbacks can't take captured locals.
	protected Faction m_CombatCheckFriendlyFaction;
	protected bool m_bCombatCheckEnemyFound;

//=============================================================================================================================================================================================================================================================================================================================================================
//	 ONFRAME METHOD
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//Needed so when we teleport players/vehicles the aren't spawning on top of each other.
	float m_fBuffer = 0;
	override void EOnFrame(IEntity owner, float timeSlice)
	{
	    super.EOnFrame(owner, timeSlice);
	    m_fBuffer += timeSlice;
	    if (m_fBuffer > 0.1)
	    {
	        m_fBuffer = 0;
	        if (m_aForwardDeployRequests.Count() > 0)
	        {
	            COA_ForwardDeployRequest request = m_aForwardDeployRequests.Get(0);
	            if (request)
	            {
	                PerformForwardDeploy(request.m_iPlayerId, request.m_vTransform);
	                m_aForwardDeployRequests.RemoveOrdered(0);
	            }
	        }
	        if (m_aForwardDeployRequests.Count() == 0)
	            ClearEventMask(owner, EntityEvent.FRAME);
	    }
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 FORWARD DEPLOY ZONE METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void AddForwardDeployZone(IEntity entity)
	{
		if (entity && !m_aForwardDeployZones.Contains(entity))
			m_aForwardDeployZones.Insert(entity);
	}
	
	//------------------------------------------------------------------------------------------------
	array<IEntity> GetForwardDeployZones()
	{
		return m_aForwardDeployZones;
	}
		
	//------------------------------------------------------------------------------------------------
	void DeleteAllForwardDeployZones()
	{
		foreach(IEntity zone : m_aForwardDeployZones)
		{
			if(zone)
				SCR_EntityHelper.DeleteEntityAndChildren(zone);
		}
		
		m_aForwardDeployZones.Clear();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 FORWARD DEPLOY METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void CreateForwardDeployRequest(int playerId, vector transform)
	{
		ref COA_ForwardDeployRequest request = new COA_ForwardDeployRequest();
		request.m_iPlayerId = playerId;
		request.m_vTransform = transform;
		m_aForwardDeployRequests.Insert(request);
		SetEventMask(GetOwner(), EntityEvent.FRAME);
	}
	
	//------------------------------------------------------------------------------------------------
	void PerformForwardDeploy(int playerId, vector transform)
	{
		vector initialSpawnLocation;
		SCR_WorldTools.FindEmptyTerrainPosition(initialSpawnLocation, transform, 10);
		vector params[4];
		params[3] = initialSpawnLocation;
		SCR_TerrainHelper.OrientToTerrain(params, GetGame().GetWorld(), true);
		vector finalSpawnLocation;
		SCR_TerrainHelper.SnapToGeometry(finalSpawnLocation, params[3], null);
		params[3] = finalSpawnLocation;
		SCR_Global.TeleportPlayer(playerId, finalSpawnLocation, SCR_EPlayerTeleportedReason.NONE);
		COA_RplBroadcastManager.GetInstance().ForwardDeployUpdate(finalSpawnLocation, playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Whether a player may forward deploy their group. Decided by slot rather than by the vanilla
	//! group leader, which isn't reliably the squad leader: the group is joined before characters
	//! exist at round start, so CRF's promote-the-squad-leader step often can't run. In order:
	//!  - the group's squad lead slot (also platoon lead / company command; not the medical officer,
	//!    who shares that slot type)
	//!  - the group's team leader, when no squad lead slot in the group is filled
	//!  - the vanilla group leader, for groups without leader slots
	//! Used by both the map menu option and the server's request check.
	static bool CanLeadForwardDeploy(int playerId, SCR_AIGroup group)
	{
		if (playerId <= 0 || !group)
			return false;

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		COA_SlotData playerSlot;
		if (slottingManager)
			playerSlot = slottingManager.GetPlayerSlotData(playerId);

		if (playerSlot)
		{
			if (IsSquadLeadSlot(playerSlot))
				return true;

			if (playerSlot.GetSlotType() == COA_ESlotType.TEAM_LEADER && !HasFilledSquadLeadSlot(slottingManager, group))
				return true;
		}

		return group.IsPlayerLeader(playerId);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsSquadLeadSlot(COA_SlotData slotData)
	{
		return slotData.GetSlotType() == COA_ESlotType.SQUAD_LEADER && slotData.GetSlotRole() != COA_EGearRole.MEDICAL_OFFICER;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HasFilledSquadLeadSlot(COA_SlottingManager slottingManager, SCR_AIGroup group)
	{
		RplId groupId;
		if (!COA_ReplicationHelper.GetRplId(group, groupId))
			return false;

		foreach (int slotId : slottingManager.GetAllSlotIDsForGroup(groupId))
		{
			COA_SlotData slotData = slottingManager.GetSlotData(slotId);
			if (slotData && slotData.GetSlotCurrentPlayerId() > 0 && IsSquadLeadSlot(slotData))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Checks if there are any forward deploy zones active for this faction.
	//! These are deleted and then removed from m_aVisibleForFactions on safestart ending.
	//! \param[in] factionKey is the faction of the group you are checking.
	//! \return True if there is an active forward deploy zone.
	bool IsForwardDeployActive(string factionKey)
	{
		if (m_aForwardDeployZones.Count() == 0)
			return false;
		
		bool isActive = false;
		foreach (IEntity zone: m_aForwardDeployZones)
		{
			COA_GameBorder border = COA_GameBorder.Cast(zone);
			if (!border || !border.m_aVisibleForFactions.Contains(factionKey))
				continue;
			
			isActive = true;
			break;
		}
		
		return isActive;
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 COMBAT CHECK
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Checks whether any entity hostile to friendlyFaction is within checkRadius of position.
	//! Used to deny a JIP forward deploy request when the target unit is in/near contact with the enemy.
	//! \param[in] position World position to check around (the unit being deployed to).
	//! \param[in] friendlyFaction Faction of the player requesting forward deploy.
	//! \param[in] checkRadius Radius in meters to search for hostile entities.
	//! \return True if a hostile entity was found nearby.
	bool IsPositionNearEnemy(vector position, Faction friendlyFaction, float checkRadius = 100)
	{
		if (!friendlyFaction)
			return false;

		m_CombatCheckFriendlyFaction = friendlyFaction;
		m_bCombatCheckEnemyFound = false;

		GetGame().GetWorld().QueryEntitiesBySphere(position, checkRadius, FilterEnemyNearPosition, null, EQueryEntitiesFlags.DYNAMIC | EQueryEntitiesFlags.WITH_OBJECT);

		return m_bCombatCheckEnemyFound;
	}

	//------------------------------------------------------------------------------------------------
	protected bool FilterEnemyNearPosition(IEntity ent)
	{
		if (m_bCombatCheckEnemyFound)
			return true;

		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(ent);
		Vehicle vehicle = Vehicle.Cast(ent);

		SCR_DamageManagerComponent damageManager;
		FactionAffiliationComponent factionAffiliation;

		if (character)
		{
			damageManager = character.GetDamageManager();
			factionAffiliation = character.m_pFactionComponent;
		}
		else if (vehicle)
		{
			damageManager = vehicle.GetDamageManager();
			factionAffiliation = vehicle.GetFactionAffiliation();
		}
		else
		{
			return true;
		}

		if (damageManager && damageManager.GetState() == EDamageState.DESTROYED)
			return true;

		if (!factionAffiliation)
			return true;

		Faction otherFaction = factionAffiliation.GetAffiliatedFaction();
		if (!otherFaction)
			return true;

		if (m_CombatCheckFriendlyFaction.IsFactionEnemy(otherFaction))
			m_bCombatCheckEnemyFound = true;

		return true;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	static protected COA_ForwardDeployManager m_sInstance;
	void COA_ForwardDeployManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	static COA_ForwardDeployManager GetInstance()
	{
		return m_sInstance;
	}
}