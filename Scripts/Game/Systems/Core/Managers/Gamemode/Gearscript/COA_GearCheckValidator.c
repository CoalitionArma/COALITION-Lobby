//------------------------------------------------------------------------------------------------
// Static gearscript check, run on the server when a mission loads (COA_GearFixManager), on demand
// from the admin Gear Fix tab, and by CRF's Mission QA menu. Reads configs and prefab containers
// only - nothing is spawned. What it can't see from data (an item that really didn't fit at spawn,
// an overweight character) is caught at runtime instead: COA_LoggingHelper reports those to
// COA_GearFixManager.RecordSpawnFailure as they happen.
//
// Checks, per playable faction:
//  - the faction has a gearscript and it loads;
//  - every prefab in it resolves (mod loaded);
//  - weapon entries are weapons, magazines fit their weapon's wells (incl. UGL attachments),
//    attachments fit the weapon's attachment slots (best effort - skipped without type data);
// and per role actually present in the slotting:
//  - the role's weapon slots are filled and come with ammo (assistant ammo for assistants);
//  - leaders' binoculars, radios and medics' medical items are configured;
//  - an estimate of whether the kit fits the storage the role actually wears, and its weight.
//------------------------------------------------------------------------------------------------
class COA_GearCheckValidator
{
	protected static const float MAX_KIT_WEIGHT = 60; // same limit COA_GearscriptManager.SetEntityGear logs

	//------------------------------------------------------------------------------------------------
	//! Validate every playable faction
	static void ValidateAll(notnull array<ref COA_GearCheckIssue> issues)
	{
		array<string> factions = {};
		COA_GearFixSlotHelper.GetPlayableFactions(factions);
		foreach (string faction : factions)
			ValidateFaction(faction, issues);
	}

	//------------------------------------------------------------------------------------------------
	static void ValidateFaction(FactionKey faction, notnull array<ref COA_GearCheckIssue> issues)
	{
		set<string> seen = new set<string>();
		foreach (COA_GearCheckIssue existing : issues)
			seen.Insert(existing.GetKey());

		ResourceName gearscriptResource = COA_GearFixSlotHelper.GetGearscriptResource(faction);
		if (gearscriptResource.IsEmpty())
		{
			Add(issues, seen, COA_EGearCheckSeverity.ERROR, faction, -1, -1, "No gearscript assigned to this faction", "");
			return;
		}

		COA_GearScriptConfig gearConfig = COA_GearFixSlotHelper.LoadGearscript(faction);
		if (!gearConfig)
		{
			Add(issues, seen, COA_EGearCheckSeverity.ERROR, faction, -1, -1, "Gearscript did not load", gearscriptResource);
			return;
		}

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		COA_GearScriptContainer gearSettings = gamemode.GetGearScriptSettings(faction);

		CheckFactionPrefabs(faction, gearConfig, gearSettings, issues, seen);
		CheckFactionWeapons(faction, gearConfig, issues, seen);

		COA_RolesConfig rolesConfig = COA_GearscriptManager.GetRolesConfig();
		array<int> roles = {};
		COA_GearFixSlotHelper.GetSlottedRoles(faction, roles);
		foreach (int role : roles)
		{
			COA_RoleConfig roleConfig;
			if (rolesConfig)
				roleConfig = rolesConfig.FindRoleConfig(role);

			if (!roleConfig)
			{
				Add(issues, seen, COA_EGearCheckSeverity.ERROR, faction, role, -1, "Role is slotted but missing from the global roles config - it spawns as a rifleman", "");
				continue;
			}

			CheckRoleWeapons(faction, gearConfig, roleConfig, role, issues, seen);
			CheckRoleItems(faction, gearConfig, gearSettings, roleConfig, role, issues, seen);
			CheckRoleCapacity(faction, gearConfig, gearSettings, roleConfig, role, issues, seen);
		}
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 FACTION-WIDE
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Every prefab outside the weapon slots resolves
	protected static void CheckFactionPrefabs(FactionKey faction, COA_GearScriptConfig gearConfig, COA_GearScriptContainer gearSettings, notnull array<ref COA_GearCheckIssue> issues, notnull set<string> seen)
	{
		if (gearConfig.m_DefaultClothing)
		{
			foreach (COA_Clothing clothing : gearConfig.m_DefaultClothing)
				CheckClothing(faction, -1, clothing, issues, seen);
		}

		CheckPrefab(faction, -1, -1, gearConfig.m_sLeadershipBinocularsPrefab, "Leadership binoculars", issues, seen);
		CheckPrefab(faction, -1, -1, gearConfig.m_sAssistantBinocularsPrefab, "Assistant binoculars", issues, seen);
		CheckItemList(faction, -1, gearConfig.m_DefaultInventoryItems, "Default items", issues, seen);
		CheckItemList(faction, -1, gearConfig.m_InfantryMedicalItems, "Infantry medical items", issues, seen);
		CheckItemList(faction, -1, gearConfig.m_MedicMedicalItems, "Medic medical items", issues, seen);

		if (gearSettings)
		{
			if (gearSettings.m_bEnableGIRadios || gearSettings.m_bEnableLeadershipRadios)
				CheckPrefab(faction, -1, -1, gearSettings.m_rShortRangeRadioPrefab, "Short range radio", issues, seen);

			if (gearSettings.m_bEnableLeadershipRadios)
				CheckPrefab(faction, -1, -1, gearSettings.m_rLongRangeRadioPrefab, "Long range radio", issues, seen);

			if (gearSettings.m_bEnableRTORadios)
				CheckPrefab(faction, -1, -1, gearSettings.m_rRTORadiosPrefab, "RTO radio", issues, seen);
		}

		if (!gearConfig.m_RolesToSetCustomSettings)
			return;

		foreach (COA_Role_Custom_Gear customGear : gearConfig.m_RolesToSetCustomSettings)
		{
			if (!customGear)
				continue;

			if (customGear.m_Clothing)
			{
				foreach (COA_Clothing clothing : customGear.m_Clothing)
					CheckClothing(faction, customGear.m_Role, clothing, issues, seen);
			}

			CheckItemList(faction, customGear.m_Role, customGear.m_AdditionalInventoryItems, "Custom role items", issues, seen);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void CheckClothing(FactionKey faction, int role, COA_Clothing clothing, notnull array<ref COA_GearCheckIssue> issues, notnull set<string> seen)
	{
		if (!clothing || !clothing.m_ClothingPrefabs)
			return;

		string label = "Clothing " + SCR_Enum.GetEnumName(COA_EGearscriptClothing, clothing.m_iClothingType);
		foreach (ResourceName prefab : clothing.m_ClothingPrefabs)
		{
			// An empty entry is a deliberate "nothing" option in a random pool
			if (!prefab.IsEmpty())
				CheckPrefab(faction, role, -1, prefab, label, issues, seen);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void CheckItemList(FactionKey faction, int role, array<ref COA_Inventory_Item> items, string label, notnull array<ref COA_GearCheckIssue> issues, notnull set<string> seen)
	{
		if (!items)
			return;

		foreach (COA_Inventory_Item item : items)
		{
			if (item)
				CheckPrefab(faction, role, -1, item.m_sItemPrefab, label, issues, seen);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \return true if the prefab is set and resolves
	protected static bool CheckPrefab(FactionKey faction, int role, int slot, ResourceName prefab, string label, notnull array<ref COA_GearCheckIssue> issues, notnull set<string> seen)
	{
		if (prefab.IsEmpty())
			return false;

		if (COA_GearPrefabHelper.IsValidPrefab(prefab))
			return true;

		Add(issues, seen, COA_EGearCheckSeverity.ERROR, faction, role, slot, label + ": prefab not found (mod not loaded or file moved)", prefab);
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Every weapon entry of every slot: weapon, magazines, attachments
	protected static void CheckFactionWeapons(FactionKey faction, COA_GearScriptConfig gearConfig, notnull array<ref COA_GearCheckIssue> issues, notnull set<string> seen)
	{
		for (int slot = COA_EGearFixSlot.RIFLE; slot <= COA_EGearFixSlot.AA; slot++)
		{
			array<ref COA_GearFixSlotEntry> entries = {};
			COA_GearFixSlotHelper.GetSlotEntries(gearConfig, -1, slot, entries);
			foreach (COA_GearFixSlotEntry entry : entries)
				CheckWeaponEntry(faction, -1, slot, entry, issues, seen);
		}

		if (!gearConfig.m_RolesToSetCustomSettings)
			return;

		foreach (COA_Role_Custom_Gear customGear : gearConfig.m_RolesToSetCustomSettings)
		{
			if (!customGear)
				continue;

			for (int customSlot = COA_EGearFixSlot.CUSTOM_PRIMARY; customSlot <= COA_EGearFixSlot.CUSTOM_PISTOL; customSlot++)
			{
				array<ref COA_GearFixSlotEntry> customEntries = {};
				COA_GearFixSlotHelper.GetSlotEntries(gearConfig, customGear.m_Role, customSlot, customEntries);
				foreach (COA_GearFixSlotEntry customEntry : customEntries)
					CheckWeaponEntry(faction, customGear.m_Role, customSlot, customEntry, issues, seen);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	static void CheckWeaponEntry(FactionKey faction, int role, int slot, COA_GearFixSlotEntry entry, notnull array<ref COA_GearCheckIssue> issues, notnull set<string> seen)
	{
		string label = COA_GearFixText.SlotLabel(slot);
		bool weaponValid = CheckPrefab(faction, role, slot, entry.m_rWeapon, label + " weapon", issues, seen);
		if (weaponValid && !COA_GearPrefabHelper.IsWeapon(entry.m_rWeapon))
		{
			Add(issues, seen, COA_EGearCheckSeverity.ERROR, faction, role, slot, label + " weapon entry is not a weapon prefab", entry.m_rWeapon);
			weaponValid = false;
		}

		array<ResourceName> validAttachments = {};
		foreach (ResourceName attachment : entry.m_aAttachments)
		{
			if (!CheckPrefab(faction, role, slot, attachment, label + " attachment", issues, seen))
				continue;

			validAttachments.Insert(attachment);
			if (!weaponValid)
				continue;

			int fits = COA_GearPrefabHelper.AttachmentFits(entry.m_rWeapon, attachment);
			if (fits == 0)
				Add(issues, seen, COA_EGearCheckSeverity.WARNING, faction, role, slot, string.Format("Attachment %1 has no matching slot on %2 (it will not be attached)", COA_GearFixText.Name(attachment), COA_GearFixText.Name(entry.m_rWeapon)), attachment);
		}

		foreach (ResourceName magazine : entry.m_aMagazines)
		{
			if (!CheckPrefab(faction, role, slot, magazine, label + " magazine", issues, seen) || !weaponValid)
				continue;

			int magazineFits = COA_GearPrefabHelper.MagazineFits(entry.m_rWeapon, magazine, validAttachments);
			if (magazineFits == 0)
				Add(issues, seen, COA_EGearCheckSeverity.ERROR, faction, role, slot, string.Format("Magazine %1 does not fit %2 (well %3)", COA_GearFixText.Name(magazine), COA_GearFixText.Name(entry.m_rWeapon), COA_GearPrefabHelper.GetMagazineWell(magazine)), magazine);
		}
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 PER ROLE
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected static void CheckRoleWeapons(FactionKey faction, COA_GearScriptConfig gearConfig, COA_RoleConfig roleConfig, int role, notnull array<ref COA_GearCheckIssue> issues, notnull set<string> seen)
	{
		bool assistant = COA_GearFixSlotHelper.IsAssistantRole(roleConfig);

		if (COA_GearFixSlotHelper.HasCustomWeapons(gearConfig, role))
		{
			array<int> customSlots = {};
			COA_GearFixSlotHelper.GetRoleSlots(gearConfig, roleConfig, role, customSlots);
			foreach (int customSlot : customSlots)
			{
				array<ref COA_GearFixSlotEntry> customEntries = {};
				COA_GearFixSlotHelper.GetSlotEntries(gearConfig, role, customSlot, customEntries);
				foreach (COA_GearFixSlotEntry customEntry : customEntries)
				{
					if (customEntry.GetMagazineTotal(false) <= 0 && !IsSingleShotLauncher(customEntry.m_rWeapon))
						Add(issues, seen, COA_EGearCheckSeverity.WARNING, faction, role, customSlot, string.Format("Custom %1 %2 comes without magazines", COA_GearFixText.SlotLabel(customSlot), COA_GearFixText.Name(customEntry.m_rWeapon)), customEntry.m_rWeapon);
				}
			}

			// Custom weapons replace the defaults, including the role's assistant ammo
			if (assistant && roleConfig.m_aMagazines != null && !roleConfig.m_aMagazines.IsEmpty())
				Add(issues, seen, COA_EGearCheckSeverity.WARNING, faction, role, -1, "Assistant role has custom weapons, so it gets none of its default (assistant) magazines", "");

			return;
		}

		// Default weapons the role is issued
		if (roleConfig.m_aWeapons)
		{
			foreach (COA_EGearscriptWeapons weaponType : roleConfig.m_aWeapons)
			{
				int slot = COA_GearFixSlotHelper.SlotFromWeaponType(weaponType);
				array<ref COA_GearFixSlotEntry> entries = {};
				COA_GearFixSlotHelper.GetSlotEntries(gearConfig, role, slot, entries);
				if (entries.IsEmpty())
				{
					Add(issues, seen, COA_EGearCheckSeverity.ERROR, faction, role, slot, string.Format("Role is issued a %1 but the gearscript has none (%2 is empty)", COA_GearFixText.SlotLabel(slot), COA_GearFixText.SlotFieldName(slot)), "");
					continue;
				}

				int slotMagazineType = COA_GearFixSlotHelper.GetMagazineType(slot);
				if (!roleConfig.m_aMagazines || !roleConfig.m_aMagazines.Contains(slotMagazineType))
				{
					if (!IsSingleShotLauncher(entries[0].m_rWeapon))
						Add(issues, seen, COA_EGearCheckSeverity.WARNING, faction, role, slot, string.Format("Role is issued a %1 but its role config has no %2 - it spawns without spare ammo", COA_GearFixText.SlotLabel(slot), SCR_Enum.GetEnumName(COA_EGearscriptMagazines, slotMagazineType)), "");

					continue;
				}

				if (entries[0].GetMagazineTotal(false) <= 0 && !IsSingleShotLauncher(entries[0].m_rWeapon))
					Add(issues, seen, COA_EGearCheckSeverity.ERROR, faction, role, slot, string.Format("%1 %2 has no magazines in the gearscript", COA_GearFixText.SlotLabel(slot), COA_GearFixText.Name(entries[0].m_rWeapon)), entries[0].m_rWeapon);
			}
		}

		// Ammo carried for another role's weapon
		if (!assistant || !roleConfig.m_aMagazines)
			return;

		foreach (COA_EGearscriptMagazines magazineType : roleConfig.m_aMagazines)
		{
			int ammoSlot = magazineType; // COA_EGearscriptMagazines and COA_EGearFixSlot share values
			if (!COA_GearFixSlotHelper.IsSpecSlot(ammoSlot))
				continue;

			array<ref COA_GearFixSlotEntry> ammoEntries = {};
			COA_GearFixSlotHelper.GetSlotEntries(gearConfig, role, ammoSlot, ammoEntries);
			if (ammoEntries.IsEmpty())
			{
				Add(issues, seen, COA_EGearCheckSeverity.ERROR, faction, role, ammoSlot, string.Format("Assistant carries %1 ammo but the gearscript has no %1 (%2 is empty)", COA_GearFixText.SlotLabel(ammoSlot), COA_GearFixText.SlotFieldName(ammoSlot)), "");
				continue;
			}

			if (ammoEntries[0].GetMagazineTotal(true) <= 0)
				Add(issues, seen, COA_EGearCheckSeverity.WARNING, faction, role, ammoSlot, string.Format("Assistant gets no %1 ammo: every m_AssistantMagazineCount of %2 is 0", COA_GearFixText.SlotLabel(ammoSlot), COA_GearFixText.Name(ammoEntries[0].m_rWeapon)), ammoEntries[0].m_rWeapon);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Disposable launchers (M72, RPG-26...) are loaded and have nothing to reload with
	protected static bool IsSingleShotLauncher(ResourceName weapon)
	{
		if (weapon.IsEmpty() || !COA_GearPrefabHelper.IsWeapon(weapon))
			return false;

		if (COA_GearPrefabHelper.HasMuzzleInMag(weapon))
			return true;

		// No reloadable well at all
		array<string> wells = {};
		COA_GearPrefabHelper.GetOwnMagazineWells(weapon, wells);
		return wells.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	protected static void CheckRoleItems(FactionKey faction, COA_GearScriptConfig gearConfig, COA_GearScriptContainer gearSettings, COA_RoleConfig roleConfig, int role, notnull array<ref COA_GearCheckIssue> issues, notnull set<string> seen)
	{
		if (!roleConfig.m_aItems)
			return;

		COA_ESlotType slotType = roleConfig.m_SlottingType;
		bool leadershipType = slotType == COA_ESlotType.TEAM_LEADER || slotType == COA_ESlotType.SQUAD_LEADER || slotType == COA_ESlotType.SPECIALTY || slotType == COA_ESlotType.SPECIALTY_ASSISTANT;

		foreach (COA_EGearscriptItems item : roleConfig.m_aItems)
		{
			switch (item)
			{
				case COA_EGearscriptItems.LEADERSHIP_BINO:
					if (gearConfig.m_sLeadershipBinocularsPrefab.IsEmpty())
						Add(issues, seen, COA_EGearCheckSeverity.WARNING, faction, role, -1, "Leader role gets binoculars but m_sLeadershipBinocularsPrefab is empty", "");
					break;

				case COA_EGearscriptItems.ASSISTANT_BINO:
					if (gearConfig.m_sAssistantBinocularsPrefab.IsEmpty())
						Add(issues, seen, COA_EGearCheckSeverity.WARNING, faction, role, -1, "Role gets assistant binoculars but m_sAssistantBinocularsPrefab is empty", "");
					break;

				case COA_EGearscriptItems.MEDIC_ITEMS:
					CheckMedicItems(faction, gearConfig, role, issues, seen);
					break;

				case COA_EGearscriptItems.SHORTRANGE_RADIO:
					if (gearSettings != null && (gearSettings.m_bEnableGIRadios || (gearSettings.m_bEnableLeadershipRadios && leadershipType)) && gearSettings.m_rShortRangeRadioPrefab.IsEmpty())
						Add(issues, seen, COA_EGearCheckSeverity.ERROR, faction, role, -1, "Role should get a short range radio but none is configured", "");
					break;

				case COA_EGearscriptItems.LONGRANGE_RADIO:
					if (gearSettings != null && gearSettings.m_bEnableLeadershipRadios && gearSettings.m_rLongRangeRadioPrefab.IsEmpty())
						Add(issues, seen, COA_EGearCheckSeverity.ERROR, faction, role, -1, "Role should get a long range radio but none is configured", "");
					break;

				case COA_EGearscriptItems.RTO_RADIO:
					if (gearSettings != null && gearSettings.m_bEnableRTORadios && gearSettings.m_rRTORadiosPrefab.IsEmpty())
						Add(issues, seen, COA_EGearCheckSeverity.ERROR, faction, role, -1, "RTO should get a radio but none is configured", "");
					else if (gearSettings != null && !gearSettings.m_bEnableRTORadios)
						Add(issues, seen, COA_EGearCheckSeverity.INFO, faction, role, -1, "RTO radios are disabled for this faction - the RTO spawns without one", "");
					break;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void CheckMedicItems(FactionKey faction, COA_GearScriptConfig gearConfig, int role, notnull array<ref COA_GearCheckIssue> issues, notnull set<string> seen)
	{
		if (!gearConfig.m_MedicMedicalItems || gearConfig.m_MedicMedicalItems.IsEmpty())
		{
			Add(issues, seen, COA_EGearCheckSeverity.ERROR, faction, role, -1, "Medic role but m_MedicMedicalItems is empty", "");
			return;
		}

		// Same heuristic as CRF's Mission QA menu: a medic needs a medical kit to fully heal others
		foreach (COA_Inventory_Item item : gearConfig.m_MedicMedicalItems)
		{
			if (!item)
				continue;

			string path = item.m_sItemPrefab.GetPath();
			path.ToLower();
			if (path.Contains("medicalkit") || path.Contains("medkit"))
				return;
		}

		Add(issues, seen, COA_EGearCheckSeverity.INFO, faction, role, -1, "No medical kit among the medic items (by name) - check medics can fully heal", "");
	}

	//------------------------------------------------------------------------------------------------
	//! Estimate: everything that goes into storage vs what the worn clothing holds, by volume
	//! (ItemPhysAttributes.ItemVolume vs storages' MaxCumulativeVolume), worst and best case of the
	//! random clothing pools. Items without a volume on the prefab are skipped and counted.
	protected static void CheckRoleCapacity(FactionKey faction, COA_GearScriptConfig gearConfig, COA_GearScriptContainer gearSettings, COA_RoleConfig roleConfig, int role, notnull array<ref COA_GearCheckIssue> issues, notnull set<string> seen)
	{
		// Worn clothing per type: default clothing, then the role's custom clothing replaces types
		map<int, ref array<ResourceName>> worn = new map<int, ref array<ResourceName>>();
		if (gearConfig.m_DefaultClothing)
		{
			foreach (COA_Clothing clothing : gearConfig.m_DefaultClothing)
			{
				if (clothing != null && clothing.m_ClothingPrefabs != null && !clothing.m_ClothingPrefabs.IsEmpty())
					worn.Set(clothing.m_iClothingType, clothing.m_ClothingPrefabs);
			}
		}

		COA_Role_Custom_Gear customGear = COA_LoadoutPreviewHelper.FindCustomGear(gearConfig, role);
		if (customGear != null && customGear.m_Clothing != null)
		{
			foreach (COA_Clothing customClothing : customGear.m_Clothing)
			{
				if (customClothing != null && customClothing.m_ClothingPrefabs != null && !customClothing.m_ClothingPrefabs.IsEmpty())
					worn.Set(customClothing.m_iClothingType, customClothing.m_ClothingPrefabs);
			}
		}

		float minCapacity;
		float maxCapacity;
		float clothingWeight;
		bool capacityUnknown;
		foreach (int clothingType, array<ResourceName> options : worn)
		{
			float lowest = -1;
			float highest;
			float heaviest;
			foreach (ResourceName option : options)
			{
				float volume;
				if (!option.IsEmpty() && COA_GearPrefabHelper.IsValidPrefab(option))
				{
					volume = COA_GearPrefabHelper.GetStorageVolume(option);
					heaviest = Math.Max(heaviest, COA_GearPrefabHelper.GetItemWeight(option));
				}

				if (volume < 0)
				{
					capacityUnknown = true;
					volume = 0;
				}

				if (lowest < 0 || volume < lowest)
					lowest = volume;

				highest = Math.Max(highest, volume);
			}

			minCapacity += Math.Max(lowest, 0);
			maxCapacity += highest;
			clothingWeight += heaviest;
		}

		// Everything stored: magazines for the role's weapons + kit items (gadgets go in gadget slots)
		map<ResourceName, int> stored = new map<ResourceName, int>();
		COA_GearFixSlotHelper.GetKitItems(gearConfig, gearSettings, roleConfig, role, stored);
		AddRoleMagazines(gearConfig, roleConfig, role, stored);

		float neededVolume;
		float kitWeight = clothingWeight;
		int unknownVolume;
		foreach (ResourceName prefab, int count : stored)
		{
			if (!COA_GearPrefabHelper.IsValidPrefab(prefab))
				continue;

			kitWeight += COA_GearPrefabHelper.GetItemWeight(prefab) * count;

			if (COA_GearPrefabHelper.GetKind(prefab) == COA_GearPrefabHelper.KIND_GADGET)
				continue;

			float itemVolume = COA_GearPrefabHelper.GetItemVolume(prefab);
			if (itemVolume <= 0)
			{
				unknownVolume += count;
				continue;
			}

			neededVolume += itemVolume * count;
		}

		// Weapons carried in weapon slots still weigh something
		array<int> slots = {};
		COA_GearFixSlotHelper.GetRoleSlots(gearConfig, roleConfig, role, slots);
		foreach (int slot : slots)
		{
			array<ref COA_GearFixSlotEntry> entries = {};
			COA_GearFixSlotHelper.GetSlotEntries(gearConfig, role, slot, entries);
			if (!entries.IsEmpty() && COA_GearPrefabHelper.IsValidPrefab(entries[0].m_rWeapon))
				kitWeight += COA_GearPrefabHelper.GetItemWeight(entries[0].m_rWeapon);
		}

		if (kitWeight > MAX_KIT_WEIGHT)
			Add(issues, seen, COA_EGearCheckSeverity.WARNING, faction, role, -1, string.Format("Kit weighs about %1 kg (limit %2 kg)", Math.Round(kitWeight), MAX_KIT_WEIGHT), "");

		if (maxCapacity <= 0 || neededVolume <= 0)
		{
			if (capacityUnknown || maxCapacity <= 0)
				Add(issues, seen, COA_EGearCheckSeverity.INFO, faction, role, -1, "Could not estimate whether the kit fits (no storage volume data on the worn clothing)", "");

			return;
		}

		string unknownNote;
		if (unknownVolume > 0)
			unknownNote = string.Format(", %1 items without volume data not counted", unknownVolume);

		if (neededVolume > maxCapacity)
		{
			Add(issues, seen, COA_EGearCheckSeverity.ERROR, faction, role, -1, string.Format("Kit needs about %1 L of storage but the worn clothing holds at most %2 L - items will be lost at spawn%3", (neededVolume / 1000).ToString(-1, 1), (maxCapacity / 1000).ToString(-1, 1), unknownNote), "");
		}
		else if (neededVolume > minCapacity)
		{
			Add(issues, seen, COA_EGearCheckSeverity.WARNING, faction, role, -1, string.Format("Kit needs about %1 L but the smallest random clothing combination holds %2 L - some spawns may lose items%3", (neededVolume / 1000).ToString(-1, 1), (minCapacity / 1000).ToString(-1, 1), unknownNote), "");
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Magazines the role is issued (first entry of each pool)
	protected static void AddRoleMagazines(COA_GearScriptConfig gearConfig, COA_RoleConfig roleConfig, int role, notnull map<ResourceName, int> stored)
	{
		array<int> slots = {};
		if (COA_GearFixSlotHelper.HasCustomWeapons(gearConfig, role))
		{
			COA_GearFixSlotHelper.GetRoleSlots(gearConfig, roleConfig, role, slots);
		}
		else if (roleConfig.m_aMagazines)
		{
			foreach (COA_EGearscriptMagazines magazineType : roleConfig.m_aMagazines)
				slots.Insert(magazineType);
		}

		bool assistant = COA_GearFixSlotHelper.IsAssistantRole(roleConfig);
		foreach (int slot : slots)
		{
			array<ref COA_GearFixSlotEntry> entries = {};
			COA_GearFixSlotHelper.GetSlotEntries(gearConfig, role, slot, entries);
			if (entries.IsEmpty())
				continue;

			COA_GearFixSlotEntry entry = entries[0];
			for (int i = 0; i < entry.m_aMagazines.Count(); i++)
			{
				int count = entry.m_aCounts[i];
				if (assistant && COA_GearFixSlotHelper.IsSpecSlot(slot))
					count = entry.m_aAssistantCounts[i];

				if (count <= 0)
					continue;

				int current;
				stored.Find(entry.m_aMagazines[i], current);
				stored.Set(entry.m_aMagazines[i], current + count);
			}
		}
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 HELPERS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected static void Add(notnull array<ref COA_GearCheckIssue> issues, notnull set<string> seen, int severity, FactionKey faction, int role, int slot, string message, ResourceName resource)
	{
		COA_GearCheckIssue issue = new COA_GearCheckIssue(severity, faction, role, slot, message, resource);
		string key = issue.GetKey();
		if (seen.Contains(key))
			return;

		seen.Insert(key);
		issues.Insert(issue);
	}

	//------------------------------------------------------------------------------------------------
	//! Errors first, then warnings, then info; stable within a severity
	static void SortBySeverity(notnull array<ref COA_GearCheckIssue> issues)
	{
		array<ref COA_GearCheckIssue> sorted = {};
		for (int severity = COA_EGearCheckSeverity.ERROR; severity >= COA_EGearCheckSeverity.INFO; severity--)
		{
			foreach (COA_GearCheckIssue issue : issues)
			{
				if (issue.m_iSeverity == severity)
					sorted.Insert(issue);
			}
		}

		issues.Clear();
		foreach (COA_GearCheckIssue sortedIssue : sorted)
			issues.Insert(sortedIssue);
	}

	//------------------------------------------------------------------------------------------------
	static int CountSeverity(notnull array<ref COA_GearCheckIssue> issues, int severity)
	{
		int count;
		foreach (COA_GearCheckIssue issue : issues)
		{
			if (issue.m_iSeverity == severity)
				count++;
		}

		return count;
	}
}
