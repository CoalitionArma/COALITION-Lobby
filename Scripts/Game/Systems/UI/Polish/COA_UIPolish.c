//------------------------------------------------------------------------------------------------
// Shared "modern menu" behaviour for Lobby and CRF menus:
//  - hover/pressed/focus feedback on buttons that have none of their own (COA_UIHoverEffect)
//  - a quick staggered fade-in of a menu's top-level panels when it opens
//
// Menus use it through COA_MenuPolish:
//     OnMenuOpen   (at the end)  m_UIPolish = new COA_MenuPolish(GetRootWidget(), true);
//     OnMenuUpdate               if (m_UIPolish) m_UIPolish.Update(tDelta);
//     OnMenuClose                if (m_UIPolish) m_UIPolish.Cleanup();
//------------------------------------------------------------------------------------------------
class COA_UIPolish
{
	// Entrance: each top-level panel fades in over ~0.3 s, the next one starting a moment later
	protected static const float ENTRANCE_SPEED = 3.3;
	protected static const float ENTRANCE_STAGGER = 0.04;
	protected static const int ENTRANCE_MAX_STAGGERED = 12;

	//------------------------------------------------------------------------------------------------
	//! Give every button under root that has no feedback of its own a COA_UIHoverEffect. Safe to call
	//! repeatedly - buttons that already have one (or their own) are skipped - so menus that build
	//! their lists after opening can simply call it again.
	static void AttachHoverEffects(Widget root)
	{
		if (!root)
			return;

		Widget child = root.GetChildren();
		while (child)
		{
			AttachHoverEffects(child);
			child = child.GetSibling();
		}

		if (ButtonWidget.Cast(root) && NeedsHoverEffect(root))
			COA_UIHoverEffect.AttachAuto(root);
	}

	//------------------------------------------------------------------------------------------------
	//! Buttons that already react to hover are left alone
	protected static bool NeedsHoverEffect(Widget button)
	{
		if (button.FindHandler(COA_UIHoverEffect))
			return false;

		// Vanilla components with their own hover/press colouring
		if (button.FindHandler(ButtonComponent) || button.FindHandler(SCR_ModularButtonComponent))
			return false;

		SCR_ButtonBaseComponent widgetLibraryButton = SCR_ButtonBaseComponent.Cast(button.FindHandler(SCR_ButtonBaseComponent));
		if (widgetLibraryButton && widgetLibraryButton.m_bUseColorization && widgetLibraryButton.m_wBackground)
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Fade root's top-level panels in one after another. Panels a menu has deliberately hidden
	//! (invisible, or not fully opaque) are left as they are.
	static void PlayEntrance(Widget root, notnull array<Widget> animatedWidgets)
	{
		if (!root)
			return;

		int index;
		Widget child = root.GetChildren();
		while (child)
		{
			if (child.IsVisible() && child.GetOpacity() >= 0.99)
			{
				child.SetOpacity(0);
				WidgetAnimationOpacity fade = AnimateWidget.Opacity(child, 1, ENTRANCE_SPEED);
				if (fade)
				{
					fade.SetCurve(EAnimationCurve.EASE_OUT_CUBIC);
					fade.SetDelay(Math.Min(index, ENTRANCE_MAX_STAGGERED) * ENTRANCE_STAGGER);
					animatedWidgets.Insert(child);
					index++;
				}
				else
				{
					child.SetOpacity(1); // animation system unavailable - just show it
				}
			}

			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Stop running entrance animations, leaving those widgets fully shown
	static void StopAnimations(notnull array<Widget> animatedWidgets)
	{
		foreach (Widget widget : animatedWidgets)
		{
			if (!widget)
				continue;

			AnimateWidget.StopAnimation(widget, WidgetAnimationBase);
			widget.SetOpacity(1);
		}

		animatedWidgets.Clear();
	}
}

//------------------------------------------------------------------------------------------------
//! Per-menu helper: applies COA_UIPolish when the menu opens and keeps hover effects applied to
//! buttons the menu creates later (list rows, sub-panels), checking about once a second.
class COA_MenuPolish
{
	protected static const float RESCAN_INTERVAL = 1;

	protected Widget m_wRoot;
	protected ref array<Widget> m_aAnimatedWidgets = {};
	protected float m_fRescanTimer;

	//------------------------------------------------------------------------------------------------
	//! \param[in] root the menu's root widget
	//! \param[in] playEntrance fade the top-level panels in - off for menus that reopen constantly
	void COA_MenuPolish(Widget root, bool playEntrance)
	{
		m_wRoot = root;
		COA_UIPolish.AttachHoverEffects(root);

		// First rescan shortly after opening, for menus that build their content a frame later
		// (e.g. COA_AdminMenu's DelayedMenuInitialization); then once a second
		m_fRescanTimer = RESCAN_INTERVAL - 0.15;

		if (playEntrance)
		{
			Widget content = root;
			// A layout's root frame often holds everything in a single child container
			if (root && root.GetChildren() && !root.GetChildren().GetSibling())
				content = root.GetChildren();

			COA_UIPolish.PlayEntrance(content, m_aAnimatedWidgets);
		}
	}

	//------------------------------------------------------------------------------------------------
	void Update(float tDelta)
	{
		m_fRescanTimer += tDelta;
		if (m_fRescanTimer < RESCAN_INTERVAL)
			return;

		m_fRescanTimer = 0;
		COA_UIPolish.AttachHoverEffects(m_wRoot);
	}

	//------------------------------------------------------------------------------------------------
	void Cleanup()
	{
		COA_UIPolish.StopAnimations(m_aAnimatedWidgets);
	}
}
