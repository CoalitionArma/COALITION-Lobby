//------------------------------------------------------------------------------------------------
//! How a hover effect reacts
enum COA_EHoverStyle
{
	LIGHTEN,	//!< the target's colour gets lighter on hover, more so while pressed
	OVERLAY		//!< the target (a transparent button) fades in a faint white overlay
}

//------------------------------------------------------------------------------------------------
// Hover, pressed and (gamepad) focus feedback for a clickable widget. The button receives the
// events; the target widget smoothly animates its colour with vanilla AnimateWidget.
//
// Attach explicitly:   COA_UIHoverEffect.Attach(button, backgroundWidget);
// or automatically:    COA_UIHoverEffect.AttachAuto(button)   (see COA_UIPolish)
//
// The colour the target returns to is captured when hover starts rather than when the effect is
// attached, so state a menu sets in code (e.g. a selected filter) isn't undone on mouse-leave.
// Focus only highlights with a gamepad: with a mouse a clicked button keeps focus, so a focus
// highlight would stay stuck on after the click (which is why layouts set ButtonComponent's
// focused colour to transparent).
//
// The widget keeps a reference to the handler (Widget.AddHandler), so nothing else needs to.
//------------------------------------------------------------------------------------------------
class COA_UIHoverEffect : ScriptedWidgetEventHandler
{
	// Progress per second - a transition takes about an eighth of a second
	protected static const float ANIMATION_SPEED = 8;

	// LIGHTEN amounts, added to each colour channel. Widget colours are linear (see Color.FromSRGBA),
	// so small amounts are already very visible on dark fills: +0.02 takes #15171D to about #2A2C31.
	protected static const float HOVER_LIGHTEN = 0.02;
	protected static const float PRESS_LIGHTEN = 0.035;

	// Below this alpha a fill is treated as transparent (see ResolveTarget)
	protected static const float MIN_FILL_ALPHA = 0.2;

	protected Widget m_wButton;
	protected Widget m_wTarget;
	protected COA_EHoverStyle m_eStyle;
	protected bool m_bAutoTarget;

	protected ref Color m_RestColor;
	protected bool m_bHovered;
	protected bool m_bFocused;
	protected bool m_bPressed;
	protected bool m_bReturning; // animating back to the rest colour

	//------------------------------------------------------------------------------------------------
	//! Explicit target - e.g. a tile's background behind a transparent button
	static COA_UIHoverEffect Attach(Widget button, Widget target, COA_EHoverStyle style = COA_EHoverStyle.LIGHTEN)
	{
		if (!button || !target || button.FindHandler(COA_UIHoverEffect))
			return null;

		COA_UIHoverEffect effect = new COA_UIHoverEffect();
		effect.m_wButton = button;
		effect.m_wTarget = target;
		effect.m_eStyle = style;
		button.AddHandler(effect);
		return effect;
	}

	//------------------------------------------------------------------------------------------------
	//! Target chosen on first hover, once the layout has real on-screen sizes (see ResolveTarget)
	static COA_UIHoverEffect AttachAuto(Widget button)
	{
		if (!button || button.FindHandler(COA_UIHoverEffect))
			return null;

		COA_UIHoverEffect effect = new COA_UIHoverEffect();
		effect.m_wButton = button;
		effect.m_bAutoTarget = true;
		button.AddHandler(effect);
		return effect;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		BeginActive();
		m_bHovered = true;
		Refresh();
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		m_bHovered = false;
		m_bPressed = false;
		Refresh();
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnFocus(Widget w, int x, int y)
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager && inputManager.IsUsingMouseAndKeyboard())
			return false;

		BeginActive();
		m_bFocused = true;
		Refresh();
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnFocusLost(Widget w, int x, int y)
	{
		if (!m_bFocused)
			return false;

		m_bFocused = false;
		Refresh();
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (button != 0)
			return false;

		BeginActive();
		m_bPressed = true;
		Refresh();
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (button != 0 || !m_bPressed)
			return false;

		m_bPressed = false;
		Refresh();
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Going from idle to active: pick the target if needed and remember its resting colour (unless
	//! it is still animating back to that colour from the last hover)
	protected void BeginActive()
	{
		if (m_bHovered || m_bFocused || m_bPressed)
			return;

		if (!m_wTarget && m_bAutoTarget)
			ResolveTarget();

		if (!m_wTarget || (m_bReturning && m_RestColor))
			return;

		Color current = m_wTarget.GetColor();
		m_RestColor = new Color(current.R(), current.G(), current.B(), current.A());
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		if (!m_wTarget || !m_RestColor)
			return;

		Color color;
		bool returning = false;
		if (m_bPressed)
			color = GetActiveColor(true);
		else if (m_bHovered || m_bFocused)
			color = GetActiveColor(false);
		else
		{
			color = m_RestColor;
			returning = true;
		}

		WidgetAnimationColor animation = AnimateWidget.Color(m_wTarget, color, ANIMATION_SPEED);
		if (!animation)
		{
			m_wTarget.SetColor(color);
			m_bReturning = false;
			return;
		}

		animation.SetCurve(EAnimationCurve.EASE_OUT_CUBIC);
		m_bReturning = returning;
		if (returning)
			animation.GetOnCompleted().Insert(OnReturnCompleted);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnReturnCompleted(WidgetAnimationBase animation)
	{
		m_bReturning = false;
	}

	//------------------------------------------------------------------------------------------------
	protected Color GetActiveColor(bool pressed)
	{
		if (m_eStyle == COA_EHoverStyle.OVERLAY)
		{
			// White overlays blend in linear space - these read like ~10% / 16% in a browser
			if (pressed)
				return new Color(1, 1, 1, 0.035);

			return new Color(1, 1, 1, 0.02);
		}

		float amount = HOVER_LIGHTEN;
		if (pressed)
			amount = PRESS_LIGHTEN;

		return new Color(
			Math.Min(m_RestColor.R() + amount, 1),
			Math.Min(m_RestColor.G() + amount, 1),
			Math.Min(m_RestColor.B() + amount, 1),
			m_RestColor.A());
	}

	//------------------------------------------------------------------------------------------------
	//! 1. A button with a visible fill lightens itself.
	//! 2. Otherwise a background drawn just behind it (an earlier sibling named like a background,
	//!    covering roughly the same area) lightens - the common "transparent button over a panel" setup.
	//! 3. Otherwise the transparent button fades in a faint white overlay.
	protected void ResolveTarget()
	{
		if (m_wButton.GetColor().A() >= MIN_FILL_ALPHA)
		{
			m_wTarget = m_wButton;
			m_eStyle = COA_EHoverStyle.LIGHTEN;
			return;
		}

		Widget background = FindBackgroundBehind();
		if (background)
		{
			m_wTarget = background;
			m_eStyle = COA_EHoverStyle.LIGHTEN;
			return;
		}

		m_wTarget = m_wButton;
		m_eStyle = COA_EHoverStyle.OVERLAY;
	}

	//------------------------------------------------------------------------------------------------
	protected Widget FindBackgroundBehind()
	{
		Widget parent = m_wButton.GetParent();
		if (!parent)
			return null;

		float buttonWidth, buttonHeight;
		m_wButton.GetScreenSize(buttonWidth, buttonHeight);
		float buttonArea = buttonWidth * buttonHeight;
		if (buttonArea <= 0)
			return null;

		Widget found;
		Widget sibling = parent.GetChildren();
		while (sibling && sibling != m_wButton)
		{
			if (IsBackgroundCandidate(sibling))
			{
				float width, height;
				sibling.GetScreenSize(width, height);

				// Same element, not a whole panel the button happens to sit on
				float ratio = (width * height) / buttonArea;
				if (ratio >= 0.5 && ratio <= 2)
					found = sibling; // keep the closest one drawn under the button
			}

			sibling = sibling.GetSibling();
		}

		return found;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsBackgroundCandidate(Widget widget)
	{
		if (!widget.IsVisible() || widget.GetColor().A() < MIN_FILL_ALPHA)
			return false;

		if (!ImageWidget.Cast(widget) && !PanelWidget.Cast(widget))
			return false;

		string name = widget.GetName();
		return name.Contains("BG") || name.Contains("Background") || name == "Image0" || name == "Panel0";
	}
}
