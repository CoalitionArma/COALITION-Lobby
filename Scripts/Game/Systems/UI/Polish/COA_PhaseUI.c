//------------------------------------------------------------------------------------------------
// Shared pieces of the lobby phase screens (UI/Phases/Slotting.layout, Briefing.layout)
//------------------------------------------------------------------------------------------------
class COA_PhaseUI
{
	// Header phase stepper, in order: the frames holding each step's ButtonText
	protected static const ref array<string> STEP_FRAMES = {"PreviewFrame", "SlottingFrame", "GameFrame", "AARFrame"};

	static const ref Color ACCENT = Color.FromSRGBA(201, 54, 54, 255);
	protected static const ref Color STEP_DONE = Color.FromSRGBA(201, 209, 225, 255);
	protected static const ref Color STEP_CURRENT = Color.FromSRGBA(255, 255, 255, 255);
	protected static const ref Color STEP_UPCOMING = Color.FromSRGBA(140, 150, 171, 255);
	protected static const ref Color LINE_DONE = Color.FromSRGBA(143, 163, 199, 255);
	protected static const ref Color LINE_UPCOMING = Color.FromSRGBA(255, 255, 255, 8); // alpha blends in linear space: ~0.03 here reads like the mockup's 0.16

	//------------------------------------------------------------------------------------------------
	//! Colour the header stepper for the current gamemode state (0 briefing .. 3 AAR): finished
	//! phases in a soft tone, the current one white, later ones dimmed. The current phase's
	//! underline is coloured by the menus themselves (SetupPhaseIndicators).
	static void StyleStepper(Widget root, int currentState)
	{
		if (!root)
			return;

		foreach (int i, string frameName : STEP_FRAMES)
		{
			Widget stepFrame = root.FindAnyWidget(frameName);
			if (!stepFrame)
				continue;

			TextWidget label = TextWidget.Cast(stepFrame.FindAnyWidget("ButtonText"));
			if (label)
			{
				if (i < currentState)
					label.SetColor(STEP_DONE);
				else if (i == currentState)
					label.SetColor(STEP_CURRENT);
				else
					label.SetColor(STEP_UPCOMING);
			}

			// StepLine1 joins steps 0 and 1, and so on
			if (i == 0)
				continue;

			Widget line = root.FindAnyWidget("StepLine" + i);
			if (!line)
				continue;

			if (i <= currentState)
				line.SetColor(LINE_DONE);
			else
				line.SetColor(LINE_UPCOMING);
		}
	}
}

//------------------------------------------------------------------------------------------------
//! A panel tucked against a screen edge that slides out while the cursor is over it - the AAR
//! faction sidebar's behaviour, reusable. The drawer is positioned by its FrameSlot PositionX.
class COA_HoverDrawer
{
	// Ease-out rate for the per-frame slide - 12 settles in about a quarter of a second
	protected static const float SLIDE_SPEED = 12;

	protected Widget m_wDrawer;
	protected Widget m_wClosedHitArea;	// optional: while closed, only this widget opens the drawer
	protected bool m_bVertical;			// slide on Y (PositionY) instead of X
	protected float m_fLayoutX;			// PositionX from the layout, restored when switching to vertical
	protected float m_fClosedX;
	protected float m_fOpenX;
	protected bool m_bOpen;

	ref ScriptInvoker m_OnOpenChanged = new ScriptInvoker(); // (bool open)

	//------------------------------------------------------------------------------------------------
	//! \param[in] drawer widget to slide (its FrameSlot PositionX is animated)
	//! \param[in] closedX PositionX when closed - only its tab on screen
	//! \param[in] openX PositionX when fully open
	void COA_HoverDrawer(Widget drawer, float closedX, float openX)
	{
		m_wDrawer = drawer;
		m_fClosedX = closedX;
		m_fOpenX = openX;

		if (m_wDrawer)
		{
			m_fLayoutX = FrameSlot.GetPosX(m_wDrawer);
			FrameSlot.SetPosX(m_wDrawer, closedX);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Slide vertically: closed/open values are then PositionY (e.g. a bar tucked below the screen)
	void SetVertical(bool vertical)
	{
		m_bVertical = vertical;
		if (!m_wDrawer)
			return;

		if (vertical)
		{
			// The constructor parked it horizontally; put X back where the layout had it
			FrameSlot.SetPosX(m_wDrawer, m_fLayoutX);
			FrameSlot.SetPosY(m_wDrawer, m_fClosedX);
		}
		else
		{
			FrameSlot.SetPosX(m_wDrawer, m_fClosedX);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected float GetPos()
	{
		if (m_bVertical)
			return FrameSlot.GetPosY(m_wDrawer);

		return FrameSlot.GetPosX(m_wDrawer);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetPos(float value)
	{
		if (m_bVertical)
			FrameSlot.SetPosY(m_wDrawer, value);
		else
			FrameSlot.SetPosX(m_wDrawer, value);
	}

	//------------------------------------------------------------------------------------------------
	//! While closed, open only when the cursor is over this widget (typically the tab) instead of
	//! anywhere along the drawer's on-screen edge - for tall drawers that would otherwise catch the
	//! cursor over other UI near the screen edge. Once open, the whole drawer keeps it open.
	void SetClosedHitArea(Widget hitArea)
	{
		m_wClosedHitArea = hitArea;
	}

	//------------------------------------------------------------------------------------------------
	bool IsOpen()
	{
		return m_bOpen;
	}

	//------------------------------------------------------------------------------------------------
	void Update(float tDelta)
	{
		if (!m_wDrawer || !m_wDrawer.IsVisible())
			return;

		float currentX = GetPos();
		bool open = IsHovered(m_fOpenX - currentX);
		if (open != m_bOpen)
		{
			m_bOpen = open;
			m_OnOpenChanged.Invoke(open);
		}

		float targetX = m_fClosedX;
		if (m_bOpen)
			targetX = m_fOpenX;

		if (currentX == targetX)
			return;

		float nextX = currentX + (targetX - currentX) * (1 - Math.Pow(2.71828, -SLIDE_SPEED * tDelta));
		if (Math.AbsFloat(targetX - nextX) < 0.5)
			nextX = targetX;

		SetPos(nextX);
	}

	//------------------------------------------------------------------------------------------------
	//! Rectangle test with hysteresis (same as COA_AARMenu.IsDrawerHovered): closed, it opens when
	//! the cursor touches the part on screen; open, it stays open anywhere the fully-open drawer covers
	protected bool IsHovered(float toOpenX)
	{
		int mouseX, mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);

		float left, top, width, height;
		if (!m_bOpen && m_wClosedHitArea && m_wClosedHitArea.IsVisible())
		{
			m_wClosedHitArea.GetScreenPos(left, top);
			m_wClosedHitArea.GetScreenSize(width, height);
			return mouseX >= left && mouseX <= left + width && mouseY >= top && mouseY <= top + height;
		}

		m_wDrawer.GetScreenPos(left, top);
		m_wDrawer.GetScreenSize(width, height);
		float right = left + width;
		float bottom = top + height;

		if (m_bOpen)
		{
			float shift = GetGame().GetWorkspace().DPIScale(toOpenX);
			if (m_bVertical)
			{
				top += shift;
				bottom += shift;
			}
			else
			{
				left += shift;
				right += shift;
			}
		}
		else
		{
			Widget parent = m_wDrawer.GetParent();
			if (parent)
			{
				float screenX, screenY, screenWidth, screenHeight;
				parent.GetScreenPos(screenX, screenY);
				parent.GetScreenSize(screenWidth, screenHeight);
				if (m_bVertical)
				{
					top = Math.Max(top, screenY);
					bottom = Math.Min(bottom, screenY + screenHeight);
				}
				else
				{
					left = Math.Max(left, screenX);
					right = Math.Min(right, screenX + screenWidth);
				}
			}
		}

		return mouseX >= left && mouseX <= right && mouseY >= top && mouseY <= bottom;
	}
}
