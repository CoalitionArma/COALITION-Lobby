//------------------------------------------------------------------------------------------------
// Shared data types for the admin "Gear Fix" tool (session-wide gear overrides) and the automatic
// gearscript check. The server owns the real objects (COA_GearFixManager); clients only ever see
// the compact "|"-separated strings built by Pack() and read back by Unpack().
//------------------------------------------------------------------------------------------------

//! Which weapon of a loadout an override targets. The first twelve values match
//! COA_EGearscriptWeapons (and COA_EGearscriptMagazines) one to one; the CUSTOM_* values are the
//! weapon lists of a role's COA_Role_Custom_Gear entry.
enum COA_EGearFixSlot
{
	RIFLE = 0,
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
	CUSTOM_PRIMARY,
	CUSTOM_SECONDARY,
	CUSTOM_PISTOL,
}

//------------------------------------------------------------------------------------------------
enum COA_EGearFixAction
{
	WEAPON = 0,		//!< replace the weapon in a slot (+ its magazines, + assistants' magazines)
	ITEM_SWAP,		//!< every A becomes a B (same count)
	ITEM_REMOVE,	//!< remove A (all of them, or at most m_iItemCount)
	ITEM_ADD,		//!< give m_iItemCount of B (<= 0: the role's gearscript count, used by reverts)
}

//------------------------------------------------------------------------------------------------
enum COA_EGearCheckSeverity
{
	INFO = 0,
	WARNING,
	ERROR,
}

//------------------------------------------------------------------------------------------------
//! One weapon entry of a gearscript slot, flattened so pool (COA_Weapon_Class) and specialty
//! (COA_Spec_Weapon_Class) entries can be handled the same way.
class COA_GearFixSlotEntry
{
	ResourceName m_rWeapon;
	ref array<ResourceName> m_aAttachments = {};
	ref array<ResourceName> m_aMagazines = {};
	ref array<int> m_aCounts = {};
	ref array<int> m_aAssistantCounts = {};

	//------------------------------------------------------------------------------------------------
	//! Magazines for the main muzzle, or rounds for its launcher
	int GetPoolTotal(bool assistant, bool launcher)
	{
		int total;
		for (int i = 0; i < m_aMagazines.Count(); i++)
		{
			if (COA_GearPrefabHelper.IsLauncherMagazine(m_rWeapon, m_aMagazines[i]) != launcher)
				continue;

			if (assistant)
				total += m_aAssistantCounts[i];
			else
				total += m_aCounts[i];
		}

		return total;
	}

	//------------------------------------------------------------------------------------------------
	int GetMagazineTotal(bool assistant)
	{
		int total;
		for (int i = 0; i < m_aMagazines.Count(); i++)
		{
			if (assistant)
				total += m_aAssistantCounts[i];
			else
				total += m_aCounts[i];
		}

		return total;
	}
}

//------------------------------------------------------------------------------------------------
//! How a role relates to a weapon slot of its faction's gearscript
class COA_GearFixRoleUse
{
	bool m_bAffected;		//!< the role gets the slot's weapon or its magazines
	bool m_bGivesWeapon;	//!< the role is issued the weapon itself
	bool m_bGetsMagazines;	//!< the role is issued magazines for it
	bool m_bAssistant;		//!< assistant slot type: carries the assistant count of specialty ammo
}

//------------------------------------------------------------------------------------------------
//! A session-wide gear override. Server-side fields after m_sAdminName are never replicated.
class COA_GearOverride
{
	int m_iId;
	string m_sFaction;
	int m_iRole = -1;				//!< COA_EGearRole, -1 = every role of the faction
	int m_iSlot = -1;				//!< COA_EGearFixSlot, weapon overrides only
	int m_iAction;					//!< COA_EGearFixAction

	ResourceName m_rNewWeapon;
	ref array<ResourceName> m_aNewAttachments = {};
	ref array<ResourceName> m_aNewMagazines = {};
	ref array<int> m_aNewCounts = {};
	ref array<int> m_aNewAssistantCounts = {};

	ResourceName m_rOldItem;
	ResourceName m_rNewItem;
	int m_iItemCount;

	string m_sAdminName;

	// Server only ---------------------------------------------------------------------------------

	//! Weapons/magazines that also count as "the old weapon" - the new weapon of an override this
	//! one replaced, or of a faction-wide override a role-scoped one sits on top of
	ref array<ResourceName> m_aExtraOldWeapons = {};
	ref array<ResourceName> m_aExtraOldMagazines = {};
	int m_iExtraOldCount = -1;
	int m_iExtraOldAssistantCount = -1;
	int m_iExtraOldLauncherCount = -1;
	int m_iExtraOldLauncherAssistantCount = -1;

	//! Reverts: the old weapon/magazines are ONLY the extras (the override's new gear), not the gearscript's
	bool m_bExplicitOld;

	//! Reverts: the new weapon/magazines come from the gearscript per role, not from m_aNewCounts
	bool m_bRevertToGearscript;

	//------------------------------------------------------------------------------------------------
	//! Rounds for the new weapon's launcher, not magazines for its main muzzle
	bool IsLauncherAmmo(ResourceName magazine)
	{
		return COA_GearPrefabHelper.IsLauncherMagazine(m_rNewWeapon, magazine);
	}

	//------------------------------------------------------------------------------------------------
	int GetPoolCount(bool assistant, bool launcher)
	{
		int total;
		for (int i = 0; i < m_aNewMagazines.Count(); i++)
		{
			if (IsLauncherAmmo(m_aNewMagazines[i]) != launcher)
				continue;

			if (assistant)
				total += m_aNewAssistantCounts[i];
			else
				total += m_aNewCounts[i];
		}

		return total;
	}

	//------------------------------------------------------------------------------------------------
	//! The new magazine for the main muzzle
	ResourceName GetFirstNewMagazine()
	{
		foreach (ResourceName magazine : m_aNewMagazines)
		{
			if (!IsLauncherAmmo(magazine))
				return magazine;
		}

		return ResourceName.Empty;
	}

	//------------------------------------------------------------------------------------------------
	ResourceName GetNewLauncherAmmo()
	{
		foreach (ResourceName magazine : m_aNewMagazines)
		{
			if (IsLauncherAmmo(magazine))
				return magazine;
		}

		return ResourceName.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! "id|faction|role|slot|action|weapon|magazine|count|assistantCount|oldItem|newItem|itemCount|admin|launcherAmmo|launcherCount"
	string Pack()
	{
		array<string> fields = {};
		fields.Insert(m_iId.ToString());
		fields.Insert(m_sFaction);
		fields.Insert(m_iRole.ToString());
		fields.Insert(m_iSlot.ToString());
		fields.Insert(m_iAction.ToString());
		fields.Insert(m_rNewWeapon);
		fields.Insert(GetFirstNewMagazine());
		fields.Insert(GetPoolCount(false, false).ToString());
		fields.Insert(GetPoolCount(true, false).ToString());
		fields.Insert(m_rOldItem);
		fields.Insert(m_rNewItem);
		fields.Insert(m_iItemCount.ToString());
		fields.Insert(COA_GearFixText.Sanitize(m_sAdminName));
		fields.Insert(GetNewLauncherAmmo());
		fields.Insert(GetPoolCount(false, true).ToString());
		return SCR_StringHelper.Join("|", fields);
	}

	//------------------------------------------------------------------------------------------------
	static COA_GearOverride Unpack(string packed)
	{
		array<string> fields = {};
		packed.Split("|", fields, false);
		if (fields.Count() < 15)
			return null;

		COA_GearOverride gearOverride = new COA_GearOverride();
		gearOverride.m_iId = fields[0].ToInt();
		gearOverride.m_sFaction = fields[1];
		gearOverride.m_iRole = fields[2].ToInt();
		gearOverride.m_iSlot = fields[3].ToInt();
		gearOverride.m_iAction = fields[4].ToInt();
		gearOverride.m_rNewWeapon = fields[5];
		if (!fields[6].IsEmpty())
		{
			gearOverride.m_aNewMagazines.Insert(fields[6]);
			gearOverride.m_aNewCounts.Insert(fields[7].ToInt());
			gearOverride.m_aNewAssistantCounts.Insert(fields[8].ToInt());
		}

		if (!fields[13].IsEmpty())
		{
			gearOverride.m_aNewMagazines.Insert(fields[13]);
			gearOverride.m_aNewCounts.Insert(fields[14].ToInt());
			gearOverride.m_aNewAssistantCounts.Insert(0);
		}

		gearOverride.m_rOldItem = fields[9];
		gearOverride.m_rNewItem = fields[10];
		gearOverride.m_iItemCount = fields[11].ToInt();
		gearOverride.m_sAdminName = fields[12];
		return gearOverride;
	}

	//------------------------------------------------------------------------------------------------
	//! Same target as another override (the newer one replaces the older)
	bool HasSameTarget(COA_GearOverride other)
	{
		if (!other || other.m_sFaction != m_sFaction || other.m_iRole != m_iRole)
			return false;

		if (m_iAction == COA_EGearFixAction.WEAPON)
			return other.m_iAction == COA_EGearFixAction.WEAPON && other.m_iSlot == m_iSlot;

		if (other.m_iAction == COA_EGearFixAction.WEAPON)
			return false;

		// Item overrides: one override per "from" item (swap/remove) or per added item
		if (m_iAction == COA_EGearFixAction.ITEM_ADD)
			return other.m_iAction == COA_EGearFixAction.ITEM_ADD && other.m_rNewItem == m_rNewItem;

		return other.m_iAction != COA_EGearFixAction.ITEM_ADD && other.m_rOldItem == m_rOldItem;
	}

	//------------------------------------------------------------------------------------------------
	//! One-line description for logs, hints and the admin list
	string Describe()
	{
		string target = COA_GearFixText.TargetLabel(m_iRole, m_iSlot, m_iAction);
		switch (m_iAction)
		{
			case COA_EGearFixAction.WEAPON:
			{
				string ammo;
				ResourceName magazine = GetFirstNewMagazine();
				if (!magazine.IsEmpty())
					ammo = string.Format(", %1x %2 (assistants %3)", GetPoolCount(false, false), COA_GearFixText.Name(magazine), GetPoolCount(true, false));

				ResourceName launcherAmmo = GetNewLauncherAmmo();
				if (!launcherAmmo.IsEmpty())
					ammo += string.Format(", %1x %2", GetPoolCount(false, true), COA_GearFixText.Name(launcherAmmo));

				return string.Format("%1 %2: %3 -> %4%5", m_sFaction, target, COA_GearFixText.SlotLabel(m_iSlot), COA_GearFixText.Name(m_rNewWeapon), ammo);
			}

			case COA_EGearFixAction.ITEM_SWAP:
				return string.Format("%1 %2: %3 -> %4", m_sFaction, target, COA_GearFixText.Name(m_rOldItem), COA_GearFixText.Name(m_rNewItem));

			case COA_EGearFixAction.ITEM_REMOVE:
				return string.Format("%1 %2: remove %3", m_sFaction, target, COA_GearFixText.Name(m_rOldItem));

			case COA_EGearFixAction.ITEM_ADD:
				return string.Format("%1 %2: add %3x %4", m_sFaction, target, m_iItemCount, COA_GearFixText.Name(m_rNewItem));
		}

		return m_sFaction + " " + target;
	}
}

//------------------------------------------------------------------------------------------------
//! One finding of the gearscript check
class COA_GearCheckIssue
{
	int m_iSeverity;			//!< COA_EGearCheckSeverity
	string m_sFaction;
	int m_iRole = -1;			//!< COA_EGearRole, -1 = faction-wide
	int m_iSlot = -1;			//!< COA_EGearFixSlot, -1 = not about a weapon slot
	ResourceName m_rResource;	//!< the offending prefab, if any
	string m_sMessage;
	int m_iOccurrences = 1;		//!< runtime issues: how many spawns hit it

	//------------------------------------------------------------------------------------------------
	void COA_GearCheckIssue(int severity = 0, string faction = "", int role = -1, int slot = -1, string message = "", ResourceName resource = "")
	{
		m_iSeverity = severity;
		m_sFaction = faction;
		m_iRole = role;
		m_iSlot = slot;
		m_sMessage = message;
		m_rResource = resource;
	}

	//------------------------------------------------------------------------------------------------
	string GetKey()
	{
		return string.Format("%1|%2|%3|%4|%5|%6", m_iSeverity, m_sFaction, m_iRole, m_iSlot, m_rResource, m_sMessage);
	}

	//------------------------------------------------------------------------------------------------
	//! "severity|faction|role|slot|resource|occurrences|message"
	string Pack()
	{
		return string.Format("%1|%2|%3|%4|%5|%6|%7", m_iSeverity, m_sFaction, m_iRole, m_iSlot, m_rResource, m_iOccurrences, COA_GearFixText.Sanitize(m_sMessage));
	}

	//------------------------------------------------------------------------------------------------
	static COA_GearCheckIssue Unpack(string packed)
	{
		array<string> fields = {};
		packed.Split("|", fields, false);
		if (fields.Count() < 7)
			return null;

		COA_GearCheckIssue issue = new COA_GearCheckIssue(fields[0].ToInt(), fields[1], fields[2].ToInt(), fields[3].ToInt(), fields[6], fields[4]);
		issue.m_iOccurrences = fields[5].ToInt();
		return issue;
	}

	//------------------------------------------------------------------------------------------------
	//! "[ERROR] BLUFOR AUTOMATIC_RIFLEMAN AR: message | resource" for the server log
	string ToLogLine()
	{
		string where = m_sFaction;
		if (m_iRole >= 0)
			where += " " + SCR_Enum.GetEnumName(COA_EGearRole, m_iRole);
		else
			where += " (all roles)";

		if (m_iSlot >= 0)
			where += " " + SCR_Enum.GetEnumName(COA_EGearFixSlot, m_iSlot);

		string line = string.Format("[COA_GearCheck] %1 %2: %3", COA_GearFixText.SeverityTag(m_iSeverity), where, m_sMessage);
		if (m_iOccurrences > 1)
			line += string.Format(" (x%1)", m_iOccurrences);

		if (!m_rResource.IsEmpty())
			line += " | " + m_rResource;

		return line;
	}

	//------------------------------------------------------------------------------------------------
	//! Short line for UI lists
	string ToListLine()
	{
		string where = m_sFaction;
		if (m_iRole >= 0)
			where += " " + COA_GearFixText.RoleName(m_iRole);

		if (m_iSlot >= 0)
			where += " " + COA_GearFixText.SlotLabel(m_iSlot);

		string line = string.Format("%1 %2: %3", COA_GearFixText.SeverityShortTag(m_iSeverity), where, m_sMessage);
		if (m_iOccurrences > 1)
			line += string.Format(" (x%1)", m_iOccurrences);

		return line;
	}
}

//------------------------------------------------------------------------------------------------
//! Text helpers shared by the server log, hints and the admin UI
class COA_GearFixText
{
	//------------------------------------------------------------------------------------------------
	//! Strips the "|" separator (and line breaks) from free text that goes into a packed string
	static string Sanitize(string text)
	{
		string result = text;
		result.Replace("|", "/");
		result.Replace("\n", " ");
		result.Replace("\r", " ");
		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Item display name, falling back to the file name - also when the localisation key could not
	//! be translated (dedicated servers)
	static string Name(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return "nothing";

		string displayName = COA_LoadoutPreviewHelper.GetDisplayName(prefab);
		if (displayName.IsEmpty() || displayName.StartsWith("#"))
			displayName = FilePath.StripExtension(FilePath.StripPath(prefab));

		return displayName;
	}

	//------------------------------------------------------------------------------------------------
	static string SlotLabel(int slot)
	{
		switch (slot)
		{
			case COA_EGearFixSlot.RIFLE: return "Rifle";
			case COA_EGearFixSlot.RIFLEUGL: return "Rifle (UGL)";
			case COA_EGearFixSlot.CARBINE: return "Carbine";
			case COA_EGearFixSlot.SNIPER: return "Sniper";
			case COA_EGearFixSlot.PISTOL: return "Pistol";
			case COA_EGearFixSlot.AR: return "AR";
			case COA_EGearFixSlot.AT: return "AT";
			case COA_EGearFixSlot.MMG: return "MMG";
			case COA_EGearFixSlot.HMG: return "HMG";
			case COA_EGearFixSlot.MAT: return "MAT";
			case COA_EGearFixSlot.HAT: return "HAT";
			case COA_EGearFixSlot.AA: return "AA";
			case COA_EGearFixSlot.CUSTOM_PRIMARY: return "Primary";
			case COA_EGearFixSlot.CUSTOM_SECONDARY: return "Secondary";
			case COA_EGearFixSlot.CUSTOM_PISTOL: return "Sidearm";
		}

		return "Items";
	}

	//------------------------------------------------------------------------------------------------
	//! Gearscript field the slot lives in, for the copy-pasteable server log line
	static string SlotFieldName(int slot)
	{
		switch (slot)
		{
			case COA_EGearFixSlot.RIFLE: return "m_Rifles";
			case COA_EGearFixSlot.RIFLEUGL: return "m_RifleUGLs";
			case COA_EGearFixSlot.CARBINE: return "m_Carbines";
			case COA_EGearFixSlot.SNIPER: return "m_SNIPER";
			case COA_EGearFixSlot.PISTOL: return "m_Pistols";
			case COA_EGearFixSlot.AR: return "m_AR";
			case COA_EGearFixSlot.AT: return "m_AT";
			case COA_EGearFixSlot.MMG: return "m_MMG";
			case COA_EGearFixSlot.HMG: return "m_HMG";
			case COA_EGearFixSlot.MAT: return "m_MAT";
			case COA_EGearFixSlot.HAT: return "m_HAT";
			case COA_EGearFixSlot.AA: return "m_AA";
			case COA_EGearFixSlot.CUSTOM_PRIMARY: return "m_PrimaryWeapon";
			case COA_EGearFixSlot.CUSTOM_SECONDARY: return "m_SecondaryWeapon";
			case COA_EGearFixSlot.CUSTOM_PISTOL: return "m_Pistols";
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	static string RoleName(int role)
	{
		if (role < 0)
			return "All roles";

		COA_RolesConfig rolesConfig = COA_GearscriptManager.GetRolesConfig();
		if (rolesConfig)
		{
			COA_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
			if (roleConfig != null && !roleConfig.m_sRoleName.IsEmpty())
				return roleConfig.m_sRoleName;
		}

		return SCR_Enum.GetEnumName(COA_EGearRole, role);
	}

	//------------------------------------------------------------------------------------------------
	static string TargetLabel(int role, int slot, int action)
	{
		if (action == COA_EGearFixAction.WEAPON)
			return string.Format("%1 / %2", RoleName(role), SlotLabel(slot));

		return string.Format("%1 / Items", RoleName(role));
	}

	//------------------------------------------------------------------------------------------------
	static string SeverityTag(int severity)
	{
		switch (severity)
		{
			case COA_EGearCheckSeverity.ERROR: return "ERROR";
			case COA_EGearCheckSeverity.WARNING: return "WARNING";
		}

		return "INFO";
	}

	//------------------------------------------------------------------------------------------------
	static string SeverityShortTag(int severity)
	{
		switch (severity)
		{
			case COA_EGearCheckSeverity.ERROR: return "[X]";
			case COA_EGearCheckSeverity.WARNING: return "[!]";
		}

		return "[i]";
	}
}
