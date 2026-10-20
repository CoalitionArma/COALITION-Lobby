//------------------------------------------------------------------------------------------------
//! How a hover effect reacts
enum COA_EHoverStyle
{
	LIGHTEN,	//!< the target's colour gets lighter on hover, more so while pressed
	OVERLAY,	//!< the target (a transparent button) fades in a faint white overlay
	BRIGHTEN,	//!< the target is an icon or a label - a bigger step towards white
	RAISE		//!< the target is a fill that may be transparent at rest (an unselected nav item) - it fades in
}

//------------------------------------------------------------------------------------------------
// Hover, pressed and (gamepad) focus feedback for a clickable widget. The button receives the
// events; the target widget smoothly animates its colour with vanilla AnimateWidget.
//
// Attach explicitly:   COA_UIHoverEffect.Attach(button, backgroundWidget);
// or automatically:    COA_UIHoverEffect.AttachAuto(button)   (see COA_UIPolish)
//
// The colour the target returns to is captured when hover starts rather than when the effect is
// attached, so state a menu sets in code (e.g. a selected filter) isn't undone on mouse-leave. A menu
// that recolours the target while it may be hovered (a nav item selected by clicking it) sets it with
// COA_UIHoverEffect.SetTargetColor instead, or the running animation paints over it.
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

	// LIGHTEN amounts, added to each channel in sRGB - widget colours are linear (see Color.FromSRGBA),
	// and a fixed linear step that shows on dark fills (#15171D) all but vanishes on bright ones (the
	// accent red), so the step is taken in sRGB where it looks the same on both
	protected static const float HOVER_LIGHTEN = 0.1;
	protected static const float PRESS_LIGHTEN = 0.16;
	protected static const float GAMMA = 2.2;

	// BRIGHTEN amounts (sRGB), for icons and labels
	protected static const float HOVER_BRIGHTEN = 0.22;
	protected static const float PRESS_BRIGHTEN = 0.3;

	// RAISE: least alpha while hovered / pressed
	protected static const float HOVER_RAISE_ALPHA = 0.6;
	protected static const float PRESS_RAISE_ALPHA = 0.85;

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
	//! Recolour a button's hover target from code: becomes the colour it rests at, with the hover /
	//! pressed look re-applied on top if the cursor is on it. Without an effect it just sets the colour.
	static void SetTargetColor(Widget button, Widget target, Color color)
	{
		COA_UIHoverEffect effect;
		if (button)
			effect = COA_UIHoverEffect.Cast(button.FindHandler(COA_UIHoverEffect));

		if (effect && effect.m_wTarget == target)
		{
			effect.SetRestColor(color);
			return;
		}

		if (target)
			target.SetColor(color);
	}

	//------------------------------------------------------------------------------------------------
	void SetRestColor(Color color)
	{
		m_RestColor = new Color(color.R(), color.G(), color.B(), color.A());
		if (!m_wTarget)
			return;

		AnimateWidget.StopAnimation(m_wTarget, WidgetAnimationColor);
		m_bReturning = false;
		if (m_bPressed)
			m_wTarget.SetColor(GetActiveColor(true));
		else if (m_bHovered || m_bFocused)
			m_wTarget.SetColor(GetActiveColor(false));
		else
			m_wTarget.SetColor(m_RestColor);
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

		if (m_eStyle == COA_EHoverStyle.BRIGHTEN)
		{
			amount = HOVER_BRIGHTEN;
			if (pressed)
				amount = PRESS_BRIGHTEN;
		}

		float alpha = m_RestColor.A();
		if (m_eStyle == COA_EHoverStyle.RAISE)
		{
			if (pressed)
				alpha = Math.Max(alpha, PRESS_RAISE_ALPHA);
			else
				alpha = Math.Max(alpha, HOVER_RAISE_ALPHA);
		}

		return new Color(Lighten(m_RestColor.R(), amount), Lighten(m_RestColor.G(), amount), Lighten(m_RestColor.B(), amount), alpha);
	}

	//------------------------------------------------------------------------------------------------
	//! A linear channel, lightened by amount in sRGB
	protected static float Lighten(float linear, float amount)
	{
		float srgb = Math.Pow(Math.Clamp(linear, 0, 1), 1 / GAMMA);
		return Math.Pow(Math.Min(srgb + amount, 1), GAMMA);
	}

	//------------------------------------------------------------------------------------------------
	//! 1. A button with a visible fill lightens itself.
	//! 2. Otherwise its background lightens (a sibling named after it, or named like a background and
	//!    covering the same area) - the common "transparent button over a panel" setup.
	//! 3. Otherwise an icon-only button (a search magnifier) brightens its icon.
	//! 4. Otherwise the transparent button fades in a faint white overlay.
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

		ImageWidget icon = ImageWidget.Cast(m_wButton.GetChildren());
		if (icon && !icon.GetSibling() && icon.IsVisible())
		{
			m_wTarget = icon;
			m_eStyle = COA_EHoverStyle.BRIGHTEN;
			return;
		}

		m_wTarget = m_wButton;
		m_eStyle = COA_EHoverStyle.OVERLAY;
	}

	//------------------------------------------------------------------------------------------------
	//! The button's own background: one named after it ("SendButton" -> "SendButtonBG" or "SendBG"), else
	//! the background covering the same area. Not "the last background before the button": children are
	//! ordered by Z order, not as laid out, so with every background on one Z and every button on the next
	//! that was the last button's background for all of them (the admin menu's hint buttons all lit up
	//! "Send to everyone").
	protected Widget FindBackgroundBehind()
	{
		Widget parent = m_wButton.GetParent();
		if (!parent)
			return null;

		string name = m_wButton.GetName();
		string stem = name;
		stem.Replace("Button", "");
		array<string> ownNames = {name + "BG", name + "Background", stem + "BG", stem + "Background"};

		Widget sibling = parent.GetChildren();
		while (sibling)
		{
			if (sibling != m_wButton && ownNames.Contains(sibling.GetName()) && IsBackgroundCandidate(sibling))
				return sibling;

			sibling = sibling.GetSibling();
		}

		float buttonX, buttonY, buttonWidth, buttonHeight;
		m_wButton.GetScreenPos(buttonX, buttonY);
		m_wButton.GetScreenSize(buttonWidth, buttonHeight);
		if (buttonWidth * buttonHeight <= 0)
			return null;

		// Same element, not a whole panel the button happens to sit on: at least half the combined area shared
		Widget found;
		float bestOverlap = 0.5;
		sibling = parent.GetChildren();
		while (sibling)
		{
			if (sibling != m_wButton && IsBackgroundCandidate(sibling))
			{
				float x, y, width, height;
				sibling.GetScreenPos(x, y);
				sibling.GetScreenSize(width, height);
				float overlap = OverlapRatio(buttonX, buttonY, buttonWidth, buttonHeight, x, y, width, height);
				if (overlap > bestOverlap)
				{
					bestOverlap = overlap;
					found = sibling;
				}
			}

			sibling = sibling.GetSibling();
		}

		return found;
	}

	//------------------------------------------------------------------------------------------------
	//! Shared area over combined area of two rectangles (1 = the same rectangle, 0 = apart)
	protected static float OverlapRatio(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh)
	{
		float shareW = Math.Min(ax + aw, bx + bw) - Math.Max(ax, bx);
		float shareH = Math.Min(ay + ah, by + bh) - Math.Max(ay, by);
		if (shareW <= 0 || shareH <= 0)
			return 0;

		float shared = shareW * shareH;
		return shared / (aw * ah + bw * bh - shared);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsBackgroundCandidate(Widget widget)
	{
		if (!widget.IsVisible() || widget.GetColor().A() < MIN_FILL_ALPHA)
			return false;

		// SmartPanelWidget: rounded nine-slice backgrounds (vanilla rounded_* styles)
		if (!ImageWidget.Cast(widget) && !PanelWidget.Cast(widget) && !SmartPanelWidget.Cast(widget))
			return false;

		string name = widget.GetName();
		return name.Contains("BG") || name.Contains("Background") || name == "Image0" || name == "Panel0";
	}
}
