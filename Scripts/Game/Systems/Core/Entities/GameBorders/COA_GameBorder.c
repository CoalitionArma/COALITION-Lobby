class COA_GameBorderClass : COA_BorderBaseClass
{
}

class COA_GameBorder : COA_BorderBase
{
//=============================================================================================================================================================================================================================================================================================================================================================
//	 ATTRIBUTES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	// ------------------------------------------------------ GENERAL SETTINGS ------------------------------------------------------
	[Attribute("", category: "Game Border - General")]
	ref COA_GameBorderSettings m_aGameBorderSettings;
	
	[Attribute("false", category: "Game Border - General")]
	bool m_bHeliRestricted;
	
	// ------------------------------------------------------ MESH SETTINGS --------------------------------------------------------
	[Attribute("10", category: "Game Border - Mesh")]
	protected float m_fHeight;
	
	[Attribute("10", category: "Game Border - Mesh")]
	protected float m_fUndergroundHeight;
	
	[Attribute(desc: "Material mapped on outside and inside of the mesh. Inside mapping is mirrored.", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "emat", category: "Game Border - Mesh")]
	protected ResourceName m_Material;
	
	[Attribute(desc: "True to stretch the material along the whole circumference instead of mapping it on each segment.", category: "Game Border - Mesh")]
	protected bool m_bStretchMaterial;
	
	protected SCR_BaseTriggerEntity m_BorderMeshTrigger;
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 OVERRIDES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		m_BorderMeshTrigger = SCR_BaseTriggerEntity.Cast(this.GetChildren());
		
		UpdateAreaMesh();
	}
	
	//------------------------------------------------------------------------------------------------
	#ifdef WORKBENCH
	//! Makes sure mesh area is generated at the correct position in workbench
	override void _WB_SetTransform(inout vector mat[4], IEntitySource src)
	{
		UpdateAreaMesh();
	}
	#endif
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 MESH UPDATERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void UpdateAreaMesh(bool visibility = true)
	{		
		if (!m_BorderMeshTrigger)
			return;
		
		array<vector> positions = new array<vector>();
		this.GetPointsPositions(positions);
		BaseWorld world = m_BorderMeshTrigger.GetWorld();
		vector worldPos;
		foreach (int i, vector pos: positions)
		{
			worldPos = m_BorderMeshTrigger.CoordToParent(pos);
			worldPos[1] = Math.Max(world.GetSurfaceY(worldPos[0], worldPos[2]) - m_fUndergroundHeight, -m_fUndergroundHeight);
			positions[i] = m_BorderMeshTrigger.CoordToLocal(worldPos);
		}
		
		ResourceName meshMat = m_Material;
		if (!visibility)
			meshMat = "{0A94C84B94134E73}Assets/Materials/Invisibility/InvisibiltyGoesSoHard.emat";
		
		Resource res = SCR_Shape.CreateAreaMesh(positions, m_fHeight + m_fUndergroundHeight, meshMat, m_bStretchMaterial);
		
		if(!res)
			return;
		
		MeshObject meshObject = res.GetResource().ToMeshObject();
		if (meshObject)
		{
			m_BorderMeshTrigger.SetObject(meshObject, "");
		}
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 CONSTRUCTOR/DESTRUCTOR
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	// constructor
	//! \param[in] src
	//! \param[in] parent
	void COA_GameBorder(IEntitySource src, IEntity parent)
	{
		COA_BorderCheckSystem borderCheck = COA_BorderCheckSystem.GetInstance();
		if (borderCheck)
			borderCheck.RegisterBorder(this);
	}
	
	//------------------------------------------------------------------------------------------------
	// destructor
	void ~COA_GameBorder()
	{
		COA_BorderCheckSystem borderCheck = COA_BorderCheckSystem.GetInstance();
		if (borderCheck)
			borderCheck.UnRegisterBorder(this);
	}
}