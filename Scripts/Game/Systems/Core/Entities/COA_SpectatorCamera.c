class COA_SpectatorCameraClass : SCR_ManualCameraClass
{
}

class COA_SpectatorCamera : SCR_ManualCamera
{
//=============================================================================================================================================================================================================================================================================================================================================================
//	 SPECTATOR ENTITY METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Attatch the spectator entity to the current active camera entity, note that AttachEntity method will keep the entity offset so we need to teleport the entity then execute
	//! \param[in] camera Camera entity to attatch the current spectator too
	void AttatchSpectatorToCamera(IEntity camera)
	{
		IEntity playerEntity = SCR_PlayerController.GetLocalMainEntity();

		// Skip initialization if conditions aren't met
		if (!GetGame().InPlayMode() || !COA_Gamemode.GetInstance() || !playerEntity || !COA_EntityHelper.IsSpectator(playerEntity))
			return;

		// Get the slot component for camera positioning
		SlotManagerComponent slotComp = SlotManagerComponent.Cast(camera.FindComponent(SlotManagerComponent));
		if (!slotComp)
			return;

		EntitySlotInfo cameraPoint = slotComp.GetSlotByName("SpectatorEntityHolder");
		if (!cameraPoint)
			return;

		vector mat[4];
		camera.GetTransform(mat);

		playerEntity.SetOrigin(camera.GetOrigin());
		playerEntity.SetTransform(mat);
		cameraPoint.AttachEntity(playerEntity);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 OVERRIDE FOR SPEC MENU
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Determine if camera control is disabled by menu
	//! Modified to allow camera control in spectator menu
	//! \return True if camera should be disabled, false otherwise
	override protected bool IsDisabledByMenu()
	{
		MenuManager menuManager = GetGame().GetMenuManager();

		if (!menuManager)
			return false;

		if (menuManager.IsAnyDialogOpen())
			return true;

		MenuBase topMenu = menuManager.GetTopMenu();

		// Allow camera control in editor and spectator menus
		return topMenu && (!topMenu.IsInherited(EditorMenuUI) && !topMenu.IsInherited(COA_SpectatorMenu));
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 ON-RAILS FOLLOW / ORBIT
//	 Runs on EOnPostFrame (inherited from SCR_ManualCamera, which already masks EntityEvent.POSTFRAME
//	 in its constructor) so tracking always reads a frame's animation pose after it has updated, and
//	 lives on the camera entity itself rather than a separate polling component — matching
//	 PS_ManualCameraSpectator in PlayableSelector.
//=============================================================================================================================================================================================================================================================================================================================================================

	protected bool m_bCameraOnRails;
	protected bool m_bTPPMode = false; // True = third-person, false = first-person (helmet cam)
	protected bool m_bEyeMode = false; // True first-person "eye cam" - see SetOnRailsEntityEyeMode

	// Orbit camera state (TPP mode)
	protected float m_fOrbitYaw    = 0.0;   // Accumulated horizontal orbit angle (degrees)
	protected float m_fOrbitPitch  = 20.0;  // Accumulated vertical orbit angle (degrees, clamped 5–80)
	protected float m_fOrbitRadius = 4.0;   // Distance from entity in meters (clamped 1.5–20)

	// Slow auto-orbit camera state (SetOnRailsOrbit / SetOnRailsOrbitEntity)
	protected IEntity m_eOrbitTargetEntity;          // entity to orbit around (null = use fixed point)
	protected float m_fOrbitSpeed     = 10.0;        // deg/sec
	protected float m_fSlowOrbitAngle =  0.0;        // current angle along the orbit circle (degrees)

	protected IEntity m_eCameraEntity;
	protected vector m_vCameraOrbitPoint;
	protected float m_vCameraOrbitDistance;
	protected float m_vCameraOrbitHeight;
	protected PolylineShapeEntity m_CameraPolyLine;

	// Smoothing state
	protected vector m_vSmoothedPivot;   // eased target/orbit-center position, absorbs replication jitter
	protected bool m_bHasSmoothedPivot;  // false until a pivot has been captured once (avoids glide-in from zero)
	protected vector m_vOldTransform[4];    // previous camera transform, used to ease into a newly-switched target
	protected bool m_bEaseToTarget;      // true for the first few frames after switching targets while already on rails
	protected float m_fEaseToTargetTimeSlice;

	//------------------------------------------------------------------------------------------------
	override protected void EOnPostFrame(IEntity owner, float timeSlice)
	{
		super.EOnPostFrame(owner, timeSlice);

		if (!m_bCameraOnRails)
			return;

		switch (true)
		{
			case (m_vCameraOrbitPoint != vector.Zero) : FrameUpdateOrbit(timeSlice); break;
			case (m_eCameraEntity) :
			{
				if (m_bTPPMode) FrameUpdateEntityTPP(timeSlice);
				else FrameUpdateEntity(timeSlice);
				break;
			}
			case (m_CameraPolyLine) : FrameUpdatePolyline(); break;
		}
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 ON-RAILS GETTER METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	bool GetIfCameraOnRails()
	{
		return m_bCameraOnRails;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 ON-RAILS SETTER METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	void SetOnRailsEntity(IEntity entity, bool tpp = false)
	{
		if (!entity)
		{
			ClearOnRails();
			return;
		}

		// Ease from wherever the camera currently is when switching between targets;
		// snap immediately on first-ever activation (mirrors PS_ManualCameraSpectator's move-link blend).
		if (m_bCameraOnRails && m_eCameraEntity)
		{
			GetTransform(m_vOldTransform);
			m_bEaseToTarget = true;
		}
		else
		{
			m_bEaseToTarget = false;
		}

		ClearOnRailsVariables();
		m_eCameraEntity = entity;
		m_bTPPMode = tpp;

		m_bCameraOnRails = true;
	}

	//------------------------------------------------------------------------------------------------
	//! Orbit the camera around a fixed world-space point at constant speed.
	//! \param point    World position to orbit around.
	//! \param radius   Orbit radius in metres.
	//! \param height   Camera height above \p point.
	//! \param speed    Angular speed in degrees/sec (default 10).
	void SetOnRailsOrbit(vector point, float radius, float height, float speed = 10.0)
	{
		ClearOnRailsVariables();
		m_vCameraOrbitPoint = point;
		m_vCameraOrbitDistance = radius;
		m_vCameraOrbitHeight = height;
		m_fOrbitSpeed = speed;
		m_bCameraOnRails = true;
	}

	//------------------------------------------------------------------------------------------------
	//! Orbit the camera around an entity at constant speed.
	//! The orbit center updates every frame to the entity's current (smoothed) position.
	//! \param entity   Entity to orbit around.
	//! \param radius   Orbit radius in metres.
	//! \param height   Camera height above the entity origin.
	//! \param speed    Angular speed in degrees/sec (default 10).
	void SetOnRailsOrbitEntity(IEntity entity, float radius, float height, float speed = 10.0)
	{
		if (!entity)
		{
			ClearOnRails();
			return;
		}

		ClearOnRailsVariables();
		m_eOrbitTargetEntity = entity;
		// Store a non-zero sentinel so EOnPostFrame routes to FrameUpdateOrbit.
		m_vCameraOrbitPoint = entity.GetOrigin();
		m_vCameraOrbitDistance = radius;
		m_vCameraOrbitHeight = height;
		m_fOrbitSpeed = speed;
		m_bCameraOnRails = true;
	}

	//------------------------------------------------------------------------------------------------
	void SetOnRailsPolyline(PolylineShapeEntity poly)
	{
		ClearOnRailsVariables();
		m_CameraPolyLine = poly;
		m_bCameraOnRails = true;
	}

	//------------------------------------------------------------------------------------------------
	void ClearOnRails()
	{
		m_bCameraOnRails = false;
		ClearOnRailsVariables();
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearOnRailsVariables()
	{
		if (m_bEyeMode && m_eCameraEntity)
			SetHeadAlpha(m_eCameraEntity, 0);

		m_eCameraEntity = null;
		m_CameraPolyLine = null;
		m_vCameraOrbitPoint = vector.Zero;
		m_vCameraOrbitDistance = 0;
		m_vCameraOrbitHeight = 0;
		m_bTPPMode = false;
		m_bEyeMode = false;
		m_eOrbitTargetEntity = null;
		m_fOrbitSpeed        = 10.0;
		m_fSlowOrbitAngle    = 0.0;
		m_bHasSmoothedPivot  = false;
	}

	//------------------------------------------------------------------------------------------------
	//! Switches to true first-person eye-cam on the given entity - distinct from the helmet-cam FPP
	//! mode above, which sits just off the head's Camera bone rather than exactly at the eye. Reuses
	//! SetOnRailsEntity for its target-switch bookkeeping (easing, ClearOnRailsVariables clearing any
	//! previous mode/alpha) then flags eye mode on top.
	void SetOnRailsEntityEyeMode(IEntity entity)
	{
		if (!entity)
		{
			ClearOnRails();
			return;
		}

		SetOnRailsEntity(entity, false);
		m_bEyeMode = true;
	}

	//------------------------------------------------------------------------------------------------
	//! 255 fully hides the head/face mesh (alpha-test cutout), 0 restores normal visibility - see
	//! CharacterCameraHandlerComponent.OnAlphatestChange in the vanilla source.
	protected void SetHeadAlpha(IEntity entity, int alpha)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		if (!character)
			return;

		SCR_CharacterCameraHandlerComponent cameraHandler = SCR_CharacterCameraHandlerComponent.Cast(
			character.FindComponent(SCR_CharacterCameraHandlerComponent));
		if (cameraHandler)
			cameraHandler.OnAlphatestChange(alpha);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 FRAME UPDATE METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected void FrameUpdateEntity(float timeSlice)
	{
		if (m_bEyeMode)
		{
			FrameUpdateEntityEyeCam(timeSlice);
			return;
		}

		vector cameraBoneMat[4];
		m_eCameraEntity.GetAnimation().GetBoneMatrix(m_eCameraEntity.GetAnimation().GetBoneIndex("Camera"), cameraBoneMat);

		vector cameraEntMat[4];
		m_eCameraEntity.GetWorldTransform(cameraEntMat);
		Math3D.MatrixMultiply4(cameraEntMat, cameraBoneMat, cameraBoneMat);

		// Flip Yaw, otherwise we are looking the wrong way
		cameraBoneMat[2] = cameraBoneMat[2].FromYaw(cameraBoneMat[2].ToYaw() - 180);

		// Calculate offset position (0.1m back, 0.3m right for over-shoulder view)
		vector offsetPosition = cameraBoneMat[3] - (cameraBoneMat[2] * 0.1) + (cameraBoneMat[0] * -0.3);
		cameraBoneMat[3] = offsetPosition;

		ApplyCameraTransform(cameraBoneMat, timeSlice);
	};

	//------------------------------------------------------------------------------------------------
	//! True first-person camera: position from the leftEye bone, orientation from the Head bone,
	//! matching exactly what the character sees rather than the helmet-cam's over-shoulder offset.
	//! Ports PS_ManualCameraSpectator.CameraPositionUpdate from PlayableSelector.
	protected void FrameUpdateEntityEyeCam(float timeSlice)
	{
		if (!m_eCameraEntity)
			return;

		int boneHead = m_eCameraEntity.GetAnimation().GetBoneIndex("Head");
		int boneEyeLeft = m_eCameraEntity.GetAnimation().GetBoneIndex("leftEye");

		vector charMat[4];
		m_eCameraEntity.GetTransform(charMat);

		vector headBoneMat[4];
		m_eCameraEntity.GetAnimation().GetBoneMatrix(boneHead, headBoneMat);
		vector eyeBoneMat[4];
		m_eCameraEntity.GetAnimation().GetBoneMatrix(boneEyeLeft, eyeBoneMat);

		vector headWorldMat[4];
		Math3D.MatrixMultiply4(charMat, headBoneMat, headWorldMat);
		vector eyeWorldMat[4];
		Math3D.MatrixMultiply4(charMat, eyeBoneMat, eyeWorldMat);

		// Head bone's own facing doesn't line up with how the eye actually looks - flip Z and nudge
		// yaw/pitch to compensate, same correction PS_ManualCameraSpectator applies.
		vector flipZ[3] = { "1 0 0", "0 1 0", "0 0 -1" };
		vector orientMat[4];
		Math3D.MatrixMultiply3(headWorldMat, flipZ, orientMat);

		vector angles = Math3D.MatrixToAngles(orientMat);
		angles[2] = 0.0;
		angles[1] = angles[1] + 10.0;
		angles[0] = angles[0] - 5.0;
		Math3D.AnglesToMatrix(angles, orientMat);
		orientMat[3] = eyeWorldMat[3];

		// Re-assert every frame - other systems can reset this between frames.
		SetHeadAlpha(m_eCameraEntity, 255);

		ApplyCameraTransform(orientMat, timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	//! Third-person orbit camera: orbits around the entity using mouse look input.
	//! Rotation only occurs while RMB (ManualCameraRotate) is held.
	//! ManualCameraRotateYaw / ManualCameraRotatePitch provide angular deltas (degrees/s at timeSlice=1).
	//! Pitch is clamped 5–80 degrees. Radius is fixed at m_fOrbitRadius.
	//! Scroll wheel while RMB is held (ManualCameraSpeedAdjust) zooms the orbit radius in/out.
	//! Radius is clamped 1.5–20 meters.
	protected void FrameUpdateEntityTPP(float timeSlice)
	{
		if (!m_eCameraEntity)
			return;

		InputManager im = GetGame().GetInputManager();

		// Only rotate/zoom while RMB is held (ManualCameraRotate action)
		if (im.GetActionValue("ManualCameraRotate") != 0)
		{
			float rawYaw   = im.GetActionValue("ManualCameraRotateYaw");
			float rawPitch = im.GetActionValue("ManualCameraRotatePitch");

			// Values are angular deltas; scale by timeSlice for frame-rate independence.
			// Speed constant matches SCR_RotateManualCameraComponent default (90 deg/s).
			const float SPEED = 90.0;
			m_fOrbitYaw   += rawYaw   * SPEED * timeSlice;
			m_fOrbitPitch  = Math.Clamp(m_fOrbitPitch - rawPitch * SPEED * timeSlice, 5.0, 80.0);

			// Scroll wheel while RMB held: ManualCameraSpeedAdjust gives a signed
			// per-frame impulse (positive = scroll up). Multiply radius by it to zoom.
			float zoomInput = im.GetActionValue("ManualCameraSpeedAdjust");
			if (zoomInput != 0)
			{
				// zoomInput is a rate value; scale so one notch (~1.0 unit) moves radius by 1 m/s
				const float ZOOM_SPEED = 8.0;
				m_fOrbitRadius = Math.Clamp(m_fOrbitRadius - zoomInput * ZOOM_SPEED * timeSlice, 1.5, 20.0);
			}
		}

		// --- Track pivot position, smoothing out replication jitter (stands in for PS riding the
		// animation system's own interpolation) without adding lag to the mouse-driven rotation. ---
		vector rawPivot = m_eCameraEntity.GetOrigin() + Vector(0, 1.0, 0);
		if (!m_bHasSmoothedPivot)
		{
			m_vSmoothedPivot = rawPivot;
			m_bHasSmoothedPivot = true;
		}
		else
		{
			m_vSmoothedPivot = vector.Lerp(m_vSmoothedPivot, rawPivot, Math.Clamp(timeSlice * 5, 0, 1));
		}

		// --- Build camera position on sphere around entity -------------------
		float yawRad   = m_fOrbitYaw   * Math.DEG2RAD;
		float pitchRad = m_fOrbitPitch * Math.DEG2RAD;

		float cosPitch = Math.Cos(pitchRad);
		vector offset = Vector(
			m_fOrbitRadius * cosPitch * Math.Sin(yawRad),
			m_fOrbitRadius * Math.Sin(pitchRad),
			m_fOrbitRadius * cosPitch * Math.Cos(yawRad)
		);
		vector camPos = m_vSmoothedPivot + offset;

		// --- Build look-at transform -----------------------------------------
		vector lookDir = vector.Direction(camPos, m_vSmoothedPivot);
		lookDir.Normalize();

		vector worldUp = Vector(0, 1, 0);
		vector right   = lookDir * worldUp; // forward × up (cross product)
		right.Normalize();
		vector up = right * lookDir; // right × forward
		up.Normalize();

		vector camTransform[4];
		camTransform[0] = right;
		camTransform[1] = up;
		camTransform[2] = lookDir;
		camTransform[3] = camPos;
		ApplyCameraTransform(camTransform, timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	protected void FrameUpdateOrbit(float timeSlice)
	{
		// Resolve orbit center — track entity position dynamically if one is set, smoothing out
		// jitter. Fixed-point orbits (e.g. menu background) have no entity to smooth against.
		vector rawCenter;
		if (m_eOrbitTargetEntity)
		{
			rawCenter = m_eOrbitTargetEntity.GetOrigin();
			if (!m_bHasSmoothedPivot)
			{
				m_vSmoothedPivot = rawCenter;
				m_bHasSmoothedPivot = true;
			}
			else
			{
				m_vSmoothedPivot = vector.Lerp(m_vSmoothedPivot, rawCenter, Math.Clamp(timeSlice * 5, 0, 1));
			}
		}
		else
		{
			m_vSmoothedPivot = m_vCameraOrbitPoint;
		}

		vector center = m_vSmoothedPivot;

		// Advance the orbit angle.
		m_fSlowOrbitAngle += m_fOrbitSpeed * timeSlice;
		if (m_fSlowOrbitAngle >= 360.0)
			m_fSlowOrbitAngle -= 360.0;

		float angleRad = m_fSlowOrbitAngle * Math.DEG2RAD;

		// Camera sits at (radius out, height up) relative to the center.
		vector camPos = Vector(
			center[0] + m_vCameraOrbitDistance * Math.Sin(angleRad),
			center[1] + m_vCameraOrbitHeight,
			center[2] + m_vCameraOrbitDistance * Math.Cos(angleRad)
		);

		// Build a look-at transform so the camera always faces the center.
		vector lookDir = vector.Direction(camPos, center);
		lookDir.Normalize();

		vector worldUp = Vector(0, 1, 0);
		vector right   = lookDir * worldUp;
		right.Normalize();
		vector up = right * lookDir;
		up.Normalize();

		vector camTransform[4];
		camTransform[0] = right;
		camTransform[1] = up;
		camTransform[2] = lookDir;
		camTransform[3] = camPos;
		SetTransform(camTransform);
	}

	//------------------------------------------------------------------------------------------------
	protected void FrameUpdatePolyline()
	{

	}

	//------------------------------------------------------------------------------------------------
	//! Applies a freshly computed camera transform, easing in from the previous transform for the
	//! first few frames right after switching targets (mirrors PS_ManualCameraSpectator's move-link
	//! blend) so the camera glides onto the new target instead of popping.
	protected void ApplyCameraTransform(vector transform[4], float timeSlice)
	{
		if (m_bEaseToTarget)
		{
			vector eased[4];
			eased = transform;
			m_fEaseToTargetTimeSlice = m_fEaseToTargetTimeSlice + timeSlice;
			float t = Math.Map(Math.Clamp(m_fEaseToTargetTimeSlice, 0, 3), 0, 3, 0.1, 1);
			
			eased[3] = vector.Lerp(m_vOldTransform[3], transform[3], t);

			if (vector.Distance(eased[3], transform[3]) < 0.05 || m_fEaseToTargetTimeSlice > 3)
				m_bEaseToTarget = false;

			SetTransform(eased);
			m_vOldTransform = eased;
			return;
		}

		SetTransform(transform);
		m_vOldTransform = transform;
		m_fEaseToTargetTimeSlice = 0;
	}
}
