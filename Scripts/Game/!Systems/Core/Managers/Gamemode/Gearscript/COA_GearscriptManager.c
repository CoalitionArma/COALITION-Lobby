class COA_GearscriptManagerClass : ScriptComponentClass {}

class COA_GearscriptManager : ScriptComponent
{
	// NOTE: COA_MissionConfigurationPlugin.c intentionally keeps its own copy of the non-VON path
	// for its Workbench documentation tool, which must work without a live COA_GearscriptManager
	// instance - see the comment on AppendSlottingSection there before removing that duplication.
	protected const ResourceName C_RolesConfigResource = "{4388548E9F600148}Configs/Gearscripts/COA_Global_Roles_Config.conf";
	protected const ResourceName C_CVONRolesConfigResource = "{F04F02DBFC65553E}Configs/Gearscripts/Additional Configs/COA_CVON_Global_Roles_Config.conf";

	protected ref COA_ResourceCache m_ResourceCache;
	static ref COA_RolesConfig m_RolesConfig;

	protected COA_Gamemode m_Gamemode;

	// Parsed gearscript/identity config instances are static prefab-config data that never changes
	// mid-mission, so cache them here rather than re-parsing the container from disk on every call
	// (previously happened on every player spawn, every vehicle refit, and every inventory move).
	protected ref map<ResourceName, ref COA_GearScriptConfig> m_mGearScriptConfigCache = new map<ResourceName, ref COA_GearScriptConfig>();
	protected ref map<ResourceName, ref COA_CharacterIdentity> m_mIdentityConfigCache = new map<ResourceName, ref COA_CharacterIdentity>();
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 MANAGER INITILIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
	
		// Only run on in-game post init
		if (!GetGame().InPlayMode())
			return;
		
		LoadRoleConfig();
		m_ResourceCache = new COA_ResourceCache;
		m_Gamemode = COA_Gamemode.GetInstance();
		
		// 10APR26 - Pat, this has started causing crashing as of today. No idea why. Commenting out for now.
		// 3AUG26 - Believe this was simply a bad gearscript on your end. Did some efficency and NPE passes rq just in case.
		#ifdef WORKBENCH
			GetGame().GetCallqueue().CallLater(DEBUG_GearscriptSelfCheck, 1250, false);
		#endif
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 APPLYING GEAR METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Set gear for an entity based on its resource name
	//! \param[in] entity The entity to equip
	//! \param[in] resourceNameToScan Resource name containing faction info
	void SetEntityGear(IEntity entity, ResourceName resourceNameToScan)
	{
		if (!entity)
			return;

		// Determine faction from resource name
		FactionKey factionKey = COA_EntityHelper.DetermineFactionKey(entity);
		if (factionKey.IsEmpty())
			return;

		// Get gearscript resources
		ResourceName gearScriptResourceName = m_Gamemode.GetGearScriptResource(factionKey);
		COA_GearScriptContainer gearScriptSettings = m_Gamemode.GetGearScriptSettings(factionKey);

		if (gearScriptResourceName.IsEmpty() || !gearScriptSettings)
			return;

		// Get required components
		SCR_CharacterInventoryStorageComponent inventory = SCR_CharacterInventoryStorageComponent.Cast(entity.FindComponent(SCR_CharacterInventoryStorageComponent));
		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));

		if (!inventory || !inventoryManager)
		{
			string errorMsg = string.Format("Entity %1 is missing required inventory components (SCR_CharacterInventoryStorageComponent or SCR_InventoryStorageManagerComponent)", entity);
			Print("[GEARSCRIPT ERROR] " + errorMsg, LogLevel.ERROR);
			
			return;
		}

		// Get role and clear entity
		COA_EGearRole role = COA_RoleHelper.ResourceToRole(resourceNameToScan);
		COA_GearscriptCharacter.Cast(entity).SetGearRole(role);
		ClearEntityGear(inventory, inventoryManager);

		// Load gearscript config
		COA_GearScriptConfig gearConfig = LoadGearScriptConfig(gearScriptResourceName);
		
		// Prepare spawn parameters
		EntitySpawnParams spawnParams = COA_EntityHelper.CreateSpawnParams(entity.GetOrigin());
		
		// Apply gear
		ApplyClothing(gearConfig, role, spawnParams, inventory, inventoryManager);
		
		// Apply weapons
		ApplyWeapons(gearConfig, role, gearScriptSettings, spawnParams, inventory, inventoryManager);
		
		// Apply inventory items
		ApplyInventoryItems(gearConfig, role, gearScriptSettings, spawnParams, inventory, inventoryManager);
		
		int weight = inventoryManager.GetTotalWeightOfAllStorages();
		if (weight > 60) // We dont enjoy people being over 60kg
			COA_LoggingHelper.LogWeightError(entity, weight);
		
		// Initialize radios for player
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
		if (playerId > 0)
		{
			COA_PlayerController pc = COA_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			COA_PlayerRplToOwnerManager rplToOwnerManager = COA_PlayerRplToOwnerManager.GetInstance();
			// Cache groups manager reference - PERFORMANCE OPTIMIZATION
			SCR_GroupsManagerComponent groupsMan = SCR_GroupsManagerComponent.GetInstance();
			
			// Rebuild the radio list after replacing the player's gear. Tuning before
			// this can access stale entities left behind by ClearEntityGear().
			if (pc)
				pc.InitializeRadios(entity);

			if (groupsMan)
				groupsMan.TuneFreqDelayWithPresets(playerId, entity);
			
			if (rplToOwnerManager && pc)
			{
				rplToOwnerManager.InitializeRadioFromServer();
			}
		}
	}	
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 GEAR METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Apply clothing to entity based on config
	//! \param[in] gearConfig Gear configuration
	//! \param[in] role Role identifier
	//! \param[in] spawnParams Spawn parameters
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
	protected void ApplyClothing(COA_GearScriptConfig gearConfig, COA_EGearRole role, EntitySpawnParams spawnParams, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		// Apply default faction clothing
		if (gearConfig)
		{
			foreach (COA_Clothing clothing : gearConfig.m_DefaultClothing)
			{
				COA_ClothingHelper.UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_iClothingType, role, false, spawnParams, inventory, inventoryManager);
			}
		}
		
		// Apply custom clothing if available
		if (gearConfig)
		{
			foreach (ref COA_Role_Custom_Gear customGear : gearConfig.m_RolesToSetCustomSettings)
			{
				if (customGear.m_Role != role)
					continue;
		
				foreach (COA_Clothing clothing : customGear.m_Clothing)
				{
					COA_ClothingHelper.UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_iClothingType, role, true, spawnParams, inventory, inventoryManager);
				}
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Apply weapons to entity based on config
	//! \param[in] gearConfig Gear configuration
	//! \param[in] role Role identifier
	//! \param[in] gearScriptSettings Gearscript settings
	//! \param[in] spawnParams Spawn parameters
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
	protected void ApplyWeapons(COA_GearScriptConfig gearConfig, COA_EGearRole role, COA_GearScriptContainer gearScriptSettings,
		EntitySpawnParams spawnParams, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if(!inventory || !inventoryManager || !gearConfig)
			return;
		
		bool customWeaponsSet = ApplyCustomWeapons(gearConfig, role, spawnParams, inventory, inventoryManager);
		
		// Apply default weapons if no custom weapons were set
		if (!customWeaponsSet)
		{
			ApplyDefaultWeapons(gearConfig, role, spawnParams, inventory, inventoryManager);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Apply custom weapons based on role
	//! \param[in] gearConfig Gear configuration
	//! \param[in] role Role identifier
	//! \param[in] spawnParams Spawn parameters
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
	//! \return True if custom weapons were applied
	protected bool ApplyCustomWeapons(COA_GearScriptConfig gearConfig, COA_EGearRole role, EntitySpawnParams spawnParams,
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (!gearConfig)
			return false;
			
		bool customWeaponsSet = false;
		
		foreach (ref COA_Role_Custom_Gear customGear : gearConfig.m_RolesToSetCustomSettings)
		{
			if (customGear.m_Role != role)
				continue;
			
			// Primary weapon
			if (!customGear.m_PrimaryWeapon.IsEmpty())
			{
				COA_Weapon_Class primary = COA_WeaponHelper.SelectRandomWeapon(customGear.m_PrimaryWeapon);
				if(primary.m_Weapon)
				{
					COA_WeaponHelper.SpawnWeapon(primary.m_Weapon, primary.m_Attachments, spawnParams, inventory, inventoryManager);
					AddMagazines(primary.m_MagazineArray, spawnParams, inventory, inventoryManager);
					customWeaponsSet = true;
				};
			}
			
			// Secondary weapon
			if (!customGear.m_SecondaryWeapon.IsEmpty())
			{
				COA_Weapon_Class secondary = COA_WeaponHelper.SelectRandomWeapon(customGear.m_SecondaryWeapon);
				if(secondary.m_Weapon)
				{
					COA_WeaponHelper.SpawnWeapon(secondary.m_Weapon, secondary.m_Attachments, spawnParams, inventory, inventoryManager);
					AddMagazines(secondary.m_MagazineArray, spawnParams, inventory, inventoryManager);
					customWeaponsSet = true;
				};
			}
			
			// Pistol
			if (!customGear.m_Pistols.IsEmpty())
			{
				COA_Weapon_Class pistol = COA_WeaponHelper.SelectRandomWeapon(customGear.m_Pistols);
				if(pistol.m_Weapon)
				{
					COA_WeaponHelper.SpawnWeapon(pistol.m_Weapon, pistol.m_Attachments, spawnParams, inventory, inventoryManager);
					AddMagazines(pistol.m_MagazineArray, spawnParams, inventory, inventoryManager);
					customWeaponsSet = true;
				};
			}
		}
		
		return customWeaponsSet;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Apply default weapons based on role
	//! \param[in] gearConfig Gear configuration
	//! \param[in] role Role identifier
	//! \param[in] spawnParams Spawn parameters
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
	protected void ApplyDefaultWeapons(COA_GearScriptConfig gearConfig, COA_EGearRole role, EntitySpawnParams spawnParams,
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (!gearConfig)
			return;
		
		COA_RoleConfig rolesConfig = COA_GearscriptManager.GetRolesConfig().FindRoleConfig(role);
		array<COA_Weapon_Class> weaponsSelected = {};
		
		foreach (COA_EGearscriptWeapons weaponType : rolesConfig.m_aWeapons)
		{
			COA_Weapon_Class weapon;
			COA_Spec_Weapon_Class specWeapon;
			
			switch (weaponType)
			{
				case COA_EGearscriptWeapons.RIFLE:
					if(gearConfig.m_Rifles && !gearConfig.m_Rifles.IsEmpty())
					{
						weapon = COA_WeaponHelper.SelectRandomWeapon(gearConfig.m_Rifles);
						weaponsSelected.Insert(weapon); // Need to store the weapon we selected for magazines
					};
					break;
				
				case COA_EGearscriptWeapons.RIFLEUGL:
					if(gearConfig.m_RifleUGLs && !gearConfig.m_RifleUGLs.IsEmpty())
					{
						weapon = COA_WeaponHelper.SelectRandomWeapon(gearConfig.m_RifleUGLs);
						weaponsSelected.Insert(weapon); // Need to store the weapon we selected for magazines
					};
					break;
				
				case COA_EGearscriptWeapons.CARBINE:
					if(gearConfig.m_Carbines && !gearConfig.m_Carbines.IsEmpty())
					{
						weapon = COA_WeaponHelper.SelectRandomWeapon(gearConfig.m_Carbines);
						weaponsSelected.Insert(weapon); // Need to store the weapon we selected for magazines
					};
					break;

				case COA_EGearscriptWeapons.PISTOL:
					if(gearConfig.m_Pistols && !gearConfig.m_Pistols.IsEmpty())
					{
						weapon = COA_WeaponHelper.SelectRandomWeapon(gearConfig.m_Pistols);
						weaponsSelected.Insert(weapon); // Need to store the weapon we selected for magazines
					};
					break;

				case COA_EGearscriptWeapons.SNIPER:
					if(gearConfig.m_SNIPER)
						weapon = gearConfig.m_SNIPER;
					break;

				case COA_EGearscriptWeapons.AR:
					if(gearConfig.m_AR)
						specWeapon = gearConfig.m_AR;
					break;

				case COA_EGearscriptWeapons.MMG:
					if(gearConfig.m_MMG)
						specWeapon = gearConfig.m_MMG;
					break;

				case COA_EGearscriptWeapons.AT:
					if(gearConfig.m_AT)
						specWeapon = gearConfig.m_AT;
					break;
	
				case COA_EGearscriptWeapons.MAT:
					if(gearConfig.m_MAT)
						specWeapon = gearConfig.m_MAT;
					break;
	
				case COA_EGearscriptWeapons.HAT:
					if(gearConfig.m_HAT)
						specWeapon = gearConfig.m_HAT;
					break;

				case COA_EGearscriptWeapons.AA:
					if(gearConfig.m_AA)
						specWeapon = gearConfig.m_AA;
					break;

				case COA_EGearscriptWeapons.HMG:
					if(gearConfig.m_HMG)
						specWeapon = gearConfig.m_HMG;
					break;
			}
			
			if (weapon && weapon.m_Weapon)
				COA_WeaponHelper.SpawnWeapon(weapon.m_Weapon, weapon.m_Attachments, spawnParams, inventory, inventoryManager);
			
			if (specWeapon && specWeapon.m_Weapon)
				COA_WeaponHelper.SpawnWeapon(specWeapon.m_Weapon, specWeapon.m_Attachments, spawnParams, inventory, inventoryManager);
		}
		
		ApplyDefaultMagazines(weaponsSelected, gearConfig, role, spawnParams, inventory, inventoryManager);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Apply default magazines based on role
	//! \param[in] gearConfig Gear configuration
	//! \param[in] role Role identifier
	//! \param[in] spawnParams Spawn parameters
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
	protected void ApplyDefaultMagazines(array<COA_Weapon_Class> weaponsSelected, COA_GearScriptConfig gearConfig, COA_EGearRole role, EntitySpawnParams spawnParams,
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (!gearConfig)
			return;
		
		COA_RoleConfig rolesConfig = COA_GearscriptManager.GetRolesConfig().FindRoleConfig(role);
		bool isAssistant = (rolesConfig.m_SlottingType == COA_ESlotType.ASSISTANT || rolesConfig.m_SlottingType == COA_ESlotType.SPECIALTY_ASSISTANT);
		
		foreach (COA_EGearscriptMagazines roleMags : rolesConfig.m_aMagazines)
		{
			array<ref COA_Magazine_Class> magazineArray;
			COA_Weapon_Class selectedWeapon;
			
			switch (roleMags)
			{
				case COA_EGearscriptMagazines.RIFLE_MAG:
					if(gearConfig.m_Rifles && !gearConfig.m_Rifles.IsEmpty())
						magazineArray = COA_WeaponHelper.FindMagArrayForWeaponsSelected(weaponsSelected, gearConfig.m_Rifles, selectedWeapon);
					break;
				
				case COA_EGearscriptMagazines.RIFLEUGL_MAG:
					if(gearConfig.m_RifleUGLs && !gearConfig.m_RifleUGLs.IsEmpty())
						magazineArray = COA_WeaponHelper.FindMagArrayForWeaponsSelected(weaponsSelected, gearConfig.m_RifleUGLs, selectedWeapon);
					break;
				
				case COA_EGearscriptMagazines.CARBINE_MAG:
					if(gearConfig.m_Carbines && !gearConfig.m_Carbines.IsEmpty())
						magazineArray = COA_WeaponHelper.FindMagArrayForWeaponsSelected(weaponsSelected, gearConfig.m_Carbines, selectedWeapon);
					break;

				case COA_EGearscriptMagazines.PISTOL_MAG:
					if(gearConfig.m_Pistols && !gearConfig.m_Pistols.IsEmpty())
						magazineArray = COA_WeaponHelper.FindMagArrayForWeaponsSelected(weaponsSelected, gearConfig.m_Pistols, selectedWeapon);
					break;

				case COA_EGearscriptMagazines.SNIPER_MAG:
					if(gearConfig.m_SNIPER && gearConfig.m_SNIPER.m_MagazineArray)
						magazineArray = gearConfig.m_SNIPER.m_MagazineArray;
					break;

				case COA_EGearscriptMagazines.AR_MAG:
					if(gearConfig.m_AR && gearConfig.m_AR.m_MagazineArray)
						magazineArray = COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_AR.m_MagazineArray, isAssistant);
					break;

				case COA_EGearscriptMagazines.MMG_MAG:
					if(gearConfig.m_MMG && gearConfig.m_MMG.m_MagazineArray)
						magazineArray = COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_MMG.m_MagazineArray, isAssistant);
					break;

				case COA_EGearscriptMagazines.AT_MAG:
					if(gearConfig.m_AT && gearConfig.m_AT.m_MagazineArray)
						magazineArray = COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_AT.m_MagazineArray, isAssistant);
					break;
	
				case COA_EGearscriptMagazines.MAT_MAG:
					if(gearConfig.m_MAT && gearConfig.m_MAT.m_MagazineArray)
						magazineArray = COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_MAT.m_MagazineArray, isAssistant);
					break;
	
				case COA_EGearscriptMagazines.HAT_MAG:
					if(gearConfig.m_HAT && gearConfig.m_HAT.m_MagazineArray)
						magazineArray = COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_HAT.m_MagazineArray, isAssistant);
					break;

				case COA_EGearscriptMagazines.AA_MAG:
					if(gearConfig.m_AA && gearConfig.m_AA.m_MagazineArray)
						magazineArray = COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_AA.m_MagazineArray, isAssistant);
					break;

				case COA_EGearscriptMagazines.HMG_MAG:
					if(gearConfig.m_HMG && gearConfig.m_HMG.m_MagazineArray)
						magazineArray = COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_HMG.m_MagazineArray, isAssistant);
					break;
			}
			
			if (magazineArray && !magazineArray.IsEmpty())
				AddMagazines(magazineArray, spawnParams, inventory, inventoryManager);
			
			if (selectedWeapon)
				weaponsSelected.RemoveItem(selectedWeapon)
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Add a weapons magazines
	//! \param[in] magazineArray Magazines to add
	//! \param[in] spawnParams Spawn parameters
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
	protected void AddMagazines(array<ref COA_Magazine_Class> magazineArray, EntitySpawnParams spawnParams, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		// Add magazines
		if (magazineArray != null)
		{
			foreach (COA_Magazine_Class magazine : magazineArray)
			{
				if (magazine != null)
				{
					AddInventoryItem(magazine.m_Magazine, magazine.m_MagazineCount, spawnParams, inventory, inventoryManager);
				}
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Apply inventory items based on role and config
	//! \param[in] gearConfig Gear configuration
	//! \param[in] role Role identifier
	//! \param[in] gearScriptSettings Gearscript settings
	//! \param[in] spawnParams Spawn parameters
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
	protected void ApplyInventoryItems(COA_GearScriptConfig gearConfig, COA_EGearRole role, COA_GearScriptContainer gearScriptSettings,
		EntitySpawnParams spawnParams, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if(!inventory || !inventoryManager || !gearConfig)
			return;
		
		foreach (ref COA_Role_Custom_Gear customGear : gearConfig.m_RolesToSetCustomSettings)
		{
			if (customGear.m_Role != role)
				continue;
	
			foreach (COA_Inventory_Item item : customGear.m_AdditionalInventoryItems)
			{
				AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role);
			}
		}
		
		// Then apply default gear
		COA_RoleConfig rolesConfig = COA_GearscriptManager.GetRolesConfig().FindRoleConfig(role);
		
		foreach (COA_EGearscriptItems roleItem : rolesConfig.m_aItems)
		{
			switch (roleItem)
			{
				case COA_EGearscriptItems.SHORTRANGE_RADIO:
					if (gearScriptSettings.m_bEnableGIRadios)
						AddInventoryItem(gearScriptSettings.m_rShortRangeRadioPrefab, 1, spawnParams, inventory, inventoryManager);
					else if (gearScriptSettings.m_bEnableLeadershipRadios && (rolesConfig.m_SlottingType == COA_ESlotType.TEAM_LEADER || rolesConfig.m_SlottingType == COA_ESlotType.SQUAD_LEADER || rolesConfig.m_SlottingType == COA_ESlotType.SPECIALTY || rolesConfig.m_SlottingType == COA_ESlotType.SPECIALTY_ASSISTANT))
						AddInventoryItem(gearScriptSettings.m_rShortRangeRadioPrefab, 1, spawnParams, inventory, inventoryManager);
					break;
				
				case COA_EGearscriptItems.LONGRANGE_RADIO:
					if (gearScriptSettings.m_bEnableLeadershipRadios)
						AddInventoryItem(gearScriptSettings.m_rLongRangeRadioPrefab, 1, spawnParams, inventory, inventoryManager);
					break;
				
				case COA_EGearscriptItems.RTO_RADIO:
					if (gearScriptSettings.m_bEnableRTORadios)
						AddInventoryItem(gearScriptSettings.m_rRTORadiosPrefab, 1, spawnParams, inventory, inventoryManager);
					break;
				
				case COA_EGearscriptItems.LEADERSHIP_BINO:
					if (gearConfig.m_sLeadershipBinocularsPrefab != "")
						AddInventoryItem(gearConfig.m_sLeadershipBinocularsPrefab, 1, spawnParams, inventory, inventoryManager);
					break;
				
				case COA_EGearscriptItems.ASSISTANT_BINO:
					if (gearConfig.m_sAssistantBinocularsPrefab != "")
						AddInventoryItem(gearConfig.m_sAssistantBinocularsPrefab, 1, spawnParams, inventory, inventoryManager);
					break;

				case COA_EGearscriptItems.MEDIC_ITEMS:
					foreach (COA_Inventory_Item item : gearConfig.m_MedicMedicalItems)
						AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role);
					break;
			}
		}
		
		// Default medical items
		foreach (COA_Inventory_Item item : gearConfig.m_InfantryMedicalItems)
			AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role);
		
		// Default inventory items
		foreach (COA_Inventory_Item item : gearConfig.m_DefaultInventoryItems)
			AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Add inventory item
	//! \param[in] item Item resource to add
	//! \param[in] itemAmount Number of items to add
	//! \param[in] spawnParams Spawn parameters (unused - kept for compatibility)
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
	//! \param[in] role Role identifier
	void AddInventoryItem(ResourceName item, int itemAmount, EntitySpawnParams spawnParams, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager, 
		COA_EGearRole role = 0)
	{
		if (item.IsEmpty())
			return;
			
		Resource resource = m_ResourceCache.GetCachedResource(item);
		if (!resource || !resource.IsValid())
			return;
			
		IEntitySource itemSource = SCR_BaseContainerTools.FindEntitySource(resource);
		if (!itemSource)
			return;
		
		// Determine item type to use appropriate storage priority
		TIntArray clothingIDs = COA_InventoryHelper.FilterItemToClothing(itemSource, role);
		
		for (int i = 1; i <= itemAmount; i++)
		{
			bool spawned = false;

			foreach (int clothingID : clothingIDs)
			{
				IEntity clothing = inventory.Get(clothingID);
				if (clothing)
				{
					BaseInventoryStorageComponent clothingStorage = BaseInventoryStorageComponent.Cast(clothing.FindComponent(BaseInventoryStorageComponent));
					if (clothingStorage)
					{
						if (inventoryManager.CanInsertResourceInStorage(item, clothingStorage))
							spawned = inventoryManager.TrySpawnPrefabToStorage(item, clothingStorage);
						
						if (!spawned && clothingID == COA_EGearscriptClothing.VEST) // unable to insert directly into some vests storage comp, so we just let the item/inventoryManager decide (99% of the time it's a vest)
							spawned = inventoryManager.TrySpawnPrefabToStorage(item);
						
						if (spawned)
							break;
					};
				};
			}
			
			if (!spawned) // One final effort is all that remains
				spawned = inventoryManager.TrySpawnPrefabToStorage(item);
			
			if (!spawned)
				COA_LoggingHelper.LogItemError(item, inventoryManager.GetOwner());
		};
	}
	
	//------------------------------------------------------------------------------------------------
	//! Clear all gear from an entity
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
	protected void ClearEntityGear(SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		array<IEntity> items = {};
		array<IEntity> itemsRoot = {};
		inventoryManager.GetAllItems(items, inventory);
		inventoryManager.GetItems(itemsRoot);

		items.InsertAll(itemsRoot);

		// Separate weapons from other items to delete weapons first
		// This prevents MuzzleInMagComponent crashes when projectiles are deleted before weapon detaches them
		array<IEntity> weapons = {};
		array<IEntity> otherItems = {};
		
		foreach (IEntity item : items)
		{
			if (!item)
				continue;
				
			// Check if item is a weapon
			if (item.FindComponent(WeaponComponent))
				weapons.Insert(item);
			else
				otherItems.Insert(item);
		}
		
		// Delete weapons FIRST - this allows them to properly detach projectiles from MuzzleInMagComponent
		foreach (IEntity weapon : weapons)
		{
			if (weapon)
				SCR_EntityHelper.DeleteEntityAndChildren(weapon);
		}
		
		// Small delay before deleting other items to ensure weapon cleanup is complete
		// This prevents race conditions with MuzzleInMagComponent projectile attachment
		if (!otherItems.IsEmpty())
			GetGame().GetCallqueue().Call(DeleteRemainingItems, otherItems);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Delete remaining non-weapon items after weapon cleanup
	//! \param[in] items Array of items to delete
	protected void DeleteRemainingItems(array<IEntity> items)
	{
		foreach (IEntity item : items)
		{
			if (item)
				SCR_EntityHelper.DeleteEntityAndChildren(item);
		}
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 IDENTITY METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------	
	//! Set identity for an entity
	//! \param[in] entity The entity to equip
	void SetEntityIdentity(IEntity entity)
	{
		if (!entity)
			return;

		// Determine faction from resource name
		FactionKey factionKey = COA_EntityHelper.DetermineFactionKey(entity);
		if (factionKey.IsEmpty())
			return;

		// Get gearscript resources
		ResourceName gearScriptResourceName = m_Gamemode.GetGearScriptResource(factionKey);
		if (gearScriptResourceName.IsEmpty())
			return;

		// Load gearscript config
		COA_GearScriptConfig gearConfig = LoadGearScriptConfig(gearScriptResourceName);
		if (!gearConfig)
			return;
		
		SCR_CharacterIdentityComponent identityComp = SCR_CharacterIdentityComponent.Cast(entity.FindComponent(SCR_CharacterIdentityComponent));
		if (!identityComp)
			return;
		
		// Get both sound and visual identities from the identity identityComp
		VisualIdentity visIdentity = identityComp.GetIdentity().GetVisualIdentity();
		SoundIdentity sndIdentity = identityComp.GetIdentity().GetSoundIdentity();
		if (!visIdentity || !sndIdentity)
			return;
			
		COA_CharacterIdentity gsCharIdentity = LoadIdentityConfig(gearConfig.m_FactionIdentity);
		
		if (gsCharIdentity)
		{
			COA_Character_Visual_Identity gsVisIdentity;
			COA_Character_Sound_Identity gsSndIdentity;
			
			if (!gsCharIdentity.m_VisualIdentityArray.IsEmpty())
				gsVisIdentity = gsCharIdentity.m_VisualIdentityArray.GetRandomElement();			if (!gsCharIdentity.m_SoundIdentityArray.IsEmpty())
					gsSndIdentity = gsCharIdentity.m_SoundIdentityArray.GetRandomElement();
			
			if (gsVisIdentity)
			{
	        		visIdentity.SetHead(gsVisIdentity.m_Head);
	        		visIdentity.SetBody(gsVisIdentity.m_Body);
			};
			
			if (gsSndIdentity)
			{
	        		sndIdentity.SetVoiceID(gsSndIdentity.m_VoiceID);
				sndIdentity.SetPitch(gsSndIdentity.m_VoicePitch);
			};
			
			// Commit all changes to the identity comp
	        identityComp.CommitChanges();
		};
    }
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 ROLE CONFIG ACCESSORS/LOADERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	static COA_RolesConfig GetRolesConfig()
	{
		return m_RolesConfig;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Load necessary configurations for gearscript
	protected void LoadRoleConfig()
	{
		ResourceName rolesConfigPath;
		if (!CVON_VONGameModeComponent.GetInstance())
			rolesConfigPath = C_RolesConfigResource;
		else
			rolesConfigPath = C_CVONRolesConfigResource;
		
		m_RolesConfig = COA_RolesConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
			BaseContainerTools.LoadContainer(rolesConfigPath).GetResource().ToBaseContainer()));
	}
	
	//------------------------------------------------------------------------------------------------
	//! Load gear script config from resource, caching the parsed instance so repeated calls
	//! (every spawn, every vehicle refit, every inventory move) don't re-parse it from disk.
	//! \param[in] resourceName Resource to load
	//! \return Loaded config or null if failed
	COA_GearScriptConfig LoadGearScriptConfig(ResourceName resourceName)
	{
		if (resourceName.IsEmpty())
			return null;

		COA_GearScriptConfig cachedConfig = m_mGearScriptConfigCache.Get(resourceName);
		if (cachedConfig)
			return cachedConfig;

		COA_GearScriptConfig config = COA_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
			BaseContainerTools.LoadContainer(resourceName).GetResource().ToBaseContainer()));

		if (config)
			m_mGearScriptConfigCache.Set(resourceName, config);

		return config;
	}

	//------------------------------------------------------------------------------------------------
	//! Load character identity config from resource, caching the parsed instance (see LoadGearScriptConfig).
	//! \param[in] resourceName Resource to load
	//! \return Loaded config or null if failed
	COA_CharacterIdentity LoadIdentityConfig(ResourceName resourceName)
	{
		if (resourceName.IsEmpty())
			return null;

		COA_CharacterIdentity cachedIdentity = m_mIdentityConfigCache.Get(resourceName);
		if (cachedIdentity)
			return cachedIdentity;

		COA_CharacterIdentity identity = COA_CharacterIdentity.Cast(BaseContainerTools.CreateInstanceFromContainer(
			BaseContainerTools.LoadContainer(resourceName).GetResource().ToBaseContainer()));

		if (identity)
			m_mIdentityConfigCache.Set(resourceName, identity);

		return identity;
	}
	
	#ifdef WORKBENCH
//=============================================================================================================================================================================================================================================================================================================================================================
//	 SELF-CHECK METHODS (ONLY CALLED IN WORKBENCH)
//=============================================================================================================================================================================================================================================================================================================================================================
	
	protected COA_PlayerCharacter m_CharacterToCheckAgainst;
	
	//------------------------------------------------------------------------------------------------
	protected void DEBUG_GearscriptSelfCheck()
	{
		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		
		// Setup Faction/Role Arrays
		array<FactionKey> factionKeys = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
		int delayTime;
		int delay;
		
		foreach (FactionKey factionKey : factionKeys)
		{
			COA_SpawnPointData initialSpawnData = respawnManager.FindInitalFactionSpawnpoint(factionKey);
			if (initialSpawnData)
			{		
				IEntity spawnPointEnt = COA_EntityHelper.GetEntityFromRplId(initialSpawnData.GetSpawnPointEntity());
				if (spawnPointEnt)
				{
					EntitySpawnParams spawnParams = new EntitySpawnParams();
					spawnParams.TransformMode = ETransformMode.WORLD;
					spawnPointEnt.GetWorldTransform(spawnParams.Transform);
					
					array<ref COA_RoleConfig> roleArray = {};
					
					foreach(COA_SlottingGroup slotGroup : m_Gamemode.GetSlots(factionKey))
					{
						foreach(COA_EGearRole role : slotGroup.m_aSlots)
						{
							COA_RoleConfig roleConfig = m_RolesConfig.FindRoleConfig(role);
							if (!roleArray.Contains(roleConfig))
							{
								roleArray.Insert(roleConfig);
								
								delay++;
								delayTime = (delay * 125); // need a delay to prevent massive lag spikes on mission load (and to see what role actually casued the error lol)
								GetGame().GetCallqueue().CallLater(DEBUG_CheckGear, delayTime, false, factionKey, roleConfig, spawnParams);
							}
						}
					}
				};
			};
		}

		GetGame().GetCallqueue().CallLater(DEBUG_DeleteCharacterCheck, delayTime + 150, false, "", true);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void DEBUG_CheckGear(FactionKey factionKey, COA_RoleConfig roleConfig, EntitySpawnParams spawnParams)
	{	
		DEBUG_DeleteCharacterCheck(factionKey);
		
		if (!m_CharacterToCheckAgainst)
		{	
			m_CharacterToCheckAgainst = COA_PlayerCharacter.Cast(GetGame().SpawnEntityPrefab(m_ResourceCache.GetCachedResource(roleConfig.m_RoleResource), GetGame().GetWorld(), spawnParams));
			
			// Update character faction
			Faction faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
			FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(m_CharacterToCheckAgainst.FindComponent(FactionAffiliationComponent));
			facComp.SetAffiliatedFaction(faction);
			
			PrintFormat("==========================================================================================================================================================");
			PrintFormat("----------------------------------------------------------------------------");
			PrintFormat("|     [GEARSCIRPT VALIDATION] : ALL SLOTS FOR %1 ARE BEING CHECKED...", factionKey);
			PrintFormat("----------------------------------------------------------------------------");
			PrintFormat("[GEARSCIRPT VALIDATION] : CHECKING GEAR FOR: %1, %2", factionKey, SCR_Enum.GetEnumName(COA_EGearRole, roleConfig.m_Role));
		} else {
			PrintFormat("[GEARSCIRPT VALIDATION] : CHECKING GEAR FOR: %1, %2", factionKey, SCR_Enum.GetEnumName(COA_EGearRole, roleConfig.m_Role));
			SetEntityGear(m_CharacterToCheckAgainst, roleConfig.m_RoleResource);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	protected FactionKey m_FactionInUse;
	protected void DEBUG_DeleteCharacterCheck(FactionKey factionKey, bool forceDelete = false)
	{	
		if (m_FactionInUse != factionKey || forceDelete)
		{
			m_FactionInUse = factionKey;
			
			if (m_CharacterToCheckAgainst)
			{
				PrintFormat("----------------------------------------------------------------------------");
				PrintFormat("|     [GEARSCIRPT VALIDATION] : ALL SLOTS FOR %1 HAVE BEEN CHECKED!", FactionAffiliationComponent.Cast(m_CharacterToCheckAgainst.FindComponent(FactionAffiliationComponent)).GetAffiliatedFaction().GetFactionKey());
				PrintFormat("----------------------------------------------------------------------------");
				
				SCR_EntityHelper.DeleteEntityAndChildren(m_CharacterToCheckAgainst);
			}
		}
	}
	#endif
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static COA_GearscriptManager m_sInstance;
	void COA_GearscriptManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_GearscriptManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static COA_GearscriptManager GetInstance()
	{
		return m_sInstance;
	}
};
