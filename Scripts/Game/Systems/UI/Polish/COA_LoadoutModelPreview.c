//------------------------------------------------------------------------------------------------
//! Shows a role's character dressed in its faction gearscript in an ItemPreviewWidget, so players
//! can see what each side looks like before they spawn (slotting menu loadout card).
//!
//! Follows vanilla's arsenal loadout preview (SCR_LoadoutPreviewComponent): the item preview manager
//! keeps a copy of the role's character prefab in its own preview world, and its clothing and
//! weapons are swapped by attaching locally spawned prefabs straight to the equipment slots. Nothing
//! goes through the inventory system or the network, and nothing appears in the game world.
//!
//! Mirrors the gearscript's choices (COA_GearscriptManager.ApplyClothing / ApplyWeapons): faction
//! default clothing, then the role's custom clothing per slot; custom role weapons if any, else the
//! role's default weapon types. Randomised pools show their first entry so the model doesn't change
//! between hovers - the text beside it lists every option.
//------------------------------------------------------------------------------------------------
class COA_LoadoutModelPreview
{
	protected static const ResourceName PREVIEW_MANAGER_PREFAB = "{9F18C476AB860F3B}Prefabs/World/Game/ItemPreviewManager.et";

	//------------------------------------------------------------------------------------------------
	//! \param[in] widget Where to render the character
	//! \param[in] factionKey Faction whose gearscript dresses it
	//! \param[in] role Role to dress as
	//! \param[in] characterPrefab The role's character prefab (COA_SlotData.GetSlotResource)
	//! \return false if nothing could be shown
	static bool Show(ItemPreviewWidget widget, FactionKey factionKey, COA_EGearRole role, ResourceName characterPrefab)
	{
		if (!widget || characterPrefab.IsEmpty())
			return false;

		ItemPreviewManagerEntity previewManager = GetPreviewManager();
		if (!previewManager)
			return false;

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		COA_GearscriptManager gearscriptManager = COA_GearscriptManager.GetInstance();
		COA_RolesConfig rolesConfig = COA_GearscriptManager.GetRolesConfig();
		if (!gamemode || !gearscriptManager || !rolesConfig)
			return false;

		COA_GearScriptConfig gearConfig = gearscriptManager.LoadGearScriptConfig(gamemode.GetGearScriptResource(factionKey));
		COA_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
		if (!gearConfig || !roleConfig)
			return false;

		// The preview world keeps one copy per prefab, so it is shared between factions using the
		// same role prefab - strip it and dress it again every time
		IEntity character = previewManager.ResolvePreviewEntityForPrefab(characterPrefab);
		if (!character)
			return false;

		DeleteAttachedItems(character, false);

		COA_Role_Custom_Gear customGear = COA_LoadoutPreviewHelper.FindCustomGear(gearConfig, role);
		DressClothing(character, gearConfig, customGear);
		DressWeapons(character, gearConfig, roleConfig, customGear);

		previewManager.SetPreviewItem(widget, character, GetRenderAttributes(character), true);
		return true;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 CLOTHING
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected static void DressClothing(IEntity character, COA_GearScriptConfig gearConfig, COA_Role_Custom_Gear customGear)
	{
		EquipedLoadoutStorageComponent loadoutStorage = EquipedLoadoutStorageComponent.Cast(character.FindComponent(EquipedLoadoutStorageComponent));
		if (!loadoutStorage)
			return;

		// Clothing type -> prefab; the role's custom clothing replaces the faction default per slot
		map<int, ResourceName> clothingBySlot = new map<int, ResourceName>();
		CollectClothing(gearConfig.m_DefaultClothing, clothingBySlot);
		if (customGear)
			CollectClothing(customGear.m_Clothing, clothingBySlot);

		foreach (int clothingType, ResourceName prefab : clothingBySlot)
		{
			IEntity cloth = SpawnLocal(prefab, character.GetWorld());
			if (!cloth)
				continue;

			InventoryStorageSlot slot = FindClothingSlot(loadoutStorage, clothingType, cloth);
			if (!slot)
			{
				SCR_EntityHelper.DeleteEntityAndChildren(cloth);
				continue;
			}

			slot.AttachEntity(cloth);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void CollectClothing(array<ref COA_Clothing> clothing, notnull map<int, ResourceName> clothingBySlot)
	{
		if (!clothing)
			return;

		foreach (COA_Clothing piece : clothing)
		{
			if (!piece || !piece.m_ClothingPrefabs || piece.m_ClothingPrefabs.IsEmpty())
				continue;

			// An empty first option means "nothing in this slot" for part of the faction
			clothingBySlot.Set(piece.m_iClothingType, piece.m_ClothingPrefabs[0]);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Gearscript clothing types are the character loadout's slot indices (the gearscript spawns into
	//! inventory slot m_iClothingType). UNDEFINED, or a slot that doesn't fit, falls back to the slot
	//! matching the cloth's own loadout area.
	protected static InventoryStorageSlot FindClothingSlot(EquipedLoadoutStorageComponent loadoutStorage, int clothingType, IEntity cloth)
	{
		if (clothingType != COA_EGearscriptClothing.UNDEFINED && clothingType >= 0 && clothingType < loadoutStorage.GetSlotsCount())
		{
			InventoryStorageSlot indexedSlot = loadoutStorage.GetSlot(clothingType);
			if (indexedSlot && !indexedSlot.GetAttachedEntity())
				return indexedSlot;
		}

		BaseLoadoutClothComponent clothComponent = BaseLoadoutClothComponent.Cast(cloth.FindComponent(BaseLoadoutClothComponent));
		if (!clothComponent || !clothComponent.GetAreaType())
			return null;

		LoadoutSlotInfo areaSlot = loadoutStorage.GetSlotFromArea(clothComponent.GetAreaType().Type());
		if (!areaSlot || areaSlot.GetAttachedEntity())
			return null;

		return areaSlot;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 WEAPONS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected static void DressWeapons(IEntity character, COA_GearScriptConfig gearConfig, COA_RoleConfig roleConfig, COA_Role_Custom_Gear customGear)
	{
		BaseWeaponManagerComponent weaponManager = BaseWeaponManagerComponent.Cast(character.FindComponent(BaseWeaponManagerComponent));
		if (!weaponManager)
			return;

		array<COA_Base_Weapon_Class> weapons = {};
		if (customGear)
		{
			AddFirstOfPool(customGear.m_PrimaryWeapon, weapons);
			AddFirstOfPool(customGear.m_SecondaryWeapon, weapons);
			AddFirstOfPool(customGear.m_Pistols, weapons);
		}

		// Custom role weapons replace the defaults entirely (COA_GearscriptManager.ApplyWeapons)
		if (weapons.IsEmpty())
			CollectDefaultWeapons(gearConfig, roleConfig, weapons);

		array<WeaponSlotComponent> weaponSlots = {};
		weaponManager.GetWeaponsSlots(weaponSlots);

		WeaponSlotComponent firstFilledSlot;
		foreach (COA_Base_Weapon_Class weaponClass : weapons)
		{
			IEntity weapon = SpawnLocal(weaponClass.m_Weapon, character.GetWorld());
			if (!weapon)
				continue;

			WeaponSlotComponent slot = FindWeaponSlot(weaponSlots, weapon);
			if (!slot || !slot.GetSlotInfo())
			{
				SCR_EntityHelper.DeleteEntityAndChildren(weapon);
				continue;
			}

			slot.GetSlotInfo().AttachEntity(weapon);
			AttachAttachments(weapon, weaponClass.m_Attachments);

			if (!firstFilledSlot)
				firstFilledSlot = slot;
		}

		// First weapon in hand, like vanilla's preview of a spawned loadout
		if (firstFilledSlot)
			weaponManager.SelectWeapon(firstFilledSlot);
	}

	//------------------------------------------------------------------------------------------------
	protected static void CollectDefaultWeapons(COA_GearScriptConfig gearConfig, COA_RoleConfig roleConfig, notnull array<COA_Base_Weapon_Class> weapons)
	{
		if (!roleConfig.m_aWeapons)
			return;

		foreach (COA_EGearscriptWeapons weaponType : roleConfig.m_aWeapons)
		{
			switch (weaponType)
			{
				case COA_EGearscriptWeapons.RIFLE:		AddFirstOfPool(gearConfig.m_Rifles, weapons); break;
				case COA_EGearscriptWeapons.RIFLEUGL:	AddFirstOfPool(gearConfig.m_RifleUGLs, weapons); break;
				case COA_EGearscriptWeapons.CARBINE:	AddFirstOfPool(gearConfig.m_Carbines, weapons); break;
				case COA_EGearscriptWeapons.PISTOL:		AddFirstOfPool(gearConfig.m_Pistols, weapons); break;
				case COA_EGearscriptWeapons.SNIPER:		AddWeapon(gearConfig.m_SNIPER, weapons); break;
				case COA_EGearscriptWeapons.AR:			AddWeapon(gearConfig.m_AR, weapons); break;
				case COA_EGearscriptWeapons.MMG:		AddWeapon(gearConfig.m_MMG, weapons); break;
				case COA_EGearscriptWeapons.HMG:		AddWeapon(gearConfig.m_HMG, weapons); break;
				case COA_EGearscriptWeapons.AT:			AddWeapon(gearConfig.m_AT, weapons); break;
				case COA_EGearscriptWeapons.MAT:		AddWeapon(gearConfig.m_MAT, weapons); break;
				case COA_EGearscriptWeapons.HAT:		AddWeapon(gearConfig.m_HAT, weapons); break;
				case COA_EGearscriptWeapons.AA:			AddWeapon(gearConfig.m_AA, weapons); break;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddFirstOfPool(array<ref COA_Weapon_Class> pool, notnull array<COA_Base_Weapon_Class> weapons)
	{
		if (!pool)
			return;

		foreach (COA_Weapon_Class weapon : pool)
		{
			if (weapon && !weapon.m_Weapon.IsEmpty())
			{
				weapons.Insert(weapon);
				return;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddWeapon(COA_Base_Weapon_Class weapon, notnull array<COA_Base_Weapon_Class> weapons)
	{
		if (weapon && !weapon.m_Weapon.IsEmpty())
			weapons.Insert(weapon);
	}

	//------------------------------------------------------------------------------------------------
	//! A free character weapon slot of the weapon's own slot type (primary, secondary, ...)
	protected static WeaponSlotComponent FindWeaponSlot(notnull array<WeaponSlotComponent> weaponSlots, IEntity weapon)
	{
		BaseWeaponComponent weaponComponent = BaseWeaponComponent.Cast(weapon.FindComponent(BaseWeaponComponent));
		if (!weaponComponent)
			return null;

		string slotType = weaponComponent.GetWeaponSlotType();
		foreach (WeaponSlotComponent slot : weaponSlots)
		{
			if (slot && !slot.GetWeaponEntity() && slot.GetWeaponSlotType() == slotType)
				return slot;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Optics, suppressors, etc. - each goes in the first attachment slot that accepts it, replacing
	//! whatever the weapon prefab had there by default
	protected static void AttachAttachments(IEntity weapon, array<ResourceName> attachments)
	{
		if (!attachments || attachments.IsEmpty())
			return;

		BaseWeaponComponent weaponComponent = BaseWeaponComponent.Cast(weapon.FindComponent(BaseWeaponComponent));
		if (!weaponComponent)
			return;

		array<AttachmentSlotComponent> attachmentSlots = {};
		weaponComponent.GetAttachments(attachmentSlots);

		foreach (ResourceName attachmentPrefab : attachments)
		{
			IEntity attachment = SpawnLocal(attachmentPrefab, weapon.GetWorld());
			if (!attachment)
				continue;

			bool attached = false;
			foreach (AttachmentSlotComponent attachmentSlot : attachmentSlots)
			{
				if (!attachmentSlot || !attachmentSlot.CanSetAttachment(attachment))
					continue;

				IEntity previous = attachmentSlot.GetAttachedEntity();
				if (previous)
					SCR_EntityHelper.DeleteEntityAndChildren(previous);

				attachmentSlot.SetAttachment(attachment);
				attached = true;
				break;
			}

			if (!attached)
				SCR_EntityHelper.DeleteEntityAndChildren(attachment);
		}
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 HELPERS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected static ItemPreviewManagerEntity GetPreviewManager()
	{
		ChimeraWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		ItemPreviewManagerEntity previewManager = world.GetItemPreviewManager();
		if (previewManager)
			return previewManager;

		// Same fallback as SCR_LoadoutPreviewComponent: the world may not have spawned one yet
		Resource resource = Resource.Load(PREVIEW_MANAGER_PREFAB);
		if (resource && resource.IsValid())
			GetGame().SpawnEntityPrefabLocal(resource, world);

		return world.GetItemPreviewManager();
	}

	//------------------------------------------------------------------------------------------------
	//! Character framing (camera distance/angle) from the character's own inventory attributes, the
	//! same ones the inventory screen uses for its character render
	protected static PreviewRenderAttributes GetRenderAttributes(IEntity character)
	{
		SCR_CharacterInventoryStorageComponent storage = SCR_CharacterInventoryStorageComponent.Cast(character.FindComponent(SCR_CharacterInventoryStorageComponent));
		if (!storage)
			return null;

		ItemAttributeCollection attributes = storage.GetAttributes();
		if (!attributes)
			return null;

		return PreviewRenderAttributes.Cast(attributes.FindAttribute(SCR_CharacterInventoryPreviewAttributes));
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity SpawnLocal(ResourceName prefab, BaseWorld world)
	{
		if (prefab.IsEmpty() || !world)
			return null;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return null;

		return GetGame().SpawnEntityPrefabLocal(resource, world);
	}

	//------------------------------------------------------------------------------------------------
	//! Removes every inventory item attached to the character (clothing, weapons and what they
	//! hold) - from SCR_LoadoutPreviewComponent.DeleteChildrens
	protected static void DeleteAttachedItems(IEntity entity, bool deleteRoot)
	{
		if (!entity)
			return;

		IEntity child = entity.GetChildren();
		while (child)
		{
			IEntity sibling = child.GetSibling();
			if (child.FindComponent(InventoryItemComponent))
				DeleteAttachedItems(child, true);
			child = sibling;
		}

		if (deleteRoot && !entity.IsDeleted())
			delete entity;
	}
}
