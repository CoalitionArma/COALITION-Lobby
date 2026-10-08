//------------------------------------------------------------------------------------------------
// Compares what a character carries with what its role's gearscript gave it, for the player
// radial menu's "Check my gear". Follows the same rules as COA_GearscriptManager.SetEntityGear
// (custom role weapons replace the defaults; radios depend on the faction's radio settings).
// A randomised weapon pool counts as present if any weapon from it is carried.
//------------------------------------------------------------------------------------------------
class COA_GearCheckHelper
{
	//------------------------------------------------------------------------------------------------
	//! \param[in] character Character to check
	//! \param[in] factionKey Faction whose gearscript applies
	//! \param[in] role Role the character was equipped as
	//! \param[out] missing One entry per missing item, e.g. "M72 LAW" or "3x Bandage"
	//! \return false if the gearscript couldn't be resolved (nothing to compare against)
	static bool FindMissing(IEntity character, FactionKey factionKey, COA_EGearRole role, notnull array<string> missing)
	{
		missing.Clear();

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		COA_GearscriptManager gearscriptManager = COA_GearscriptManager.GetInstance();
		COA_RolesConfig rolesConfig = COA_GearscriptManager.GetRolesConfig();
		if (!character || !gamemode || !gearscriptManager || !rolesConfig)
			return false;

		COA_GearScriptConfig gearConfig = gearscriptManager.LoadGearScriptConfig(gamemode.GetGearScriptResource(factionKey));
		COA_GearScriptContainer gearSettings = gamemode.GetGearScriptSettings(factionKey);
		COA_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
		if (!gearConfig || !gearSettings || !roleConfig)
			return false;

		map<ResourceName, int> carried = new map<ResourceName, int>();
		CountCarriedItems(character, carried);

		// Weapons, and the magazines expected for the weapon from each pool the character carries
		map<ResourceName, int> expectedMagazines = new map<ResourceName, int>();
		COA_Role_Custom_Gear customGear = COA_LoadoutPreviewHelper.FindCustomGear(gearConfig, role);
		bool customWeapons = false;
		if (customGear)
		{
			if (CheckPool(customGear.m_PrimaryWeapon, carried, missing, expectedMagazines))
				customWeapons = true;
			if (CheckPool(customGear.m_SecondaryWeapon, carried, missing, expectedMagazines))
				customWeapons = true;
			if (CheckPool(customGear.m_Pistols, carried, missing, expectedMagazines))
				customWeapons = true;
		}

		if (!customWeapons)
			CheckDefaultWeapons(gearConfig, roleConfig, carried, missing, expectedMagazines);

		// Items
		map<ResourceName, int> expectedItems = new map<ResourceName, int>();
		AddExpectedItems(gearConfig, gearSettings, roleConfig, customGear, expectedItems);

		foreach (ResourceName magazine, int count : expectedMagazines)
			AddExpected(expectedItems, magazine, count);

		foreach (ResourceName item, int expected : expectedItems)
		{
			int have;
			carried.Find(item, have);
			if (have >= expected)
				continue;

			int shortfall = expected - have;
			if (shortfall == 1)
				missing.Insert(COA_LoadoutPreviewHelper.GetDisplayName(item));
			else
				missing.Insert(string.Format("%1x %2", shortfall, COA_LoadoutPreviewHelper.GetDisplayName(item)));
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Prefab -> count of everything in the character's inventory, plus magazines loaded in weapons
	protected static void CountCarriedItems(IEntity character, notnull map<ResourceName, int> carried)
	{
		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (inventory)
		{
			array<IEntity> items = {};
			inventory.GetItems(items);
			foreach (IEntity item : items)
			{
				if (item && item.GetPrefabData())
					AddExpected(carried, item.GetPrefabData().GetPrefabName(), 1);
			}
		}

		ChimeraCharacter chimera = ChimeraCharacter.Cast(character);
		if (!chimera || !chimera.GetWeaponManager())
			return;

		array<WeaponSlotComponent> slots = {};
		chimera.GetWeaponManager().GetWeaponsSlots(slots);
		foreach (WeaponSlotComponent slot : slots)
		{
			IEntity weaponEntity = slot.GetWeaponEntity();
			if (!weaponEntity)
				continue;

			// Make sure the weapon itself counts even if the inventory listing skipped weapon slots
			if (weaponEntity.GetPrefabData() && !carried.Contains(weaponEntity.GetPrefabData().GetPrefabName()))
				AddExpected(carried, weaponEntity.GetPrefabData().GetPrefabName(), 1);

			BaseWeaponComponent weapon = BaseWeaponComponent.Cast(weaponEntity.FindComponent(BaseWeaponComponent));
			if (!weapon || !weapon.GetCurrentMagazine())
				continue;

			IEntity magazine = weapon.GetCurrentMagazine().GetOwner();
			if (magazine && magazine.GetPrefabData())
				AddExpected(carried, magazine.GetPrefabData().GetPrefabName(), 1);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \return true if the pool holds any weapon (whether or not it's carried)
	protected static bool CheckPool(array<ref COA_Weapon_Class> pool, notnull map<ResourceName, int> carried, notnull array<string> missing, notnull map<ResourceName, int> expectedMagazines)
	{
		if (!pool || pool.IsEmpty())
			return false;

		COA_Weapon_Class first;
		foreach (COA_Weapon_Class weapon : pool)
		{
			if (!weapon || weapon.m_Weapon.IsEmpty())
				continue;

			if (!first)
				first = weapon;

			if (carried.Contains(weapon.m_Weapon))
			{
				AddMagazines(weapon.m_MagazineArray, expectedMagazines);
				return true;
			}
		}

		if (!first)
			return false;

		missing.Insert(COA_LoadoutPreviewHelper.GetDisplayName(first.m_Weapon));
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static void CheckSpecWeapon(COA_Spec_Weapon_Class weapon, bool assistant, notnull map<ResourceName, int> carried, notnull array<string> missing, notnull map<ResourceName, int> expectedMagazines)
	{
		if (!weapon || weapon.m_Weapon.IsEmpty())
			return;

		if (!assistant && !carried.Contains(weapon.m_Weapon))
			missing.Insert(COA_LoadoutPreviewHelper.GetDisplayName(weapon.m_Weapon));

		if (!weapon.m_MagazineArray)
			return;

		foreach (COA_Spec_Magazine_Class magazine : weapon.m_MagazineArray)
		{
			if (!magazine)
				continue;

			if (assistant)
				AddExpected(expectedMagazines, magazine.m_Magazine, magazine.m_AssistantMagazineCount);
			else
				AddExpected(expectedMagazines, magazine.m_Magazine, magazine.m_MagazineCount);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void CheckDefaultWeapons(COA_GearScriptConfig gearConfig, COA_RoleConfig roleConfig, notnull map<ResourceName, int> carried, notnull array<string> missing, notnull map<ResourceName, int> expectedMagazines)
	{
		if (roleConfig.m_aWeapons)
		{
			foreach (COA_EGearscriptWeapons weaponType : roleConfig.m_aWeapons)
			{
				switch (weaponType)
				{
					case COA_EGearscriptWeapons.RIFLE:		CheckPool(gearConfig.m_Rifles, carried, missing, expectedMagazines); break;
					case COA_EGearscriptWeapons.RIFLEUGL:	CheckPool(gearConfig.m_RifleUGLs, carried, missing, expectedMagazines); break;
					case COA_EGearscriptWeapons.CARBINE:	CheckPool(gearConfig.m_Carbines, carried, missing, expectedMagazines); break;
					case COA_EGearscriptWeapons.PISTOL:		CheckPool(gearConfig.m_Pistols, carried, missing, expectedMagazines); break;
					case COA_EGearscriptWeapons.SNIPER:
						if (gearConfig.m_SNIPER && !gearConfig.m_SNIPER.m_Weapon.IsEmpty())
						{
							if (!carried.Contains(gearConfig.m_SNIPER.m_Weapon))
								missing.Insert(COA_LoadoutPreviewHelper.GetDisplayName(gearConfig.m_SNIPER.m_Weapon));
							AddMagazines(gearConfig.m_SNIPER.m_MagazineArray, expectedMagazines);
						}
						break;
					case COA_EGearscriptWeapons.AR:			CheckSpecWeapon(gearConfig.m_AR, false, carried, missing, expectedMagazines); break;
					case COA_EGearscriptWeapons.MMG:		CheckSpecWeapon(gearConfig.m_MMG, false, carried, missing, expectedMagazines); break;
					case COA_EGearscriptWeapons.HMG:		CheckSpecWeapon(gearConfig.m_HMG, false, carried, missing, expectedMagazines); break;
					case COA_EGearscriptWeapons.AT:			CheckSpecWeapon(gearConfig.m_AT, false, carried, missing, expectedMagazines); break;
					case COA_EGearscriptWeapons.MAT:		CheckSpecWeapon(gearConfig.m_MAT, false, carried, missing, expectedMagazines); break;
					case COA_EGearscriptWeapons.HAT:		CheckSpecWeapon(gearConfig.m_HAT, false, carried, missing, expectedMagazines); break;
					case COA_EGearscriptWeapons.AA:			CheckSpecWeapon(gearConfig.m_AA, false, carried, missing, expectedMagazines); break;
				}
			}
		}

		// Assistants carry ammo for a weapon they don't have
		bool isAssistant = roleConfig.m_SlottingType == COA_ESlotType.ASSISTANT || roleConfig.m_SlottingType == COA_ESlotType.SPECIALTY_ASSISTANT;
		if (!isAssistant || !roleConfig.m_aMagazines)
			return;

		foreach (COA_EGearscriptMagazines magType : roleConfig.m_aMagazines)
		{
			switch (magType)
			{
				case COA_EGearscriptMagazines.AR_MAG:	CheckSpecWeapon(gearConfig.m_AR, true, carried, missing, expectedMagazines); break;
				case COA_EGearscriptMagazines.MMG_MAG:	CheckSpecWeapon(gearConfig.m_MMG, true, carried, missing, expectedMagazines); break;
				case COA_EGearscriptMagazines.HMG_MAG:	CheckSpecWeapon(gearConfig.m_HMG, true, carried, missing, expectedMagazines); break;
				case COA_EGearscriptMagazines.AT_MAG:	CheckSpecWeapon(gearConfig.m_AT, true, carried, missing, expectedMagazines); break;
				case COA_EGearscriptMagazines.MAT_MAG:	CheckSpecWeapon(gearConfig.m_MAT, true, carried, missing, expectedMagazines); break;
				case COA_EGearscriptMagazines.HAT_MAG:	CheckSpecWeapon(gearConfig.m_HAT, true, carried, missing, expectedMagazines); break;
				case COA_EGearscriptMagazines.AA_MAG:	CheckSpecWeapon(gearConfig.m_AA, true, carried, missing, expectedMagazines); break;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Radios, binoculars and medical gear - mirrors COA_GearscriptManager.ApplyInventoryItems
	protected static void AddExpectedItems(COA_GearScriptConfig gearConfig, COA_GearScriptContainer gearSettings, COA_RoleConfig roleConfig, COA_Role_Custom_Gear customGear, notnull map<ResourceName, int> expectedItems)
	{
		if (customGear && customGear.m_AdditionalInventoryItems)
			AddItems(customGear.m_AdditionalInventoryItems, expectedItems);

		if (roleConfig.m_aItems)
		{
			COA_ESlotType slotType = roleConfig.m_SlottingType;
			bool leadershipType = slotType == COA_ESlotType.TEAM_LEADER || slotType == COA_ESlotType.SQUAD_LEADER || slotType == COA_ESlotType.SPECIALTY || slotType == COA_ESlotType.SPECIALTY_ASSISTANT;

			foreach (COA_EGearscriptItems item : roleConfig.m_aItems)
			{
				switch (item)
				{
					case COA_EGearscriptItems.SHORTRANGE_RADIO:
						if (gearSettings.m_bEnableGIRadios || (gearSettings.m_bEnableLeadershipRadios && leadershipType))
							AddExpected(expectedItems, gearSettings.m_rShortRangeRadioPrefab, 1);
						break;

					case COA_EGearscriptItems.LONGRANGE_RADIO:
						if (gearSettings.m_bEnableLeadershipRadios)
							AddExpected(expectedItems, gearSettings.m_rLongRangeRadioPrefab, 1);
						break;

					case COA_EGearscriptItems.RTO_RADIO:
						if (gearSettings.m_bEnableRTORadios)
							AddExpected(expectedItems, gearSettings.m_rRTORadiosPrefab, 1);
						break;

					case COA_EGearscriptItems.LEADERSHIP_BINO:
						AddExpected(expectedItems, gearConfig.m_sLeadershipBinocularsPrefab, 1);
						break;

					case COA_EGearscriptItems.ASSISTANT_BINO:
						AddExpected(expectedItems, gearConfig.m_sAssistantBinocularsPrefab, 1);
						break;

					case COA_EGearscriptItems.MEDIC_ITEMS:
						AddItems(gearConfig.m_MedicMedicalItems, expectedItems);
						break;
				}
			}
		}

		AddItems(gearConfig.m_InfantryMedicalItems, expectedItems);
		AddItems(gearConfig.m_DefaultInventoryItems, expectedItems);
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddItems(array<ref COA_Inventory_Item> items, notnull map<ResourceName, int> expected)
	{
		if (!items)
			return;

		foreach (COA_Inventory_Item item : items)
		{
			if (item)
				AddExpected(expected, item.m_sItemPrefab, item.m_iItemCount);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddMagazines(array<ref COA_Magazine_Class> magazines, notnull map<ResourceName, int> expected)
	{
		if (!magazines)
			return;

		foreach (COA_Magazine_Class magazine : magazines)
		{
			if (magazine)
				AddExpected(expected, magazine.m_Magazine, magazine.m_MagazineCount);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddExpected(notnull map<ResourceName, int> counts, ResourceName prefab, int count)
	{
		if (prefab.IsEmpty() || count <= 0)
			return;

		int current;
		counts.Find(prefab, current);
		counts.Set(prefab, current + count);
	}
}
