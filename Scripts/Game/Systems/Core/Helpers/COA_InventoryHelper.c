class COA_InventoryHelper
{
	const static ref array<EWeaponType> WEAPON_TYPES_THROWABLE = {EWeaponType.WT_FRAGGRENADE, EWeaponType.WT_SMOKEGRENADE};
	
	//------------------------------------------------------------------------------------------------
	//! Check if a prefab is a throwable weapon by inspecting its weapon type
	//! \param[in] entitySource Prefab source
	//! \return True if prefab is a throwable (grenade)
	static bool IsThrowableFromSource(IEntitySource entitySource)
	{
		if (!entitySource)
			return false;
			
		IEntityComponentSource componentSource = SCR_BaseContainerTools.FindComponentSource(entitySource, "WeaponComponent");
		if (!componentSource)
			return false;
			
		int weaponType;
		if (componentSource.Get("WeaponType", weaponType))
		{
			return WEAPON_TYPES_THROWABLE.Contains(weaponType);
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Determine appropriate clothing slots for an item
	//! \param[in] itemSource Item to filter
	//! \param[in] role Role identifier
	//! \return Array of appropriate clothing slot IDs
	static TIntArray FilterItemToClothing(IEntitySource itemSource, COA_EGearRole role = 0)
	{
		array<int> clothingIDs = {};
		
		// Determine item type
		bool isThrowable = COA_InventoryHelper.IsThrowableFromSource(itemSource);
		
		bool isGL = !isThrowable && 
			(SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_ShellSoundComponent") || 
			SCR_BaseContainerTools.FindComponentSource(itemSource, "ShellMoveComponent"));
		
		bool isMagazine = !isGL && !isThrowable && 
			(SCR_BaseContainerTools.FindComponentSource(itemSource, "MagazineComponent") || 
			SCR_BaseContainerTools.FindComponentSource(itemSource, "InventoryMagazineComponent"));
		
		bool isMedical = !isThrowable && !isMagazine && 
			COA_GearscriptManager.GetRolesConfig().FindRoleConfig(role).m_aItems.Contains(COA_EGearscriptItems.MEDIC_ITEMS) && 
			SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_ConsumableItemComponent");
		
		bool isEquipment = !isGL && !isThrowable && !isMagazine && !isMedical && 
			(SCR_BaseContainerTools.FindComponentSource(itemSource, "BaseRadioComponent") ||
			SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_CompassComponent") ||
			SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_MapGadgetComponent") ||
			SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_BallisticTableComponent"));
		
		bool isExplosive = !isGL && !isThrowable && !isMagazine && !isMedical && !isEquipment && 
			(SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_DetonatorGadgetComponent") || 
			SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_ExplosiveChargeComponent") ||
			SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_MineWeaponComponent"));
		
		bool isTool = !isGL && !isThrowable && !isMagazine && !isMedical && !isEquipment && !isExplosive && 
			(SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_RepairSupportStationComponent") ||
			SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_HealSupportStationComponent"));
		
		bool isBino = !isGL && !isThrowable && !isMagazine && !isMedical && !isEquipment && !isExplosive && !isTool &&
			SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_BinocularsComponent");
	
		bool isFlashlight = !isGL && !isThrowable && !isMagazine && !isMedical && !isEquipment && !isExplosive && !isTool && !isBino &&
			SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_FlashlightComponent");
		
		switch (true)
		{
			//--------------------------------------------------------------------------
			// Magazine/Explosives/Medical/Tool go in backpack, vest, armor, primarily
			case (isMagazine || isExplosive || isMedical || isTool) : {
				clothingIDs = {
					COA_EGearscriptClothing.BACKPACK,
					COA_EGearscriptClothing.VEST, 
					COA_EGearscriptClothing.ARMOREDVEST,
					COA_EGearscriptClothing.PANTS, 
					COA_EGearscriptClothing.SHIRT
				}; break;
			}
			//--------------------------------------------------------------------------
			// Throwables go in pants, vest primarily
			case (isThrowable) : {
				clothingIDs = {
					COA_EGearscriptClothing.VEST, 
					COA_EGearscriptClothing.ARMOREDVEST, 
					COA_EGearscriptClothing.PANTS, 
					COA_EGearscriptClothing.BACKPACK
				}; break;
			}
			//--------------------------------------------------------------------------
			// GLs go in vest, pants, backpack, primarily
			case (isGL) : {
				clothingIDs = {
					COA_EGearscriptClothing.VEST,
					COA_EGearscriptClothing.ARMOREDVEST, 
					COA_EGearscriptClothing.PANTS, 
					COA_EGearscriptClothing.BACKPACK,
					COA_EGearscriptClothing.SHIRT
				}; break;
			}
			//--------------------------------------------------------------------------
			// Equipment go in pants, shirt, vest primarily
			case (isEquipment) : {
				clothingIDs = {
					COA_EGearscriptClothing.SHIRT, 
					COA_EGearscriptClothing.PANTS, 
					COA_EGearscriptClothing.ARMOREDVEST, 
					COA_EGearscriptClothing.VEST, 
					COA_EGearscriptClothing.BACKPACK
				}; break;
			}
			//--------------------------------------------------------------------------
			// Flashlights/Binos go in Vests, ArmouredVests, primarily
			case (isFlashlight || isBino) : {
				clothingIDs = {
					COA_EGearscriptClothing.VEST, 
					COA_EGearscriptClothing.ARMOREDVEST, 
					COA_EGearscriptClothing.SHIRT, 
					COA_EGearscriptClothing.PANTS, 
					COA_EGearscriptClothing.BACKPACK
				}; break;
			}
			//--------------------------------------------------------------------------
			// Non-sorted items go in shirt, pants, vest primarily
			default : {
				clothingIDs = {
					COA_EGearscriptClothing.PANTS, 
					COA_EGearscriptClothing.SHIRT, 
					COA_EGearscriptClothing.VEST, 
					COA_EGearscriptClothing.ARMOREDVEST, 
					COA_EGearscriptClothing.BACKPACK
				};
			}
		}

		return clothingIDs;
	}
}