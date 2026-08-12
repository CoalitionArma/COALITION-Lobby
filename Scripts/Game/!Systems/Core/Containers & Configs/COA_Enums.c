/*
	HOW TO ADD A ROLE 101:
	Step 1 - Create the specified role prefab in Prefabs\Characters\!GS_Characters folder and name it with the method: COA_GS_(Role)_P, ie: 
		COA_GS_AAR_P

	Step 2 -  Create a role in all caps with spaces having underscores in the bellow enum class COA_EGearRole, ie:
		ASSISTANT_AUTOMATIC_RIFLEMAN

	Step 3 -  Now you have to go to the corresponding global file:
		Configs\Gearscripts\COA_Global_Roles_Config.conf

		- Add a new item into the RoleConfigs array, Set the role with the role you created in step 2
		- Add the prerequisite prefab you made in step 1
		- Add whatever weapons/gear you want for your new role
		
	There, you have added a role, good for you, now stop bothering me about adding in roles manually -Njpatman
*/

enum COA_EGearRole
{
	UNARMED = 0,
	//-------------------------------------------- LEADERSHIP --------------------------------------------
	COMPANY_COMMANDER,
	FIRST_SERGEANT,
	PLATOON_LEADER,
	PLATOON_SERGEANT,
	MEDICAL_OFFICER,
	FORWARD_OBSERVER,
	JTAC,
	SQUAD_LEAD,
	VEHICLE_LEAD,
	INDIRECT_LEAD,
	LOGI_LEAD,
	//-------------------------------------------- SQUAD LEVEL -------------------------------------------
	TEAM_LEAD,
	MEDIC,
	RADIO_TELEPHONE_OPERATOR,
	GRENADIER,
	AUTOMATIC_RIFLEMAN,
	ASSISTANT_AUTOMATIC_RIFLEMAN,
	RIFLEMAN,
	RIFLEMAN_ANTITANK,
	ASSISTANT_RIFLEMAN_ANTITANK,
	RIFLEMAN_DEMO,
	//------------------------------------------- SPECIALITIES -------------------------------------------
	HEAVY_ANTITANK,
	ASSISTANT_HEAVY_ANTITANK,
	MEDIUM_ANTITANK,
	ASSISTANT_MEDIUM_ANTITANK,
	HEAVY_MACHINEGUN,
	ASSISTANT_HEAVY_MACHINEGUN,
	MEDIUM_MACHINEGUN,
	ASSISTANT_MEDIUM_MACHINEGUN,
	ANTI_AIR,
	ASSISTANT_ANTI_AIR,
	SNIPER,
	SPOTTER,
	DRONE_OPERATOR,
	COMBAT_ENGINEER,
	//--------------------------------------- VEHICLE SPECIALITIES ---------------------------------------
	VEHICLE_DRIVER,
	VEHICLE_GUNNER,
	VEHICLE_LOADER,
	PILOT,
	CREW_CHIEF,
	LOGI_RUNNER,
	INDIRECT_GUNNER,
	INDIRECT_LOADER,
	//-------------------------------------------- OTHER -------------------------------------------------
	ZEUS,
	VIP
}

//------------------------------------------------------------------------------------------------
// Enum for slot update field types (batching system)
//------------------------------------------------------------------------------------------------
enum COA_ESlotUpdateField
{
	PLAYER_ID,
	CHARACTER,
	GROUP,
	LOCKED,
	DEATH,
	ROLE,
	RESPAWNS_REMAINING
}

//------------------------------------------------------------------------------------
// Enumeration for gearscript clothing
//------------------------------------------------------------------------------------

enum COA_EGearscriptClothing
{
	UNDEFINED = 123456789,
	HEADGEAR = 0,
	SHIRT,
	ARMOREDVEST,
	PANTS,
	BOOTS,
	BACKPACK,
	VEST,
	HANDWEAR,
	HEAD,
	EYES,
	EARS,
	FACE,
	NECK,
	EXTRA1,
	EXTRA2,
	WAIST,
	EXTRA3,
	EXTRA4,
}

//------------------------------------------------------------------------------------
// Enumeration for gearscript weapons
//------------------------------------------------------------------------------------

enum COA_EGearscriptWeapons
{
	RIFLE,
	RIFLEUGL,
	CARBINE,
	SNIPER,
	PISTOL,
	AR,
	AT,
	MMG,
	HMG,
	MAT,
	HAT,
	AA,
}


//------------------------------------------------------------------------------------
// Enumeration for gearscript magazines
//------------------------------------------------------------------------------------

enum COA_EGearscriptMagazines
{
	RIFLE_MAG,
	RIFLEUGL_MAG,
	CARBINE_MAG,
	SNIPER_MAG,
	PISTOL_MAG,
	AR_MAG,
	AT_MAG,
	MMG_MAG,
	HMG_MAG,
	MAT_MAG,
	HAT_MAG,
	AA_MAG,
}


//------------------------------------------------------------------------------------
// Enumeration for additional gearscript items
//------------------------------------------------------------------------------------

enum COA_EGearscriptItems
{
	SHORTRANGE_RADIO,
	LONGRANGE_RADIO,
	RTO_RADIO,
	ASSISTANT_BINO,
	LEADERSHIP_BINO,
	MEDIC_ITEMS,
}

//------------------------------------------------------------------------------------
// Enumerations for menus
//------------------------------------------------------------------------------------

modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	COA_AARMenu,
	COA_AdminMenu,
	COA_PreviewMenu,
	COA_RespawnMenu,
	COA_SlottingMenu,
	COA_SpectatorMenu,
	COA_CharacterLoading,
	COA_Outro,
}

//------------------------------------------------------------------------------------
// Enumerations for our custom factions
//------------------------------------------------------------------------------------

modded enum EEditableEntityLabel
{
	FACTION_COA_BLUFOR = 51870,
	FACTION_COA_OPFOR = 51871,
	FACTION_COA_INDFOR = 51872,
	FACTION_COA_CIV = 51873,
}

enum COA_EFactions
{
	BLUFOR,
	OPFOR,
	INDFOR,
	CIV,
}

//------------------------------------------------------------------------------------
// Enumerations for game state tracking
//------------------------------------------------------------------------------------

enum COA_EGamemodeState
{
	BRIEFING,   // Initial mission briefing phase
	SLOTTING,   // Player role selection phase
	GAME,       // Active gameplay phase
	AAR         // After Action Report phase
}

enum COA_ESlottingState
{
	LEADERSANDMEDICS,  // Only leaders and medics can select slots
	SPECIALTIES,       // Specialist roles become available
	EVERYONE           // All roles available to all players
}

//------------------------------------------------------------------------------------
// Enumeration for mission-wide respawn gating mode
//------------------------------------------------------------------------------------

enum COA_ERespawnMode
{
	TEAM = 0,   // Shared faction ticket pool (default)
	SLOT        // Per-role/per-group respawn counts configured on each COA_SlottingGroup
}

//------------------------------------------------------------------------------------
// Enumeration for how a squad's respawn pool is shared (only used in COA_ERespawnMode.SLOT)
//------------------------------------------------------------------------------------

enum COA_ERespawnPoolType
{
	PER_SLOT = 0,   // Each role/slot in the group tracks its own respawn count
	PER_GROUP       // The whole group shares one respawn count
}

//------------------------------------------------------------------------------------
// Enumeration for group flag types
//------------------------------------------------------------------------------------

enum COA_EFlagType
{
	INFANTRY = 0,
	AMPHIBIOUS,
	ANTI_AIR,
	ANTI_AIR_ARTILLERY,
	ANTI_ARMOR_MOTORIZED,
	ANTI_ARMOR,
	ARMORED,
	ARTILLERY,
	COMBINED,
	HELICOPER,
	ATTACK_HELICOPTER,
	INFANTRY_AIR,
	MACHINEGUN,
	MAINTENANCE,
	MEDICAL,
	MORTAR,
	MOTORIZED,
	MOTORIZED_INFANTRY,
	RECON,
	RECON_MOTORIZED,
	SIGNAL,
	SNIPER,
	SUPPLY_MOTORIZED,
	UNDEFINED,
}

//------------------------------------------------------------------------------------
// Enumeration for slot types
//------------------------------------------------------------------------------------

enum COA_ESlotType
{
	GENERAL_INFANTRY = 0,
	SQUAD_LEADER,
	TEAM_LEADER,
	MEDIC,
	ASSISTANT,
	SPECIALTY,
	SPECIALTY_ASSISTANT,
}

//------------------------------------------------------------------------------------
// Enumeration for polyzone effects
//------------------------------------------------------------------------------------

enum COA_EPolyZoneEffectHUDType
{
	FreezeZone,
	FreezeZoneLeave,
	RestrictedZone,
	ScreenBlure,
	TriggerCapture,
}

//------------------------------------------------------------------------------------
// Enumeration for logging level for the admin menu
//------------------------------------------------------------------------------------

enum COA_EAdminLogLevel
{
	Low,
	Medium,
	High,
}