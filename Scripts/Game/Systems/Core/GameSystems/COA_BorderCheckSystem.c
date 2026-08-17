class COA_BorderCheckSystem : GameSystem
{		
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================

	bool m_bPlayerHasEffect = false;
	COA_GameBorderHUD m_GameBorderHUD;
	COA_GameBorder m_PlayerOutsideBorder;
	SCR_PlayerController m_PlayerController;
	ref array<COA_GameBorder> m_aBorders = new array<COA_GameBorder>;

//=============================================================================================================================================================================================================================================================================================================================================================
//	 UPDATE METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	protected int m_iUpdate;
	//------------------------------------------------------------------------------------------------
	override void OnUpdatePoint(WorldUpdatePointArgs args)
	{	
		m_iUpdate++;
		
		// only update every 45 frames
		if (!(m_iUpdate >= 45))
			return;
		else
			m_iUpdate = 0;
		
		if (!m_PlayerController)
			m_PlayerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		
		SCR_ChimeraCharacter player = SCR_ChimeraCharacter.Cast(m_PlayerController.GetLocalMainEntity());
		if (!player)
			return;
		
		bool playerOutsideBorder = false;
		COA_GameBorder outsideBorder = null;
		foreach (COA_GameBorder border : m_aBorders)
		{
			if (!border || !border.m_aGameBorderSettings)
				continue;
			
			if (!border.m_aVisibleForFactions.IsEmpty() && player.m_pFactionComponent)
			{
				FactionKey factionKey = player.m_pFactionComponent.GetAffiliatedFactionKey();
				if (!factionKey.IsEmpty() && !border.m_aVisibleForFactions.Contains(factionKey))
				{
					border.UpdateAreaMesh(false);
					continue;
				} else {
					border.UpdateAreaMesh(true);
				};
			}
			
			if (!border.IsInsidePolygon(player.GetOrigin()))
			{
				playerOutsideBorder = true;
				outsideBorder = border;
				break;
			};
		}
		
		if (!COA_EntityHelper.IsSpectator(player) && playerOutsideBorder)
		{
			PlayerLeftBorder(player, outsideBorder);
		} else if (m_PlayerOutsideBorder && m_bPlayerHasEffect) {
			m_GameBorderHUD.HideEffect(m_PlayerOutsideBorder.m_aGameBorderSettings.GetEffectContainer().m_iId);
			m_PlayerOutsideBorder = null;
			m_bPlayerHasEffect = false;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void PlayerLeftBorder(SCR_ChimeraCharacter player, COA_GameBorder border)
	{
		if (m_bPlayerHasEffect)
			return;
		
		// Allow Admins to teleport out of the game borders during safestart
		//if (COA_SafestartManager.GetInstance().GetSafestartStatus() && SCR_Global.IsAdmin(GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(player)))
		//	return;
		
		CompartmentAccessComponent compAccess = CompartmentAccessComponent.Cast(player.FindComponent(CompartmentAccessComponent)); // TODO nullcheck
		if (compAccess)
		{
			BaseCompartmentSlot compartment = compAccess.GetCompartment();
			if (compartment)
			{
				VehicleHelicopterSimulation heli = VehicleHelicopterSimulation.Cast(compartment.GetVehicle().FindComponent(VehicleHelicopterSimulation));
				
				if(heli && !border.m_bHeliRestricted)
					return;
			}
		}
		
		m_bPlayerHasEffect = true;
		m_PlayerOutsideBorder = border;
		m_GameBorderHUD.ShowEffect(border.m_aGameBorderSettings.GetEffectContainer());
	};
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 REGISTRATION METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	void RegisterHUD(COA_GameBorderHUD hud)
	{
		m_GameBorderHUD = hud;
	}
	
	//------------------------------------------------------------------------------------------------
	void RegisterBorder(COA_GameBorder border)
	{
		m_aBorders.Insert(border);
	}
	
	//------------------------------------------------------------------------------------------------
	void UnRegisterBorder(COA_GameBorder border)
	{
		m_aBorders.RemoveItem(border);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSOR/SYSTEM INFO
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	override static void InitInfo(WorldSystemInfo outInfo)
	{
		super.InitInfo(outInfo);
		outInfo
			.SetAbstract(false)
			.SetUnique(true)
			.SetLocation(WorldSystemLocation.Client)
			.AddPoint(WorldSystemPoint.FixedFrame);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Provides instance of the system
	static COA_BorderCheckSystem GetInstance()
	{
		World world = GetGame().GetWorld();
		if (!world)
			return null;
		return COA_BorderCheckSystem.Cast(world.FindSystem(COA_BorderCheckSystem));
	}
}