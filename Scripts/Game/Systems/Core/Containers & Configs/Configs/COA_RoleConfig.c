[BaseContainerProps(configRoot: true)]
class COA_RolesConfig
{		
	[Attribute()]
	ref array<ref COA_RoleConfig> m_RoleConfigs;
	
	protected ref map<COA_EGearRole, COA_RoleConfig> m_RoleConfigsMap = new map<COA_EGearRole, COA_RoleConfig>;
	
	array<ref COA_RoleConfig> GetRoleConfigArray()
	{
		return m_RoleConfigs;
	}
	
	map<COA_EGearRole,COA_RoleConfig> GetRoleConfigMap()
	{
		return m_RoleConfigsMap;
	}
	
	COA_RoleConfig FindRoleConfig(COA_EGearRole role)
	{
		return m_RoleConfigsMap.Get(role);
	}
	
	void COA_RolesConfig()
	{
		foreach(COA_RoleConfig roleConfig : m_RoleConfigs)
			m_RoleConfigsMap.Set(roleConfig.m_Role, roleConfig);	
	}
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(COA_EGearRole, "m_Role")]
class COA_RoleConfig
{		
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(COA_EGearRole))]
	COA_EGearRole m_Role;
	
	[Attribute()]
	string m_sRoleName;
	
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBoxMultiline)]
	string m_sRoleDescription;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "edds")]
	ResourceName m_RoleIcon;
	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(COA_ESlotType))]
	COA_ESlotType m_SlottingType;

	[Attribute(uiwidget: "resourcePickerSimple", params: "et")]
	ResourceName m_RoleResource;
	
	[Attribute("", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(COA_EGearscriptWeapons))]
	ref array<COA_EGearscriptWeapons> m_aWeapons;
	
	[Attribute("", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(COA_EGearscriptMagazines))]
	ref array<COA_EGearscriptMagazines> m_aMagazines;
	
	[Attribute("", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(COA_EGearscriptItems))]
	ref array<COA_EGearscriptItems> m_aItems;
}