class COA_LoggingHelper
{
	//------------------------------------------------------------------------------------------------
	//! Log item error
	//! \param[in] item Item that failed to insert
	//! \param[in] entity Entity that the item was being added to
	//! \param[in] itemType type of item to display (default is "ITEM")
	static void LogItemError(IEntity item, IEntity entity, string itemType = "ITEM")
	{
		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		
		if (facComp)
		{
			COA_EGearRole role = COA_GearscriptCharacter.Cast(entity).GetGearRole();
			
			string error = string.Format("[%3 %4 GEARSCRIPT ERROR] \n\n UNABLE TO INSERT %1 %2 \n NOT ENOUGH SPACE IN ENTITY/INVALID %1!", itemType, SanitizeResourceName(item.GetPrefabData().GetPrefabName()), facComp.GetAffiliatedFaction().GetFactionKey(), SCR_Enum.GetEnumName(COA_EGearRole, role));
			Debug.Error(error);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	//! Alt Log item error
	//! \param[in] itemResource ResourceName of the item that failed to insert
	//! \param[in] entity Entity that the item was being added to
	//! \param[in] itemType type of item to display (default is "ITEM")
	static void LogItemError(ResourceName itemResource, IEntity entity, string itemType = "ITEM")
	{	
		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		
		if (facComp)
		{
			COA_EGearRole role = COA_GearscriptCharacter.Cast(entity).GetGearRole();
			
			string error = string.Format("[%3 %4 GEARSCRIPT ERROR] \n\n UNABLE TO INSERT %1 %2 \n NOT ENOUGH SPACE IN ENTITY/INVALID %1!", itemType, SanitizeResourceName(itemResource), facComp.GetAffiliatedFaction().GetFactionKey(), SCR_Enum.GetEnumName(COA_EGearRole, role));
			Debug.Error(error);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	//! Log Weight Error
	//! \param[in] entity Entity that is considered overweight
	//! \param[in] kg The actual weight of the entity
	static void LogWeightError(IEntity entity, int kg)
	{	
		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		
		if (facComp)
		{
			COA_EGearRole role = COA_GearscriptCharacter.Cast(entity).GetGearRole();
			int dif = kg - 50;
			
			string error = string.Format("[%2 %3 GEARSCRIPT ERROR] \n\n %1kg OVERWEIGHT OF TARGET WEIGHT (50kg) \n PLEASE REDUCE THE AMMOUNT OF GEAR ON THIS ENTITY TO GET IT BELLOW THE TARGET WEIGHT", dif, facComp.GetAffiliatedFaction().GetFactionKey(), SCR_Enum.GetEnumName(COA_EGearRole, role));
			Debug.Error(error);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	//! Sanitize Resource Name of its path and extension
	//! \param[in] resName Resource name to sanitize and strip (kinky)
	//! \return sanatized string in uppercase
	static string SanitizeResourceName(ResourceName resName)
	{
		resName = FilePath.StripPath(resName);
		resName = FilePath.StripExtension(resName);
		resName.ToUpper();

		return resName;
	}
}
