//------------------------------------------------------------------------------------------------
//! Best-effort COA_EFlagType guess for a GM-marked group's header icon, from the role composition
//! COA_GMRoleGuessHelper already computed per member (see COA_SlottingManager.
//! RegisterGMPossessionGroup). Nothing in the codebase maps a squad's makeup to a flag/icon today -
//! COA_EFlagType is only ever set from mission-config data (COA_SlottingGroup.m_FlagType) - so this
//! is new heuristic logic, modeled after COA_GMRoleGuessHelper's "when in doubt, fall through to a
//! sensible default" shape rather than reusing anything directly.
class COA_GMFlagGuessHelper
{
	//------------------------------------------------------------------------------------------------
	//! \param[in] memberRoles every member's guessed COA_EGearRole, as already computed for the slots
	//! \return a COA_EFlagType icon that best represents the group's composition
	static COA_EFlagType GuessFlagForRoles(array<COA_EGearRole> memberRoles)
	{
		if (!memberRoles || memberRoles.IsEmpty())
			return COA_EFlagType.INFANTRY;

		bool hasSniper = false;
		bool hasAntiArmor = false;
		bool hasMachinegun = false;
		bool hasMedic = false;

		foreach (COA_EGearRole role : memberRoles)
		{
			switch (role)
			{
				case COA_EGearRole.SNIPER:
					hasSniper = true;
					break;
				case COA_EGearRole.RIFLEMAN_ANTITANK:
				case COA_EGearRole.ASSISTANT_RIFLEMAN_ANTITANK:
				case COA_EGearRole.HEAVY_ANTITANK:
				case COA_EGearRole.ASSISTANT_HEAVY_ANTITANK:
				case COA_EGearRole.MEDIUM_ANTITANK:
				case COA_EGearRole.ASSISTANT_MEDIUM_ANTITANK:
					hasAntiArmor = true;
					break;
				case COA_EGearRole.AUTOMATIC_RIFLEMAN:
				case COA_EGearRole.ASSISTANT_AUTOMATIC_RIFLEMAN:
				case COA_EGearRole.HEAVY_MACHINEGUN:
				case COA_EGearRole.ASSISTANT_HEAVY_MACHINEGUN:
				case COA_EGearRole.MEDIUM_MACHINEGUN:
				case COA_EGearRole.ASSISTANT_MEDIUM_MACHINEGUN:
					hasMachinegun = true;
					break;
				case COA_EGearRole.MEDIC:
				case COA_EGearRole.MEDICAL_OFFICER:
					hasMedic = true;
					break;
			}
		}

		// Priority order: a single specialist is usually the most distinctive thing about a small
		// GM-placed squad, so check the rarer specialties before the more common ones.
		if (hasSniper)
			return COA_EFlagType.SNIPER;

		if (hasAntiArmor)
			return COA_EFlagType.ANTI_ARMOR;

		if (hasMedic)
			return COA_EFlagType.MEDICAL;

		if (hasMachinegun)
			return COA_EFlagType.MACHINEGUN;

		return COA_EFlagType.INFANTRY;
	}
}
