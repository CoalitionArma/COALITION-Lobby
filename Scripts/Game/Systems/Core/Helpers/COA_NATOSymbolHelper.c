//------------------------------------------------------------------------------------------------
// Builds a NATO (APP-6) military symbol for a group, for the squad leader map markers
// (COA_SCR_SquadLeaderMarkers.c). Vanilla shows each faction's group flag there instead, which
// depends on how that faction's flag imageset was authored and differs between sides.
//
//  - Frame shape by side, so each side is recognisable at a glance: BLUFOR rectangle, OPFOR
//    diamond, INDFOR square, CIV civilian frame. (Colour is the faction colour, applied by the marker.)
//  - Icon (unit type) comes from the roles in the group's slots - a group counts as a specialty
//    unit when at least half its slots are that specialty, otherwise it is infantry.
//  - Echelon: company / platoon HQ by their leader slots, squad when led by a squad leader or five
//    slots or more, otherwise team.
//------------------------------------------------------------------------------------------------
class COA_NATOSymbolHelper
{
	//------------------------------------------------------------------------------------------------
	static SCR_MilitarySymbol BuildForGroup(notnull SCR_AIGroup group)
	{
		SCR_MilitarySymbol symbol = new SCR_MilitarySymbol();
		symbol.SetIdentity(GetIdentity(group.GetFaction()));
		symbol.SetDimension(EMilitarySymbolDimension.LAND);

		array<COA_EGearRole> roles = {};
		CollectGroupRoles(group, roles);

		symbol.SetIcons(GetIcon(roles));
		symbol.SetAmplifier(GetEchelon(roles));
		return symbol;
	}

	//------------------------------------------------------------------------------------------------
	protected static EMilitarySymbolIdentity GetIdentity(Faction groupFaction)
	{
		if (!groupFaction)
			return EMilitarySymbolIdentity.UNKNOWN;

		switch (groupFaction.GetFactionKey())
		{
			case "BLUFOR":	return EMilitarySymbolIdentity.BLUFOR;
			case "OPFOR":	return EMilitarySymbolIdentity.OPFOR;
			case "INDFOR":	return EMilitarySymbolIdentity.INDFOR;
			case "CIV":		return EMilitarySymbolIdentity.CIVILIAN;
		}

		return EMilitarySymbolIdentity.UNKNOWN;
	}

	//------------------------------------------------------------------------------------------------
	//! Every slot's role in the group, filled or not - the symbol describes the unit as organised
	protected static void CollectGroupRoles(SCR_AIGroup group, notnull array<COA_EGearRole> roles)
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		RplId groupId;
		if (!slottingManager || !COA_ReplicationHelper.GetRplId(group, groupId))
			return;

		foreach (int slotId : slottingManager.GetAllSlotIDsForGroup(groupId))
		{
			COA_SlotData slotData = slottingManager.GetSlotData(slotId);
			if (slotData)
				roles.Insert(slotData.GetSlotRole());
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static EMilitarySymbolIcon GetIcon(notnull array<COA_EGearRole> roles)
	{
		if (roles.IsEmpty())
			return EMilitarySymbolIcon.INFANTRY;

		// Count each specialty; the biggest wins if it makes up at least half the group
		map<int, int> counts = new map<int, int>(); // icon -> slots
		foreach (COA_EGearRole role : roles)
		{
			int roleIcon = GetSpecialtyIcon(role);
			if (roleIcon == 0)
				continue;

			int roleCount;
			counts.Find(roleIcon, roleCount);
			counts.Set(roleIcon, roleCount + 1);
		}

		EMilitarySymbolIcon bestIcon = EMilitarySymbolIcon.INFANTRY;
		int bestCount = 0;
		foreach (int specialtyIcon, int specialtyCount : counts)
		{
			if (specialtyCount > bestCount)
			{
				bestIcon = specialtyIcon;
				bestCount = specialtyCount;
			}
		}

		if (bestCount * 2 >= roles.Count())
			return bestIcon;

		return EMilitarySymbolIcon.INFANTRY;
	}

	//------------------------------------------------------------------------------------------------
	//! The unit type a role points to, or 0 for ordinary infantry and leadership roles
	protected static EMilitarySymbolIcon GetSpecialtyIcon(COA_EGearRole role)
	{
		switch (role)
		{
			case COA_EGearRole.MEDICAL_OFFICER:
			case COA_EGearRole.MEDIC:
				return EMilitarySymbolIcon.MEDICAL;

			case COA_EGearRole.HEAVY_ANTITANK:
			case COA_EGearRole.ASSISTANT_HEAVY_ANTITANK:
			case COA_EGearRole.MEDIUM_ANTITANK:
			case COA_EGearRole.ASSISTANT_MEDIUM_ANTITANK:
				return EMilitarySymbolIcon.ANTITANK;

			case COA_EGearRole.HEAVY_MACHINEGUN:
			case COA_EGearRole.ASSISTANT_HEAVY_MACHINEGUN:
			case COA_EGearRole.MEDIUM_MACHINEGUN:
			case COA_EGearRole.ASSISTANT_MEDIUM_MACHINEGUN:
				return EMilitarySymbolIcon.MACHINEGUN;

			case COA_EGearRole.SNIPER:
			case COA_EGearRole.SPOTTER:
				return EMilitarySymbolIcon.SNIPER;

			case COA_EGearRole.DRONE_OPERATOR:
			case COA_EGearRole.FORWARD_OBSERVER:
			case COA_EGearRole.JTAC:
				return EMilitarySymbolIcon.RECON;

			case COA_EGearRole.INDIRECT_LEAD:
			case COA_EGearRole.INDIRECT_GUNNER:
			case COA_EGearRole.INDIRECT_LOADER:
				return EMilitarySymbolIcon.MORTAR;

			case COA_EGearRole.LOGI_LEAD:
			case COA_EGearRole.LOGI_RUNNER:
				return EMilitarySymbolIcon.SUPPLY;

			case COA_EGearRole.PILOT:
			case COA_EGearRole.CREW_CHIEF:
				return EMilitarySymbolIcon.ROTARY_WING;

			case COA_EGearRole.VEHICLE_LEAD:
			case COA_EGearRole.VEHICLE_DRIVER:
			case COA_EGearRole.VEHICLE_GUNNER:
			case COA_EGearRole.VEHICLE_LOADER:
				return EMilitarySymbolIcon.ARMOR;
		}

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	protected static EMilitarySymbolAmplifier GetEchelon(notnull array<COA_EGearRole> roles)
	{
		if (roles.Contains(COA_EGearRole.COMPANY_COMMANDER) || roles.Contains(COA_EGearRole.FIRST_SERGEANT))
			return EMilitarySymbolAmplifier.COMPANY;

		if (roles.Contains(COA_EGearRole.PLATOON_LEADER) || roles.Contains(COA_EGearRole.PLATOON_SERGEANT))
			return EMilitarySymbolAmplifier.PLATOON;

		if (roles.Contains(COA_EGearRole.SQUAD_LEAD) || roles.Count() >= 5)
			return EMilitarySymbolAmplifier.SQUAD;

		return EMilitarySymbolAmplifier.TEAM;
	}
}
