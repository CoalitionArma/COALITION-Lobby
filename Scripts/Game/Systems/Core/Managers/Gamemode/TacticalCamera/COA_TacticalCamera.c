//------------------------------------------------------------------------------------------------
// Tactical Camera
//
// A distance-limited Game Master style camera for platoon leaders and commanders. It uses the
// engine's editor in STRATEGY mode with the Lobby's own mode prefab (COA_EditorModeTactical.et):
//  - no placing, attributes or admin tools - only friendly units are visible and selectable
//  - the camera stays within a radius of the leader's own body (COA_TacticalLimitManualCameraComponent)
//  - right-clicking a friendly squad gives it an order (COA_TacticalOrderAction), which every
//    member receives as a hint and a marker on their map
//
// Access is decided on the server from the leader's slot role (COA_Gamemode "Tactical Camera"
// settings) and applied through the Lobby's SCR_EditorManagerEntity mod, which swaps the STRATEGY
// mode prefab for the tactical one and adds the mode to qualifying players.
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
enum COA_ETacticalOrder
{
	MOVE = 0,
	ATTACK,
	DEFEND,
	HOLD,
	REGROUP,
	CANCEL,
}

//------------------------------------------------------------------------------------------------
class COA_TacticalCamera
{
	static const ResourceName MODE_PREFAB = "{6A6612AB7E3CA001}Prefabs/!Systems/Editor/Modes/COA_EditorModeTactical.et";

	protected static const float DEFAULT_RANGE = 400;
	protected static const float DEFAULT_MAX_HEIGHT = 120;

	//------------------------------------------------------------------------------------------------
	//! Mission setting: is the tactical camera available at all
	static bool IsEnabled()
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		return gamemode && gamemode.m_bTacticalCameraEnabled;
	}

	//------------------------------------------------------------------------------------------------
	//! Server: whether this player's slot role grants the tactical camera right now
	static bool PlayerQualifies(int playerId)
	{
		if (playerId <= 0 || !IsEnabled())
			return false;

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return false;

		COA_SlotData slotData = slottingManager.GetPlayerSlotData(playerId);
		if (!slotData || slotData.GetIsDeadSlot())
			return false;

		return IsLeaderRole(slotData.GetSlotRole());
	}

	//------------------------------------------------------------------------------------------------
	//! Roles from the mission settings; company commanders and platoon leaders when none are set
	static bool IsLeaderRole(COA_EGearRole role)
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (gamemode && gamemode.m_aTacticalCameraRoles && !gamemode.m_aTacticalCameraRoles.IsEmpty())
			return gamemode.m_aTacticalCameraRoles.Contains(role);

		return role == COA_EGearRole.COMPANY_COMMANDER || role == COA_EGearRole.PLATOON_LEADER;
	}

	//------------------------------------------------------------------------------------------------
	//! Server: re-evaluate a player's editor modes after their slot changed. The Lobby's
	//! SCR_EditorManagerEntity mod adds or drops the tactical mode while re-applying them.
	static void RefreshAccess(int playerId)
	{
		if (playerId <= 0 || RplSession.Mode() == RplMode.Client)
			return;

		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return;

		SCR_EditorManagerEntity editorManager = core.GetEditorManager(playerId);
		if (editorManager)
			editorManager.COA_RefreshTacticalMode();
	}

	//------------------------------------------------------------------------------------------------
	//! Client: is the local player's editor open in the tactical mode right now
	static bool IsLocalPlayerInTacticalCamera()
	{
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		return editorManager && editorManager.IsOpened() && editorManager.GetCurrentMode() == EEditorMode.STRATEGY;
	}

	//------------------------------------------------------------------------------------------------
	//! Horizontal distance the camera may travel from the leader's body (metres)
	static float GetRange()
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (gamemode && gamemode.m_fTacticalCameraRange > 0)
			return gamemode.m_fTacticalCameraRange;

		return DEFAULT_RANGE;
	}

	//------------------------------------------------------------------------------------------------
	//! Height above ground the camera may climb to (metres)
	static float GetMaxHeight()
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (gamemode && gamemode.m_fTacticalCameraMaxHeight > 0)
			return gamemode.m_fTacticalCameraMaxHeight;

		return DEFAULT_MAX_HEIGHT;
	}

	//------------------------------------------------------------------------------------------------
	static string GetOrderName(COA_ETacticalOrder order)
	{
		switch (order)
		{
			case COA_ETacticalOrder.MOVE: return "MOVE";
			case COA_ETacticalOrder.ATTACK: return "ATTACK";
			case COA_ETacticalOrder.DEFEND: return "DEFEND";
			case COA_ETacticalOrder.HOLD: return "HOLD";
			case COA_ETacticalOrder.REGROUP: return "REGROUP";
			case COA_ETacticalOrder.CANCEL: return "CANCEL";
		}

		return "ORDER";
	}
}

//------------------------------------------------------------------------------------------------
//! Keeps the tactical camera within range of the leader's own character - attached to the camera
//! prefab (COA_ManualCameraTactical.et). Range and height come from the mission settings.
[BaseContainerProps(), SCR_BaseManualCameraComponentTitle()]
class COA_TacticalLimitManualCameraComponent : SCR_BaseManualCameraComponent
{
	//------------------------------------------------------------------------------------------------
	override bool EOnCameraInit()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void EOnCameraFrame(SCR_ManualCameraParam param)
	{
		IEntity anchor = SCR_PlayerController.GetLocalControlledEntity();
		if (!anchor)
			return;

		vector position = param.transform[3];
		vector center = anchor.GetOrigin();

		// Horizontal: slide back to the edge of the circle around the leader
		float range = COA_TacticalCamera.GetRange();
		vector offset = position - center;
		offset[1] = 0;
		float distance = offset.Length();
		if (distance > range && distance > 0)
		{
			offset = offset * (range / distance);
			position[0] = center[0] + offset[0];
			position[2] = center[2] + offset[2];
		}

		// Vertical: cap the height above the terrain under the camera
		float ceiling = GetGame().GetWorld().GetSurfaceY(position[0], position[2]) + COA_TacticalCamera.GetMaxHeight();
		if (position[1] > ceiling)
			position[1] = ceiling;

		param.transform[3] = position;
	}
}

//------------------------------------------------------------------------------------------------
//! Client side of an order: a hint and a marker on the receiving player's own map (local marker,
//! so only the ordered squad sees it). One order marker is kept per player; a new order replaces it.
class COA_TacticalOrderClient
{
	protected static ref SCR_MapMarkerBase s_OrderMarker;

	protected static const ResourceName ORDER_SOUND = "{E23715DAF7FE2E8A}Sounds/Items/Equipment/Radios/Samples/Items_Radio_Turn_On.wav";
	protected static const float HINT_DURATION = 12;

	//------------------------------------------------------------------------------------------------
	static void Receive(COA_ETacticalOrder order, vector position, string issuerName)
	{
		ClearMarker();

		if (order == COA_ETacticalOrder.CANCEL)
		{
			SCR_HintManagerComponent.ShowCustomHint("Previous order cancelled.", "Order from " + issuerName, HINT_DURATION);
			AudioSystem.PlaySound(ORDER_SOUND);
			return;
		}

		string orderName = COA_TacticalCamera.GetOrderName(order);
		string grid = SCR_MapEntity.GetGridLabel(position);

		SCR_HintManagerComponent.ShowCustomHint(string.Format("%1 - grid %2. Check your map.", orderName, grid), "Order from " + issuerName, HINT_DURATION);
		AudioSystem.PlaySound(ORDER_SOUND);

		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerManager)
			return;

		SCR_MapMarkerBase marker = new SCR_MapMarkerBase();
		marker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
		marker.SetWorldPos(position[0], position[2]);
		marker.SetIconEntry(GetOrderIcon(order));
		marker.SetColorEntry(GetFactionColorEntry());
		marker.SetCustomText(orderName + " - " + issuerName);
		markerManager.InsertStaticMarker(marker, true);
		s_OrderMarker = marker;
	}

	//------------------------------------------------------------------------------------------------
	static void ClearMarker()
	{
		if (!s_OrderMarker)
			return;

		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (markerManager)
			markerManager.RemoveStaticMarker(s_OrderMarker);

		s_OrderMarker = null;
	}

	//------------------------------------------------------------------------------------------------
	protected static int GetOrderIcon(COA_ETacticalOrder order)
	{
		switch (order)
		{
			case COA_ETacticalOrder.MOVE: return SCR_EScenarioFrameworkMarkerCustom.DROP_POINT;
			case COA_ETacticalOrder.ATTACK: return SCR_EScenarioFrameworkMarkerCustom.MARK_EXCLAMATION;
			case COA_ETacticalOrder.DEFEND: return SCR_EScenarioFrameworkMarkerCustom.FORTIFICATION;
			case COA_ETacticalOrder.HOLD: return SCR_EScenarioFrameworkMarkerCustom.OBSERVATION_POST;
			case COA_ETacticalOrder.REGROUP: return SCR_EScenarioFrameworkMarkerCustom.FLAG;
		}

		return SCR_EScenarioFrameworkMarkerCustom.POINT_OF_INTEREST;
	}

	//------------------------------------------------------------------------------------------------
	protected static int GetFactionColorEntry()
	{
		Faction faction = SCR_FactionManager.SGetLocalPlayerFaction();
		if (!faction)
			return SCR_EScenarioFrameworkMarkerCustomColor.WHITE;

		switch (faction.GetFactionKey())
		{
			case "BLUFOR": return SCR_EScenarioFrameworkMarkerCustomColor.BLUFOR;
			case "OPFOR": return SCR_EScenarioFrameworkMarkerCustomColor.OPFOR;
			case "INDFOR": return SCR_EScenarioFrameworkMarkerCustomColor.INDEPENDENT;
		}

		return SCR_EScenarioFrameworkMarkerCustomColor.WHITE;
	}
}
