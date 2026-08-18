class COA_RoleHelper
{	
	//------------------------------------------------------------------------------------------------
	static COA_EGearRole ResourceToRole(ResourceName roleResource)
	{
		foreach(COA_RoleConfig roleConfig : COA_GearscriptManager.GetRolesConfig().m_RoleConfigs)
			if (roleConfig.m_RoleResource == roleResource)
				return roleConfig.m_Role;

		return COA_EGearRole.RIFLEMAN;
	}

	//------------------------------------------------------------------------------------------------
	static ResourceName RoleToResource(COA_EGearRole role)
	{
		COA_RoleConfig roleConfig = COA_GearscriptManager.GetRolesConfig().FindRoleConfig(role);

		return roleConfig.m_RoleResource;
	}

	//------------------------------------------------------------------------------------------------
	//! Data-driven check for whether a resource is a registered gearscript role prefab.
	//! Checks against COA_RolesConfig.m_RoleResource rather than guessing from the folder
	//! path, so it keeps working if role prefabs are ever renamed/relocated.
	static bool IsValidGearscriptResource(ResourceName resource)
	{
		COA_RolesConfig rolesConfig = COA_GearscriptManager.GetRolesConfig();
		if (!rolesConfig)
			return false;

		foreach (COA_RoleConfig roleConfig : rolesConfig.m_RoleConfigs)
			if (roleConfig.m_RoleResource == resource)
				return true;

		return false;
	};

	// Pulled from the respawn manager, need to find a better solution eventually^tm.
	//------------------------------------------------------------------------------------------------
	static bool IsSquadLeaderRole(IEntity entity)
	{
		ref TIntArray roles = {COA_EGearRole.COMPANY_COMMANDER, COA_EGearRole.PLATOON_LEADER, COA_EGearRole.MEDICAL_OFFICER, COA_EGearRole.SQUAD_LEAD, COA_EGearRole.VEHICLE_LEAD, COA_EGearRole.INDIRECT_LEAD, COA_EGearRole.LOGI_LEAD};
		ResourceName prefab = entity.GetPrefabData().GetPrefabName();
		if (!IsValidGearscriptResource(prefab))
			return false;

		COA_EGearRole role = ResourceToRole(prefab);

		return roles.Contains(role);
	}

	// Pulled from the respawn manager, need to find a better solution eventually^tm.
	//------------------------------------------------------------------------------------------------
	static bool IsTeamLeaderRole(IEntity entity)
	{
		ResourceName prefab = entity.GetPrefabData().GetPrefabName();
		if (!IsValidGearscriptResource(prefab))
			return false;

		COA_EGearRole role = ResourceToRole(prefab);

		return (role == COA_EGearRole.TEAM_LEAD);
	}

	//------------------------------------------------------------------------------------------------
	static bool IsMedicRole(IEntity entity)
	{
		ResourceName prefab = entity.GetPrefabData().GetPrefabName();
		if (!IsValidGearscriptResource(prefab))
			return false;

		COA_EGearRole role = ResourceToRole(prefab);

		return (role == COA_EGearRole.MEDIC || role == COA_EGearRole.MEDICAL_OFFICER);
	}

	//------------------------------------------------------------------------------------------------
	static bool IsCombatEngineerRole(IEntity entity)
	{
		ResourceName prefab = entity.GetPrefabData().GetPrefabName();
		if (!IsValidGearscriptResource(prefab))
			return false;

		COA_EGearRole role = ResourceToRole(prefab);

		return (role == COA_EGearRole.COMBAT_ENGINEER);
	}
}
