//------------------------------------------------------------------------------------------------
// How gearscript weapon slots map onto roles, mirroring COA_GearscriptManager.ApplyWeapons:
//  - a role with any COA_Role_Custom_Gear weapon gets ONLY its custom weapons (and their
//    magazines); the role config's default weapons and magazines are skipped entirely;
//  - otherwise each COA_EGearscriptWeapons in the role config's m_aWeapons gives that slot's
//    weapon, and each COA_EGearscriptMagazines in m_aMagazines gives that slot's magazines -
//    the assistant count of specialty magazines when the role's slot type is an assistant one.
//------------------------------------------------------------------------------------------------
class COA_GearFixSlotHelper
{
	//------------------------------------------------------------------------------------------------
	static bool IsSpecSlot(int slot)
	{
		return slot == COA_EGearFixSlot.AR || slot == COA_EGearFixSlot.AT || slot == COA_EGearFixSlot.MMG || slot == COA_EGearFixSlot.HMG
			|| slot == COA_EGearFixSlot.MAT || slot == COA_EGearFixSlot.HAT || slot == COA_EGearFixSlot.AA;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsCustomSlot(int slot)
	{
		return slot == COA_EGearFixSlot.CUSTOM_PRIMARY || slot == COA_EGearFixSlot.CUSTOM_SECONDARY || slot == COA_EGearFixSlot.CUSTOM_PISTOL;
	}

	//------------------------------------------------------------------------------------------------
	//! Randomised pools: these (and custom slots) can also be overridden for a single role
	static bool IsPoolSlot(int slot)
	{
		return slot == COA_EGearFixSlot.RIFLE || slot == COA_EGearFixSlot.RIFLEUGL || slot == COA_EGearFixSlot.CARBINE || slot == COA_EGearFixSlot.PISTOL;
	}

	//------------------------------------------------------------------------------------------------
	//! COA_EGearscriptMagazines for a default slot, -1 for custom slots
	static int GetMagazineType(int slot)
	{
		switch (slot)
		{
			case COA_EGearFixSlot.RIFLE: return COA_EGearscriptMagazines.RIFLE_MAG;
			case COA_EGearFixSlot.RIFLEUGL: return COA_EGearscriptMagazines.RIFLEUGL_MAG;
			case COA_EGearFixSlot.CARBINE: return COA_EGearscriptMagazines.CARBINE_MAG;
			case COA_EGearFixSlot.SNIPER: return COA_EGearscriptMagazines.SNIPER_MAG;
			case COA_EGearFixSlot.PISTOL: return COA_EGearscriptMagazines.PISTOL_MAG;
			case COA_EGearFixSlot.AR: return COA_EGearscriptMagazines.AR_MAG;
			case COA_EGearFixSlot.AT: return COA_EGearscriptMagazines.AT_MAG;
			case COA_EGearFixSlot.MMG: return COA_EGearscriptMagazines.MMG_MAG;
			case COA_EGearFixSlot.HMG: return COA_EGearscriptMagazines.HMG_MAG;
			case COA_EGearFixSlot.MAT: return COA_EGearscriptMagazines.MAT_MAG;
			case COA_EGearFixSlot.HAT: return COA_EGearscriptMagazines.HAT_MAG;
			case COA_EGearFixSlot.AA: return COA_EGearscriptMagazines.AA_MAG;
		}

		return -1;
	}

	//------------------------------------------------------------------------------------------------
	//! COA_EGearscriptWeapons -> COA_EGearFixSlot (same values for the default slots)
	static int SlotFromWeaponType(COA_EGearscriptWeapons weaponType)
	{
		return weaponType;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsAssistantRole(COA_RoleConfig roleConfig)
	{
		if (!roleConfig)
			return false;

		return roleConfig.m_SlottingType == COA_ESlotType.ASSISTANT || roleConfig.m_SlottingType == COA_ESlotType.SPECIALTY_ASSISTANT;
	}

	//------------------------------------------------------------------------------------------------
	static bool HasCustomWeapons(COA_GearScriptConfig gearConfig, int role)
	{
		if (!gearConfig || role < 0)
			return false;

		COA_Role_Custom_Gear customGear = COA_LoadoutPreviewHelper.FindCustomGear(gearConfig, role);
		if (!customGear)
			return false;

		return HasWeapons(customGear.m_PrimaryWeapon) || HasWeapons(customGear.m_SecondaryWeapon) || HasWeapons(customGear.m_Pistols);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HasWeapons(array<ref COA_Weapon_Class> pool)
	{
		if (!pool)
			return false;

		foreach (COA_Weapon_Class weapon : pool)
		{
			if (weapon != null && !weapon.m_Weapon.IsEmpty())
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! The gearscript entries a slot can give (several for randomised pools)
	//! \param[in] role needed for custom slots only
	static void GetSlotEntries(COA_GearScriptConfig gearConfig, int role, int slot, notnull array<ref COA_GearFixSlotEntry> entries)
	{
		entries.Clear();
		if (!gearConfig)
			return;

		switch (slot)
		{
			case COA_EGearFixSlot.RIFLE: AddPool(gearConfig.m_Rifles, entries); return;
			case COA_EGearFixSlot.RIFLEUGL: AddPool(gearConfig.m_RifleUGLs, entries); return;
			case COA_EGearFixSlot.CARBINE: AddPool(gearConfig.m_Carbines, entries); return;
			case COA_EGearFixSlot.PISTOL: AddPool(gearConfig.m_Pistols, entries); return;
			case COA_EGearFixSlot.SNIPER: AddWeapon(gearConfig.m_SNIPER, entries); return;
			case COA_EGearFixSlot.AR: AddSpec(gearConfig.m_AR, entries); return;
			case COA_EGearFixSlot.AT: AddSpec(gearConfig.m_AT, entries); return;
			case COA_EGearFixSlot.MMG: AddSpec(gearConfig.m_MMG, entries); return;
			case COA_EGearFixSlot.HMG: AddSpec(gearConfig.m_HMG, entries); return;
			case COA_EGearFixSlot.MAT: AddSpec(gearConfig.m_MAT, entries); return;
			case COA_EGearFixSlot.HAT: AddSpec(gearConfig.m_HAT, entries); return;
			case COA_EGearFixSlot.AA: AddSpec(gearConfig.m_AA, entries); return;
		}

		COA_Role_Custom_Gear customGear;
		if (role >= 0)
			customGear = COA_LoadoutPreviewHelper.FindCustomGear(gearConfig, role);

		if (!customGear)
			return;

		switch (slot)
		{
			case COA_EGearFixSlot.CUSTOM_PRIMARY: AddPool(customGear.m_PrimaryWeapon, entries); return;
			case COA_EGearFixSlot.CUSTOM_SECONDARY: AddPool(customGear.m_SecondaryWeapon, entries); return;
			case COA_EGearFixSlot.CUSTOM_PISTOL: AddPool(customGear.m_Pistols, entries); return;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddPool(array<ref COA_Weapon_Class> pool, notnull array<ref COA_GearFixSlotEntry> entries)
	{
		if (!pool)
			return;

		foreach (COA_Weapon_Class weapon : pool)
			AddWeapon(weapon, entries);
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddWeapon(COA_Weapon_Class weapon, notnull array<ref COA_GearFixSlotEntry> entries)
	{
		if (!weapon || weapon.m_Weapon.IsEmpty())
			return;

		COA_GearFixSlotEntry entry = new COA_GearFixSlotEntry();
		entry.m_rWeapon = weapon.m_Weapon;
		if (weapon.m_Attachments)
			entry.m_aAttachments.InsertAll(weapon.m_Attachments);

		if (weapon.m_MagazineArray)
		{
			foreach (COA_Magazine_Class magazine : weapon.m_MagazineArray)
			{
				if (!magazine || magazine.m_Magazine.IsEmpty())
					continue;

				entry.m_aMagazines.Insert(magazine.m_Magazine);
				entry.m_aCounts.Insert(magazine.m_MagazineCount);
				entry.m_aAssistantCounts.Insert(magazine.m_MagazineCount); // pools have no assistant count
			}
		}

		entries.Insert(entry);
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddSpec(COA_Spec_Weapon_Class weapon, notnull array<ref COA_GearFixSlotEntry> entries)
	{
		if (!weapon || weapon.m_Weapon.IsEmpty())
			return;

		COA_GearFixSlotEntry entry = new COA_GearFixSlotEntry();
		entry.m_rWeapon = weapon.m_Weapon;
		if (weapon.m_Attachments)
			entry.m_aAttachments.InsertAll(weapon.m_Attachments);

		if (weapon.m_MagazineArray)
		{
			foreach (COA_Spec_Magazine_Class magazine : weapon.m_MagazineArray)
			{
				if (!magazine || magazine.m_Magazine.IsEmpty())
					continue;

				entry.m_aMagazines.Insert(magazine.m_Magazine);
				entry.m_aCounts.Insert(magazine.m_MagazineCount);
				entry.m_aAssistantCounts.Insert(magazine.m_AssistantMagazineCount);
			}
		}

		entries.Insert(entry);
	}

	//------------------------------------------------------------------------------------------------
	//! Whether, and how, a role takes part in a slot
	static COA_GearFixRoleUse GetRoleUse(COA_GearScriptConfig gearConfig, COA_RoleConfig roleConfig, int role, int slot)
	{
		COA_GearFixRoleUse use = new COA_GearFixRoleUse();
		if (!gearConfig || !roleConfig)
			return use;

		use.m_bAssistant = IsAssistantRole(roleConfig);

		if (IsCustomSlot(slot))
		{
			array<ref COA_GearFixSlotEntry> entries = {};
			GetSlotEntries(gearConfig, role, slot, entries);
			use.m_bAffected = !entries.IsEmpty();
			use.m_bGivesWeapon = use.m_bAffected;
			use.m_bGetsMagazines = use.m_bAffected;
			use.m_bAssistant = false; // custom weapons always get their full count
			return use;
		}

		if (HasCustomWeapons(gearConfig, role))
			return use;

		if (roleConfig.m_aWeapons)
			use.m_bGivesWeapon = roleConfig.m_aWeapons.Contains(slot);

		if (roleConfig.m_aMagazines)
			use.m_bGetsMagazines = roleConfig.m_aMagazines.Contains(GetMagazineType(slot));

		use.m_bAffected = use.m_bGivesWeapon || use.m_bGetsMagazines;

		// Only specialty ammo has an assistant count
		if (!IsSpecSlot(slot))
			use.m_bAssistant = false;

		return use;
	}

	//------------------------------------------------------------------------------------------------
	//! Weapon slots a role is issued (custom slots if it has custom weapons, else its m_aWeapons)
	static void GetRoleSlots(COA_GearScriptConfig gearConfig, COA_RoleConfig roleConfig, int role, notnull array<int> slots)
	{
		slots.Clear();
		if (!gearConfig || !roleConfig)
			return;

		if (HasCustomWeapons(gearConfig, role))
		{
			array<int> customSlots = {COA_EGearFixSlot.CUSTOM_PRIMARY, COA_EGearFixSlot.CUSTOM_SECONDARY, COA_EGearFixSlot.CUSTOM_PISTOL};
			foreach (int customSlot : customSlots)
			{
				array<ref COA_GearFixSlotEntry> entries = {};
				GetSlotEntries(gearConfig, role, customSlot, entries);
				if (!entries.IsEmpty())
					slots.Insert(customSlot);
			}

			return;
		}

		if (!roleConfig.m_aWeapons)
			return;

		foreach (COA_EGearscriptWeapons weaponType : roleConfig.m_aWeapons)
		{
			int slot = SlotFromWeaponType(weaponType);
			if (!slots.Contains(slot))
				slots.Insert(slot);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Magazines the role is issued for its OTHER slots (prefab -> highest expected count), so a
	//! weapon swap never takes away ammo another weapon of the same role also uses
	static void GetOtherSlotMagazines(COA_GearScriptConfig gearConfig, COA_RoleConfig roleConfig, int role, int excludedSlot, notnull map<ResourceName, int> magazines)
	{
		magazines.Clear();
		if (!gearConfig || !roleConfig)
			return;

		array<int> slots = {};
		if (HasCustomWeapons(gearConfig, role))
		{
			GetRoleSlots(gearConfig, roleConfig, role, slots);
		}
		else if (roleConfig.m_aMagazines)
		{
			// Default slots hand out magazines per m_aMagazines, whether or not the weapon is issued
			array<int> defaultSlots = {COA_EGearFixSlot.RIFLE, COA_EGearFixSlot.RIFLEUGL, COA_EGearFixSlot.CARBINE, COA_EGearFixSlot.SNIPER, COA_EGearFixSlot.PISTOL,
				COA_EGearFixSlot.AR, COA_EGearFixSlot.AT, COA_EGearFixSlot.MMG, COA_EGearFixSlot.HMG, COA_EGearFixSlot.MAT, COA_EGearFixSlot.HAT, COA_EGearFixSlot.AA};
			foreach (int defaultSlot : defaultSlots)
			{
				if (roleConfig.m_aMagazines.Contains(GetMagazineType(defaultSlot)))
					slots.Insert(defaultSlot);
			}
		}

		bool assistant = IsAssistantRole(roleConfig);
		foreach (int slot : slots)
		{
			if (slot == excludedSlot)
				continue;

			array<ref COA_GearFixSlotEntry> entries = {};
			GetSlotEntries(gearConfig, role, slot, entries);
			foreach (COA_GearFixSlotEntry entry : entries)
			{
				for (int i = 0; i < entry.m_aMagazines.Count(); i++)
				{
					int count = entry.m_aCounts[i];
					if (assistant && IsSpecSlot(slot))
						count = entry.m_aAssistantCounts[i];

					int current;
					magazines.Find(entry.m_aMagazines[i], current);
					if (count > current)
						magazines.Set(entry.m_aMagazines[i], count);
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! How many of an item the gearscript gives a role outside its weapons - mirrors
	//! COA_GearscriptManager.ApplyInventoryItems (custom additional items, radios, binoculars,
	//! medic items, infantry medical, default items)
	static int CountKitItem(COA_GearScriptConfig gearConfig, COA_GearScriptContainer gearSettings, COA_RoleConfig roleConfig, int role, ResourceName prefab)
	{
		map<ResourceName, int> kit = new map<ResourceName, int>();
		GetKitItems(gearConfig, gearSettings, roleConfig, role, kit);

		int count;
		kit.Find(prefab, count);
		return count;
	}

	//------------------------------------------------------------------------------------------------
	//! Every non-weapon item the gearscript gives a role, with counts
	static void GetKitItems(COA_GearScriptConfig gearConfig, COA_GearScriptContainer gearSettings, COA_RoleConfig roleConfig, int role, notnull map<ResourceName, int> kit)
	{
		kit.Clear();
		if (!gearConfig || !roleConfig)
			return;

		COA_Role_Custom_Gear customGear = COA_LoadoutPreviewHelper.FindCustomGear(gearConfig, role);
		if (customGear)
			AddItems(customGear.m_AdditionalInventoryItems, kit);

		if (roleConfig.m_aItems)
		{
			COA_ESlotType slotType = roleConfig.m_SlottingType;
			bool leadershipType = slotType == COA_ESlotType.TEAM_LEADER || slotType == COA_ESlotType.SQUAD_LEADER || slotType == COA_ESlotType.SPECIALTY || slotType == COA_ESlotType.SPECIALTY_ASSISTANT;

			foreach (COA_EGearscriptItems item : roleConfig.m_aItems)
			{
				switch (item)
				{
					case COA_EGearscriptItems.SHORTRANGE_RADIO:
						if (gearSettings != null && (gearSettings.m_bEnableGIRadios || (gearSettings.m_bEnableLeadershipRadios && leadershipType)))
							AddItem(gearSettings.m_rShortRangeRadioPrefab, 1, kit);
						break;

					case COA_EGearscriptItems.LONGRANGE_RADIO:
						if (gearSettings != null && gearSettings.m_bEnableLeadershipRadios)
							AddItem(gearSettings.m_rLongRangeRadioPrefab, 1, kit);
						break;

					case COA_EGearscriptItems.RTO_RADIO:
						if (gearSettings != null && gearSettings.m_bEnableRTORadios)
							AddItem(gearSettings.m_rRTORadiosPrefab, 1, kit);
						break;

					case COA_EGearscriptItems.LEADERSHIP_BINO:
						AddItem(gearConfig.m_sLeadershipBinocularsPrefab, 1, kit);
						break;

					case COA_EGearscriptItems.ASSISTANT_BINO:
						AddItem(gearConfig.m_sAssistantBinocularsPrefab, 1, kit);
						break;

					case COA_EGearscriptItems.MEDIC_ITEMS:
						AddItems(gearConfig.m_MedicMedicalItems, kit);
						break;
				}
			}
		}

		AddItems(gearConfig.m_InfantryMedicalItems, kit);
		AddItems(gearConfig.m_DefaultInventoryItems, kit);
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddItems(array<ref COA_Inventory_Item> items, notnull map<ResourceName, int> kit)
	{
		if (!items)
			return;

		foreach (COA_Inventory_Item item : items)
		{
			if (item)
				AddItem(item.m_sItemPrefab, item.m_iItemCount, kit);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddItem(ResourceName prefab, int count, notnull map<ResourceName, int> kit)
	{
		if (prefab.IsEmpty() || count <= 0)
			return;

		int current;
		kit.Find(prefab, current);
		kit.Set(prefab, current + count);
	}

	//------------------------------------------------------------------------------------------------
	//! Roles slotted for a faction, in slotting order, with how many slots each has
	static void GetSlottedRoles(FactionKey factionKey, notnull array<int> roles, map<int, int> counts = null)
	{
		roles.Clear();
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode)
			return;

		array<ref COA_SlottingGroup> groups = gamemode.GetSlots(factionKey);
		if (!groups)
			return;

		foreach (COA_SlottingGroup group : groups)
		{
			if (!group || !group.m_aSlots)
				continue;

			foreach (COA_EGearRole role : group.m_aSlots)
			{
				if (!roles.Contains(role))
					roles.Insert(role);

				if (counts)
				{
					int current;
					counts.Find(role, current);
					counts.Set(role, current + 1);
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Factions that have slots and a gearscript in this mission
	static void GetPlayableFactions(notnull array<string> factions)
	{
		factions.Clear();
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode)
			return;

		array<string> keys = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
		foreach (string key : keys)
		{
			array<int> roles = {};
			GetSlottedRoles(key, roles);
			if (roles.IsEmpty())
				continue;

			factions.Insert(key);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Gearscript resource of a faction without the "no gearscript" warning GetGearScriptResource prints
	static ResourceName GetGearscriptResource(FactionKey factionKey)
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode || !gamemode.GetGearScriptSettings(factionKey))
			return ResourceName.Empty;

		return gamemode.GetGearScriptResource(factionKey);
	}

	//------------------------------------------------------------------------------------------------
	static COA_GearScriptConfig LoadGearscript(FactionKey factionKey)
	{
		COA_GearscriptManager gearscriptManager = COA_GearscriptManager.GetInstance();
		if (!gearscriptManager)
			return null;

		ResourceName resource = GetGearscriptResource(factionKey);
		if (resource.IsEmpty())
			return null;

		// LoadGearScriptConfig assumes the file exists; a gearscript from a mod that isn't loaded doesn't
		Resource check = Resource.Load(resource);
		if (check == null || !check.IsValid())
			return null;

		return gearscriptManager.LoadGearScriptConfig(resource);
	}
}
