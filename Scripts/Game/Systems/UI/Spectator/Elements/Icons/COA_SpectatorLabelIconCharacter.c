//! Floating spectator marker over a character: a dark ring around a faction-coloured disc with the
//! role glyph, a state badge (unconscious) and a rounded name pill above it.
//! Layout: UI/Spectator/SpectatorLabelIconCharacter.layout
class COA_SpectatorLabelIconCharacter : COA_SpectatorLabelIcon
{
	//------------------------------------------------------------------------------------------------
	// Class member variables
	//------------------------------------------------------------------------------------------------

	// Character state
	protected bool m_bDead = false;
	protected bool m_bWounded = false;
	protected bool m_bGhost = false;			// a spectator entity, drawn as a small neutral dot
	protected float m_fClickIgnoreTime;
	protected float m_fMarkerSize = -1;
	protected float m_fNextNameRefresh;
	protected float m_fNameOpacity;

	// Spectator "Names" cycle (COA_SpecToggleNames). Static so it applies to every icon and survives
	// respawning into spectator again.
	protected static COA_ESpecNameMode s_eNameMode = COA_ESpecNameMode.WHEN_CLOSE;

	// Spectator menu reference (set by SetIconForEntity so clicks can call SelectSpecCursor directly)
	protected COA_SpectatorMenu m_SpectatorMenu;

	// Character components
	protected SCR_ChimeraCharacter m_eChimeraCharacter;
	protected SCR_CharacterControllerComponent m_ControllerComponent;
	protected SCR_EditableCharacterComponent m_EditableCharacterComponent;
	protected COA_SlotData m_SlotData;			// cached once found, a character never changes slot
	protected COA_Gamemode m_Gamemode;

	// UI widgets
	protected ImageWidget m_wRing;			// only shown (amber) while unconscious
	protected ImageWidget m_wSpectatorLabelIconBackground;	// faction disc
	protected ImageWidget m_wStateBadge;
	protected ImageWidget m_wSpectatorLabelIconWounded;
	protected ImageWidget m_wLabelAccent;

	// Appearance
	protected ref Color m_FactionColor = new Color(0.3968, 0.4564, 0.6038, 1);

	protected static const ResourceName ATLAS = "{F3A9B47F55BE8D2B}UI/Images/SpecImages/Spec_Atlas_x64.imageset";

	protected static const float CHAR_ICON_FADE_NEAR = 250.0; // fully opaque closer than this
	protected static const float CHAR_ICON_FADE_FAR  = 500.0; // fully transparent beyond this

	protected static const float MARKER_MIN = 14.0;			// dot diameter far away
	protected static const float MARKER_MAX = 22.0;			// dot diameter up close
	protected static const float ROLE_ICON_SCALE = 0.7;		// role icon inside the dot, relative to the dot
	protected static const float HOVER_SCALE = 1.3;			// dot grows while hovered
	protected static const float NAME_FULL_DISTANCE = 20.0;	// name fully shown closer than this
	protected static const float NAME_HIDDEN_DISTANCE = 45.0;	// name hidden beyond this (hover still shows it)
	protected static const float MARKER_SCALE_NEAR = 8.0;	// distance at which the dot is largest
	protected static const float MARKER_SCALE_FAR = 300.0;	// distance at which the dot is smallest
	protected static const float HIT_AREA = 24.0;			// hover / click target, at least the dot's size
	protected static const float LABEL_HEIGHT = 1.1;		// metres above the tracked bone (chest) the name sits, clear of the head
	protected static const float RING = 2.0;				// ring thickness
	protected static const float LABEL_GAP = 2.0;

	//------------------------------------------------------------------------------------------------
	// Palette (linear)
	//------------------------------------------------------------------------------------------------
	protected static ref Color s_HoverColor = new Color(0.8632, 0.8879, 0.9301, 1);
	protected static ref Color s_WoundedColor = new Color(0.8070, 0.4020, 0.0648, 1);
	protected static ref Color s_DeadColor = new Color(0.0262, 0.0284, 0.0343, 1);
	protected static ref Color s_GhostColor = new Color(0.3968, 0.4564, 0.6038, 0.7);

	//------------------------------------------------------------------------------------------------
	// Widget initialization
	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		m_wRing = ImageWidget.Cast(w.FindAnyWidget("Ring"));
		m_wSpectatorLabelIconBackground = ImageWidget.Cast(w.FindAnyWidget("SpectatorLabelIconBackground"));
		m_wStateBadge = ImageWidget.Cast(w.FindAnyWidget("StateBadge"));
		m_wSpectatorLabelIconWounded = ImageWidget.Cast(w.FindAnyWidget("SpectatorLabelIconWounded"));
		m_wLabelAccent = ImageWidget.Cast(w.FindAnyWidget("LabelAccent"));

		m_Gamemode = COA_Gamemode.GetInstance();

		super.HandlerAttached(w);
	}

	//------------------------------------------------------------------------------------------------
	static COA_ESpecNameMode GetNameMode()
	{
		return s_eNameMode;
	}

	//------------------------------------------------------------------------------------------------
	//! when close -> always -> hidden -> when close; returns the new mode
	static COA_ESpecNameMode CycleNameMode()
	{
		if (s_eNameMode == COA_ESpecNameMode.WHEN_CLOSE)
			s_eNameMode = COA_ESpecNameMode.ALWAYS;
		else if (s_eNameMode == COA_ESpecNameMode.ALWAYS)
			s_eNameMode = COA_ESpecNameMode.HIDDEN;
		else
			s_eNameMode = COA_ESpecNameMode.WHEN_CLOSE;

		return s_eNameMode;
	}

	//------------------------------------------------------------------------------------------------
	//! True while the cursor is on this (visible, non-spectator) player's dot - drives the hover card
	bool IsHoveredNow()
	{
		return m_bForceShowName && !m_bGhost && m_wRoot && m_wRoot.IsEnabled();
	}

	//------------------------------------------------------------------------------------------------
	// Returns the button component from the label
	//------------------------------------------------------------------------------------------------
	SCR_ButtonTextComponent GetButton()
	{
		if (!m_wRoot)
			return null;

		Widget buttonWidget = m_wRoot.FindAnyWidget("LabelButton");
		if (!buttonWidget)
			return null;

		return SCR_ButtonTextComponent.Cast(buttonWidget.FindHandler(SCR_ButtonTextComponent));
	}

	//------------------------------------------------------------------------------------------------
	// Store a reference to the spectator menu so clicks can call SelectSpecCursor
	//------------------------------------------------------------------------------------------------
	void SetSpectatorMenu(COA_SpectatorMenu menu)
	{
		m_SpectatorMenu = menu;
	}

	//------------------------------------------------------------------------------------------------
	// Called when the player left-clicks this icon.
	// Bypasses cursor hit-testing by using the already-known m_eEntity directly.
	//------------------------------------------------------------------------------------------------
	void OnLMBClicked()
	{
		if (m_SpectatorMenu && m_eEntity)
			m_SpectatorMenu.SelectSpecCursor(m_eEntity);
	}

	//------------------------------------------------------------------------------------------------
	// Override Update to apply far-distance fade-out on character icons
	//------------------------------------------------------------------------------------------------
	override void Update()
	{
		// The name only shows while the dot is hovered; checked directly rather than relying on the
		// button's mouse events reaching this handler
		m_bForceShowName = m_wLabelButton && WidgetManager.GetWidgetUnderCursor() == m_wLabelButton;

		// Base: world-position projection, distance, hard-culling, label fade, sizing and z-order
		super.Update();

		// "Hidden" names mode wins over hover (hover still enlarges the dot)
		if (s_eNameMode == COA_ESpecNameMode.HIDDEN && m_wSpectatorLabel)
			m_wSpectatorLabel.SetOpacity(0.0);
		vector screenPosition = GetGame().GetWorkspace().ProjWorldToScreen(m_vWorldPosition, GetGame().GetWorld());

		// Soft-fade icons as the camera pulls away, so they dissolve rather than pop out
		if (!m_eEntity || screenPosition[2] < 0 || m_fDistanceToIcon >= CHAR_ICON_FADE_FAR)
		{
			m_wRoot.SetOpacity(0.0);
			m_wRoot.SetEnabled(false); // Disable so the invisible widget cannot block player clicks
			return;
		}

		float opacity = 1.0;
		if (m_fDistanceToIcon > CHAR_ICON_FADE_NEAR)
			opacity = 1.0 - (m_fDistanceToIcon - CHAR_ICON_FADE_NEAR) / (CHAR_ICON_FADE_FAR - CHAR_ICON_FADE_NEAR);
		if (m_bDead)
			opacity *= 0.75;

		m_wRoot.SetOpacity(opacity);
	}

	//------------------------------------------------------------------------------------------------
	// Set up the entity associated with this label
	//------------------------------------------------------------------------------------------------
	override void SetEntity(IEntity entity, string boneName)
	{
		super.SetEntity(entity, boneName);

		if (!entity)
			return;

		m_eChimeraCharacter = SCR_ChimeraCharacter.Cast(entity);
		if (!m_eChimeraCharacter)
			return;

		m_ControllerComponent = SCR_CharacterControllerComponent.Cast(m_eChimeraCharacter.FindComponent(SCR_CharacterControllerComponent));
		m_EditableCharacterComponent = SCR_EditableCharacterComponent.Cast(m_eChimeraCharacter.FindComponent(SCR_EditableCharacterComponent));

		SetupFactionColors();
		SetupSpectatorView();
		LoadCharacterIcon();
		ApplyStateColors();
	}

	//------------------------------------------------------------------------------------------------
	// Setup faction-based colors
	//------------------------------------------------------------------------------------------------
	protected void SetupFactionColors()
	{
		if (!m_eChimeraCharacter)
			return;

		SCR_Faction faction = SCR_Faction.Cast(m_eChimeraCharacter.GetFaction());
		if (faction)
			m_FactionColor = faction.GetFactionColor();	// exact faction colour, same as the squad icons

		if (m_wLabelAccent)
			m_wLabelAccent.SetColor(m_FactionColor);
	}

	//------------------------------------------------------------------------------------------------
	// Spectator entities (other spectators' free cameras) are a small neutral dot without a label
	//------------------------------------------------------------------------------------------------
	protected void SetupSpectatorView()
	{
		if (!m_EditableCharacterComponent)
			return;

		if (!COA_EntityHelper.IsSpectator(m_EditableCharacterComponent.GetOwner()))
			return;

		m_bGhost = true;
		m_fMinIconOpacity = 1;
		m_fMaxIconOpacity = 1;
		m_fMaxIconDistance = 425;
		m_fMaxLabelDistance = 425;

		if (m_wSpectatorLabel)
			m_wSpectatorLabel.SetVisible(false);
		if (m_wSpectatorLabelIcon)
			m_wSpectatorLabelIcon.SetVisible(false);
		if (m_wRing)
			m_wRing.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	// Load the character's role glyph (editor UI info icon)
	//------------------------------------------------------------------------------------------------
	protected void LoadCharacterIcon()
	{
		if (!m_EditableCharacterComponent || !m_wSpectatorLabelIcon)
			return;

		SCR_UIInfo uiInfo = m_EditableCharacterComponent.GetInfo();
		if (uiInfo)
			uiInfo.SetIconTo(m_wSpectatorLabelIcon);

		m_wSpectatorLabelIcon.SetColor(s_HoverColor);
	}

	//------------------------------------------------------------------------------------------------
	// Update the label with current character information
	//------------------------------------------------------------------------------------------------
	override void UpdateLabel()
	{
		super.UpdateLabel();

		UpdateCharacterState();

		// Names only change on slot / connection changes, no need to rebuild the string every frame
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fNextNameRefresh)
			return;

		m_fNextNameRefresh = now + 1000;
		UpdatePlayerName();
	}

	//------------------------------------------------------------------------------------------------
	// Update the label with the player's name (slot name when nobody is in it)
	//------------------------------------------------------------------------------------------------
	protected void UpdatePlayerName()
	{
		if (!m_eEntity || !m_wSpectatorLabelText || m_bGhost)
			return;

		if (!m_SlotData)
		{
			RplComponent rplComponent = RplComponent.Cast(m_eEntity.FindComponent(RplComponent));
			if (rplComponent)
				m_SlotData = COA_SlottingManager.GetInstance().GetSlotDataFromCharacter(rplComponent.Id());
		}

		string displayName;
		int playerId;
		if (m_SlotData)
			playerId = m_SlotData.GetSlotCurrentPlayerId();

		if (playerId > 0)
		{
			displayName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			if (displayName.IsEmpty())
				displayName = "Disconnected";
		}
		else if (m_SlotData)
		{
			displayName = m_SlotData.GetSlotName();
		}

		if (m_bDead && !displayName.IsEmpty())
			displayName = "† " + displayName;

		m_wSpectatorLabelText.SetText(displayName);
		if (m_bDead)
			m_wSpectatorLabelText.SetOpacity(0.55);
		else
			m_wSpectatorLabelText.SetOpacity(1.0);
	}

	//------------------------------------------------------------------------------------------------
	// Track unconscious / dead and recolour on change
	//------------------------------------------------------------------------------------------------
	protected void UpdateCharacterState()
	{
		if (!m_ControllerComponent || m_bGhost)
			return;

		bool dead = m_ControllerComponent.IsDead();
		bool wounded = !dead && m_ControllerComponent.IsUnconscious();
		if (dead == m_bDead && wounded == m_bWounded)
			return;

		m_bDead = dead;
		m_bWounded = wounded;
		m_fNextNameRefresh = 0;		// adds / removes the † right away
		ApplyStateColors();
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyStateColors()
	{
		if (m_wSpectatorLabelIconBackground)
		{
			if (m_bGhost)
				m_wSpectatorLabelIconBackground.SetColor(s_GhostColor);
			else if (m_bDead)
				m_wSpectatorLabelIconBackground.SetColor(s_DeadColor);
			else
				m_wSpectatorLabelIconBackground.SetColor(m_FactionColor);
		}

		if (m_wSpectatorLabelIcon && !m_bGhost)
		{
			if (m_bDead)
			{
				m_wSpectatorLabelIcon.LoadImageFromSet(0, ATLAS, "Dead");
			}
			else if (m_EditableCharacterComponent)
			{
				SCR_UIInfo uiInfo = m_EditableCharacterComponent.GetInfo();
				if (uiInfo)
					uiInfo.SetIconTo(m_wSpectatorLabelIcon);
			}
			if (m_bDead)
				m_wSpectatorLabelIcon.SetOpacity(0.6);
			else
				m_wSpectatorLabelIcon.SetOpacity(1.0);
		}

		if (m_wStateBadge)
			m_wStateBadge.SetVisible(m_bWounded);
		if (m_wSpectatorLabelIconWounded)
			m_wSpectatorLabelIconWounded.SetVisible(m_bWounded);
		if (m_wLabelAccent)
		{
			if (m_bDead)
				m_wLabelAccent.SetColor(s_DeadColor);
			else
				m_wLabelAccent.SetColor(m_FactionColor);
		}
	}

	//------------------------------------------------------------------------------------------------
	// The name fades in as the camera closes in (full under NAME_FULL_DISTANCE, gone past
	// NAME_HIDDEN_DISTANCE); hovering the dot shows it at any distance (Update() raises it then)
	//------------------------------------------------------------------------------------------------
	override protected void UpdateLabelVisibility()
	{
		float t = (NAME_HIDDEN_DISTANCE - m_fDistanceToIcon) / (NAME_HIDDEN_DISTANCE - NAME_FULL_DISTANCE);
		m_fNameOpacity = Math.Clamp(t, 0.0, 1.0);
		if (s_eNameMode == COA_ESpecNameMode.ALWAYS)
			m_fNameOpacity = 1.0;
		else if (s_eNameMode == COA_ESpecNameMode.HIDDEN)
			m_fNameOpacity = 0.0;

		if (m_wSpectatorLabel)
			m_wSpectatorLabel.SetOpacity(m_fNameOpacity);
	}

	//------------------------------------------------------------------------------------------------
	// A plain faction dot whose size follows distance; the name is placed above the head
	//------------------------------------------------------------------------------------------------
	override protected void UpdateIconAppearance(vector screenPosition)
	{
		float t = 1.0 - (m_fDistanceToIcon - MARKER_SCALE_NEAR) / (MARKER_SCALE_FAR - MARKER_SCALE_NEAR);
		t = Math.Clamp(t, 0.0, 1.0);
		float size = MARKER_MIN + (MARKER_MAX - MARKER_MIN) * t;
		if (m_bForceShowName)
			size *= HOVER_SCALE;

		if (Math.AbsFloat(size - m_fMarkerSize) > 0.5)
			LayoutMarker(size);

		// Amber ring only while unconscious
		if (m_wRing && m_wRing.IsVisible() != m_bWounded)
			m_wRing.SetVisible(m_bWounded);

		if (m_wSpectatorLabel && (m_bForceShowName || m_fNameOpacity > 0))
			PositionLabel(screenPosition);

		FrameSlot.SetPos(m_wRoot, screenPosition[0], screenPosition[1]);

		// Closer objects on top
		m_wRoot.SetZOrder(screenPosition[2] * -10000);
	}

	//------------------------------------------------------------------------------------------------
	//! Centres the name on a point LABEL_HEIGHT above the dot in the world, so it clears the head
	//! at any distance; never closer to the dot than the ring
	protected void PositionLabel(vector screenPosition)
	{
		vector above = m_vWorldPosition;
		above[1] = above[1] + LABEL_HEIGHT;
		vector aboveScreen = GetGame().GetWorkspace().ProjWorldToScreen(above, GetGame().GetWorld());
		float offsetY = Math.Min(aboveScreen[1] - screenPosition[1], -(m_fMarkerSize * 0.5 + RING) - LABEL_GAP);

		float labelSizeX, labelSizeY;
		m_wSpectatorLabel.GetScreenSize(labelSizeX, labelSizeY);
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		float labelW = workspace.DPIUnscale(labelSizeX);
		float labelH = workspace.DPIUnscale(labelSizeY);
		FrameSlot.SetPos(m_wSpectatorLabel, -labelW * 0.5, offsetY - labelH);
	}

	//------------------------------------------------------------------------------------------------
	protected void LayoutMarker(float size)
	{
		m_fMarkerSize = size;
		CenterWidget(m_wRing, size + RING * 2);
		CenterWidget(m_wSpectatorLabelIconBackground, size);
		CenterWidget(m_wLabelButton, Math.Max(HIT_AREA, size));

		// Role icon in the middle of the dot (spectator ghosts stay a plain dot)
		if (m_wSpectatorLabelIcon && !m_bGhost)
		{
			m_wSpectatorLabelIcon.SetVisible(true);
			CenterWidget(m_wSpectatorLabelIcon, size * ROLE_ICON_SCALE);
		}
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
//! Spectator player name display, cycled with COA_SpecToggleNames
enum COA_ESpecNameMode
{
	WHEN_CLOSE,		// fade in as the camera closes in (hover shows them at any distance)
	ALWAYS,			// on every player regardless of distance
	HIDDEN			// never, not even on hover
}
