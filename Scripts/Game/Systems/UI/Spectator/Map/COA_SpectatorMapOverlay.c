//------------------------------------------------------------------------------------------------
//! What a spectator map pin stands for
enum COA_ESpecMapPinKind
{
	UNIT,
	VEHICLE,
	SQUAD,
	CAMERA,
	KILL
}

//------------------------------------------------------------------------------------------------
//! Handler on a spectator map pin's hit area (COA_SpecMapPin.layout, COA_SpecMapSquad.layout).
//! Owns the pin's widget references and forwards hover / click to the overlay.
class COA_SpecMapPin : ScriptedWidgetComponent
{
	COA_ESpecMapPinKind m_eKind;
	Managed m_Owner;						// COA_SpecMapUnit / COA_SpecMapVehicle / COA_SpecMapSquad
	COA_SpectatorMapOverlay m_Overlay;

	Widget m_wPinRoot;
	protected Widget m_wHitArea;
	protected ImageWidget m_wArrow;
	protected ImageWidget m_wRing;
	protected ImageWidget m_wFill;
	protected ImageWidget m_wIcon;
	protected ImageWidget m_wBadgeBG;
	protected TextWidget m_wBadgeText;
	protected ImageWidget m_wStateIcon;
	protected Widget m_wLabel;
	protected TextWidget m_wLabelText;

	protected bool m_bShown = true;
	protected bool m_bLabelShown = true;
	protected float m_fSize = -1;
	protected bool m_bArrow = true;
	protected bool m_bGlyph = true;
	protected string m_sLabel;
	protected string m_sBadge;

	protected static const float RING = 2.0;
	protected static const float LABEL_GAP = 4.0;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wHitArea = w;
	}

	//------------------------------------------------------------------------------------------------
	void Init(notnull Widget pinRoot, COA_ESpecMapPinKind kind, Managed owner, COA_SpectatorMapOverlay overlay)
	{
		m_wPinRoot = pinRoot;
		m_eKind = kind;
		m_Owner = owner;
		m_Overlay = overlay;

		m_wArrow = ImageWidget.Cast(pinRoot.FindAnyWidget("Arrow"));
		m_wRing = ImageWidget.Cast(pinRoot.FindAnyWidget("Ring"));
		m_wFill = ImageWidget.Cast(pinRoot.FindAnyWidget("Fill"));
		m_wIcon = ImageWidget.Cast(pinRoot.FindAnyWidget("Icon"));
		m_wBadgeBG = ImageWidget.Cast(pinRoot.FindAnyWidget("BadgeBG"));
		m_wBadgeText = TextWidget.Cast(pinRoot.FindAnyWidget("BadgeText"));
		m_wStateIcon = ImageWidget.Cast(pinRoot.FindAnyWidget("StateIcon"));
		m_wLabel = pinRoot.FindAnyWidget("Label");
		m_wLabelText = TextWidget.Cast(pinRoot.FindAnyWidget("LabelText"));

		if (m_wArrow)
			m_wArrow.SetPivot(0.5, 0.5);

		// Kill and camera markers are informational only, and must not swallow map clicks
		if (kind == COA_ESpecMapPinKind.KILL || kind == COA_ESpecMapPinKind.CAMERA)
		{
			m_wHitArea.SetEnabled(false);
			m_wHitArea.SetVisible(false);
		}
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (m_Overlay)
			m_Overlay.OnPinHovered(this, true);

		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (m_Overlay)
			m_Overlay.OnPinHovered(this, false);

		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button != 0 || !m_Overlay)
			return false;

		m_Overlay.OnPinClicked(this);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool IsShown()
	{
		return m_bShown;
	}

	//------------------------------------------------------------------------------------------------
	void SetShown(bool shown)
	{
		if (shown == m_bShown || !m_wPinRoot)
			return;

		m_bShown = shown;
		m_wPinRoot.SetVisible(shown);
	}

	//------------------------------------------------------------------------------------------------
	//! Disc diameter, whether the heading arrow and the glyph are drawn
	void Layout(float size, bool arrow, bool glyph)
	{
		if (Math.AbsFloat(size - m_fSize) < 0.5 && arrow == m_bArrow && glyph == m_bGlyph)
			return;

		m_fSize = size;
		m_bArrow = arrow;
		m_bGlyph = glyph;

		float ring = size + RING * 2;
		CenterWidget(m_wRing, ring);
		CenterWidget(m_wFill, size);
		CenterWidget(m_wIcon, size * 0.66);
		CenterWidget(m_wArrow, ring * 1.5);
		CenterWidget(m_wHitArea, Math.Max(ring, 16));

		if (m_wArrow)
			m_wArrow.SetVisible(arrow);
		if (m_wIcon)
			m_wIcon.SetVisible(glyph);

		// Badge / state glyph on the ring's upper right
		float half = size * 0.5;
		float badge = Math.Max(size * 0.6, 12);
		float badgeX = half - badge * 0.3;
		float badgeY = -half - badge * 0.7;
		if (m_wBadgeBG)
		{
			FrameSlot.SetSize(m_wBadgeBG, badge, badge);
			FrameSlot.SetPos(m_wBadgeBG, badgeX, badgeY);
		}
		if (m_wBadgeText)
		{
			FrameSlot.SetSize(m_wBadgeText, badge, badge);
			FrameSlot.SetPos(m_wBadgeText, badgeX, badgeY);
		}
		if (m_wStateIcon)
		{
			float glyphSize = badge * 0.75;
			FrameSlot.SetSize(m_wStateIcon, glyphSize, glyphSize);
			FrameSlot.SetPos(m_wStateIcon, badgeX + (badge - glyphSize) * 0.5, badgeY + (badge - glyphSize) * 0.5);
		}
	}

	//------------------------------------------------------------------------------------------------
	void SetColors(Color fill, Color ring)
	{
		if (m_wFill)
			m_wFill.SetColor(fill);
		if (m_wRing)
			m_wRing.SetColor(ring);
		if (m_wArrow)
			m_wArrow.SetColor(ring);
	}

	//------------------------------------------------------------------------------------------------
	void SetIconFromInfo(SCR_UIInfo info, ResourceName fallbackSet, string fallbackImage)
	{
		if (!m_wIcon)
			return;

		if (!info || !info.SetIconTo(m_wIcon))
			m_wIcon.LoadImageFromSet(0, fallbackSet, fallbackImage);
	}

	//------------------------------------------------------------------------------------------------
	void SetIconImage(ResourceName imageSet, string image)
	{
		if (m_wIcon)
			m_wIcon.LoadImageFromSet(0, imageSet, image);
	}

	//------------------------------------------------------------------------------------------------
	void SetIconColor(Color color)
	{
		if (m_wIcon)
			m_wIcon.SetColor(color);
	}

	//------------------------------------------------------------------------------------------------
	//! Small glyph on the ring (unconscious / squad leader); empty image hides it
	void SetStateIcon(ResourceName imageSet, string image, Color color)
	{
		if (!m_wStateIcon || !m_wBadgeBG)
			return;

		bool show = !image.IsEmpty();
		m_wStateIcon.SetVisible(show);
		if (m_sBadge.IsEmpty())
			m_wBadgeBG.SetVisible(show);

		if (!show)
			return;

		m_wStateIcon.LoadImageFromSet(0, imageSet, image);
		m_wStateIcon.SetColor(color);
	}

	//------------------------------------------------------------------------------------------------
	//! Count shown in the corner badge (vehicle crew); empty hides it
	void SetBadge(string badge)
	{
		if (badge == m_sBadge || !m_wBadgeText || !m_wBadgeBG)
			return;

		m_sBadge = badge;
		bool show = !badge.IsEmpty();
		m_wBadgeText.SetVisible(show);
		m_wBadgeText.SetText(badge);
		m_wBadgeBG.SetVisible(show || (m_wStateIcon && m_wStateIcon.IsVisible()));
	}

	//------------------------------------------------------------------------------------------------
	void SetLabel(string label)
	{
		if (label == m_sLabel || !m_wLabelText)
			return;

		m_sLabel = label;
		m_wLabelText.SetText(label);
	}

	//------------------------------------------------------------------------------------------------
	void SetLabelShown(bool shown)
	{
		shown = shown && !m_sLabel.IsEmpty();
		if (shown == m_bLabelShown || !m_wLabel)
			return;

		m_bLabelShown = shown;
		m_wLabel.SetVisible(shown);
	}

	//------------------------------------------------------------------------------------------------
	void SetHeading(float degrees)
	{
		if (m_wArrow && m_bArrow)
			m_wArrow.SetRotation(degrees);
	}

	//------------------------------------------------------------------------------------------------
	//! Moves the pin to an (unscaled) screen point and keeps the name pill centred above the disc
	void SetScreenPos(float x, float y)
	{
		if (!m_wPinRoot)
			return;

		FrameSlot.SetPos(m_wPinRoot, x, y);

		if (!m_bLabelShown || !m_wLabel)
			return;

		float w, h;
		m_wLabel.GetScreenSize(w, h);
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		w = workspace.DPIUnscale(w);
		h = workspace.DPIUnscale(h);
		FrameSlot.SetPos(m_wLabel, -w * 0.5, -(m_fSize * 0.5 + RING) - LABEL_GAP - h);
	}

	//------------------------------------------------------------------------------------------------
	protected static void CenterWidget(Widget w, float size)
	{
		if (!w)
			return;

		FrameSlot.SetSize(w, size, size);
		FrameSlot.SetPos(w, -size * 0.5, -size * 0.5);
	}
}

//------------------------------------------------------------------------------------------------
//! One slotted character on the spectator map
class COA_SpecMapUnit : Managed
{
	RplId m_Id;
	SCR_ChimeraCharacter m_Character;
	Widget m_wRoot;
	COA_SpecMapPin m_Pin;

	int m_iPlayerId;
	string m_sName;
	string m_sRole;
	Faction m_Faction;
	SCR_AIGroup m_Group;
	COA_SpecMapSquad m_Squad;
	COA_SpecMapVehicle m_Vehicle;
	IEntity m_VehicleEntity;
	bool m_bAlive = true;
	bool m_bUncon;
	bool m_bLeader;
	bool m_bSeen;

	// Last applied style, so colours / glyphs are only touched on change
	int m_iStyle = -1;
}

//------------------------------------------------------------------------------------------------
//! A vehicle with at least one visible occupant
class COA_SpecMapVehicle : Managed
{
	RplId m_Id;
	IEntity m_Entity;
	Widget m_wRoot;
	COA_SpecMapPin m_Pin;
	Faction m_Faction;
	string m_sName;
	ref array<COA_SpecMapUnit> m_aCrew = {};
	bool m_bSeen;
}

//------------------------------------------------------------------------------------------------
//! A squad card, shown in place of its members when zoomed out
class COA_SpecMapSquad : Managed
{
	int m_iId;
	SCR_AIGroup m_Group;
	Widget m_wRoot;
	Widget m_wCard;
	COA_SpecMapPin m_Pin;
	ImageWidget m_wFlag;
	ImageWidget m_wAnchorDot;
	TextWidget m_wName;
	TextWidget m_wCount;
	ref array<COA_SpecMapUnit> m_aMembers = {};
	vector m_vAnchor;
	bool m_bHasAnchor;
	bool m_bSeen;
	bool m_bShown = true;
}

//------------------------------------------------------------------------------------------------
//! A recent death, drawn as a fading marker (fed by the spectator kill feed)
class COA_SpecMapKillEvent
{
	vector m_vPosition;
	ref Color m_Color;
	float m_fTime;				// world time, ms
	Widget m_wRoot;
	COA_SpecMapPin m_Pin;
}

//------------------------------------------------------------------------------------------------
//! Everything the spectator sees on the map: players (with heading), vehicles with their crew count,
//! squad cards when zoomed out, the free camera, recent kills and a hover card. Owned by the
//! modded SCR_MapMarkersUI (COA_MapMarker.c), which only drives it while the local player spectates.
//!
//! Detail follows the map zoom (pixels per metre, unscaled):
//!   >= ZOOM_NAMES   every player is named
//!   >= ZOOM_SQUADS  players without names (hover for the card)
//!   <  ZOOM_SQUADS  squads collapse into one card each; members far from their squad stay as dots
class COA_SpectatorMapOverlay : Managed
{
	protected static const ResourceName PIN_LAYOUT = "{6A75150000000001}UI/Spectator/COA_SpecMapPin.layout";
	protected static const ResourceName SQUAD_LAYOUT = "{6A75150000000002}UI/Spectator/COA_SpecMapSquad.layout";
	protected static const ResourceName TOOLTIP_LAYOUT = "{6A75150000000003}UI/Spectator/COA_SpecMapTooltip.layout";
	protected static const ResourceName ATLAS = "{F3A9B47F55BE8D2B}UI/Images/SpecImages/Spec_Atlas_x64.imageset";
	protected static const ResourceName CHARS = "{789DC92AF28E0AB1}UI/Imagesets/Characters/CharacterWrapperUI.imageset";

	protected static const float REFRESH_INTERVAL = 0.25;	// seconds between data refreshes (positions update every frame)
	protected static const float ZOOM_NAMES = 2.5;
	protected static const float ZOOM_SQUADS = 0.7;
	protected static const float STRAGGLER_DISTANCE = 150;	// metres from the squad card before a member is drawn on its own
	protected static const float KILL_LIFETIME = 30;		// seconds
	protected static const int KILL_MAX = 40;
	protected static const float TOOLTIP_WIDTH = 240;		// matches TooltipWidth in COA_SpecMapTooltip.layout

	protected static const int Z_KILL = 10;
	protected static const int Z_SQUAD = 20;
	protected static const int Z_UNIT = 30;
	protected static const int Z_VEHICLE = 40;
	protected static const int Z_FOCUS = 60;
	protected static const int Z_CAMERA = 70;

	// Palette (linear)
	protected static ref Color s_Ring = new Color(0.0033, 0.0037, 0.0052, 0.92);
	protected static ref Color s_White = new Color(0.8632, 0.8879, 0.9301, 1);
	protected static ref Color s_Muted = new Color(0.3968, 0.4564, 0.6038, 1);
	protected static ref Color s_Dead = new Color(0.0262, 0.0284, 0.0343, 1);
	protected static ref Color s_Amber = new Color(0.8070, 0.4020, 0.0648, 1);
	protected static ref Color s_Neutral = new Color(0.3968, 0.4564, 0.6038, 1);

	protected static ref array<ref COA_SpecMapKillEvent> s_aKills = {};

	protected Widget m_wRoot;
	protected SCR_MapEntity m_MapEntity;
	protected WorkspaceWidget m_Workspace;
	protected COA_SpectatorMenu m_SpecMenu;

	protected ref map<RplId, ref COA_SpecMapUnit> m_mUnits = new map<RplId, ref COA_SpecMapUnit>();
	protected ref map<RplId, ref COA_SpecMapVehicle> m_mVehicles = new map<RplId, ref COA_SpecMapVehicle>();
	protected ref map<int, ref COA_SpecMapSquad> m_mSquads = new map<int, ref COA_SpecMapSquad>();

	protected Widget m_wCameraRoot;
	protected COA_SpecMapPin m_CameraPin;

	protected Widget m_wTooltip;
	protected Widget m_wTooltipCard;
	protected TextWidget m_wTooltipTitle;
	protected TextWidget m_wTooltipSub;
	protected TextWidget m_wTooltipState;
	protected RichTextWidget m_wTooltipBody;
	protected Widget m_wTooltipHealthRow;
	protected Widget m_wTooltipHealthFill;
	protected Widget m_wTooltipAccent;
	protected COA_SpecMapPin m_HoveredPin;

	protected float m_fRefreshTimer;
	protected bool m_bOpen;

	//------------------------------------------------------------------------------------------------
	//! Called by the spectator kill feed. Kills outlive the map being closed, so they are kept statically.
	static void RecordKill(vector position, Color color)
	{
		if (position == vector.Zero || !GetGame().GetWorld())
			return;

		COA_SpecMapKillEvent kill = new COA_SpecMapKillEvent();
		kill.m_vPosition = position;
		kill.m_Color = color;
		kill.m_fTime = GetGame().GetWorld().GetWorldTime();
		s_aKills.Insert(kill);

		while (s_aKills.Count() > KILL_MAX)
		{
			if (s_aKills[0].m_wRoot)
				s_aKills[0].m_wRoot.RemoveFromHierarchy();
			s_aKills.RemoveOrdered(0);
		}
	}

	//------------------------------------------------------------------------------------------------
	void Open(Widget root, SCR_MapEntity mapEntity)
	{
		if (m_bOpen)
			Close();

		m_wRoot = root;
		m_MapEntity = mapEntity;
		m_Workspace = GetGame().GetWorkspace();
		m_bOpen = m_wRoot && m_MapEntity;
		m_fRefreshTimer = REFRESH_INTERVAL;		// refresh on the first update
	}

	//------------------------------------------------------------------------------------------------
	void Close()
	{
		foreach (RplId unitId, COA_SpecMapUnit unit : m_mUnits)
		{
			if (unit.m_wRoot)
				unit.m_wRoot.RemoveFromHierarchy();
		}
		m_mUnits.Clear();

		foreach (RplId vehicleId, COA_SpecMapVehicle vehicle : m_mVehicles)
		{
			if (vehicle.m_wRoot)
				vehicle.m_wRoot.RemoveFromHierarchy();
		}
		m_mVehicles.Clear();

		foreach (int squadId, COA_SpecMapSquad squad : m_mSquads)
		{
			if (squad.m_wRoot)
				squad.m_wRoot.RemoveFromHierarchy();
		}
		m_mSquads.Clear();

		foreach (COA_SpecMapKillEvent kill : s_aKills)
		{
			if (kill.m_wRoot)
				kill.m_wRoot.RemoveFromHierarchy();
			kill.m_wRoot = null;
			kill.m_Pin = null;
		}

		if (m_wCameraRoot)
			m_wCameraRoot.RemoveFromHierarchy();
		m_wCameraRoot = null;
		m_CameraPin = null;

		if (m_wTooltip)
			m_wTooltip.RemoveFromHierarchy();
		m_wTooltip = null;
		m_HoveredPin = null;

		m_bOpen = false;
	}

	//------------------------------------------------------------------------------------------------
	bool IsOpen()
	{
		return m_bOpen;
	}

	//------------------------------------------------------------------------------------------------
	void Update(float timeSlice)
	{
		if (!m_bOpen || !m_MapEntity)
			return;

		m_SpecMenu = COA_SpectatorMenu.Cast(GetGame().GetMenuManager().GetTopMenu());

		m_fRefreshTimer += timeSlice;
		if (m_fRefreshTimer >= REFRESH_INTERVAL)
		{
			m_fRefreshTimer = 0;
			Refresh();
		}

		float ppu = m_Workspace.DPIUnscale(m_MapEntity.GetCurrentZoom());
		int detail = 1;
		if (ppu < ZOOM_SQUADS)
			detail = 0;
		else if (ppu >= ZOOM_NAMES)
			detail = 2;

		IEntity followed;
		if (m_SpecMenu)
			followed = m_SpecMenu.GetSpecEntity();

		if (detail == 0)
		{
			foreach (int squadId, COA_SpecMapSquad squad : m_mSquads)
			{
				ComputeSquadAnchor(squad);
			}
		}

		foreach (RplId unitId, COA_SpecMapUnit unit : m_mUnits)
		{
			PositionUnit(unit, detail, followed);
		}

		foreach (RplId vehicleId, COA_SpecMapVehicle vehicle : m_mVehicles)
		{
			PositionVehicle(vehicle, detail, followed);
		}

		foreach (int squadId, COA_SpecMapSquad squad : m_mSquads)
		{
			PositionSquad(squad, detail);
		}

		UpdateCamera(followed);
		UpdateKills();
		PositionTooltip();
	}

	//================================================================================================
	// DATA (REFRESH_INTERVAL)
	//================================================================================================

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return;

		map<int, ref COA_SlotData> slotMap = slottingManager.GetSlotMap();
		if (!slotMap)
			return;

		foreach (RplId unitId, COA_SpecMapUnit unit : m_mUnits)
		{
			unit.m_bSeen = false;
		}

		foreach (int slotId, COA_SlotData slotData : slotMap)
		{
			RplId charId = slotData.GetSlotCurrentCharacter();
			if (!charId.IsValid())
				continue;

			SCR_ChimeraCharacter character = COA_EntityHelper.GetCharacterFromRplId(charId);
			if (!character)
				continue;

			Faction faction = character.GetFaction();
			if (!faction || !ShouldShowFaction(faction))
				continue;

			COA_SpecMapUnit unit = m_mUnits.Get(charId);
			if (!unit || unit.m_Character != character)
			{
				if (unit && unit.m_wRoot)
					unit.m_wRoot.RemoveFromHierarchy();

				unit = CreateUnit(charId, character);
				if (!unit)
					continue;
			}

			unit.m_bSeen = true;
			unit.m_Faction = faction;
			RefreshUnit(unit, slotData);
		}

		array<RplId> stale = {};
		foreach (RplId unitId, COA_SpecMapUnit unit : m_mUnits)
		{
			if (!unit.m_bSeen || !unit.m_Character)
				stale.Insert(unitId);
		}
		foreach (RplId staleUnitId : stale)
		{
			COA_SpecMapUnit staleUnit = m_mUnits.Get(staleUnitId);
			if (staleUnit.m_wRoot)
				staleUnit.m_wRoot.RemoveFromHierarchy();
			m_mUnits.Remove(staleUnitId);
		}

		RebuildVehicles();
		RebuildSquads();

		foreach (RplId unitId, COA_SpecMapUnit unit : m_mUnits)
		{
			StyleUnit(unit);
		}

		RefreshTooltip();
	}

	//------------------------------------------------------------------------------------------------
	protected COA_SpecMapUnit CreateUnit(RplId id, SCR_ChimeraCharacter character)
	{
		COA_SpecMapPin pin;
		Widget root = CreatePin(COA_ESpecMapPinKind.UNIT, Z_UNIT, pin);
		if (!root)
			return null;

		COA_SpecMapUnit unit = new COA_SpecMapUnit();
		unit.m_Id = id;
		unit.m_Character = character;
		unit.m_wRoot = root;
		unit.m_Pin = pin;
		pin.Init(root, COA_ESpecMapPinKind.UNIT, unit, this);
		m_mUnits.Set(id, unit);
		return unit;
	}

	//------------------------------------------------------------------------------------------------
	protected Widget CreatePin(COA_ESpecMapPinKind kind, int z, out COA_SpecMapPin pin)
	{
		if (!m_wRoot)
			return null;

		Widget root = GetGame().GetWorkspace().CreateWidgets(PIN_LAYOUT, m_wRoot);
		if (!root)
			return null;

		Widget hitArea = root.FindAnyWidget("HitArea");
		if (hitArea)
			pin = COA_SpecMapPin.Cast(hitArea.FindHandler(COA_SpecMapPin));

		if (!pin)
		{
			root.RemoveFromHierarchy();
			return null;
		}

		root.SetZOrder(z);
		return root;
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshUnit(COA_SpecMapUnit unit, COA_SlotData slotData)
	{
		unit.m_iPlayerId = slotData.GetSlotCurrentPlayerId();
		unit.m_sRole = slotData.GetSlotName();

		if (unit.m_iPlayerId > 0)
		{
			unit.m_sName = GetGame().GetPlayerManager().GetPlayerName(unit.m_iPlayerId);
			if (unit.m_sName.IsEmpty())
				unit.m_sName = "Disconnected";
		}
		else
		{
			unit.m_sName = unit.m_sRole;
		}

		unit.m_Group = COA_EntityHelper.GetGroupFromRplId(slotData.GetSlotCurrentGroup());
		unit.m_bLeader = unit.m_Group && unit.m_iPlayerId > 0 && unit.m_Group.GetLeaderID() == unit.m_iPlayerId;

		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(unit.m_Character.GetCharacterController());
		unit.m_bAlive = !controller || !controller.IsDead();
		unit.m_bUncon = unit.m_bAlive && controller && controller.IsUnconscious();

		unit.m_VehicleEntity = null;
		if (unit.m_bAlive)
			unit.m_VehicleEntity = CompartmentAccessComponent.GetVehicleIn(unit.m_Character);

		if (unit.m_Pin)
		{
			if (unit.m_bAlive)
				unit.m_Pin.SetLabel(unit.m_sName);
			else
				unit.m_Pin.SetLabel("† " + unit.m_sName);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Colours and glyphs, only re-applied when the unit's state changes
	protected void StyleUnit(COA_SpecMapUnit unit)
	{
		if (!unit.m_Pin)
			return;

		int style = 0;
		if (!unit.m_bAlive)
			style = 1;
		else if (unit.m_bUncon)
			style = 2;
		else if (unit.m_bLeader)
			style = 3;

		if (style == unit.m_iStyle)
			return;

		unit.m_iStyle = style;
		COA_SpecMapPin pin = unit.m_Pin;

		if (!unit.m_bAlive)
		{
			pin.SetColors(s_Dead, s_Ring);
			pin.SetIconImage(ATLAS, "Dead");
			pin.SetIconColor(s_Muted);
			pin.SetStateIcon(ATLAS, "", s_White);
			return;
		}

		pin.SetColors(FactionColor(unit.m_Faction), s_Ring);
		pin.SetIconColor(s_White);

		SCR_EditableCharacterComponent editable = SCR_EditableCharacterComponent.Cast(unit.m_Character.FindComponent(SCR_EditableCharacterComponent));
		SCR_UIInfo info;
		if (editable)
			info = editable.GetInfo();
		pin.SetIconFromInfo(info, CHARS, "Rifleman");

		if (unit.m_bUncon)
			pin.SetStateIcon(ATLAS, "Wounded", s_Amber);
		else if (unit.m_bLeader)
			pin.SetStateIcon(ATLAS, "Leader", s_White);
		else
			pin.SetStateIcon(ATLAS, "", s_White);
	}

	//------------------------------------------------------------------------------------------------
	protected void RebuildVehicles()
	{
		foreach (RplId vehicleId, COA_SpecMapVehicle vehicle : m_mVehicles)
		{
			vehicle.m_bSeen = false;
			vehicle.m_aCrew.Clear();
		}

		foreach (RplId unitId, COA_SpecMapUnit unit : m_mUnits)
		{
			unit.m_Vehicle = null;
			if (!unit.m_VehicleEntity)
				continue;

			RplComponent vehicleRpl = RplComponent.Cast(unit.m_VehicleEntity.FindComponent(RplComponent));
			if (!vehicleRpl)
				continue;

			RplId vehicleId = vehicleRpl.Id();
			COA_SpecMapVehicle vehicle = m_mVehicles.Get(vehicleId);
			if (!vehicle || vehicle.m_Entity != unit.m_VehicleEntity)
			{
				if (vehicle && vehicle.m_wRoot)
					vehicle.m_wRoot.RemoveFromHierarchy();

				vehicle = CreateVehicle(vehicleId, unit.m_VehicleEntity, unit.m_Faction);
				if (!vehicle)
					continue;
			}

			vehicle.m_bSeen = true;
			vehicle.m_aCrew.Insert(unit);
			unit.m_Vehicle = vehicle;
		}

		array<RplId> stale = {};
		foreach (RplId vehicleId, COA_SpecMapVehicle vehicle : m_mVehicles)
		{
			if (!vehicle.m_bSeen || !vehicle.m_Entity)
			{
				stale.Insert(vehicleId);
				continue;
			}

			vehicle.m_Pin.SetBadge(vehicle.m_aCrew.Count().ToString());
		}
		foreach (RplId staleVehicleId : stale)
		{
			COA_SpecMapVehicle staleVehicle = m_mVehicles.Get(staleVehicleId);
			if (staleVehicle.m_wRoot)
				staleVehicle.m_wRoot.RemoveFromHierarchy();
			m_mVehicles.Remove(staleVehicleId);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected COA_SpecMapVehicle CreateVehicle(RplId id, IEntity entity, Faction faction)
	{
		COA_SpecMapPin pin;
		Widget root = CreatePin(COA_ESpecMapPinKind.VEHICLE, Z_VEHICLE, pin);
		if (!root)
			return null;

		COA_SpecMapVehicle vehicle = new COA_SpecMapVehicle();
		vehicle.m_Id = id;
		vehicle.m_Entity = entity;
		vehicle.m_wRoot = root;
		vehicle.m_Pin = pin;
		vehicle.m_Faction = faction;
		pin.Init(root, COA_ESpecMapPinKind.VEHICLE, vehicle, this);

		SCR_UIInfo info;
		SCR_EditableVehicleComponent editable = SCR_EditableVehicleComponent.Cast(entity.FindComponent(SCR_EditableVehicleComponent));
		if (editable)
			info = editable.GetInfo();
		if (info)
			vehicle.m_sName = info.GetName();
		if (vehicle.m_sName.IsEmpty())
			vehicle.m_sName = "Vehicle";

		pin.SetColors(FactionColor(faction), s_Ring);
		pin.SetIconFromInfo(info, ATLAS, "Car");
		pin.SetIconColor(s_White);
		pin.SetLabel(vehicle.m_sName);

		m_mVehicles.Set(id, vehicle);
		return vehicle;
	}

	//------------------------------------------------------------------------------------------------
	protected void RebuildSquads()
	{
		foreach (int squadId, COA_SpecMapSquad squad : m_mSquads)
		{
			squad.m_bSeen = false;
			squad.m_aMembers.Clear();
		}

		foreach (RplId unitId, COA_SpecMapUnit unit : m_mUnits)
		{
			unit.m_Squad = null;
			if (!unit.m_Group)
				continue;

			int groupId = unit.m_Group.GetGroupID();
			COA_SpecMapSquad squad = m_mSquads.Get(groupId);
			if (!squad || squad.m_Group != unit.m_Group)
			{
				if (squad && squad.m_wRoot)
					squad.m_wRoot.RemoveFromHierarchy();

				squad = CreateSquad(groupId, unit.m_Group);
				if (!squad)
					continue;
			}

			squad.m_bSeen = true;
			squad.m_aMembers.Insert(unit);
			unit.m_Squad = squad;
		}

		array<int> stale = {};
		foreach (int squadId, COA_SpecMapSquad squad : m_mSquads)
		{
			if (!squad.m_bSeen || !squad.m_Group)
			{
				stale.Insert(squadId);
				continue;
			}

			int alive;
			foreach (COA_SpecMapUnit member : squad.m_aMembers)
			{
				if (member.m_bAlive)
					alive++;
			}

			if (squad.m_wName)
				squad.m_wName.SetText(SquadName(squad.m_Group));
			if (squad.m_wCount)
				squad.m_wCount.SetText(string.Format("%1/%2", alive, squad.m_aMembers.Count()));
		}
		foreach (int staleSquadId : stale)
		{
			COA_SpecMapSquad staleSquad = m_mSquads.Get(staleSquadId);
			if (staleSquad.m_wRoot)
				staleSquad.m_wRoot.RemoveFromHierarchy();
			m_mSquads.Remove(staleSquadId);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected COA_SpecMapSquad CreateSquad(int id, SCR_AIGroup group)
	{
		if (!m_wRoot)
			return null;

		Widget root = GetGame().GetWorkspace().CreateWidgets(SQUAD_LAYOUT, m_wRoot);
		if (!root)
			return null;

		COA_SpecMapPin pin;
		Widget hitArea = root.FindAnyWidget("HitArea");
		if (hitArea)
			pin = COA_SpecMapPin.Cast(hitArea.FindHandler(COA_SpecMapPin));
		if (!pin)
		{
			root.RemoveFromHierarchy();
			return null;
		}

		root.SetZOrder(Z_SQUAD);

		COA_SpecMapSquad squad = new COA_SpecMapSquad();
		squad.m_iId = id;
		squad.m_Group = group;
		squad.m_wRoot = root;
		squad.m_Pin = pin;
		squad.m_wCard = root.FindAnyWidget("Card");
		squad.m_wFlag = ImageWidget.Cast(root.FindAnyWidget("Flag"));
		squad.m_wAnchorDot = ImageWidget.Cast(root.FindAnyWidget("AnchorDot"));
		squad.m_wName = TextWidget.Cast(root.FindAnyWidget("SquadName"));
		squad.m_wCount = TextWidget.Cast(root.FindAnyWidget("SquadCount"));
		pin.Init(root, COA_ESpecMapPinKind.SQUAD, squad, this);

		Color color = FactionColor(group.GetFaction());
		if (squad.m_wAnchorDot)
			squad.m_wAnchorDot.SetColor(color);

		SCR_Faction faction = SCR_Faction.Cast(group.GetFaction());
		if (squad.m_wFlag)
		{
			squad.m_wFlag.SetColor(color);
			if (faction && !faction.GetGroupFlagImageSet().IsEmpty() && !group.GetGroupFlag().IsEmpty())
				squad.m_wFlag.LoadImageFromSet(0, faction.GetGroupFlagImageSet(), group.GetGroupFlag());
		}

		m_mSquads.Set(id, squad);
		return squad;
	}

	//================================================================================================
	// POSITIONING (EVERY FRAME)
	//================================================================================================

	//------------------------------------------------------------------------------------------------
	//! The squad leader when alive and on foot, otherwise the middle of the members on foot
	protected void ComputeSquadAnchor(COA_SpecMapSquad squad)
	{
		squad.m_bHasAnchor = false;

		vector sum;
		int count;
		foreach (COA_SpecMapUnit member : squad.m_aMembers)
		{
			if (!member.m_bAlive || member.m_Vehicle || !member.m_Character)
				continue;

			if (member.m_bLeader)
			{
				squad.m_vAnchor = member.m_Character.GetOrigin();
				squad.m_bHasAnchor = true;
				return;
			}

			sum = sum + member.m_Character.GetOrigin();
			count++;
		}

		if (count == 0)
			return;

		squad.m_vAnchor = sum * (1.0 / count);
		squad.m_bHasAnchor = true;
	}

	//------------------------------------------------------------------------------------------------
	protected void PositionUnit(COA_SpecMapUnit unit, int detail, IEntity followed)
	{
		COA_SpecMapPin pin = unit.m_Pin;
		if (!pin || !unit.m_Character)
			return;

		bool focused = followed && followed == unit.m_Character;
		bool hovered = m_HoveredPin == pin;

		bool show = true;
		float size = 22;
		bool arrow = unit.m_bAlive;
		bool glyph = true;
		bool label = detail == 2;

		if (unit.m_Vehicle)
		{
			// The vehicle pin stands in for its crew
			show = false;
		}
		else if (!unit.m_bAlive)
		{
			show = detail >= 1 || focused;
			size = 16;
			label = false;
		}
		else if (detail == 0)
		{
			// Zoomed out: the squad card stands in for members near it, stragglers stay as dots
			bool straggler = true;
			COA_SpecMapSquad squad = unit.m_Squad;
			if (squad && squad.m_bHasAnchor)
			{
				vector delta = unit.m_Character.GetOrigin() - squad.m_vAnchor;
				delta[1] = 0;
				straggler = delta.Length() > STRAGGLER_DISTANCE;
			}

			show = straggler;
			size = 14;
			arrow = false;
			glyph = false;
			label = false;
		}
		else if (detail == 2)
		{
			size = 26;
		}

		if (focused)
		{
			show = true;
			size = Math.Max(size, 26);
			glyph = true;
			arrow = unit.m_bAlive;
			label = true;
		}

		pin.SetShown(show);
		if (!show)
			return;

		pin.Layout(size, arrow, glyph);
		pin.SetLabelShown(label || hovered);

		// Followed / hovered pins get a white ring and come to the front
		if (focused || hovered)
		{
			if (unit.m_bAlive)
				pin.SetColors(FactionColor(unit.m_Faction), s_White);
			else
				pin.SetColors(s_Dead, s_White);
			unit.m_wRoot.SetZOrder(Z_FOCUS);
		}
		else
		{
			if (!unit.m_bAlive)
				pin.SetColors(s_Dead, s_Ring);
			else if (unit.m_bUncon)
				pin.SetColors(FactionColor(unit.m_Faction), s_Amber);
			else
				pin.SetColors(FactionColor(unit.m_Faction), s_Ring);
			unit.m_wRoot.SetZOrder(Z_UNIT);
		}

		PlacePin(pin, unit.m_Character.GetOrigin());
		if (arrow)
			pin.SetHeading(Heading(unit.m_Character));
	}

	//------------------------------------------------------------------------------------------------
	protected void PositionVehicle(COA_SpecMapVehicle vehicle, int detail, IEntity followed)
	{
		COA_SpecMapPin pin = vehicle.m_Pin;
		if (!pin || !vehicle.m_Entity)
			return;

		bool focused;
		if (followed)
		{
			foreach (COA_SpecMapUnit crew : vehicle.m_aCrew)
			{
				if (crew.m_Character == followed)
				{
					focused = true;
					break;
				}
			}
		}
		bool hovered = m_HoveredPin == pin;

		float size = 28;
		if (detail == 0)
			size = 22;

		pin.SetShown(true);
		pin.Layout(size, true, true);
		pin.SetLabelShown(detail == 2 || focused || hovered);

		if (focused || hovered)
		{
			pin.SetColors(FactionColor(vehicle.m_Faction), s_White);
			vehicle.m_wRoot.SetZOrder(Z_FOCUS);
		}
		else
		{
			pin.SetColors(FactionColor(vehicle.m_Faction), s_Ring);
			vehicle.m_wRoot.SetZOrder(Z_VEHICLE);
		}

		PlacePin(pin, vehicle.m_Entity.GetOrigin());
		pin.SetHeading(Heading(vehicle.m_Entity));
	}

	//------------------------------------------------------------------------------------------------
	protected void PositionSquad(COA_SpecMapSquad squad, int detail)
	{
		bool show = detail == 0 && squad.m_bHasAnchor;
		if (show != squad.m_bShown)
		{
			squad.m_bShown = show;
			squad.m_wRoot.SetVisible(show);
			squad.m_Pin.SetShown(show);
		}

		if (!show)
			return;

		float x, y;
		WorldToMap(squad.m_vAnchor, x, y);
		FrameSlot.SetPos(squad.m_wRoot, x, y);

		// Card sits centred above its anchor dot
		if (squad.m_wCard)
		{
			float w, h;
			squad.m_wCard.GetScreenSize(w, h);
			FrameSlot.SetPos(squad.m_wCard, -m_Workspace.DPIUnscale(w) * 0.5, -m_Workspace.DPIUnscale(h) - 8);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Free camera marker, only while not following anyone (the followed pin is highlighted instead)
	protected void UpdateCamera(IEntity followed)
	{
		CameraBase camera = GetGame().GetCameraManager().CurrentCamera();
		bool show = camera && !followed;

		if (!m_wCameraRoot)
		{
			if (!show)
				return;

			COA_SpecMapPin cameraPin;
			m_wCameraRoot = CreatePin(COA_ESpecMapPinKind.CAMERA, Z_CAMERA, cameraPin);
			if (!m_wCameraRoot)
				return;

			m_CameraPin = cameraPin;

			m_CameraPin.Init(m_wCameraRoot, COA_ESpecMapPinKind.CAMERA, null, this);
			m_CameraPin.SetColors(s_White, s_Ring);
			m_CameraPin.SetIconImage(ATLAS, "Binoculars");
			m_CameraPin.SetIconColor(s_Ring);
			m_CameraPin.SetLabel("Camera");
			m_CameraPin.Layout(22, true, true);
		}

		m_CameraPin.SetShown(show);
		if (!show)
			return;

		m_CameraPin.SetLabelShown(true);
		PlacePin(m_CameraPin, camera.GetOrigin());
		m_CameraPin.SetHeading(Heading(camera));
	}

	//------------------------------------------------------------------------------------------------
	//! Recent deaths: a pulse when new, then a fading skull in the victim's faction colour
	protected void UpdateKills()
	{
		if (s_aKills.IsEmpty())
			return;

		float now = GetGame().GetWorld().GetWorldTime();
		for (int i = s_aKills.Count() - 1; i >= 0; i--)
		{
			COA_SpecMapKillEvent kill = s_aKills[i];
			float age = (now - kill.m_fTime) / 1000;
			if (age > KILL_LIFETIME || age < 0)
			{
				if (kill.m_wRoot)
					kill.m_wRoot.RemoveFromHierarchy();
				s_aKills.RemoveOrdered(i);
				continue;
			}

			if (!kill.m_wRoot)
			{
				COA_SpecMapPin killPin;
				kill.m_wRoot = CreatePin(COA_ESpecMapPinKind.KILL, Z_KILL, killPin);
				if (!kill.m_wRoot)
					continue;

				kill.m_Pin = killPin;

				kill.m_Pin.Init(kill.m_wRoot, COA_ESpecMapPinKind.KILL, null, this);
				kill.m_Pin.SetColors(kill.m_Color, s_Ring);
				kill.m_Pin.SetIconImage(ATLAS, "Dead");
				kill.m_Pin.SetIconColor(s_White);
				kill.m_Pin.SetLabel("");
				kill.m_Pin.SetLabelShown(false);
			}

			float pulse = Math.Max(0, 1 - age);
			kill.m_Pin.Layout(13 + 12 * pulse, false, true);
			kill.m_wRoot.SetOpacity(Math.Clamp(1.2 - age / KILL_LIFETIME, 0.2, 1));
			PlacePin(kill.m_Pin, kill.m_vPosition);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void PlacePin(COA_SpecMapPin pin, vector worldPos)
	{
		float x, y;
		WorldToMap(worldPos, x, y);
		pin.SetScreenPos(x, y);
	}

	//------------------------------------------------------------------------------------------------
	//! World position to unscaled screen position on the map (same as the scripted map markers)
	protected void WorldToMap(vector worldPos, out float x, out float y)
	{
		int screenX, screenY;
		m_MapEntity.WorldToScreen(worldPos[0], worldPos[2], screenX, screenY, true);
		x = m_Workspace.DPIUnscale(screenX);
		y = m_Workspace.DPIUnscale(screenY);
	}

	//------------------------------------------------------------------------------------------------
	//! Compass heading in degrees, clockwise from north (map up)
	protected static float Heading(IEntity entity)
	{
		vector forward = entity.GetTransformAxis(2);
		return Math.Atan2(forward[0], forward[2]) * Math.RAD2DEG;
	}

	//================================================================================================
	// HOVER CARD
	//================================================================================================

	//------------------------------------------------------------------------------------------------
	void OnPinHovered(COA_SpecMapPin pin, bool hovered)
	{
		if (hovered)
		{
			m_HoveredPin = pin;
			RefreshTooltip();
			PositionTooltip();
		}
		else if (m_HoveredPin == pin)
		{
			m_HoveredPin = null;
			if (m_wTooltip)
				m_wTooltip.SetVisible(false);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool EnsureTooltip()
	{
		if (m_wTooltip)
			return true;

		if (!m_wRoot)
			return false;

		m_wTooltip = GetGame().GetWorkspace().CreateWidgets(TOOLTIP_LAYOUT, m_wRoot);
		if (!m_wTooltip)
			return false;

		m_wTooltipCard = m_wTooltip.FindAnyWidget("Card");
		m_wTooltipTitle = TextWidget.Cast(m_wTooltip.FindAnyWidget("Title"));
		m_wTooltipSub = TextWidget.Cast(m_wTooltip.FindAnyWidget("Sub"));
		m_wTooltipState = TextWidget.Cast(m_wTooltip.FindAnyWidget("State"));
		m_wTooltipBody = RichTextWidget.Cast(m_wTooltip.FindAnyWidget("Body"));
		m_wTooltipHealthRow = m_wTooltip.FindAnyWidget("HealthRow");
		m_wTooltipHealthFill = m_wTooltip.FindAnyWidget("HealthFill");
		m_wTooltipAccent = m_wTooltip.FindAnyWidget("Accent");
		m_wTooltip.SetVisible(false);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshTooltip()
	{
		if (!m_HoveredPin)
			return;

		if (!EnsureTooltip())
			return;

		COA_SpecMapUnit unit = COA_SpecMapUnit.Cast(m_HoveredPin.m_Owner);
		COA_SpecMapVehicle vehicle = COA_SpecMapVehicle.Cast(m_HoveredPin.m_Owner);
		COA_SpecMapSquad squad = COA_SpecMapSquad.Cast(m_HoveredPin.m_Owner);

		if (unit && unit.m_Character)
			FillUnitTooltip(unit);
		else if (vehicle && vehicle.m_Entity)
			FillVehicleTooltip(vehicle);
		else if (squad && squad.m_Group)
			FillSquadTooltip(squad);
		else
		{
			m_wTooltip.SetVisible(false);
			return;
		}

		m_wTooltip.SetVisible(true);
	}

	//------------------------------------------------------------------------------------------------
	protected void FillUnitTooltip(COA_SpecMapUnit unit)
	{
		string sub = unit.m_sRole;
		if (unit.m_Group)
			sub = SquadName(unit.m_Group) + "  ·  " + sub;

		string state;
		if (!unit.m_bAlive)
		{
			state = "Killed in action";
			COA_SlotData slotData = COA_SlottingManager.GetInstance().GetSlotDataFromCharacter(unit.m_Id);
			if (slotData && !slotData.GetKillerName().IsEmpty())
				state = "Killed by " + slotData.GetKillerName();
		}
		else if (unit.m_bUncon)
			state = "Unconscious";
		else if (unit.m_bLeader)
			state = "Squad leader  ·  click to follow";
		else
			state = "Click to follow";

		float health = 0;
		if (unit.m_bAlive)
			health = CharacterHealth(unit.m_Character);

		SetTooltip(unit.m_sName, sub, health, true, state, "", FactionColor(unit.m_Faction));
	}

	//------------------------------------------------------------------------------------------------
	protected void FillVehicleTooltip(COA_SpecMapVehicle vehicle)
	{
		string body;
		foreach (COA_SpecMapUnit crew : vehicle.m_aCrew)
		{
			if (!body.IsEmpty())
				body += "<br/>";
			body += crew.m_sName;
			if (crew.m_bUncon)
				body += "  <color rgba='206,103,17,255'>unconscious</color>";
		}

		float health = 1;
		DamageManagerComponent damage = DamageManagerComponent.Cast(vehicle.m_Entity.FindComponent(DamageManagerComponent));
		if (damage)
			health = damage.GetHealthScaled();

		string sub = string.Format("%1 aboard", vehicle.m_aCrew.Count());
		SetTooltip(vehicle.m_sName, sub, health, true, "Click to follow the crew", body, FactionColor(vehicle.m_Faction));
	}

	//------------------------------------------------------------------------------------------------
	protected void FillSquadTooltip(COA_SpecMapSquad squad)
	{
		string body;
		int alive;
		foreach (COA_SpecMapUnit member : squad.m_aMembers)
		{
			if (!body.IsEmpty())
				body += "<br/>";

			if (!member.m_bAlive)
			{
				body += "<color rgba='120,128,148,255'>† " + member.m_sName + "</color>";
				continue;
			}

			alive++;
			body += member.m_sName;
			if (member.m_bLeader)
				body += "  <color rgba='120,128,148,255'>SL</color>";
			if (member.m_bUncon)
				body += "  <color rgba='206,103,17,255'>unconscious</color>";
		}

		string sub = string.Format("%1 of %2 alive", alive, squad.m_aMembers.Count());
		SetTooltip(SquadName(squad.m_Group), sub, 0, false, "Click to follow the squad leader", body, FactionColor(squad.m_Group.GetFaction()));
	}

	//------------------------------------------------------------------------------------------------
	protected void SetTooltip(string title, string sub, float health, bool showHealth, string state, string body, Color accent)
	{
		if (m_wTooltipTitle)
			m_wTooltipTitle.SetText(title);
		if (m_wTooltipSub)
			m_wTooltipSub.SetText(sub);
		if (m_wTooltipState)
			m_wTooltipState.SetText(state);
		if (m_wTooltipAccent)
			m_wTooltipAccent.SetColor(accent);

		if (m_wTooltipBody)
		{
			m_wTooltipBody.SetVisible(!body.IsEmpty());
			m_wTooltipBody.SetText(body);
		}

		if (m_wTooltipHealthRow)
			m_wTooltipHealthRow.SetVisible(showHealth);

		if (showHealth && m_wTooltipHealthFill)
		{
			health = Math.Clamp(health, 0, 1);
			FrameSlot.SetSizeX(m_wTooltipHealthFill, Math.Max(TOOLTIP_WIDTH * health, 1));
			m_wTooltipHealthFill.SetVisible(health > 0);
			m_wTooltipHealthFill.SetColor(HealthColor(health));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Next to the hovered pin, flipped to the left near the right screen edge
	protected void PositionTooltip()
	{
		if (!m_wTooltip || !m_HoveredPin || !m_HoveredPin.m_wPinRoot)
			return;

		if (!m_HoveredPin.IsShown())
		{
			m_wTooltip.SetVisible(false);
			return;
		}

		float pinX = FrameSlot.GetPosX(m_HoveredPin.m_wPinRoot);
		float pinY = FrameSlot.GetPosY(m_HoveredPin.m_wPinRoot);

		float cardW, cardH;
		if (m_wTooltipCard)
		{
			m_wTooltipCard.GetScreenSize(cardW, cardH);
			cardW = m_Workspace.DPIUnscale(cardW);
			cardH = m_Workspace.DPIUnscale(cardH);
		}

		float screenW, screenH;
		m_wRoot.GetScreenSize(screenW, screenH);
		screenW = m_Workspace.DPIUnscale(screenW);
		screenH = m_Workspace.DPIUnscale(screenH);

		float x = pinX + 22;
		if (x + cardW > screenW - 12)
			x = pinX - 22 - cardW;

		float y = Math.Clamp(pinY - 16, 12, Math.Max(12, screenH - cardH - 12));
		FrameSlot.SetPos(m_wTooltip, x, y);
	}

	//================================================================================================
	// CLICKS
	//================================================================================================

	//------------------------------------------------------------------------------------------------
	void OnPinClicked(COA_SpecMapPin pin)
	{
		if (!m_SpecMenu)
			m_SpecMenu = COA_SpectatorMenu.Cast(GetGame().GetMenuManager().GetTopMenu());
		if (!m_SpecMenu || !pin)
			return;

		IEntity target;
		vector position;

		COA_SpecMapUnit unit = COA_SpecMapUnit.Cast(pin.m_Owner);
		COA_SpecMapVehicle vehicle = COA_SpecMapVehicle.Cast(pin.m_Owner);
		COA_SpecMapSquad squad = COA_SpecMapSquad.Cast(pin.m_Owner);

		if (unit && unit.m_Character)
		{
			if (unit.m_bAlive)
				target = unit.m_Character;
			else
				position = unit.m_Character.GetOrigin();
		}
		else if (vehicle)
		{
			foreach (COA_SpecMapUnit crew : vehicle.m_aCrew)
			{
				if (crew.m_Character && crew.m_bAlive)
				{
					target = crew.m_Character;
					break;
				}
			}
		}
		else if (squad)
		{
			target = SquadFollowTarget(squad);
		}

		// Deferred: following closes the map, which deletes this pin's widgets
		if (target)
			GetGame().GetCallqueue().Call(m_SpecMenu.FollowFromMap, target);
		else if (position != vector.Zero)
			GetGame().GetCallqueue().Call(m_SpecMenu.MoveCameraFromMap, position);
	}

	//------------------------------------------------------------------------------------------------
	protected IEntity SquadFollowTarget(COA_SpecMapSquad squad)
	{
		IEntity fallback;
		foreach (COA_SpecMapUnit member : squad.m_aMembers)
		{
			if (!member.m_Character || !member.m_bAlive)
				continue;

			if (member.m_bLeader)
				return member.m_Character;

			if (!fallback)
				fallback = member.m_Character;
		}

		return fallback;
	}

	//================================================================================================
	// HELPERS
	//================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Respects the gamemode's "hide other spectator factions" setting, like the 3D spectator icons
	protected bool ShouldShowFaction(Faction faction)
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode || !gamemode.m_bHideOtherSpectatorFactions)
			return true;

		Faction localFaction = COA_SlottingManager.GetInstance().GetPlayerSlotFaction(SCR_PlayerController.GetLocalPlayerId());
		if (!localFaction)
			return true;

		return faction == localFaction;
	}

	//------------------------------------------------------------------------------------------------
	protected static Color FactionColor(Faction faction)
	{
		if (faction)
			return faction.GetFactionColor();

		return s_Neutral;
	}

	//------------------------------------------------------------------------------------------------
	protected static string SquadName(SCR_AIGroup group)
	{
		if (!group)
			return "";

		string name = group.GetCustomName();
		if (name.IsEmpty())
			name = group.GetCustomNameWithOriginal();

		return name;
	}

	//------------------------------------------------------------------------------------------------
	//! Blood volume when ACE medical is running (tracks bleeding), vanilla health otherwise
	protected static float CharacterHealth(SCR_ChimeraCharacter character)
	{
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!damage)
			return 1;

		SCR_CharacterBloodHitZone blood = damage.GetBloodHitZone();
		if (blood)
			return blood.GetHealthScaled();

		return damage.GetHealthScaled();
	}

	//------------------------------------------------------------------------------------------------
	//! Same scale as the "Now spectating" card
	protected static Color HealthColor(float health)
	{
		if (health > 0.75)
			return Color.FromSRGBA(92, 196, 128, 255);
		if (health > 0.5)
			return Color.FromSRGBA(232, 170, 72, 255);
		if (health > 0.25)
			return Color.FromSRGBA(232, 128, 64, 255);

		return Color.FromSRGBA(232, 96, 96, 255);
	}
}
