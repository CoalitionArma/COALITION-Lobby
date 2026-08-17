class COA_ForwardDeployBorderClass : COA_GameBorderClass
{
}

class COA_ForwardDeployBorder : COA_GameBorder
{
//=============================================================================================================================================================================================================================================================================================================================================================
//	 OVERRIDES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		//Clients track this too so we don't have to ask the server to see if theres any active forward deploy zones
		//Needed for checking if we need to add the action in the map
		if (COA_ForwardDeployManager.GetInstance())
			COA_ForwardDeployManager.GetInstance().AddForwardDeployZone(owner);
	}
}