//=============================================================================================================================================================================================================================================================================================================================================================
//	 CONTAINERS
//=============================================================================================================================================================================================================================================================================================================================================================

[BaseContainerProps()]
class COA_GearScriptContainer
{
	//------------------------------------------------------------------------------------------------
	// Vars set by plugin
	
	[Attribute("", UIWidgets.Hidden)]
	ResourceName m_rGearScript;
	
 	[Attribute("true", UIWidgets.Hidden)]
	bool m_bEnableBFT;
	
	[Attribute("true", UIWidgets.Hidden)]
	bool m_bEnableLeadershipRadios;
	
	[Attribute("false", UIWidgets.Hidden)]
	bool m_bEnableGIRadios;
	
	[Attribute("true", UIWidgets.Hidden)]
	bool m_bEnableRTORadios;
	
	//------------------------------------------------------------------------------------------------
	// Vars considered "advanced" and not set by plugin
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_rShortRangeRadioPrefab;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_rLongRangeRadioPrefab;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_rRTORadiosPrefab;
}


// Simplified Container for Faction Plugin
[BaseContainerProps()]
class COA_SimplifiedGearScriptContainer
{
	[Attribute("{6FFD426FE0C1079B}Configs/Gearscripts/Standard/80s/COA_GS_CIV.conf", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all entities on this faction", "conf class=COA_GearScriptConfig")]
	ResourceName m_rGearScript;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableLeadershipRadios;
	
	[Attribute("false", UIWidgets.CheckBox)]
	bool m_bEnableGIRadios;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableRTORadios;
  
 	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableBFT;
}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 MASTER
//=============================================================================================================================================================================================================================================================================================================================================================

[BaseContainerProps(configRoot: true)]
class COA_GearScriptConfig
{
	[Attribute(category: "Gearscript - Faction Settings")]
	string m_FactionName;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "edds", category: "Gearscript - Faction Settings")]
	ResourceName m_FactionIcon;
	
	[Attribute("{11CAD6C8909CE567}Configs/_Identities/COA_CharacterIdentity_European.conf", UIWidgets.ResourceNamePicker, desc: "Gearscript Faction Identity", "conf class=COA_CharacterIdentity", category: "Gearscript - Faction Settings")]
	ResourceName m_FactionIdentity;
	
	[Attribute(category: "Gearscript - Faction Weapons")]
	ref array<ref COA_Weapon_Class> m_Rifles;
	
	[Attribute(category: "Gearscript - Faction Weapons")]
	ref array<ref COA_Weapon_Class> m_RifleUGLs;
	
	[Attribute(category: "Gearscript - Faction Weapons")]
	ref array<ref COA_Weapon_Class> m_Carbines;
	
	[Attribute(category: "Gearscript - Faction Weapons")]
	ref array<ref COA_Weapon_Class> m_Pistols;
	
	[Attribute(category: "Gearscript - Specialty Faction Weapons")]
	ref COA_Spec_Weapon_Class m_AR;
	
	[Attribute(category: "Gearscript - Specialty Faction Weapons")]
	ref COA_Spec_Weapon_Class m_MMG;
	
	[Attribute(category: "Gearscript - Specialty Faction Weapons")]
	ref COA_Spec_Weapon_Class m_HMG;
	
	[Attribute(category: "Gearscript - Specialty Faction Weapons")]
	ref COA_Spec_Weapon_Class m_AT;
	
	[Attribute(category: "Gearscript - Specialty Faction Weapons")]
	ref COA_Spec_Weapon_Class m_MAT;
	
	[Attribute(category: "Gearscript - Specialty Faction Weapons")]
	ref COA_Spec_Weapon_Class m_HAT;
	
	[Attribute(category: "Gearscript - Specialty Faction Weapons")]
	ref COA_Spec_Weapon_Class m_AA;
	
	[Attribute(category: "Gearscript - Specialty Faction Weapons")]
	ref COA_Weapon_Class m_SNIPER;
	
	[Attribute(category: "Gearscript - Faction Clothing")]
	ref array<ref COA_Clothing> m_DefaultClothing;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et", category: "Gearscript - Faction Gear")]
	ResourceName m_sLeadershipBinocularsPrefab;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et", category: "Gearscript - Faction Gear")]
	ResourceName m_sAssistantBinocularsPrefab;
	
	[Attribute(category: "Gearscript - Faction Gear")]
	ref array<ref COA_Inventory_Item> m_DefaultInventoryItems;
	
	[Attribute(category: "Gearscript - Faction Medical Gear")]
	ref array<ref COA_Inventory_Item>  m_InfantryMedicalItems;
	
	[Attribute(category: "Gearscript - Faction Medical Gear")]
	ref array<ref COA_Inventory_Item>  m_MedicMedicalItems;
	
	[Attribute(category: "Gearscript - Custom Role Settings")]
	ref array<ref COA_Role_Custom_Gear> m_RolesToSetCustomSettings;
}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 WEAPONS
//=============================================================================================================================================================================================================================================================================================================================================================

class COA_Base_Weapon_Class
{
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_Weapon;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ref array<ResourceName> m_Attachments;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), COA_BaseContainerCustomTitleResourceFields({"m_Weapon"}, "%1")]
class COA_Weapon_Class : COA_Base_Weapon_Class
{	
	[Attribute()]
	ref array<ref COA_Magazine_Class> m_MagazineArray;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), COA_BaseContainerCustomTitleResourceFields({"m_Weapon"}, "%1")]
class COA_Spec_Weapon_Class : COA_Base_Weapon_Class
{
	[Attribute()]
	ref array<ref COA_Spec_Magazine_Class> m_MagazineArray;
}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 MAGAZINES/AMMO
//=============================================================================================================================================================================================================================================================================================================================================================

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), COA_BaseContainerCustomTitleResourceFields({"m_MagazineCount", "m_Magazine"}, "[%1] : %2")]
class COA_Magazine_Class
{
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_Magazine;
	
	[Attribute()]
	int m_MagazineCount;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), COA_BaseContainerCustomTitleResourceFields({"m_MagazineCount", "m_AssistantMagazineCount", "m_Magazine"}, "[%1 | %2] : %3")]
class COA_Spec_Magazine_Class : COA_Magazine_Class
{	
	[Attribute()]
	int m_AssistantMagazineCount;
}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 INVENTORY
//=============================================================================================================================================================================================================================================================================================================================================================

[BaseContainerProps(), COA_BaseContainerCustomTitleResourceFields({ "m_iItemCount", "m_sItemPrefab" }, "[%1] : %2")]
class COA_Inventory_Item
{
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_sItemPrefab;
	
	[Attribute("")] 
	int m_iItemCount;
}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 CLOTHING
//=============================================================================================================================================================================================================================================================================================================================================================

[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(COA_EGearscriptClothing, "m_iClothingType")]
class COA_Clothing
{
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(COA_EGearscriptClothing))]
	COA_EGearscriptClothing m_iClothingType;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ref array<ResourceName> m_ClothingPrefabs;
}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 ROLE CUSTOM GEAR
//=============================================================================================================================================================================================================================================================================================================================================================

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(COA_EGearRole, "m_Role")]
class COA_Role_Custom_Gear
{	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(COA_EGearRole))]
	COA_EGearRole m_Role;
	
	[Attribute()]
	string m_sRoleName;
	
	[Attribute()]
	ref array<ref COA_Weapon_Class> m_PrimaryWeapon;
	
	[Attribute()]
	ref array<ref COA_Weapon_Class> m_SecondaryWeapon;
	
	[Attribute()]
	ref array<ref COA_Weapon_Class> m_Pistols;
	
	[Attribute()]
	ref array<ref COA_Clothing> m_Clothing;
	
	[Attribute()]
	ref array<ref COA_Inventory_Item>  m_AdditionalInventoryItems;
}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 CHARACTER IDENTITY
//=============================================================================================================================================================================================================================================================================================================================================================

[BaseContainerProps(configRoot: true)]
class COA_CharacterIdentity
{	
	[Attribute()]
	ref array<ref COA_Character_Visual_Identity> m_VisualIdentityArray;
	
	[Attribute()]
	ref array<ref COA_Character_Sound_Identity> m_SoundIdentityArray;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class COA_Character_Visual_Identity
{	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_Head;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_Body;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class COA_Character_Sound_Identity
{	
	[Attribute()]
	int m_VoiceID;
	
	[Attribute()]
	float m_VoicePitch;
}