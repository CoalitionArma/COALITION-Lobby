//------------------------------------------------------------------------------------------------
//! Tactical Camera HUD (root of COA_ModeTactical.layout). Every frame it draws:
//!  - the range ring: how far the camera may go from the leader's body
//!  - a label over every friendly squad (name, size, current order)
//!  - each squad's current order from COA_TacticalOrderBoard: a pole at the spot and a line to it
//! and keeps the squad list (click a row to select that squad and fly the camera to it), the
//! camera distance readout and the "selected squad" line up to date.
class COA_TacticalHUDEditorUIComponent : SCR_BaseEditorUIComponent
{
	[Attribute("SquadList", desc: "Vertical layout the squad rows are created in")]
	protected string m_sSquadListName;

	[Attribute("SquadCount", desc: "Text in the squad list header")]
	protected string m_sSquadCountName;

	[Attribute("SquadLabels", desc: "Full-screen frame the floating labels are created in")]
	protected string m_sLabelsName;

	[Attribute("CameraInfo")]
	protected string m_sCameraInfoName;

	[Attribute("SelectionInfo")]
	protected string m_sSelectionInfoName;

	[Attribute("{6A6612AB7E3CA007}UI/layouts/Editor/COA_TacticalSquadRow.layout", UIWidgets.ResourceNamePicker, "Squad list row", "layout")]
	protected ResourceName m_SquadRowLayout;

	[Attribute("{6A6612AB7E3CA008}UI/layouts/Editor/COA_TacticalLabel.layout", UIWidgets.ResourceNamePicker, "Floating label", "layout")]
	protected ResourceName m_LabelLayout;

	protected static const float REFRESH_INTERVAL = 0.25;	// seconds between squad list / selection refreshes
	protected static const int RING_SEGMENTS = 96;			// ring point array holds RING_SEGMENTS + 1 points
	protected static const float RING_HEIGHT = 1.5;			// metres above the ground
	protected static const float LABEL_HEIGHT = 2.6;		// metres above a squad leader's feet
	protected static const float ORDER_POLE_HEIGHT = 14;	// metres

	protected WorkspaceWidget m_Workspace;
	protected BaseWorld m_World;
	protected Widget m_wSquadList;
	protected TextWidget m_wSquadCount;
	protected Widget m_wLabels;
	protected TextWidget m_wCameraInfo;
	protected TextWidget m_wSelectionInfo;

	protected ref array<Widget> m_aRows = {};
	protected ref array<Widget> m_aLabels = {};
	protected int m_iLabelsUsed;

	protected ref array<SCR_AIGroup> m_aSquads = {};
	protected ref array<SCR_AIGroup> m_aSelectedSquads = {};
	protected ref array<COA_TacticalIssuedOrder> m_aOrders = {};
	protected float m_fRefreshTimer;

	protected ref Color m_TextColor = Color.FromSRGBA(237, 241, 247, 255);
	protected ref Color m_MutedColor = Color.FromSRGBA(160, 172, 196, 255);
	protected ref Color m_SelectedColor = Color.FromSRGBA(255, 206, 84, 255);
	protected ref Color m_WarningColor = Color.FromSRGBA(232, 118, 92, 255);

	//------------------------------------------------------------------------------------------------
	protected void OnMenuUpdate(float tDelta)
	{
		m_fRefreshTimer -= tDelta;
		if (m_fRefreshTimer <= 0)
		{
			m_fRefreshTimer = REFRESH_INTERVAL;
			COA_TacticalCamera.GetFriendlySquads(m_aSquads);
			COA_TacticalOrderBoard.GetOrders(m_aOrders);
			RefreshSelection();
			RefreshSquadList();
		}

		IEntity body = SCR_PlayerController.GetLocalControlledEntity();
		UpdateCameraInfo(body);

		// Nothing drawn while the editor UI is hidden (e.g. for a screenshot)
		m_iLabelsUsed = 0;
		if (GetWidget().IsVisibleInHierarchy())
		{
			if (body)
				DrawRangeRing(body.GetOrigin());

			DrawOrders();
			DrawSquadLabels();
		}

		for (int i = m_iLabelsUsed, count = m_aLabels.Count(); i < count; i++)
		{
			m_aLabels[i].SetVisible(false);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Select every member of a squad and fly the camera to it (squad list rows)
	void FocusSquad(SCR_AIGroup group)
	{
		if (!group)
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		set<SCR_EditableEntityComponent> members = new set<SCR_EditableEntityComponent>();
		foreach (int memberId : group.GetPlayerIDs())
		{
			SCR_EditableEntityComponent editableMember = SCR_EditableEntityComponent.GetEditableEntity(playerManager.GetPlayerControlledEntity(memberId));
			if (editableMember)
				members.Insert(editableMember);
		}

		SCR_BaseEditableEntityFilter selectedFilter = SCR_BaseEditableEntityFilter.GetInstance(EEditableEntityState.SELECTED);
		if (selectedFilter && !members.IsEmpty())
			selectedFilter.Replace(members);

		vector squadPosition;
		SCR_ManualCamera camera = SCR_CameraEditorComponent.GetCameraInstance();
		if (camera && COA_TacticalCamera.GetSquadPosition(group, squadPosition))
		{
			SCR_TeleportToCursorManualCameraComponent teleport = SCR_TeleportToCursorManualCameraComponent.Cast(camera.FindCameraComponent(SCR_TeleportToCursorManualCameraComponent));
			if (teleport)
				teleport.TeleportCamera(squadPosition);
		}

		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.FOCUS);
		m_fRefreshTimer = 0;
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshSelection()
	{
		m_aSelectedSquads.Clear();

		SCR_BaseEditableEntityFilter selectedFilter = SCR_BaseEditableEntityFilter.GetInstance(EEditableEntityState.SELECTED);
		if (selectedFilter)
		{
			set<SCR_EditableEntityComponent> selected = new set<SCR_EditableEntityComponent>();
			selectedFilter.GetEntities(selected);
			foreach (SCR_EditableEntityComponent selectedEntity : selected)
			{
				COA_TacticalCamera.GetPlayerSquads(selectedEntity, m_aSelectedSquads);
			}
		}

		if (!m_wSelectionInfo)
			return;

		if (m_aSelectedSquads.IsEmpty())
		{
			m_wSelectionInfo.SetText("No squad selected - click a squad member or pick a squad from the list");
			m_wSelectionInfo.SetColor(m_MutedColor);
			return;
		}

		string names;
		foreach (int i, SCR_AIGroup selectedSquad : m_aSelectedSquads)
		{
			if (i > 0)
				names += ", ";

			names += COA_TacticalCamera.GetSquadName(selectedSquad);
		}

		m_wSelectionInfo.SetText(string.Format("Selected: %1  -  right-click the ground to give an order", names));
		m_wSelectionInfo.SetColor(m_SelectedColor);
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshSquadList()
	{
		if (!m_wSquadList)
			return;

		if (m_wSquadCount)
			m_wSquadCount.SetText(m_aSquads.Count().ToString());

		IEntity body = SCR_PlayerController.GetLocalControlledEntity();
		float range = COA_TacticalCamera.GetRange();
		float now = m_World.GetWorldTime();

		for (int rowIndex = 0, squadCount = m_aSquads.Count(); rowIndex < squadCount; rowIndex++)
		{
			Widget row = GetRow(rowIndex);
			if (!row)
				break;

			SCR_AIGroup squad = m_aSquads[rowIndex];
			row.SetVisible(true);

			COA_TacticalSquadRowComponent rowComponent = COA_TacticalSquadRowComponent.Cast(row.FindHandler(COA_TacticalSquadRowComponent));
			if (rowComponent)
				rowComponent.SetSquad(squad, this, m_aSelectedSquads.Contains(squad));

			// "6 players - 240 m"
			string info = string.Format("%1 players", squad.GetPlayerCount());
			bool outOfRange;
			vector squadPosition;
			if (body && COA_TacticalCamera.GetSquadPosition(squad, squadPosition))
			{
				float distance = vector.DistanceXZ(body.GetOrigin(), squadPosition);
				info += string.Format("  -  %1 m", Math.Round(distance));
				outOfRange = distance > range;
				if (outOfRange)
					info += " (out of range)";
			}

			SetRowText(row, "Name", COA_TacticalCamera.GetSquadName(squad), m_TextColor);
			if (outOfRange)
				SetRowText(row, "Info", info, m_WarningColor);
			else
				SetRowText(row, "Info", info, m_MutedColor);

			// "MOVE - 3 min ago (PL Smith)"
			COA_TacticalIssuedOrder order = COA_TacticalOrderBoard.FindForGroup(squad);
			if (order)
			{
				int minutesAgo = Math.Floor((now - order.m_fIssuedAt) / 60000);
				string age = "just now";
				if (minutesAgo > 0)
					age = string.Format("%1 min ago", minutesAgo);

				SetRowText(row, "Order", string.Format("%1  -  %2  (%3)", COA_TacticalCamera.GetOrderName(order.m_eOrder), age, order.m_sIssuer), m_SelectedColor);
			}
			else
			{
				SetRowText(row, "Order", "No order", m_MutedColor);
			}
		}

		for (int hideIndex = m_aSquads.Count(), rowCount = m_aRows.Count(); hideIndex < rowCount; hideIndex++)
		{
			m_aRows[hideIndex].SetVisible(false);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected Widget GetRow(int index)
	{
		while (m_aRows.Count() <= index)
		{
			Widget newRow = m_Workspace.CreateWidgets(m_SquadRowLayout, m_wSquadList);
			if (!newRow)
				return null;

			m_aRows.Insert(newRow);
		}

		return m_aRows[index];
	}

	//------------------------------------------------------------------------------------------------
	protected void SetRowText(Widget row, string widgetName, string text, Color color)
	{
		TextWidget textWidget = TextWidget.Cast(row.FindAnyWidget(widgetName));
		if (!textWidget)
			return;

		textWidget.SetText(text);
		textWidget.SetColor(color);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateCameraInfo(IEntity body)
	{
		if (!m_wCameraInfo)
			return;

		SCR_ManualCamera camera = SCR_CameraEditorComponent.GetCameraInstance();
		if (!camera || !body)
		{
			m_wCameraInfo.SetText(string.Empty);
			return;
		}

		vector cameraPosition = camera.GetOrigin();
		float distance = vector.DistanceXZ(cameraPosition, body.GetOrigin());
		float height = cameraPosition[1] - m_World.GetSurfaceY(cameraPosition[0], cameraPosition[2]);

		m_wCameraInfo.SetText(string.Format("Camera  %1 / %2 m from you  -  %3 / %4 m high",
			Math.Round(distance), Math.Round(COA_TacticalCamera.GetRange()),
			Math.Round(Math.Max(height, 0)), Math.Round(COA_TacticalCamera.GetMaxHeight())));
	}

	//------------------------------------------------------------------------------------------------
	protected void DrawRangeRing(vector center)
	{
		float range = COA_TacticalCamera.GetRange();
		vector ring[97];
		for (int i = 0; i <= RING_SEGMENTS; i++)
		{
			float angle = i * Math.PI2 / RING_SEGMENTS;
			vector point = Vector(center[0] + Math.Sin(angle) * range, 0, center[2] + Math.Cos(angle) * range);
			point[1] = m_World.GetSurfaceY(point[0], point[2]) + RING_HEIGHT;
			ring[i] = point;
		}

		Shape.CreateLines(ARGB(200, 255, 196, 64), ShapeFlags.VISIBLE | ShapeFlags.ONCE | ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP, ring, RING_SEGMENTS + 1);
	}

	//------------------------------------------------------------------------------------------------
	//! A pole at each ordered spot, a line from the squad to it and a label on top
	protected void DrawOrders()
	{
		ShapeFlags flags = ShapeFlags.VISIBLE | ShapeFlags.ONCE | ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP;
		vector pole[2];
		vector path[2];

		foreach (COA_TacticalIssuedOrder order : m_aOrders)
		{
			if (!order || !order.m_Group)
				continue;

			int color = GetOrderColor(order.m_eOrder);
			vector target = order.m_vPosition;
			target[1] = m_World.GetSurfaceY(target[0], target[2]);

			pole[0] = target;
			pole[1] = target + Vector(0, ORDER_POLE_HEIGHT, 0);
			Shape.CreateLines(color, flags, pole, 2);

			vector squadPosition;
			if (COA_TacticalCamera.GetSquadPosition(order.m_Group, squadPosition))
			{
				path[0] = squadPosition + Vector(0, RING_HEIGHT, 0);
				path[1] = target + Vector(0, RING_HEIGHT, 0);
				Shape.CreateLines(color, flags, path, 2);
			}

			ShowLabel(pole[1], string.Format("%1: %2", order.m_sSquadName, COA_TacticalCamera.GetOrderName(order.m_eOrder)), "by " + order.m_sIssuer, m_SelectedColor);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void DrawSquadLabels()
	{
		foreach (SCR_AIGroup squad : m_aSquads)
		{
			vector squadPosition;
			if (!COA_TacticalCamera.GetSquadPosition(squad, squadPosition))
				continue;

			string subtitle = string.Format("%1 players", squad.GetPlayerCount());
			COA_TacticalIssuedOrder order = COA_TacticalOrderBoard.FindForGroup(squad);
			if (order)
				subtitle += "  -  " + COA_TacticalCamera.GetOrderName(order.m_eOrder);

			Color titleColor = m_TextColor;
			if (m_aSelectedSquads.Contains(squad))
				titleColor = m_SelectedColor;

			ShowLabel(squadPosition + Vector(0, LABEL_HEIGHT, 0), COA_TacticalCamera.GetSquadName(squad), subtitle, titleColor);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Place the next pooled label above a world position (hidden when it is behind the camera)
	protected void ShowLabel(vector worldPosition, string title, string subtitle, Color titleColor)
	{
		if (!m_wLabels)
			return;

		vector screenPosition = m_Workspace.ProjWorldToScreen(worldPosition, m_World);
		if (screenPosition[2] <= 0)
			return;

		while (m_aLabels.Count() <= m_iLabelsUsed)
		{
			Widget newLabel = m_Workspace.CreateWidgets(m_LabelLayout, m_wLabels);
			if (!newLabel)
				return;

			FrameSlot.SetAlignment(newLabel, 0.5, 1);
			FrameSlot.SetSizeToContent(newLabel, true);
			m_aLabels.Insert(newLabel);
		}

		Widget label = m_aLabels[m_iLabelsUsed];
		m_iLabelsUsed++;

		FrameSlot.SetPos(label, screenPosition[0], screenPosition[1]);
		label.SetVisible(true);
		SetRowText(label, "Title", title, titleColor);
		SetRowText(label, "Subtitle", subtitle, m_MutedColor);
	}

	//------------------------------------------------------------------------------------------------
	protected int GetOrderColor(COA_ETacticalOrder order)
	{
		int red, green, blue;
		COA_TacticalCamera.GetOrderColorSRGB(order, red, green, blue);
		return ARGB(230, red, green, blue);
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerAttachedScripted(Widget w)
	{
		super.HandlerAttachedScripted(w);

		m_Workspace = GetGame().GetWorkspace();
		m_World = GetGame().GetWorld();

		m_wSquadList = w.FindAnyWidget(m_sSquadListName);
		m_wSquadCount = TextWidget.Cast(w.FindAnyWidget(m_sSquadCountName));
		m_wLabels = w.FindAnyWidget(m_sLabelsName);
		m_wCameraInfo = TextWidget.Cast(w.FindAnyWidget(m_sCameraInfoName));
		m_wSelectionInfo = TextWidget.Cast(w.FindAnyWidget(m_sSelectionInfoName));

		MenuRootBase menu = GetMenu();
		if (menu)
			menu.GetOnMenuUpdate().Insert(OnMenuUpdate);
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerDeattached(Widget w)
	{
		super.HandlerDeattached(w);

		MenuRootBase menu = GetMenu();
		if (menu)
			menu.GetOnMenuUpdate().Remove(OnMenuUpdate);
	}
}

//------------------------------------------------------------------------------------------------
//! One row of the Tactical Camera squad list (COA_TacticalSquadRow.layout): click to select the
//! squad and fly the camera to it.
class COA_TacticalSquadRowComponent : ScriptedWidgetComponent
{
	[Attribute("RowBG")]
	protected string m_sBackgroundName;

	[Attribute("RowAccent")]
	protected string m_sAccentName;

	protected Widget m_wRoot;
	protected SCR_AIGroup m_Squad;
	protected COA_TacticalHUDEditorUIComponent m_HUD;
	protected bool m_bSelected;
	protected bool m_bHovered;

	protected ref Color m_DefaultColor = Color.FromSRGBA(14, 16, 22, 225);
	protected ref Color m_HoveredColor = Color.FromSRGBA(34, 38, 50, 235);
	protected ref Color m_SelectedColor = Color.FromSRGBA(52, 40, 18, 240);

	//------------------------------------------------------------------------------------------------
	void SetSquad(SCR_AIGroup squad, COA_TacticalHUDEditorUIComponent hud, bool selected)
	{
		m_Squad = squad;
		m_HUD = hud;
		m_bSelected = selected;
		UpdateLook();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateLook()
	{
		if (!m_wRoot)
			return;

		Widget background = m_wRoot.FindAnyWidget(m_sBackgroundName);
		if (background)
		{
			if (m_bSelected)
				background.SetColor(m_SelectedColor);
			else if (m_bHovered)
				background.SetColor(m_HoveredColor);
			else
				background.SetColor(m_DefaultColor);
		}

		Widget accent = m_wRoot.FindAnyWidget(m_sAccentName);
		if (accent)
			accent.SetVisible(m_bSelected);
	}

	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button != 0 || !m_HUD)
			return false;

		m_HUD.FocusSquad(m_Squad);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		m_bHovered = true;
		UpdateLook();
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		m_bHovered = false;
		UpdateLook();
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wRoot = w;
		UpdateLook();
	}
}
