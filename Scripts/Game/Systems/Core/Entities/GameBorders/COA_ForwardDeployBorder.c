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
		m_iRegisterAttempts = 0;
		RegisterWithManager();
	}

	//------------------------------------------------------------------------------------------------
	//! The manager lives on the lobby entity, which may initialise after this border - without a
	//! retry the zone would silently never count and leaders would get no forward deploy option.
	protected static const int REGISTER_RETRY_MS = 200;
	protected static const int REGISTER_MAX_ATTEMPTS = 50;
	protected int m_iRegisterAttempts;

	protected void RegisterWithManager()
	{
		COA_ForwardDeployManager forwardDeployManager = COA_ForwardDeployManager.GetInstance();
		if (forwardDeployManager)
		{
			forwardDeployManager.AddForwardDeployZone(this);
			return;
		}

		m_iRegisterAttempts++;
		if (m_iRegisterAttempts < REGISTER_MAX_ATTEMPTS)
			GetGame().GetCallqueue().CallLater(RegisterWithManager, REGISTER_RETRY_MS, false);
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_ForwardDeployBorder()
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(RegisterWithManager);
	}
}