class COA_PlayerCameraManagerClass : ScriptComponentClass
{
}

class COA_PlayerCameraManager : ScriptComponent
{
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	IEntity m_eCamera;                      // Stores local camera entity for spectator mode
	protected vector m_vStoredCameraPos[4];   // Stores camera transform between sessions

//=============================================================================================================================================================================================================================================================================================================================================================
//	 UPDATE AND INITIALIZE METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Initilizes players if they have a valid spectator entity
	void InitilizeSpecCamera()
	{
		vector cameraPos[4] = COA_PlayerControllerManager.GetInstance().m_vPlayersLastDeath;
		
		// Use provided death position if available
		if (COA_EntityHelper.IsValidSpawnVector(cameraPos[3])) {
			cameraPos[3][1] = cameraPos[3][1] + 1.5; // Elevate camera slightly above death position
		}
		// Use stored camera position if available
		else if (COA_EntityHelper.IsValidSpawnVector(m_vStoredCameraPos[3])) {
			cameraPos = m_vStoredCameraPos;
		} 
		// Fallback to generic spawn position
		else {
			cameraPos[3] = COA_Gamemode.GetInstance().GetGenericSpawn();
		}

		// Spawn or reposition camera
		if (!m_eCamera)
			m_eCamera = GetGame().SpawnEntityPrefab(Resource.Load(COA_EntityHelper.GetSpectatorCameraResource()), GetGame().GetWorld(), COA_EntityHelper.CreateSpawnParams(cameraPos));
		
		// Level camera horizon
		vector mat = m_eCamera.GetAngles();
		m_eCamera.SetAngles(Vector(mat[0], mat[1], 0));
		
		if (!CVON_VONGameModeComponent.GetInstance())
			COA_SpectatorCamera.Cast(m_eCamera).AttatchSpectatorToCamera(m_eCamera);
		
		// Switch to spectator camera
		GetGame().GetCameraManager().SetCamera(CameraBase.Cast(m_eCamera));
	};
	
	//------------------------------------------------------------------------------------------------
	//! Updates stored camera position for persistence between deaths
	void UpdateStoredCameraPos()
	{
		if (m_eCamera)
		{
			vector cameraPos[4];
			m_eCamera.GetWorldTransform(cameraPos);
			m_vStoredCameraPos = cameraPos;
		};
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 CAMERA ON RAILS SETTER METHODS
//	 Thin forwards onto COA_SpectatorCamera, which owns the tracking state and does the per-frame
//	 follow/orbit work on its own EOnPostFrame (see COA_SpectatorCamera.c) — matching the architecture
//	 PlayableSelector's PS_ManualCameraSpectator uses.
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	void SetCameraOnRailsEntity(IEntity entity, bool tpp = false)
	{
		COA_SpectatorCamera camera = COA_SpectatorCamera.Cast(m_eCamera);
		if (camera)
			camera.SetOnRailsEntity(entity, tpp);
	}

	//------------------------------------------------------------------------------------------------
	//! Orbit the camera around a fixed world-space point at constant speed.
	//! \param point    World position to orbit around.
	//! \param radius   Orbit radius in metres.
	//! \param height   Camera height above \p point.
	//! \param speed    Angular speed in degrees/sec (default 10).
	void SetCameraOnRailsOrbit(vector point, float radius, float height, float speed = 10.0)
	{
		COA_SpectatorCamera camera = COA_SpectatorCamera.Cast(m_eCamera);
		if (camera)
			camera.SetOnRailsOrbit(point, radius, height, speed);
	}

	//------------------------------------------------------------------------------------------------
	//! Orbit the camera around an entity at constant speed.
	//! The orbit center updates every frame to the entity's current position.
	//! \param entity   Entity to orbit around.
	//! \param radius   Orbit radius in metres.
	//! \param height   Camera height above the entity origin.
	//! \param speed    Angular speed in degrees/sec (default 10).
	void SetCameraOnRailsOrbitEntity(IEntity entity, float radius, float height, float speed = 10.0)
	{
		COA_SpectatorCamera camera = COA_SpectatorCamera.Cast(m_eCamera);
		if (camera)
			camera.SetOnRailsOrbitEntity(entity, radius, height, speed);
	}

	//------------------------------------------------------------------------------------------------
	void SetCameraOnRailsPolyline(PolylineShapeEntity poly)
	{
		COA_SpectatorCamera camera = COA_SpectatorCamera.Cast(m_eCamera);
		if (camera)
			camera.SetOnRailsPolyline(poly);
	}

	//------------------------------------------------------------------------------------------------
	void DisableCameraOnRails()
	{
		COA_SpectatorCamera camera = COA_SpectatorCamera.Cast(m_eCamera);
		if (camera)
			camera.ClearOnRails();
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static COA_PlayerCameraManager m_sInstance;
	void COA_PlayerCameraManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_PlayerCameraManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static COA_PlayerCameraManager GetInstance()
	{
		return m_sInstance;
	}
}