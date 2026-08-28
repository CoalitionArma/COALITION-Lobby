class COA_ClothingHelper
{	
	//------------------------------------------------------------------------------------------------
	//! Update clothing in a specific slot
	//! \param[in] clothingArray Clothing options
	//! \param[in] slotInt Slot to update
	//! \param[in] role Role identifier
	//! \param[in] deletePreviousItems Whether to delete previous items
	//! \param[in] spawnParams Spawn parameters
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
	static void UpdateClothingSlot(array<ResourceName> clothingArray, int slotInt, COA_EGearRole role, bool deletePreviousItems, 
		EntitySpawnParams spawnParams, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (clothingArray.IsEmpty() || slotInt == -1)
			return;
		
		array<IEntity> removedItems = {};
		IEntity previousClothing = inventory.Get(slotInt);
		
		// Get random clothing from the array
		ResourceName clothing = clothingArray.GetRandomElement();

		// Process previous clothing and its contents
		if (previousClothing)
			ProcessPreviousClothing(previousClothing, removedItems, inventory, inventoryManager);

		// Add new clothing if exists
		if (!clothing.IsEmpty())
		{
			if (slotInt == 123456789)
				slotInt = -1;
			
			bool spawned = inventoryManager.TrySpawnPrefabToStorage(clothing, inventory, slotInt, EStoragePurpose.PURPOSE_DEPOSIT, new SCR_PrefabSpawnCallback(inventory)); //backpacks, pants, shirts, etc.
			if (!spawned)
			{
				spawned = inventoryManager.TrySpawnPrefabToStorage(clothing, inventory, slotInt, EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT, new SCR_PrefabSpawnCallback(inventory)); //armoured vests, helmets, etc.
				if (!spawned)
					COA_LoggingHelper.LogItemError(clothing, inventory.GetOwner(), "CLOTHING");
			};
		}

		// Handle previously removed items
		foreach (IEntity oldItem : removedItems)
		{
			if (!deletePreviousItems)
			{
				bool inserted = inventoryManager.TryInsertItemInStorage(oldItem, inventory, slotInt);
				if (!inserted)
					inventoryManager.TryInsertItemInStorage(oldItem, inventory);
			} else 
				SCR_EntityHelper.DeleteEntityAndChildren(oldItem);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Process previous clothing before replacement
	//! \param[in] previousClothing Previous clothing entity
	//! \param[out] removedItems Array to store removed items
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
	static void ProcessPreviousClothing(IEntity previousClothing, out array<IEntity> removedItems, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		BaseInventoryStorageComponent oldStorage = BaseInventoryStorageComponent.Cast(previousClothing.FindComponent(BaseInventoryStorageComponent));
		if (oldStorage)
		{
			array<IEntity> outItems = {};
			oldStorage.GetAll(outItems);
			
			foreach (IEntity item : outItems)
			{
				if (!item || !InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent)))
					continue;
					
				if (SCR_EquipmentStorageComponent.Cast(item.FindComponent(SCR_EquipmentStorageComponent)) != null)
					continue;
					
				if (SCR_UniversalInventoryStorageComponent.Cast(item.FindComponent(SCR_UniversalInventoryStorageComponent)) != null)
					continue;
					
				if (BaseInventoryStorageComponent.Cast(item.FindComponent(BaseInventoryStorageComponent)) != null)
					continue;

				inventoryManager.TryRemoveItemFromStorage(item, oldStorage);
				removedItems.Insert(item);
			}
		}

		inventoryManager.TryRemoveItemFromStorage(previousClothing, inventory);
		SCR_EntityHelper.DeleteEntityAndChildren(previousClothing);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Returns true if the item's prefab is listed as a clothing piece in the faction gearscript assigned to the local player's faction and role.
	//! Checks both m_DefaultClothing (faction-wide) and m_RolesToSetCustomSettings (role-specific overrides) so custom role uniforms are also covered.
	//! \param[in] item The item the player is trying to remove.
	//! \return True if removal should be blocked.
	static bool IsGearscriptClothingPiece(IEntity item)
	{
		if (!item)
			return false;

		// Resolve the gearscript manager and the local player's faction.
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode)
			return false;

		array<COA_EGearscriptClothing> restrictedClothing = {COA_EGearscriptClothing.SHIRT, COA_EGearscriptClothing.PANTS, COA_EGearscriptClothing.BOOTS};
		TStringArray factionKeys = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
		
		foreach (string factionKey : factionKeys)
		{
			// Load the gearscript config for this faction.
			ResourceName gearScriptResource = gamemode.GetGearScriptResource(factionKey);
			if (gearScriptResource.IsEmpty())
				continue;

			COA_GearScriptConfig gearConfig = COA_GearscriptManager.GetInstance().LoadGearScriptConfig(gearScriptResource);
			if (!gearConfig)
				continue;
	
			ResourceName itemPrefab = item.GetPrefabData().GetPrefabName();
	
			// --- Check default (faction-wide) clothing ---
			foreach (COA_Clothing clothing : gearConfig.m_DefaultClothing)
			{
				if (restrictedClothing.Contains(clothing.m_iClothingType) && clothing.m_ClothingPrefabs.Contains(itemPrefab))
					return true;
			}
		}

		return false;
	}
}