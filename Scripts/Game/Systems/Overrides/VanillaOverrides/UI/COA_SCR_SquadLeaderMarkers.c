
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
	protected Widget m_wNATOSymbolRoot;
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

		m_wNATOSymbolRoot = GetGame().GetWorkspace().CreateWidgets(NATO_SYMBOL_LAYOUT, m_wRoot.GetParent());
		if (!m_wNATOSymbolRoot)
			return;

		// Hidden but still laid out, so we can keep measuring where it sits
		m_wVanillaIcon.SetOpacity(0);
		m_wNATOSymbolRoot.SetVisible(false);
		m_bNATOPlacementKnown = false;
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete()
	{
		super.OnDelete();

		if (m_wNATOSymbolRoot)
			m_wNATOSymbolRoot.RemoveFromHierarchy();

		m_wNATOSymbolRoot = null;
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
