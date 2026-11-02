//------------------------------------------------------------------------------------------------
// Reads weapon/magazine/attachment/storage facts straight from prefab containers, for the gear
// check and the admin Gear Fix tool. Nothing is spawned.
//
// Compatibility follows vanilla's own predicates (SCR_InventoryStorageManagerComponent.c):
//  - a magazine fits when its MagazineComponent well class inherits from a well class of one of
//    the weapon's muzzles (SCR_MagazinePredicate: iMagWell.IsInherited(magWellType));
//  - an attachment fits when its WeaponAttachmentAttributes type inherits from the slot's
//    AttachmentType (SCR_CompatibleAttachmentPredicate).
//
// Every Resource returned by Resource.Load is kept in a local variable while its containers are
// read - an inline FindEntitySource(Resource.Load(x)) frees the prefab mid-read and crashes.
// Results are cached per prefab for the session (prefabs do not change at runtime).
//------------------------------------------------------------------------------------------------
class COA_GearPrefabHelper
{
	protected static ref map<ResourceName, bool> s_mValid = new map<ResourceName, bool>();
	protected static ref map<ResourceName, string> s_mMagazineWell = new map<ResourceName, string>();
	protected static ref map<ResourceName, ref array<string>> s_mOwnWells = new map<ResourceName, ref array<string>>();
	protected static ref map<ResourceName, ref array<string>> s_mSlotTypes = new map<ResourceName, ref array<string>>();
	protected static ref map<ResourceName, string> s_mAttachmentType = new map<ResourceName, string>();
	protected static ref map<ResourceName, float> s_mVolume = new map<ResourceName, float>();
	protected static ref map<ResourceName, float> s_mWeight = new map<ResourceName, float>();
	protected static ref map<ResourceName, float> s_mStorageVolume = new map<ResourceName, float>();
	protected static ref map<ResourceName, int> s_mKind = new map<ResourceName, int>();

	static const int KIND_INVALID = -1;
	static const int KIND_ITEM = 0;
	static const int KIND_WEAPON = 1;
	static const int KIND_MAGAZINE = 2;
	static const int KIND_GADGET = 3;		// goes to an equipment (gadget) slot, not into storage
	static const int KIND_THROWABLE = 4;

	//------------------------------------------------------------------------------------------------
	//! The prefab exists (its mod is loaded) and is an entity prefab
	static bool IsValidPrefab(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return false;

		bool cached;
		if (s_mValid.Find(prefab, cached))
			return cached;

		bool valid = false;
		Resource resource = Resource.Load(prefab);
		if (resource != null && resource.IsValid())
		{
			IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
			valid = entitySource != null;
		}

		s_mValid.Set(prefab, valid);
		return valid;
	}

	//------------------------------------------------------------------------------------------------
	//! KIND_* classification from the prefab's components
	static int GetKind(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return KIND_INVALID;

		int cached;
		if (s_mKind.Find(prefab, cached))
			return cached;

		int kind = KIND_INVALID;
		Resource resource = Resource.Load(prefab);
		if (resource != null && resource.IsValid())
		{
			IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
			if (entitySource)
			{
				kind = KIND_ITEM;
				if (COA_InventoryHelper.IsThrowableFromSource(entitySource))
					kind = KIND_THROWABLE;
				else if (SCR_BaseContainerTools.FindComponentSource(entitySource, BaseWeaponComponent))
					kind = KIND_WEAPON;
				else if (SCR_BaseContainerTools.FindComponentSource(entitySource, BaseMagazineComponent))
					kind = KIND_MAGAZINE;
				else if (IsGadgetSource(entitySource))
					kind = KIND_GADGET;
			}
		}

		s_mKind.Set(prefab, kind);
		return kind;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsWeapon(ResourceName prefab)
	{
		return GetKind(prefab) == KIND_WEAPON;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsMagazine(ResourceName prefab)
	{
		return GetKind(prefab) == KIND_MAGAZINE;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsRadio(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return false;

		Resource resource = Resource.Load(prefab);
		if (resource == null || !resource.IsValid())
			return false;

		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
		if (!entitySource)
			return false;

		return SCR_BaseContainerTools.FindComponentSource(entitySource, BaseRadioComponent) != null
			|| SCR_BaseContainerTools.FindComponentSource(entitySource, CVON_RadioComponent) != null;
	}

	//------------------------------------------------------------------------------------------------
	//! Binoculars, compass, map, watch, flashlight: AddInventoryItem puts these in equipment slots first
	protected static bool IsGadgetSource(IEntitySource entitySource)
	{
		array<string> gadgetComponents = {"SCR_BinocularsComponent", "SCR_CompassComponent", "SCR_MapGadgetComponent", "SCR_FlashlightComponent", "SCR_WristwatchComponent"};
		foreach (string componentName : gadgetComponents)
		{
			if (SCR_BaseContainerTools.FindComponentSource(entitySource, componentName))
				return true;
		}

		return false;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 MAGAZINES
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Well class name of a magazine prefab, empty if it has no MagazineComponent/well
	static string GetMagazineWell(ResourceName magazine)
	{
		if (magazine.IsEmpty())
			return string.Empty;

		string cached;
		if (s_mMagazineWell.Find(magazine, cached))
			return cached;

		string wellName;
		Resource resource = Resource.Load(magazine);
		if (resource != null && resource.IsValid())
		{
			IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
			IEntityComponentSource magazineSource = SCR_BaseContainerTools.FindComponentSource(entitySource, BaseMagazineComponent);
			if (magazineSource)
			{
				BaseContainer wellContainer = magazineSource.GetObject("MagazineWell");
				if (wellContainer)
					wellName = wellContainer.GetClassName();
			}
		}

		s_mMagazineWell.Set(magazine, wellName);
		return wellName;
	}

	//------------------------------------------------------------------------------------------------
	//! Well class names of the weapon's own muzzles (MuzzleComponent, MuzzleInMagComponent...)
	static void GetOwnMagazineWells(ResourceName weapon, notnull array<string> wells)
	{
		if (weapon.IsEmpty())
			return;

		array<string> cached = s_mOwnWells.Get(weapon);
		if (!cached)
		{
			cached = {};
			Resource resource = Resource.Load(weapon);
			if (resource != null && resource.IsValid())
			{
				IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
				array<IEntityComponentSource> muzzles = {};
				SCR_BaseContainerTools.FindComponentSourcesOfClass(entitySource, BaseMuzzleComponent, true, muzzles);
				foreach (IEntityComponentSource muzzle : muzzles)
				{
					BaseContainer wellContainer = muzzle.GetObject("MagazineWell");
					if (wellContainer != null && !cached.Contains(wellContainer.GetClassName()))
						cached.Insert(wellContainer.GetClassName());
				}
			}

			s_mOwnWells.Set(weapon, cached);
		}

		foreach (string well : cached)
		{
			if (!wells.Contains(well))
				wells.Insert(well);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Every well the weapon accepts: its own muzzles, its default attachments' (an UGL is a separate
	//! prefab on an attachment slot) and those of extra attachments the gearscript adds
	static void GetAllMagazineWells(ResourceName weapon, notnull array<string> wells, array<ResourceName> extraAttachments = null)
	{
		GetOwnMagazineWells(weapon, wells);

		array<ResourceName> defaults = {};
		GetDefaultAttachments(weapon, defaults);
		foreach (ResourceName attachment : defaults)
			GetOwnMagazineWells(attachment, wells);

		if (!extraAttachments)
			return;

		foreach (ResourceName attachment : extraAttachments)
			GetOwnMagazineWells(attachment, wells);
	}

	//------------------------------------------------------------------------------------------------
	//! Whether a magazine loads something other than the weapon's main muzzle (the first on its prefab):
	//! the 40mm rounds of an UGL, whose muzzle is on an attachment. False when there's no well data.
	static bool IsLauncherMagazine(ResourceName weapon, ResourceName magazine)
	{
		array<string> wells = {};
		GetOwnMagazineWells(weapon, wells);
		string magazineWell = GetMagazineWell(magazine);
		if (wells.IsEmpty() || magazineWell.IsEmpty())
			return false;

		return !IsClassInherited(magazineWell, wells[0]);
	}

	//------------------------------------------------------------------------------------------------
	//! \return 1 fits, 0 does not fit, -1 unknown (no well data on one side)
	static int MagazineFits(ResourceName weapon, ResourceName magazine, array<ResourceName> extraAttachments = null)
	{
		string magazineWell = GetMagazineWell(magazine);
		if (magazineWell.IsEmpty())
			return -1;

		array<string> weaponWells = {};
		GetAllMagazineWells(weapon, weaponWells, extraAttachments);
		if (weaponWells.IsEmpty())
			return -1;

		foreach (string weaponWell : weaponWells)
		{
			if (IsClassInherited(magazineWell, weaponWell))
				return 1;
		}

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	//! The magazine the weapon's first muzzle comes loaded with (MuzzleComponent.MagazineTemplate)
	static ResourceName GetDefaultMagazine(ResourceName weapon)
	{
		ResourceName magazine;
		if (weapon.IsEmpty())
			return magazine;

		Resource resource = Resource.Load(weapon);
		if (resource == null || !resource.IsValid())
			return magazine;

		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
		array<IEntityComponentSource> muzzles = {};
		SCR_BaseContainerTools.FindComponentSourcesOfClass(entitySource, BaseMuzzleComponent, true, muzzles);
		foreach (IEntityComponentSource muzzle : muzzles)
		{
			if (muzzle.Get("MagazineTemplate", magazine) && !magazine.IsEmpty())
				return magazine;
		}

		return magazine;
	}

	//------------------------------------------------------------------------------------------------
	//! Disposable launchers (M72, RPG-26...) fire from a MuzzleInMagComponent and are not reloaded
	static bool HasMuzzleInMag(ResourceName weapon)
	{
		if (weapon.IsEmpty())
			return false;

		Resource resource = Resource.Load(weapon);
		if (resource == null || !resource.IsValid())
			return false;

		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
		array<IEntityComponentSource> muzzles = {};
		SCR_BaseContainerTools.FindComponentSourcesOfClass(entitySource, BaseMuzzleComponent, true, muzzles);
		foreach (IEntityComponentSource muzzle : muzzles)
		{
			if (IsClassInherited(muzzle.GetClassName(), "MuzzleInMagComponent"))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Rounds per magazine (MagazineComponent.MaxAmmo), 0 if unknown
	static int GetMagazineCapacity(ResourceName magazine)
	{
		if (magazine.IsEmpty())
			return 0;

		Resource resource = Resource.Load(magazine);
		if (resource == null || !resource.IsValid())
			return 0;

		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
		IEntityComponentSource magazineSource = SCR_BaseContainerTools.FindComponentSource(entitySource, BaseMagazineComponent);
		if (!magazineSource)
			return 0;

		int maxAmmo;
		magazineSource.Get("MaxAmmo", maxAmmo);
		return maxAmmo;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 ATTACHMENTS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Prefabs sitting on the weapon's attachment slots by default (UGL, handguard with rails...)
	static void GetDefaultAttachments(ResourceName weapon, notnull array<ResourceName> attachments)
	{
		if (weapon.IsEmpty())
			return;

		Resource resource = Resource.Load(weapon);
		if (resource == null || !resource.IsValid())
			return;

		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
		array<IEntityComponentSource> slots = {};
		SCR_BaseContainerTools.FindComponentSourcesOfClass(entitySource, AttachmentSlotComponent, true, slots);
		foreach (IEntityComponentSource slot : slots)
		{
			BaseContainer slotInfo = slot.GetObject("AttachmentSlot");
			if (!slotInfo)
				continue;

			ResourceName attachment;
			if (slotInfo.Get("Prefab", attachment) && !attachment.IsEmpty() && !attachments.Contains(attachment))
				attachments.Insert(attachment);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Attachment slot type class names of the weapon, including slots on its default attachments
	//! (rails on a handguard)
	static void GetAttachmentSlotTypes(ResourceName weapon, notnull array<string> slotTypes)
	{
		if (weapon.IsEmpty())
			return;

		array<string> cached = s_mSlotTypes.Get(weapon);
		if (!cached)
		{
			cached = {};
			ReadSlotTypes(weapon, cached);

			array<ResourceName> defaults = {};
			GetDefaultAttachments(weapon, defaults);
			foreach (ResourceName attachment : defaults)
				ReadSlotTypes(attachment, cached);

			s_mSlotTypes.Set(weapon, cached);
		}

		foreach (string slotType : cached)
		{
			if (!slotTypes.Contains(slotType))
				slotTypes.Insert(slotType);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void ReadSlotTypes(ResourceName prefab, notnull array<string> slotTypes)
	{
		Resource resource = Resource.Load(prefab);
		if (resource == null || !resource.IsValid())
			return;

		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
		array<IEntityComponentSource> slots = {};
		SCR_BaseContainerTools.FindComponentSourcesOfClass(entitySource, AttachmentSlotComponent, true, slots);
		foreach (IEntityComponentSource slot : slots)
		{
			BaseContainer typeContainer = slot.GetObject("AttachmentType");
			if (typeContainer != null && !slotTypes.Contains(typeContainer.GetClassName()))
				slotTypes.Insert(typeContainer.GetClassName());
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Attachment type class name from the item's WeaponAttachmentAttributes, empty if none
	static string GetAttachmentType(ResourceName attachment)
	{
		if (attachment.IsEmpty())
			return string.Empty;

		string cached;
		if (s_mAttachmentType.Find(attachment, cached))
			return cached;

		string typeName;
		Resource resource = Resource.Load(attachment);
		if (resource != null && resource.IsValid())
		{
			IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
			BaseContainer attributes = GetItemAttributes(entitySource);
			if (attributes)
			{
				BaseContainerList customAttributes = attributes.GetObjectArray("CustomAttributes");
				if (customAttributes)
				{
					for (int i = 0; i < customAttributes.Count(); i++)
					{
						BaseContainer customAttribute = customAttributes.Get(i);
						if (!customAttribute || !IsClassInherited(customAttribute.GetClassName(), "AttachmentAttributes"))
							continue;

						BaseContainer typeContainer = customAttribute.GetObject("AttachmentType");
						if (typeContainer)
						{
							typeName = typeContainer.GetClassName();
							break;
						}
					}
				}
			}
		}

		s_mAttachmentType.Set(attachment, typeName);
		return typeName;
	}

	//------------------------------------------------------------------------------------------------
	//! \return 1 fits a slot, 0 fits none, -1 unknown (no type data on one side)
	static int AttachmentFits(ResourceName weapon, ResourceName attachment)
	{
		string attachmentType = GetAttachmentType(attachment);
		if (attachmentType.IsEmpty())
			return -1;

		array<string> slotTypes = {};
		GetAttachmentSlotTypes(weapon, slotTypes);
		if (slotTypes.IsEmpty())
			return -1;

		foreach (string slotType : slotTypes)
		{
			if (IsClassInherited(attachmentType, slotType))
				return 1;
		}

		return 0;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 VOLUME / WEIGHT
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Item volume (ItemPhysAttributes.ItemVolume), 0 if not set on the prefab (engine-computed)
	static float GetItemVolume(ResourceName prefab)
	{
		float cached;
		if (s_mVolume.Find(prefab, cached))
			return cached;

		float volume = ReadPhysAttribute(prefab, "ItemVolume");
		s_mVolume.Set(prefab, volume);
		return volume;
	}

	//------------------------------------------------------------------------------------------------
	//! Item weight in kg (ItemPhysAttributes.Weight), 0 if unknown
	static float GetItemWeight(ResourceName prefab)
	{
		float cached;
		if (s_mWeight.Find(prefab, cached))
			return cached;

		float weight = ReadPhysAttribute(prefab, "Weight");
		s_mWeight.Set(prefab, weight);
		return weight;
	}

	//------------------------------------------------------------------------------------------------
	protected static float ReadPhysAttribute(ResourceName prefab, string attributeName)
	{
		if (prefab.IsEmpty())
			return 0;

		Resource resource = Resource.Load(prefab);
		if (resource == null || !resource.IsValid())
			return 0;

		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
		BaseContainer attributes = GetItemAttributes(entitySource);
		if (!attributes)
			return 0;

		BaseContainer physAttributes = attributes.GetObject("ItemPhysAttributes");
		if (!physAttributes)
			return 0;

		float value;
		physAttributes.Get(attributeName, value);
		return value;
	}

	//------------------------------------------------------------------------------------------------
	//! Total MaxCumulativeVolume of the general-purpose storages on a clothing prefab (its own and
	//! nested pouch/node storages). Gadget/equipment slots are not counted.
	//! \return the volume, 0 if the prefab has no storage, -1 if it has storage of unknown size
	//! (capacity scaled by a coefficient)
	static float GetStorageVolume(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return 0;

		float cached;
		if (s_mStorageVolume.Find(prefab, cached))
			return cached;

		float total;
		bool unknown;
		Resource resource = Resource.Load(prefab);
		if (resource != null && resource.IsValid())
		{
			IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
			array<IEntityComponentSource> storages = {};
			SCR_BaseContainerTools.FindComponentSourcesOfClass(entitySource, BaseInventoryStorageComponent, true, storages);
			foreach (IEntityComponentSource storage : storages)
			{
				string className = storage.GetClassName();
				if (IsClassInherited(className, "EquipmentStorageComponent") || IsClassInherited(className, "WeaponAttachmentsStorageComponent"))
					continue;

				bool useCoefficient;
				storage.Get("UseCapacityCoefficient", useCoefficient);
				if (useCoefficient)
				{
					unknown = true;
					continue;
				}

				float volume;
				if (storage.Get("MaxCumulativeVolume", volume) && volume > 0)
					total += volume;
			}
		}

		if (total <= 0 && unknown)
			total = -1;

		s_mStorageVolume.Set(prefab, total);
		return total;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 HELPERS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! "Attributes" container of the prefab's first InventoryItemComponent (storages are item components too)
	protected static BaseContainer GetItemAttributes(IEntitySource entitySource)
	{
		if (!entitySource)
			return null;

		IEntityComponentSource itemSource = SCR_ComponentHelper.GetInventoryItemComponentSource(entitySource);
		if (!itemSource)
			return null;

		return itemSource.GetObject("Attributes");
	}

	//------------------------------------------------------------------------------------------------
	//! Class inheritance by name; falls back to name equality for classes script can't resolve
	static bool IsClassInherited(string className, string parentName)
	{
		if (className.IsEmpty() || parentName.IsEmpty())
			return false;

		if (className == parentName)
			return true;

		typename classType = className.ToType();
		typename parentType = parentName.ToType();
		if (!classType || !parentType)
			return false;

		return classType.IsInherited(parentType);
	}
}
