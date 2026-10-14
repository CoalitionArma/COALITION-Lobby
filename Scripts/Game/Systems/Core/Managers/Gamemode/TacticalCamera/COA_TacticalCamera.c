//------------------------------------------------------------------------------------------------
// Tactical Camera
//
// A distance-limited Game Master style camera for platoon leaders and commanders. It uses the
// engine's editor in STRATEGY mode with the Lobby's own mode prefab (COA_EditorModeTactical.et):
//  - no placing, attributes or admin tools - only friendly units are visible and selectable
//  - the camera stays within a radius of the leader's own body (COA_TacticalLimitManualCameraComponent)
//  - select a friendly squad (click a member, or its row in the squad list), then right-click the
//    ground to give it an order there (COA_TacticalOrderAction, COA_TacticalContextActionsEditorComponent).
//    Every member receives it as a hint and a marker on their map
//  - the HUD (COA_TacticalHUDEditorUIComponent) draws the range ring, squad labels, a squad list
//    to jump to, and the orders leaders on the side have issued (COA_TacticalOrderBoard)
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
	//! Client: is this squad on the local player's side
	static bool IsFriendlyToLocalPlayer(SCR_AIGroup group)
	{
		if (!group)
			return false;

		Faction localFaction = SCR_FactionManager.SGetLocalPlayerFaction();
		Faction groupFaction = group.GetFaction();
		if (!localFaction || !groupFaction)
			return false;

		return groupFaction == localFaction || localFaction.IsFactionFriendly(groupFaction);
	}

	//------------------------------------------------------------------------------------------------
	//! Client: the friendly player squads an editable entity stands for - a group, a player
	//! character, or a vehicle with players inside. Player squads use PlayableGroup.et, which has no
	//! SCR_EditableGroupComponent, so a player's squad is looked up from their player ID instead.
	static int GetPlayerSquads(SCR_EditableEntityComponent entity, notnull array<SCR_AIGroup> outSquads)
	{
		if (!entity)
			return outSquads.Count();

		// An editable group (AI or a modded playable group with the component)
		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(entity);
		if (!editableGroup)
			editableGroup = SCR_EditableGroupComponent.Cast(entity.GetAIGroup());
		if (editableGroup)
			InsertSquad(editableGroup.GetAIGroupComponent(), outSquads);

		// A player character
		InsertSquad(GetSquadOfCharacter(entity.GetOwner()), outSquads);

		// A vehicle: everyone inside it
		if (entity.GetEntityType() == EEditableEntityType.VEHICLE)
		{
			array<CompartmentAccessComponent> crew = {};
			entity.GetCrew(crew, false);
			foreach (CompartmentAccessComponent crewMember : crew)
			{
				if (crewMember)
					InsertSquad(GetSquadOfCharacter(crewMember.GetOwner()), outSquads);
			}
		}

		return outSquads.Count();
	}

	//------------------------------------------------------------------------------------------------
	protected static SCR_AIGroup GetSquadOfCharacter(IEntity character)
	{
		if (!character)
			return null;

		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(character);
		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (playerId <= 0 || !groupsManager)
			return null;

		SCR_AIGroup playerGroup = groupsManager.GetPlayerGroup(playerId);
		if (playerGroup)
			return playerGroup;

		// The faction manager may not know another player's faction on this client yet - search
		// the friendly squads directly
		array<SCR_AIGroup> friendlySquads = {};
		GetFriendlySquads(friendlySquads);
		foreach (SCR_AIGroup friendlySquad : friendlySquads)
		{
			if (friendlySquad.IsPlayerInGroup(playerId))
				return friendlySquad;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static void InsertSquad(SCR_AIGroup group, notnull array<SCR_AIGroup> outSquads)
	{
		if (group && group.GetPlayerCount() > 0 && IsFriendlyToLocalPlayer(group) && !outSquads.Contains(group))
			outSquads.Insert(group);
	}

	//------------------------------------------------------------------------------------------------
	//! Client: every player squad on the local player's side (own faction and allies)
	static int GetFriendlySquads(notnull array<SCR_AIGroup> outSquads)
	{
		outSquads.Clear();

		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		FactionManager factionManager = GetGame().GetFactionManager();
		Faction localFaction = SCR_FactionManager.SGetLocalPlayerFaction();
		if (!groupsManager || !factionManager || !localFaction)
			return 0;

		array<Faction> factions = {};
		factionManager.GetFactionsList(factions);
		foreach (Faction faction : factions)
		{
			if (faction != localFaction && !localFaction.IsFactionFriendly(faction))
				continue;

			array<SCR_AIGroup> factionGroups = groupsManager.GetPlayableGroupsByFaction(faction);
			if (!factionGroups)
				continue;

			foreach (SCR_AIGroup group : factionGroups)
			{
				if (group && group.GetPlayerCount() > 0)
					outSquads.Insert(group);
			}
		}

		return outSquads.Count();
	}

	//------------------------------------------------------------------------------------------------
	//! Where a squad is: its leader's body, or the first member found when the leader has none
	static bool GetSquadPosition(SCR_AIGroup group, out vector position)
	{
		if (!group)
			return false;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		IEntity body = playerManager.GetPlayerControlledEntity(group.GetLeaderID());
		if (!body)
		{
			foreach (int memberId : group.GetPlayerIDs())
			{
				body = playerManager.GetPlayerControlledEntity(memberId);
				if (body)
					break;
			}
		}

		if (!body)
			return false;

		position = body.GetOrigin();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Short squad name for labels: the custom name, else the callsign
	static string GetSquadName(SCR_AIGroup group)
	{
		if (!group)
			return "Squad";

		string squadName = group.GetCustomName();
		if (squadName.IsEmpty())
			squadName = group.GetCustomNameWithOriginal();
		if (squadName.IsEmpty())
			squadName = "Squad";

		return squadName;
	}

	//------------------------------------------------------------------------------------------------
	//! Colour of an order type (sRGB) - HUD lines and map markers
	static void GetOrderColorSRGB(COA_ETacticalOrder order, out int red, out int green, out int blue)
	{
		red = 255;
		green = 255;
		blue = 255;
		switch (order)
		{
			case COA_ETacticalOrder.MOVE: red = 90; green = 170; blue = 255; break;
			case COA_ETacticalOrder.ATTACK: red = 255; green = 80; blue = 64; break;
			case COA_ETacticalOrder.DEFEND: red = 80; green = 220; blue = 120; break;
			case COA_ETacticalOrder.HOLD: red = 255; green = 210; blue = 64; break;
			case COA_ETacticalOrder.REGROUP: red = 230; green = 230; blue = 230; break;
		}
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
//! A Tactical Camera order marker on this player's map (COA_PlayerScriptedMarkerManager, local
//! to this player). The parameters are kept because removing a scripted marker needs them all.
class COA_TacticalOrderMarker
{
	string m_sPosition;
	string m_sText;
	ResourceName m_Image;
	int m_iColor;
}

//------------------------------------------------------------------------------------------------
//! Client side of an order, on the ordered squad's leader (or every member when the squad has no
//! leader): a hint and a map marker showing the order type at the ordered spot. One order marker is
//! kept per player; a new order (or CANCEL) replaces it.
class COA_TacticalOrderClient
{
	protected static ref COA_TacticalOrderMarker s_OrderMarker;

	protected static const ResourceName ORDER_SOUND = "{E23715DAF7FE2E8A}Sounds/Items/Equipment/Radios/Samples/Items_Radio_Turn_On.wav";
	protected static const float HINT_DURATION = 12;

	protected static const int MARKER_UPDATE_DELAY = 1;
	protected static const int MARKER_Z_ORDER = 60;

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

		SCR_HintManagerComponent.ShowCustomHint(string.Format("%1 - grid %2. Marked on your map.", orderName, grid), "Order from " + issuerName, HINT_DURATION);
		AudioSystem.PlaySound(ORDER_SOUND);

		s_OrderMarker = CreateMarker(order, position, orderName + " - " + issuerName);
	}

	//------------------------------------------------------------------------------------------------
	static void ClearMarker()
	{
		RemoveMarker(s_OrderMarker);
		s_OrderMarker = null;
	}

	//------------------------------------------------------------------------------------------------
	//! A local (this player only) map marker with the order type's icon and colour
	static COA_TacticalOrderMarker CreateMarker(COA_ETacticalOrder order, vector position, string text)
	{
		COA_PlayerScriptedMarkerManager markerManager = COA_PlayerScriptedMarkerManager.GetInstance();
		if (!markerManager)
			return null;

		int red, green, blue;
		COA_TacticalCamera.GetOrderColorSRGB(order, red, green, blue);

		COA_TacticalOrderMarker marker = new COA_TacticalOrderMarker();
		marker.m_sPosition = string.Format("%1 %2 %3", position[0], position[1], position[2]);
		marker.m_sText = text;
		marker.m_Image = GetOrderIcon(order);
		marker.m_iColor = Color.FromSRGBA(red, green, blue, 255).PackToInt(); // UI colours are linear

		markerManager.AddScriptedMarker("Static Marker", marker.m_sPosition, MARKER_UPDATE_DELAY, marker.m_sText, marker.m_Image, MARKER_Z_ORDER, marker.m_iColor);
		return marker;
	}

	//------------------------------------------------------------------------------------------------
	static void RemoveMarker(COA_TacticalOrderMarker marker)
	{
		if (!marker)
			return;

		COA_PlayerScriptedMarkerManager markerManager = COA_PlayerScriptedMarkerManager.GetInstance();
		if (markerManager)
			markerManager.RemoveScriptedMarker("Static Marker", marker.m_sPosition, MARKER_UPDATE_DELAY, marker.m_sText, marker.m_Image, MARKER_Z_ORDER, marker.m_iColor);
	}

	//------------------------------------------------------------------------------------------------
	//! Map icon per order type (vanilla editor waypoint icons)
	static ResourceName GetOrderIcon(COA_ETacticalOrder order)
	{
		switch (order)
		{
			case COA_ETacticalOrder.MOVE: return "{FDE4238500835875}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_Move.edds";
			case COA_ETacticalOrder.ATTACK: return "{428583D4284BC412}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_SearchAndDestroy.edds";
			case COA_ETacticalOrder.DEFEND: return "{1B3B38AAA1577734}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_Defend.edds";
			case COA_ETacticalOrder.HOLD: return "{564794579B2DB679}UI/Textures/Editor/Attributes/Attribute_Locked.edds";
			case COA_ETacticalOrder.REGROUP: return "{F0E0F66182733D39}UI/Textures/Editor/ContextMenu/ContextAction_Teleport_Player.edds";
		}

		return "{9F5B32F85CABED6F}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint.edds";
	}
}

//------------------------------------------------------------------------------------------------
//! One squad's standing order, as the leaders on its side know it
class COA_TacticalIssuedOrder
{
	RplId m_GroupId;
	SCR_AIGroup m_Group; // weak: drops to null when the squad is deleted
	COA_ETacticalOrder m_eOrder;
	vector m_vPosition;
	string m_sIssuer;
	string m_sSquadName;
	float m_fIssuedAt; // world time (ms)
	ref COA_TacticalOrderMarker m_Marker;
}

//------------------------------------------------------------------------------------------------
//! Client, on platoon leaders and commanders: the last order each friendly squad was given by any
//! leader on the side, so the Tactical Camera can draw them and their map shows them. Filled by
//! COA_PlayerRplToOwnerManager.ReceiveTacticalOrderIssued; a CANCEL order clears the squad's entry.
class COA_TacticalOrderBoard
{
	protected static ref array<ref COA_TacticalIssuedOrder> s_aOrders = {};

	//------------------------------------------------------------------------------------------------
	static void Record(RplId groupId, COA_ETacticalOrder order, vector position, string issuerName)
	{
		Purge();

		COA_TacticalIssuedOrder entry = Find(groupId);
		if (entry)
		{
			COA_TacticalOrderClient.RemoveMarker(entry.m_Marker);
			s_aOrders.RemoveItem(entry);
		}

		if (order == COA_ETacticalOrder.CANCEL)
			return;

		SCR_AIGroup group = SCR_AIGroup.Cast(Replication.FindItem(groupId));
		if (!group)
			return;

		entry = new COA_TacticalIssuedOrder();
		entry.m_GroupId = groupId;
		entry.m_Group = group;
		entry.m_eOrder = order;
		entry.m_vPosition = position;
		entry.m_sIssuer = issuerName;
		entry.m_sSquadName = COA_TacticalCamera.GetSquadName(group);
		entry.m_fIssuedAt = GetGame().GetWorld().GetWorldTime();
		entry.m_Marker = COA_TacticalOrderClient.CreateMarker(order, position, entry.m_sSquadName + ": " + COA_TacticalCamera.GetOrderName(order));
		s_aOrders.Insert(entry);
	}

	//------------------------------------------------------------------------------------------------
	static COA_TacticalIssuedOrder Find(RplId groupId)
	{
		foreach (COA_TacticalIssuedOrder entry : s_aOrders)
		{
			if (entry && entry.m_GroupId == groupId)
				return entry;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	static COA_TacticalIssuedOrder FindForGroup(SCR_AIGroup group)
	{
		if (!group)
			return null;

		return Find(Replication.FindItemId(group));
	}

	//------------------------------------------------------------------------------------------------
	//! Current orders, oldest first. Squads that no longer exist are dropped first.
	static int GetOrders(notnull array<COA_TacticalIssuedOrder> outOrders)
	{
		Purge();
		outOrders.Clear();
		foreach (COA_TacticalIssuedOrder entry : s_aOrders)
		{
			outOrders.Insert(entry);
		}

		return outOrders.Count();
	}

	//------------------------------------------------------------------------------------------------
	//! Forget squads that were deleted (squad disbanded, or a new mission loaded)
	protected static void Purge()
	{
		for (int i = s_aOrders.Count() - 1; i >= 0; i--)
		{
			COA_TacticalIssuedOrder entry = s_aOrders[i];
			if (entry && entry.m_Group)
				continue;

			if (entry)
				COA_TacticalOrderClient.RemoveMarker(entry.m_Marker);

			s_aOrders.Remove(i);
		}
	}
}
