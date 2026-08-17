class COA_BorderBaseClass : PolylineShapeEntityClass
{
}

class COA_BorderBase : PolylineShapeEntity
{
	// ------------------------------------------------------ GENERAL SETTINGS ------------------------------------------------------
	[Attribute("", category: "Game Border - General")]
	ref array<FactionKey> m_aVisibleForFactions;
	
	// ------------------------------------------------------ MAP SETTINGS ----------------------------------------------------------
	[Attribute("{B8793707B56B2F9F}UI/Map/PolyMapMarkerBase.layout", params: "layout", category: "Game Border - Map")]
	protected ResourceName m_sPolyMarkerLayout;
	
	[Attribute("{E362BE45DB490A07}UI/data/Zone.edds", UIWidgets.ResourcePickerThumbnail, desc: "", params: "edds", category: "Game Border - Map")]
	protected ResourceName m_mPolygonTexture;
	
	[Attribute("1 1 1 1", UIWidgets.ColorPicker, desc: "", category: "Game Border - Map")]
	protected ref Color m_cPolygonColor;
	
	[Attribute("0.01", UIWidgets.Slider, desc: "", params: "0.001 4 0.01", category: "Game Border - Map")]
	protected float m_fPolygonUVScale;
	
	[Attribute("{8D8EB58699FBC40B}UI/data/ZoneBorder.edds", UIWidgets.ResourcePickerThumbnail, desc: "", params: "edds", category: "Game Border - Map")]
	protected ResourceName m_mPolygonTextureBorder;
	
	[Attribute("1 1 1 1", UIWidgets.ColorPicker, desc: "", category: "Game Border - Map")]
	protected ref Color m_cPolygonBorderColor;
	
	[Attribute("0.1", UIWidgets.Slider, desc: "", params: "0.001 40 0.01", category: "Game Border - Map")]
	protected float m_fPolygonBorderUVScale;
	
	[Attribute("15", UIWidgets.Slider, desc: "", params: "1 100 0.1", category: "Game Border - Map")]
	protected float m_fPolygonBorderWidth;
		
	[Attribute("0", category: "Game Border - Map")]
	protected bool m_bLineMode;
	
	[Attribute("0", category: "Game Border - Map")]
	protected bool m_bReversed;
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	protected ref SharedItemRef m_TextureSharedItem;
	protected ref SharedItemRef m_TextureBorderSharedItem;
	protected SCR_MapEntity m_MapEntity;
	protected ref array<float> m_aPolygon;
	
	CanvasWidget m_wCanvasWidget;
	protected ref PolygonDrawCommand m_DrawPolygon = new PolygonDrawCommand();
	protected ref LineDrawCommand m_LinePolygon = new LineDrawCommand();
	protected ref array<ref CanvasWidgetCommand> m_MapDrawCommands = { m_DrawPolygon, m_LinePolygon };
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 OVERRIDES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		m_MapEntity = SCR_MapEntity.GetMapInstance();
		ScriptInvokerBase<MapConfigurationInvoker> onMapOpen = m_MapEntity.GetOnMapOpen();
		ScriptInvokerBase<MapConfigurationInvoker> onMapClose = m_MapEntity.GetOnMapClose();
		
		onMapOpen.Insert(CreateMapWidget);
		onMapClose.Insert(DeleteMapWidget);
		
		GetGame().GetCallqueue().Call(UpdatePolygon);
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnPostFrame(IEntity owner, float timeSlice)
	{
		m_DrawPolygon.m_Vertices = new array<float>();
		m_LinePolygon.m_Vertices = new array<float>();
		float screenXold, screenYold;
		for (int i = 0; i < m_aPolygon.Count(); i += 2)
		{
			float screenX, screenY;
			m_MapEntity.WorldToScreen(m_aPolygon[i], m_aPolygon[i+1], screenX, screenY, true);
			if ((Math.AbsFloat(screenXold - screenX) + Math.AbsFloat(screenYold - screenY)) < 2.1)
			{
				continue;
			}
			if (m_bReversed && (i == 0 || i == 2))
			{
				screenX += 0.1;
			}
			screenXold = screenX;
			screenYold = screenY;
						
			m_DrawPolygon.m_Vertices.Insert(screenX);
			m_DrawPolygon.m_Vertices.Insert(screenY);
			if (!m_bReversed || i > 10)
			{
				m_LinePolygon.m_Vertices.Insert(screenX);
				m_LinePolygon.m_Vertices.Insert(screenY);
			}
		}
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 MAP MARKERS/POLYGONS UPDATERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected void UpdatePolygon()
	{	
		array<vector> outPoints = new array<vector>();
		m_aPolygon = new array<float>();
		
		outPoints = GetBorder3DPolygon(outPoints);
		
		if (m_bReversed)
		{
			// Use the map tile's exact bounds (always 0,0 → sizeX, sizeZ in world space)
			// instead of GetBoundBox(), which returns entity AABB and can misalign with
			// the visible map corners when objects are placed near or outside terrain edges.
			const float minX = 0;
			const float minZ = 0;
			float maxX = m_MapEntity.GetMapSizeX();
			float maxZ = m_MapEntity.GetMapSizeY(); // map Y axis == world Z axis (top-down)
			outPoints.InsertAt(Vector(minX, 0, minZ), 0);
			outPoints.InsertAt(Vector(minX, 0, maxZ), 0);
			outPoints.InsertAt(Vector(maxX, 0, maxZ), 0);
			outPoints.InsertAt(Vector(maxX, 0, minZ), 0);
			outPoints.InsertAt(Vector(minX, 0, minZ), 0);
			outPoints.InsertAt(outPoints[5], 0);
		}
			
		SCR_Math2D.Get2DPolygon(outPoints, m_aPolygon);
	}
	
	//------------------------------------------------------------------------------------------------
	protected array<vector> GetBorder3DPolygon(array<vector> outPoints)
	{	
		this.GetPointsPositions(outPoints);
		vector origin = this.GetOrigin();
		
		for (int i = 0; i < outPoints.Count(); i++)
		{
			outPoints[i] = outPoints[i] + origin;
		}
		for (int i = 0; i < outPoints.Count() - 1; i++)
		{
			if ((Math.AbsFloat(outPoints[i][0] - outPoints[i+1][0]) + Math.AbsFloat(outPoints[i][1] - outPoints[i+1][1])) < 0.1)
			{
				outPoints.RemoveOrdered(i);
				i--;
			}
		}
		
		return outPoints;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void CreateMapWidget(MapConfiguration mapConfig)
	{
		if (m_bLineMode)
			m_MapDrawCommands = { m_LinePolygon };
		else
			m_MapDrawCommands = { m_DrawPolygon, m_LinePolygon };
		
		if (!IsCurrentVisibility())
			return;
		
		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
		
		// Get map frame
		Widget mapFrame = m_MapEntity.GetMapMenuRoot().FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
		if (!mapFrame) mapFrame = m_MapEntity.GetMapMenuRoot();
		if (!mapFrame) return; // Somethig gone wrong
		
		m_wCanvasWidget = CanvasWidget.Cast(GetGame().GetWorkspace().CreateWidgets(m_sPolyMarkerLayout, mapFrame));
				
		if (m_mPolygonTexture != "")
			m_TextureSharedItem = m_wCanvasWidget.LoadTexture(m_mPolygonTexture);
		else
			m_TextureSharedItem = null;
		
		if (m_mPolygonTextureBorder != "")
			m_TextureBorderSharedItem = m_wCanvasWidget.LoadTexture(m_mPolygonTextureBorder);
		else
			m_TextureBorderSharedItem = null;
		
		m_DrawPolygon.m_pTexture = m_TextureSharedItem;
		m_DrawPolygon.m_fUVScale = m_fPolygonUVScale;
		m_DrawPolygon.m_iColor = m_cPolygonColor.PackToInt();
		
		m_LinePolygon.m_pTexture = m_TextureBorderSharedItem;
		m_LinePolygon.m_UVScale = Vector(1, m_fPolygonBorderUVScale, 0.01);
		m_LinePolygon.m_iColor = m_cPolygonBorderColor.PackToInt();
		m_LinePolygon.m_fWidth = m_fPolygonBorderWidth;
		m_LinePolygon.m_bShouldEnclose = !m_bLineMode;
		
		m_wCanvasWidget.SetDrawCommands(m_MapDrawCommands);
		
		SetEventMask(EntityEvent.POSTFRAME);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void DeleteMapWidget(MapConfiguration mapConfig)
	{
		ClearEventMask(EntityEvent.POSTFRAME);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 GENERAL CHECKERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------	
	protected bool IsCurrentVisibility()
	{
		COA_Gamemode gameMode = COA_Gamemode.GetInstance();
		if (!gameMode)
			return true;
		
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return true; // Somehow manager lost, show marker
		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController)
			return true; // Somehow player controller lost, show marker
		
		SCR_PlayerFactionAffiliationComponent playerFactionAffiliationComponent = SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (!playerFactionAffiliationComponent)
			return true; // Somehow player faction component lost, show marker
		
		Faction faction = playerFactionAffiliationComponent.GetAffiliatedFaction();
		FactionKey factionKey = "";
		if (faction)
			factionKey = faction.GetFactionKey();

		// Check is player faction in visibility list
		return m_aVisibleForFactions.Contains(factionKey);
	}

	//------------------------------------------------------------------------------------------------
	bool IsInsidePolygon(vector position)
	{	
		array<float> polygon2D = new array<float>;
		array<vector> polygon3D = new array<vector>();
		
		polygon3D = GetBorder3DPolygon(polygon3D);
		SCR_Math2D.Get2DPolygon(polygon3D, polygon2D);
		
		return Math2D.IsPointInPolygon(polygon2D, position[0], position[2]);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 CONSTRUCTOR/DESTRUCTOR
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	// constructor
	//! \param[in] src
	//! \param[in] parent
	void COA_BorderBase(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.INIT);
	}
	
	//------------------------------------------------------------------------------------------------
	// destructor
	void ~COA_BorderBase()
	{
		
	}
}