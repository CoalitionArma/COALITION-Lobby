//------------------------------------------------------------------------------------------------
class COA_DestroyRallypointAction : ScriptedUserAction
{	
	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		if (!GetGame().InPlayMode()) return;
	}
	
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!pOwnerEntity || !pUserEntity)
			return;

		RplId entityRplId;

		COA_ReplicationHelper.GetRplId(pOwnerEntity, entityRplId);

		COA_PlayerRplToAuthorityManager.GetInstance().RequestDestroyRallypoint(entityRplId);
	}
	
	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		return true;
	}	
	
	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		return true;
	}	
	
	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
		return false;
	}
};