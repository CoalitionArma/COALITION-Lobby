//------------------------------------------------------------------------------------------------
// Describes what a role will spawn with, for the slotting screen's loadout preview. Walks the
// faction gearscript the same way COA_GearscriptManager.SetEntityGear does (custom role gear
// first, then the role config's weapon/magazine/item lists), but only reads names - nothing is
// spawned. Randomised weapon pools are listed as "A / B".
//------------------------------------------------------------------------------------------------
class COA_LoadoutPreviewHelper
{
	protected static const string LABEL_COLOR = "140,150,171,255";
	protected static const string DETAIL_COLOR = "101,116,154,255";

	// Prefab -> display name. Prefab names never change at runtime, so this lives for the session.
	protected static ref map<ResourceName, string> s_mDisplayNames = new map<ResourceName, string>();

	//------------------------------------------------------------------------------------------------
	//! Rich text listing weapons, ammo, radios, optics and medical gear for a role.
	//! \param[in] factionKey Faction whose gearscript to read
	//! \param[in] role Role to describe
	//! \return Rich text (RichTextWidget), or empty if the faction has no gearscript
	static string BuildSummary(FactionKey factionKey, COA_EGearRole role)
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		COA_GearscriptManager gearscriptManager = COA_GearscriptManager.GetInstance();
		COA_RolesConfig rolesConfig = COA_GearscriptManager.GetRolesConfig();
		if (!gamemode || !gearscriptManager || !rolesConfig)
			return string.Empty;

		COA_GearScriptConfig gearConfig = gearscriptManager.LoadGearScriptConfig(gamemode.GetGearScriptResource(factionKey));
		COA_GearScriptContainer gearSettings = gamemode.GetGearScriptSettings(factionKey);
		COA_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
		if (!gearConfig || !gearSettings || !roleConfig)
			return string.Empty;

		array<string> lines = {};

		// Weapons - custom role gear replaces the role's default weapons entirely when it sets any
		COA_Role_Custom_Gear customGear = FindCustomGear(gearConfig, role);
		bool customWeapons = false;
		if (customGear)
		{
			// An automatic rifleman's custom primary is still the automatic rifle
			bool primaryIsAutoRifle = role == COA_EGearRole.AUTOMATIC_RIFLEMAN;
			if (AddWeaponPoolLine(lines, "Primary", customGear.m_PrimaryWeapon, primaryIsAutoRifle))
				customWeapons = true;
			if (AddWeaponPoolLine(lines, "Secondary", customGear.m_SecondaryWeapon))
				customWeapons = true;
			if (AddWeaponPoolLine(lines, "Sidearm", customGear.m_Pistols))
				customWeapons = true;
		}

		if (!customWeapons)
			AddDefaultWeaponLines(lines, gearConfig, roleConfig);

		// Ammo carried for someone else (assistant gunners, AT assistants)
		bool isAssistant = roleConfig.m_SlottingType == COA_ESlotType.ASSISTANT || roleConfig.m_SlottingType == COA_ESlotType.SPECIALTY_ASSISTANT;
		if (isAssistant)
			AddAssistantAmmoLines(lines, gearConfig, roleConfig);

		// Radios, binoculars, medical
		AddItemLines(lines, gearConfig, gearSettings, roleConfig);

		if (customGear && customGear.m_AdditionalInventoryItems)
		{
			foreach (COA_Inventory_Item item : customGear.m_AdditionalInventoryItems)
			{
				if (item && !item.m_sItemPrefab.IsEmpty() && item.m_iItemCount > 0)
					lines.Insert(FormatLine("Extra", string.Format("%1x %2", item.m_iItemCount, GetDisplayName(item.m_sItemPrefab))));
			}
		}

		if (lines.IsEmpty())
			return "No weapons";

		return SCR_StringHelper.Join("<br/>", lines);
	}

	//------------------------------------------------------------------------------------------------
	//! Display name of an item prefab from its inventory UI info, falling back to the file name.
	static string GetDisplayName(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return string.Empty;

		string cached;
		if (s_mDisplayNames.Find(prefab, cached))
			return cached;

		string displayName;
		Resource resource = Resource.Load(prefab);
		if (resource && resource.IsValid())
		{
			IEntitySource entitySource = resource.GetResource().ToEntitySource();
			IEntityComponentSource itemSource = SCR_ComponentHelper.GetInventoryItemComponentSource(entitySource);
			if (itemSource)
			{
				SCR_ItemAttributeCollection attributes = SCR_ComponentHelper.GetInventoryItemInfo(itemSource);
				if (attributes && attributes.GetUIInfo())
					displayName = WidgetManager.Translate(attributes.GetUIInfo().GetName());
			}
		}

		if (displayName.IsEmpty())
			displayName = FilePath.StripExtension(FilePath.StripPath(prefab));

		s_mDisplayNames.Set(prefab, displayName);
		return displayName;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 WEAPONS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected static void AddDefaultWeaponLines(notnull array<string> lines, COA_GearScriptConfig gearConfig, COA_RoleConfig roleConfig)
	{
		if (!roleConfig.m_aWeapons)
			return;

		foreach (COA_EGearscriptWeapons weaponType : roleConfig.m_aWeapons)
		{
			switch (weaponType)
			{
				case COA_EGearscriptWeapons.RIFLE:		AddWeaponPoolLine(lines, "Rifle", gearConfig.m_Rifles); break;
				case COA_EGearscriptWeapons.RIFLEUGL:	AddWeaponPoolLine(lines, "Rifle (GL)", gearConfig.m_RifleUGLs); break;
				case COA_EGearscriptWeapons.CARBINE:	AddWeaponPoolLine(lines, "Carbine", gearConfig.m_Carbines); break;
				case COA_EGearscriptWeapons.PISTOL:		AddWeaponPoolLine(lines, "Sidearm", gearConfig.m_Pistols); break;
				case COA_EGearscriptWeapons.SNIPER:		AddWeaponLine(lines, "Sniper", gearConfig.m_SNIPER, CountMagazines(gearConfig.m_SNIPER)); break;
				case COA_EGearscriptWeapons.AR:			AddWeaponLine(lines, "Auto rifle", gearConfig.m_AR, CountSpecMagazines(gearConfig.m_AR, false), true); break;
				case COA_EGearscriptWeapons.MMG:		AddWeaponLine(lines, "MMG", gearConfig.m_MMG, CountSpecMagazines(gearConfig.m_MMG, false)); break;
				case COA_EGearscriptWeapons.HMG:		AddWeaponLine(lines, "HMG", gearConfig.m_HMG, CountSpecMagazines(gearConfig.m_HMG, false)); break;
				case COA_EGearscriptWeapons.AT:			AddWeaponLine(lines, "Launcher", gearConfig.m_AT, CountSpecMagazines(gearConfig.m_AT, false)); break;
				case COA_EGearscriptWeapons.MAT:		AddWeaponLine(lines, "Launcher", gearConfig.m_MAT, CountSpecMagazines(gearConfig.m_MAT, false)); break;
				case COA_EGearscriptWeapons.HAT:		AddWeaponLine(lines, "Launcher", gearConfig.m_HAT, CountSpecMagazines(gearConfig.m_HAT, false)); break;
				case COA_EGearscriptWeapons.AA:			AddWeaponLine(lines, "AA launcher", gearConfig.m_AA, CountSpecMagazines(gearConfig.m_AA, false)); break;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! One line for a randomised pool: "Rifle  M16A2 / M16A1  ·  7 mags", plus its optic/attachments.
	//! \return true if the pool had at least one weapon
	protected static bool AddWeaponPoolLine(notnull array<string> lines, string label, array<ref COA_Weapon_Class> pool, bool isAutoRifle = false)
	{
		if (!pool || pool.IsEmpty())
			return false;

		array<string> names = {};
		COA_Weapon_Class first;
		foreach (COA_Weapon_Class weapon : pool)
		{
			if (!weapon || weapon.m_Weapon.IsEmpty())
				continue;

			if (!first)
				first = weapon;

			string weaponName = GetDisplayName(weapon.m_Weapon);
			if (!names.Contains(weaponName))
				names.Insert(weaponName);
		}

		if (!first)
			return false;

		string text = SCR_StringHelper.Join(" / ", names);
		int mags = CountMagazines(first);
		if (mags > 0 && isAutoRifle)
			text += Detail("  ·  " + AmmoCountText(first, mags));
		else if (mags > 0)
			text += Detail(string.Format("  ·  %1 mags", mags));

		lines.Insert(FormatLine(label, text));
		AddAttachmentLine(lines, first.m_Attachments);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddWeaponLine(notnull array<string> lines, string label, COA_Base_Weapon_Class weapon, int mags, bool isAutoRifle = false)
	{
		if (!weapon || weapon.m_Weapon.IsEmpty())
			return;

		string text = GetDisplayName(weapon.m_Weapon);
		if (mags > 0 && isAutoRifle)
			text += Detail("  ·  " + AmmoCountText(weapon, mags));
		else if (mags > 0)
			text += Detail(string.Format("  ·  %1 rounds/mags", mags));

		lines.Insert(FormatLine(label, text));
		AddAttachmentLine(lines, weapon.m_Attachments);
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddAttachmentLine(notnull array<string> lines, array<ResourceName> attachments)
	{
		if (!attachments || attachments.IsEmpty())
			return;

		array<string> names = {};
		foreach (ResourceName attachment : attachments)
		{
			if (!attachment.IsEmpty())
				names.Insert(GetDisplayName(attachment));
		}

		if (!names.IsEmpty())
			lines.Insert(FormatLine("", Detail("+ " + SCR_StringHelper.Join(", ", names))));
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddAssistantAmmoLines(notnull array<string> lines, COA_GearScriptConfig gearConfig, COA_RoleConfig roleConfig)
	{
		if (!roleConfig.m_aMagazines)
			return;

		foreach (COA_EGearscriptMagazines magType : roleConfig.m_aMagazines)
		{
			COA_Spec_Weapon_Class specWeapon;
			switch (magType)
			{
				case COA_EGearscriptMagazines.AR_MAG:	specWeapon = gearConfig.m_AR; break;
				case COA_EGearscriptMagazines.MMG_MAG:	specWeapon = gearConfig.m_MMG; break;
				case COA_EGearscriptMagazines.HMG_MAG:	specWeapon = gearConfig.m_HMG; break;
				case COA_EGearscriptMagazines.AT_MAG:	specWeapon = gearConfig.m_AT; break;
				case COA_EGearscriptMagazines.MAT_MAG:	specWeapon = gearConfig.m_MAT; break;
				case COA_EGearscriptMagazines.HAT_MAG:	specWeapon = gearConfig.m_HAT; break;
				case COA_EGearscriptMagazines.AA_MAG:	specWeapon = gearConfig.m_AA; break;
			}

			if (!specWeapon || specWeapon.m_Weapon.IsEmpty())
				continue;

			int count = CountSpecMagazines(specWeapon, true);
			if (count <= 0)
				continue;

			if (magType == COA_EGearscriptMagazines.AR_MAG)
				lines.Insert(FormatLine("Carries", string.Format("%1 for %2", AmmoCountText(specWeapon, count), GetDisplayName(specWeapon.m_Weapon))));
			else
				lines.Insert(FormatLine("Carries", string.Format("%1x ammo for %2", count, GetDisplayName(specWeapon.m_Weapon))));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Automatic rifle ammo: "4 boxes" for belt-fed guns, "4 magazines" for the RPK, which is
	//! magazine-fed. The RPK is recognised by name, in its display name or prefab path.
	protected static string AmmoCountText(COA_Base_Weapon_Class weapon, int count)
	{
		string weaponName = GetDisplayName(weapon.m_Weapon) + " " + weapon.m_Weapon;
		weaponName.ToLower();

		if (weaponName.Contains("rpk"))
		{
			if (count == 1)
				return "1 magazine";

			return string.Format("%1 magazines", count);
		}

		if (count == 1)
			return "1 box";

		return string.Format("%1 boxes", count);
	}

	//------------------------------------------------------------------------------------------------
	protected static int CountMagazines(COA_Weapon_Class weapon)
	{
		if (!weapon || !weapon.m_MagazineArray)
			return 0;

		int count;
		foreach (COA_Magazine_Class magazine : weapon.m_MagazineArray)
		{
			if (magazine)
				count += magazine.m_MagazineCount;
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	protected static int CountSpecMagazines(COA_Spec_Weapon_Class weapon, bool assistant)
	{
		if (!weapon || !weapon.m_MagazineArray)
			return 0;

		int count;
		foreach (COA_Spec_Magazine_Class magazine : weapon.m_MagazineArray)
		{
			if (!magazine)
				continue;

			if (assistant)
				count += magazine.m_AssistantMagazineCount;
			else
				count += magazine.m_MagazineCount;
		}

		return count;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 ITEMS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Mirrors the radio/binocular/medical rules in COA_GearscriptManager.ApplyInventoryItems
	protected static void AddItemLines(notnull array<string> lines, COA_GearScriptConfig gearConfig, COA_GearScriptContainer gearSettings, COA_RoleConfig roleConfig)
	{
		array<string> radios = {};
		bool hasBinoculars;
		bool isMedic;

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
							radios.Insert(RadioName(gearSettings.m_rShortRangeRadioPrefab, "Short range"));
						break;

					case COA_EGearscriptItems.LONGRANGE_RADIO:
						if (gearSettings.m_bEnableLeadershipRadios)
							radios.Insert(RadioName(gearSettings.m_rLongRangeRadioPrefab, "Long range"));
						break;

					case COA_EGearscriptItems.RTO_RADIO:
						if (gearSettings.m_bEnableRTORadios)
							radios.Insert(RadioName(gearSettings.m_rRTORadiosPrefab, "RTO"));
						break;

					case COA_EGearscriptItems.LEADERSHIP_BINO:
						if (!gearConfig.m_sLeadershipBinocularsPrefab.IsEmpty())
							hasBinoculars = true;
						break;

					case COA_EGearscriptItems.ASSISTANT_BINO:
						if (!gearConfig.m_sAssistantBinocularsPrefab.IsEmpty())
							hasBinoculars = true;
						break;

					case COA_EGearscriptItems.MEDIC_ITEMS:
						isMedic = true;
						break;
				}
			}
		}

		if (radios.IsEmpty())
			lines.Insert(FormatLine("Radio", Detail("None")));
		else
			lines.Insert(FormatLine("Radio", SCR_StringHelper.Join(", ", radios)));

		if (hasBinoculars)
			lines.Insert(FormatLine("Optics", "Binoculars"));

		if (isMedic)
			lines.Insert(FormatLine("Medical", string.Format("Medic kit  %1", Detail(string.Format("·  %1 items", CountItems(gearConfig.m_MedicMedicalItems))))));
		else
			lines.Insert(FormatLine("Medical", "Standard"));
	}

	//------------------------------------------------------------------------------------------------
	protected static string RadioName(ResourceName prefab, string fallback)
	{
		if (prefab.IsEmpty())
			return fallback;

		return string.Format("%1 %2", GetDisplayName(prefab), Detail("(" + fallback + ")"));
	}

	//------------------------------------------------------------------------------------------------
	protected static int CountItems(array<ref COA_Inventory_Item> items)
	{
		if (!items)
			return 0;

		int count;
		foreach (COA_Inventory_Item item : items)
		{
			if (item && item.m_iItemCount > 0)
				count += item.m_iItemCount;
		}

		return count;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 HELPERS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	static COA_Role_Custom_Gear FindCustomGear(COA_GearScriptConfig gearConfig, COA_EGearRole role)
	{
		if (!gearConfig.m_RolesToSetCustomSettings)
			return null;

		foreach (COA_Role_Custom_Gear customGear : gearConfig.m_RolesToSetCustomSettings)
		{
			if (customGear && customGear.m_Role == role)
				return customGear;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static string FormatLine(string label, string value)
	{
		if (label.IsEmpty())
			return value;

		string upperLabel = label;
		upperLabel.ToUpper();
		return string.Format("<color rgba='%1'>%2</color>  %3", LABEL_COLOR, upperLabel, value);
	}

	//------------------------------------------------------------------------------------------------
	protected static string Detail(string text)
	{
		return string.Format("<color rgba='%1'>%2</color>", DETAIL_COLOR, text);
	}
}
