class COA_BorderCheckSystem : GameSystem
{		
	protected const float UPDATE_TIME_SECONDS = 1;
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================

	protected COA_GameBorderHUD m_GameBorderHUD;
	protected SCR_PlayerController m_PlayerController;
	protected ref array<COA_GameBorder> m_aBorders = new array<COA_GameBorder>;
	protected ref map<COA_GameBorder, bool> m_mBordersActive = new map<COA_GameBorder, bool>;
	protected float m_fUpdate;
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 CONSTRUCTOR/DESTRUCTOR
//=============================================================================================================================================================================================================================================================================================================================================================
	
	void COA_BorderCheckSystem()
	{

	}
	
	void ~COA_BorderCheckSystem()
	{
		
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 UPDATE METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	override void OnUpdatePoint(WorldUpdatePointArgs args)
	{	
		m_fUpdate = m_fUpdate + args.GetTimeSliceSeconds();
		if (!(m_fUpdate >= UPDATE_TIME_SECONDS))
			return;
		else
			m_fUpdate = 0;
		
		if (!m_GameBorderHUD || !m_PlayerController || !m_PlayerController.GetLocalMainEntity())
		{
			m_PlayerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			return;
		}
		
		SCR_ChimeraCharacter player = SCR_ChimeraCharacter.Cast(m_PlayerController.GetLocalMainEntity());
		if (!player)
			return;

		foreach (COA_GameBorder border : m_aBorders)
		{
			if (!border || border.m_aVisibleForFactions.IsEmpty())
				continue;
			
			if (player.m_pFactionComponent)
			{
				FactionKey factionKey = player.m_pFactionComponent.GetAffiliatedFactionKey();
				if ((!factionKey.IsEmpty() && !border.m_aVisibleForFactions.Contains(factionKey)))
				{
					ForceKillBorderCheck(border);
					continue;
				} else if (COA_EntityHelper.IsSpectator(player)) { // We still want spectators to see borders they're supposed to see (game border), just not be affected by them.
					ForceKillBorderCheck(border, true);
					continue;
				};
			
				bool isInsidePolygon = border.IsInsidePolygon(player.GetOrigin());
				border.UpdateAreaMesh(true);
				
				if (isInsidePolygon && !m_mBordersActive.Get(border))
					PlayerEnteredBorder(border, player);
				else if (!isInsidePolygon && m_mBordersActive.Get(border) && ShouldPlayerBeAffected(player, border))
					PlayerLeftBorder(border, player);
			};
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool ShouldPlayerBeAffected(SCR_ChimeraCharacter player, COA_GameBorder border)
	{	
	#ifdef WORKBENCH
		if (COA_EntityHelper.IsSpectator(player))
			return false;
	#else
		// Allow Admins to teleport out of the game borders during safestart
		if (COA_EntityHelper.IsSpectator(player) || (COA_SafestartManager.GetInstance().GetSafestartStatus() && SCR_Global.IsAdmin(GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(player))))
			return false;
	#endif
		
		CompartmentAccessComponent compAccess = CompartmentAccessComponent.Cast(player.FindComponent(CompartmentAccessComponent));
		if (compAccess)
		{
			BaseCompartmentSlot compartment = compAccess.GetCompartment();
			if (compartment)
			{
				VehicleHelicopterSimulation heli = VehicleHelicopterSimulation.Cast(compartment.GetVehicle().FindComponent(VehicleHelicopterSimulation));
				VehicleFixedWingSimulation plane = VehicleFixedWingSimulation.Cast(compartment.GetVehicle().FindComponent(VehicleFixedWingSimulation));
				VehicleWheeledSimulation wheeled = VehicleWheeledSimulation.Cast(compartment.GetVehicle().FindComponent(VehicleWheeledSimulation));
				VehicleTrackedSimulation trakced = VehicleTrackedSimulation.Cast(compartment.GetVehicle().FindComponent(VehicleTrackedSimulation));
				
				if (((heli || plane) && !border.m_bAirVehiclesRestricted) || ((wheeled || trakced) && !border.m_bGroundVehiclesRestricted))
					return false;
			};
		};
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void PlayerLeftBorder(COA_GameBorder border, IEntity player)
	{
		m_mBordersActive.Set(border, false);
		
		if (!m_GameBorderHUD.m_bActive)
			m_GameBorderHUD.ShowEffect(border);
	};
	
	//------------------------------------------------------------------------------------------------
	protected void PlayerEnteredBorder(COA_GameBorder border, IEntity player)
	{
		m_mBordersActive.Set(border, true);	
		
		foreach (COA_GameBorder checkBorder : m_aBorders)
			if (checkBorder != border && (COA_SafestartBorder.Cast(checkBorder) || COA_ForwardDeployBorder.Cast(checkBorder))) // Allows players to teleport/move between safestart and forward deploy zones
				m_mBordersActive.Set(checkBorder, false);
		
		ForceStopTimer();
	};
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 DEATH TIMER STOP METHODS (for external use)
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void ForceKillBorderCheck(COA_GameBorder border, bool borderMeshVisible = false)
	{
		border.UpdateAreaMesh(borderMeshVisible);
		
		if (m_mBordersActive.Get(border))
		{
			m_mBordersActive.Set(border, false);
			ForceStopTimer();
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void ForceStopTimer()
	{
		if (m_GameBorderHUD && m_GameBorderHUD.m_bActive)
			m_GameBorderHUD.HideEffect();
	}
	
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
		if (m_mBordersActive.Contains(border))
		{
			ForceStopTimer();
			m_mBordersActive.Remove(border);
		}
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
	static COA_BorderCheckSystem GetInstance()
	{
		World world = GetGame().GetWorld();
		if (!world)
			return null;
		return COA_BorderCheckSystem.Cast(world.FindSystem(COA_BorderCheckSystem));
	}
}