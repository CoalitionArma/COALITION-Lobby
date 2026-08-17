//------------------------------------------------------------------------------------------------
//! Best-effort COA_EGearRole guess for a GM-placed AI, since it won't have a gearscript-role
//! prefab identity for COA_RoleHelper.ResourceToRole to match against (that lookup is purely
//! prefab-name-based and silently defaults to RIFLEMAN for any entity spawned from an arbitrary
//! vanilla/faction editor-catalog prefab). This inspects the character's actually equipped weapon
//! type instead - a real signal, even if a coarse one. No medic/engineer/leadership detection here;
//! that would need inventory-item inspection, not weapon type, and isn't required for v1.
class COA_GMRoleGuessHelper
{
	//------------------------------------------------------------------------------------------------
	static COA_EGearRole GuessRoleForEntity(IEntity entity)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character)
			return COA_EGearRole.RIFLEMAN;

		BaseWeaponManagerComponent weaponMan = character.GetWeaponManager();
		if (!weaponMan)
			return COA_EGearRole.UNARMED;

		array<WeaponSlotComponent> slots = {};
		weaponMan.GetWeaponsSlots(slots);

		bool hasAnyWeapon = false;

		foreach (WeaponSlotComponent slot : slots)
		{
			IEntity weaponEntity = slot.GetWeaponEntity();
			if (!weaponEntity)
				continue;

			BaseWeaponComponent weaponComp = BaseWeaponComponent.Cast(weaponEntity.FindComponent(BaseWeaponComponent));
			if (!weaponComp)
				continue;

			hasAnyWeapon = true;

			switch (weaponComp.GetWeaponType())
			{
				case EWeaponType.WT_ROCKETLAUNCHER:
					return COA_EGearRole.RIFLEMAN_ANTITANK;
				case EWeaponType.WT_MACHINEGUN:
					return COA_EGearRole.AUTOMATIC_RIFLEMAN;
				case EWeaponType.WT_SNIPERRIFLE:
					return COA_EGearRole.SNIPER;
				case EWeaponType.WT_GRENADELAUNCHER:
					return COA_EGearRole.GRENADIER;
				// WT_RIFLE / WT_HANDGUN / WT_AUTOCANNON / grenades: keep scanning other slots for
				// something more distinctive before falling through to RIFLEMAN below.
			}
		}

		if (!hasAnyWeapon)
			return COA_EGearRole.UNARMED;

		return COA_EGearRole.RIFLEMAN;
	}
}
