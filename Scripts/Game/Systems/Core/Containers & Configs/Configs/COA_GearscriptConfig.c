//------------------------------------------------------------------------------------------------
// CONTAINER
//------------------------------------------------------------------------------------------------

[BaseContainerProps()]
class COA_GearScriptContainer
{
	//------------------------------------------------------------------------------------------------
	// Vars set by plugin
	
	[Attribute("", UIWidgets.Hidden)]
	ResourceName m_rGearScript;
	
#ifdef COALITION_REFORGER_FRAMEWORK
	[Attribute("false", UIWidgets.Hidden)]
	bool m_bEnableShareableMarkers;
#endif
	
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
	
#ifdef COALITION_REFORGER_FRAMEWORK
	[Attribute("{E6555DA2F31B0EC0}Configs/Gearscripts/Additional Configs/COA_Global_SightArsenal_Regular.conf", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all entities on this faction", "conf class=COA_SightArsenalConfig")]
	ResourceName m_rSightArsenal;
	
	[Attribute("{9D8E5FA08331042D}Configs/Gearscripts/Additional Configs/COA_Global_SightArsenal_Magnified.conf", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all entities on this faction", "conf class=COA_SightArsenalConfig")]
	ResourceName m_rMagnifiedSightArsenal;
	
	[Attribute("{2E2626C733070162}Configs/Gearscripts/Additional Configs/COA_Global_VehicleGearscriptValues.conf", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all vehicles on this faction", "conf class=COA_VehicleGearscriptConfig")]
	ResourceName m_rVehicleGearscriptValues;
	
	[Attribute("", desc: "Loadout values applied to all vehicles in this faction", "conf class=COA_VehicleGearScriptLoadout")]
	ref COA_VehicleGearScriptLoadout m_VehicleLoadout;
	
	[Attribute()] 
	ref array<ref COA_VehicleGearscriptOverride> m_aVehicleGearscriptOverrides;
	
	[Attribute()]
	ref array<ref COA_VehicleGearScriptAdditionalItem> m_aAdditionalVehicleItems;
	
	[Attribute()] 
	ref array<ResourceName> m_aSupplyTrucks;
	
	[Attribute()] 
	ref array<ResourceName> m_aAdditonalItemsForSupplyArsenal;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableMiniArsenal;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableMiniWeaponArsenal;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableSightArsenal;
#endif
	
	[Attribute("false", UIWidgets.CheckBox)]
	bool m_bEnableMagnifiedOptics;
	
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
	
#ifdef COALITION_REFORGER_FRAMEWORK
	[Attribute("false", UIWidgets.CheckBox)]
	bool m_bEnableShareableMarkers;
#endif
  
 	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableBFT;
}

//------------------------------------------------------------------------------------------------
// MASTER
//------------------------------------------------------------------------------------------------

[BaseContainerProps(configRoot: true)]
class COA_GearScriptConfig
{
	[Attribute(category: "CRF Gearscript - Faction Settings")]
	string m_FactionName;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "edds", category: "CRF Gearscript - Faction Settings")]
	ResourceName m_FactionIcon;
	
	[Attribute("{11CAD6C8909CE567}Configs/_Identities/COA_CharacterIdentity_European.conf", UIWidgets.ResourceNamePicker, desc: "Gearscript Faction Identity", "conf class=COA_CharacterIdentity", category: "CRF Gearscript - Faction Settings")]
	ResourceName m_FactionIdentity;
	
	[Attribute(category: "CRF Gearscript - Faction Weapons")]
	ref array<ref COA_Weapon_Class> m_Rifles;
	
	[Attribute(category: "CRF Gearscript - Faction Weapons")]
	ref array<ref COA_Weapon_Class> m_RifleUGLs;
	
	[Attribute(category: "CRF Gearscript - Faction Weapons")]
	ref array<ref COA_Weapon_Class> m_Carbines;
	
	[Attribute(category: "CRF Gearscript - Faction Weapons")]
	ref array<ref COA_Weapon_Class> m_Pistols;
	
	[Attribute(category: "CRF Gearscript - Specialty Faction Weapons")]
	ref COA_Spec_Weapon_Class m_AR;
	
	[Attribute(category: "CRF Gearscript - Specialty Faction Weapons")]
	ref COA_Spec_Weapon_Class m_MMG;
	
	[Attribute(category: "CRF Gearscript - Specialty Faction Weapons")]
	ref COA_Spec_Weapon_Class m_HMG;
	
	[Attribute(category: "CRF Gearscript - Specialty Faction Weapons")]
	ref COA_Spec_Weapon_Class m_AT;
	
	[Attribute(category: "CRF Gearscript - Specialty Faction Weapons")]
	ref COA_Spec_Weapon_Class m_MAT;
	
	[Attribute(category: "CRF Gearscript - Specialty Faction Weapons")]
	ref COA_Spec_Weapon_Class m_HAT;
	
	[Attribute(category: "CRF Gearscript - Specialty Faction Weapons")]
	ref COA_Spec_Weapon_Class m_AA;
	
	[Attribute(category: "CRF Gearscript - Specialty Faction Weapons")]
	ref COA_Weapon_Class m_SNIPER;
	
	[Attribute(category: "CRF Gearscript - Faction Clothing")]
	ref array<ref COA_Clothing> m_DefaultClothing;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et", category: "CRF Gearscript - Faction Gear")]
	ResourceName m_sLeadershipBinocularsPrefab;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et", category: "CRF Gearscript - Faction Gear")]
	ResourceName m_sAssistantBinocularsPrefab;
	
	[Attribute(category: "CRF Gearscript - Faction Gear")]
	ref array<ref COA_Inventory_Item> m_DefaultInventoryItems;
	
	[Attribute(category: "CRF Gearscript - Faction Medical Gear")]
	ref array<ref COA_Inventory_Item>  m_InfantryMedicalItems;
	
	[Attribute(category: "CRF Gearscript - Faction Medical Gear")]
	ref array<ref COA_Inventory_Item>  m_MedicMedicalItems;
	
	[Attribute(category: "CRF Gearscript - Custom Role Settings")]
	ref array<ref COA_Role_Custom_Gear> m_RolesToSetCustomSettings;
}

//------------------------------------------------------------------------------------------------
// WEAPONS
//------------------------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------------------------
// INVENTORY
//------------------------------------------------------------------------------------------------

[BaseContainerProps(), COA_BaseContainerCustomTitleResourceFields({ "m_iItemCount", "m_sItemPrefab" }, "[%1] : %2")]
class COA_Inventory_Item
{
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_sItemPrefab;
	
	[Attribute("")] 
	int m_iItemCount;
}

//------------------------------------------------------------------------------------------------
// CLOTHING
//------------------------------------------------------------------------------------------------

[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(COA_EGearscriptClothing, "m_iClothingType")]
class COA_Clothing
{
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(COA_EGearscriptClothing))]
	COA_EGearscriptClothing m_iClothingType;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ref array<ResourceName> m_ClothingPrefabs;
}

//------------------------------------------------------------------------------------------------
// ROLE CUSTOM GEAR
//------------------------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------------------------
// CHARACTER IDENTITY
//------------------------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------------------------
[BaseContainerProps(configRoot: true)]
class COA_SightArsenalConfig
{	
	[Attribute()]
	ref array<ResourceName> m_aSights;
}