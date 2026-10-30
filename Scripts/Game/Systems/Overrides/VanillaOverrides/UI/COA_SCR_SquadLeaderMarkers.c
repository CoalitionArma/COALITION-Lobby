
modded class SCR_MapMarkerSquadLeader
{	
	// NATO symbol drawn in place of the vanilla group flag icon (see COA_NATOSymbolHelper). It lives on
	// the map frame next to the marker and follows the vanilla icon, which stays in the marker layout
	// (invisible) so the label and the rest of the marker keep their positions.
	protected static const ResourceName NATO_SYMBOL_LAYOUT = "{7C11A4D3E2B95F07}UI/Map/COA_NATOSquadSymbol.layout";
	// Symbol box relative to the vanilla icon's height; the ScaleWidget fits the symbol inside it
	// keeping its own proportions, so the box is wide enough for any frame shape
	protected static const float NATO_SYMBOL_HEIGHT_SCALE = 1.95; // 1.3 x 1.5
	protected static const float NATO_SYMBOL_BOX_ASPECT = 2;
	// Symbol parts darker than this (outline, unit icon, echelon marks) keep their colour
	protected static const float NATO_TINT_MIN_BRIGHTNESS = 0.25;
	// Your own group's symbol gets a gold rim: a gold copy of the symbol drawn behind it, this much bigger
	protected static const float NATO_OWN_BORDER_SCALE = 1.18;
	// COA gold #E8AA48, linear
	protected static const float NATO_OWN_BORDER_R = 0.807;
	protected static const float NATO_OWN_BORDER_G = 0.402;
	protected static const float NATO_OWN_BORDER_B = 0.065;
	protected Widget m_wNATOSymbolRoot;
	protected Widget m_wNATOBorderRoot;
	protected bool m_bNATOIsOwnGroup;
	protected bool m_bNATOBorderBuilt;
	protected ImageWidget m_wVanillaIcon;
	protected float m_fNATOOffsetX;
	protected float m_fNATOOffsetY;
	protected float m_fNATOWidth;
	protected float m_fNATOHeight;
	protected bool m_bNATOPlacementKnown;
	protected int m_iNATOPrevScreenX;
	protected int m_iNATOPrevScreenY;

	//------------------------------------------------------------------------------------------------
	override void OnCreateMarker()
	{
		super.OnCreateMarker();

		if (!m_wRoot || !m_wRoot.GetParent())
			return;

		m_wVanillaIcon = ImageWidget.Cast(m_wRoot.FindAnyWidget("MarkerIcon"));
		if (!m_wVanillaIcon)
			return;

		// Created first so it draws behind the symbol
		m_wNATOBorderRoot = GetGame().GetWorkspace().CreateWidgets(NATO_SYMBOL_LAYOUT, m_wRoot.GetParent());
		m_wNATOSymbolRoot = GetGame().GetWorkspace().CreateWidgets(NATO_SYMBOL_LAYOUT, m_wRoot.GetParent());
		if (!m_wNATOSymbolRoot)
			return;

		// Hidden but still laid out, so we can keep measuring where it sits
		m_wVanillaIcon.SetOpacity(0);
		m_wNATOSymbolRoot.SetVisible(false);
		if (m_wNATOBorderRoot)
			m_wNATOBorderRoot.SetVisible(false);

		m_bNATOPlacementKnown = false;
		m_bNATOIsOwnGroup = false;
		m_bNATOBorderBuilt = false;
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete()
	{
		super.OnDelete();

		if (m_wNATOSymbolRoot)
			m_wNATOSymbolRoot.RemoveFromHierarchy();

		if (m_wNATOBorderRoot)
			m_wNATOBorderRoot.RemoveFromHierarchy();

		m_wNATOSymbolRoot = null;
		m_wNATOBorderRoot = null;
		m_wVanillaIcon = null;
	}

	//------------------------------------------------------------------------------------------------
	override protected void UpdateGroupMilitarySymbol()
	{
		super.UpdateGroupMilitarySymbol();

		if (!m_Group || !m_wNATOSymbolRoot)
			return;

		Widget overlay = m_wNATOSymbolRoot.FindAnyWidget("SymbolOverlay");
		if (!overlay)
			return;

		SCR_MilitarySymbolUIComponent symbolComponent = SCR_MilitarySymbolUIComponent.Cast(overlay.FindHandler(SCR_MilitarySymbolUIComponent));
		if (!symbolComponent)
			return;

		symbolComponent.Update(COA_NATOSymbolHelper.BuildForGroup(m_Group));

		// Keep the faction colour players know from the vanilla marker
		Faction groupFaction = m_Group.GetFaction();
		if (groupFaction)
			TintNATOSymbol(overlay, groupFaction.GetFactionColor());

		// The symbol may have changed - rebuild the gold rim from it next time it's needed
		m_bNATOBorderBuilt = false;
		if (m_bNATOIsOwnGroup)
			BuildNATOBorder();
	}

	//------------------------------------------------------------------------------------------------
	//! Fills the rim layer with the same symbol, every part of it gold, so behind the real symbol only
	//! a gold edge around the frame, echelon marks and the rest shows
	protected void BuildNATOBorder()
	{
		if (!m_Group || !m_wNATOBorderRoot)
			return;

		Widget overlay = m_wNATOBorderRoot.FindAnyWidget("SymbolOverlay");
		if (!overlay)
			return;

		SCR_MilitarySymbolUIComponent symbolComponent = SCR_MilitarySymbolUIComponent.Cast(overlay.FindHandler(SCR_MilitarySymbolUIComponent));
		if (!symbolComponent)
			return;

		symbolComponent.Update(COA_NATOSymbolHelper.BuildForGroup(m_Group));
		ColorNATOBorder(overlay);
		m_bNATOBorderBuilt = true;
	}

	//------------------------------------------------------------------------------------------------
	protected void ColorNATOBorder(Widget widget)
	{
		Widget child = widget.GetChildren();
		while (child)
		{
			ImageWidget image = ImageWidget.Cast(child);
			if (image)
				image.SetColor(new Color(NATO_OWN_BORDER_R, NATO_OWN_BORDER_G, NATO_OWN_BORDER_B, image.GetColor().A()));

			ColorNATOBorder(child);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Whether the local player is in this marker's group. Checked every update, as players can switch
	//! groups without any marker event reaching this one.
	protected void UpdateNATOOwnGroup()
	{
		bool isOwnGroup;
		PlayerController pController = GetGame().GetPlayerController();
		if (m_Group && pController)
			isOwnGroup = m_Group.IsPlayerInGroup(pController.GetPlayerId());

		m_bNATOIsOwnGroup = isOwnGroup;
		if (isOwnGroup && !m_bNATOBorderBuilt)
			BuildNATOBorder();
	}

	//------------------------------------------------------------------------------------------------
	//! Colours the symbol's filled parts. Dark parts - the frame outline, unit icon and echelon
	//! marks - are left alone so they stay readable on top of the fill.
	protected void TintNATOSymbol(Widget widget, Color factionColor)
	{
		Widget child = widget.GetChildren();
		while (child)
		{
			ImageWidget image = ImageWidget.Cast(child);
			if (image)
			{
				Color current = image.GetColor();
				float brightness = Math.Max(current.R(), Math.Max(current.G(), current.B()));
				if (brightness >= NATO_TINT_MIN_BRIGHTNESS)
					image.SetColor(new Color(factionColor.R(), factionColor.G(), factionColor.B(), current.A()));
			}

			TintNATOSymbol(child, factionColor);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnUpdate()
	{
		super.OnUpdate();
		UpdateNATOOwnGroup();
		UpdateNATOSymbolPlacement();
	}

	//------------------------------------------------------------------------------------------------
	//! Puts the symbol over the (invisible) vanilla icon. The icon's screen rect is from the last
	//! layout pass, when the marker sat at the previous screen position, so its offset from that
	//! position is applied to this frame's - no lag while panning the map.
	protected void UpdateNATOSymbolPlacement()
	{
		if (!m_wNATOSymbolRoot || !m_wVanillaIcon || !m_wRoot)
			return;

		float iconX, iconY, iconW, iconH;
		m_wVanillaIcon.GetScreenPos(iconX, iconY);
		m_wVanillaIcon.GetScreenSize(iconW, iconH);

		if (iconW > 0 && iconH > 0)
		{
			m_fNATOOffsetX = iconX - m_iNATOPrevScreenX;
			m_fNATOOffsetY = iconY - m_iNATOPrevScreenY;
			m_fNATOWidth = iconW;
			m_fNATOHeight = iconH;
			m_bNATOPlacementKnown = true;
		}

		m_iNATOPrevScreenX = m_iScreenX;
		m_iNATOPrevScreenY = m_iScreenY;

		bool show = m_bNATOPlacementKnown && m_wRoot.IsVisible() && m_wVanillaIcon.IsVisible();
		m_wNATOSymbolRoot.SetVisible(show);

		bool showBorder = show && m_bNATOIsOwnGroup && m_bNATOBorderBuilt;
		if (m_wNATOBorderRoot)
			m_wNATOBorderRoot.SetVisible(showBorder);

		if (!show)
			return;

		// Centred on where the vanilla icon is, sized from its height (not its box - that is what
		// squashed the symbol)
		float centerX = m_iScreenX + m_fNATOOffsetX + m_fNATOWidth * 0.5;
		float centerY = m_iScreenY + m_fNATOOffsetY + m_fNATOHeight * 0.5;
		float boxHeight = m_fNATOHeight * NATO_SYMBOL_HEIGHT_SCALE;
		float boxWidth = boxHeight * NATO_SYMBOL_BOX_ASPECT;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		FrameSlot.SetPos(m_wNATOSymbolRoot, workspace.DPIUnscale(centerX - boxWidth * 0.5), workspace.DPIUnscale(centerY - boxHeight * 0.5));
		FrameSlot.SetSize(m_wNATOSymbolRoot, workspace.DPIUnscale(boxWidth), workspace.DPIUnscale(boxHeight));

		if (!showBorder)
			return;

		// Same centre, scaled up - the ScaleWidget keeps the symbol's proportions, so the rim follows its shape
		float borderHeight = boxHeight * NATO_OWN_BORDER_SCALE;
		float borderWidth = boxWidth * NATO_OWN_BORDER_SCALE;
		FrameSlot.SetPos(m_wNATOBorderRoot, workspace.DPIUnscale(centerX - borderWidth * 0.5), workspace.DPIUnscale(centerY - borderHeight * 0.5));
		FrameSlot.SetSize(m_wNATOBorderRoot, workspace.DPIUnscale(borderWidth), workspace.DPIUnscale(borderHeight));
	}

	//------------------------------------------------------------------------------------------------
	//! Check whether we are in a squad and if it should be visible on map
	override void UpdateLocalVisibility()
	{
		m_bDoLocalVisibilityUpdate = false;

		PlayerController pController = GetGame().GetPlayerController();
		if (!pController)
			return;

		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupManager)
			return;

		/*
		if (m_Group && !m_Group.m_bBlueForceTrackerEnabled)
			SetLocalVisible(false);
			return;
		*/
		
		SCR_AIGroup localPlayerGroup = groupManager.GetPlayerGroup(pController.GetPlayerId());
		if (!localPlayerGroup)
		{
			SetLocalVisible(false);
			return;
		}
		
		Faction groupFaction = localPlayerGroup.GetFaction();
		if (!groupFaction)
		{
			SetLocalVisible(false);
			return;
		}
		
		if (COA_Gamemode.GetInstance() && !COA_Gamemode.GetInstance().IsSideBFTEnabled(groupFaction.GetFactionKey()))
		{	
			SetLocalVisible(false);
			return;
		}

		bool isLocalPlayerLeader = localPlayerGroup.IsPlayerLeader(pController.GetPlayerId());

		if (isLocalPlayerLeader && CanLeaderSeeOtherLeaders())
		{
			SetLocalVisible(true);
			return;
		}

		if (!isLocalPlayerLeader && (CanMemberSeeOtherLeaders() || localPlayerGroup.IsPlayerInGroup(m_PlayerID)))
		{
			SetLocalVisible(true);
			return;
		}

		SetLocalVisible(false);
	}
}
