//! Hint toast (UI/layouts/Hint/hint.layout), shown by COA_RplBroadcastManager.RpcDo_SendHint.
//! A leading "[Tag]" in the message becomes the card's heading (e.g. "[SlotLottery] ..." shows
//! SLOTLOTTERY above the text); otherwise the heading is HINT. The card slides in, a progress bar
//! drains over its lifetime, then it fades out and removes itself.
class COA_Hint : SCR_ScriptedWidgetComponent
{
	//------------------------------------------------------------------------------------------------
	// Layout / animation constants
	//------------------------------------------------------------------------------------------------
	protected static const float PROGRESS_WIDTH = 328;	// HintWidth in the layout
	protected static const float FADE_IN_MS = 220;
	protected static const float FADE_OUT_MS = 450;
	protected static const float SLIDE_DISTANCE = 28;		// card slides in from the right by this much
	protected static const string DEFAULT_TITLE = "HINT";

	//------------------------------------------------------------------------------------------------
	// Widget references
	//------------------------------------------------------------------------------------------------
	protected Widget m_wMainWidget;			// layout root (full screen)
	protected Widget m_wCard;
	protected TextWidget m_wText;
	protected TextWidget m_wTitle;
	protected Widget m_wProgressFill;

	protected float m_fCardX;				// resting position from the layout
	protected float m_fStartTime;			// world time (ms) the hint appeared
	protected float m_fDuration;			// ms
	protected bool m_bLoopsRemoved;			// DestroyHint() already removed the callqueue loop

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		// Only process in play mode, not in editor
		if (!GetGame().InPlayMode())
			return;

		m_wMainWidget = w;
		super.HandlerAttached(m_wMainWidget);

		m_wCard = m_wMainWidget.FindAnyWidget("HintCard");
		m_wText = TextWidget.Cast(m_wMainWidget.FindAnyWidget("hintText"));
		m_wTitle = TextWidget.Cast(m_wMainWidget.FindAnyWidget("HintTitle"));
		m_wProgressFill = m_wMainWidget.FindAnyWidget("HintProgressFill");

		if (m_wCard)
		{
			m_fCardX = FrameSlot.GetPosX(m_wCard);
			m_wCard.SetOpacity(0);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \param[in] hinttext Message; a leading "[Tag]" becomes the heading
	//! \param[in] duration How long the hint stays, in milliseconds
	void ShowHint(string hinttext, float duration)
	{
		if (!m_wMainWidget || !m_wCard || !m_wText)
			return;

		GetGame().GetCallqueue().Remove(Tick);

		string title = DEFAULT_TITLE;
		string message = hinttext;
		SplitTag(hinttext, title, message);

		m_wText.SetText(message);
		if (m_wTitle)
			m_wTitle.SetText(title);

		m_fStartTime = GetGame().GetWorld().GetWorldTime();
		m_fDuration = Math.Max(duration, FADE_IN_MS + FADE_OUT_MS);

		AudioSystem.PlaySound("{A4D15A2A486BD70A}Sounds/UI/Samples/Editor/UI_E_Notification_Default.wav");

		// Every frame while visible: slide / fade / progress
		GetGame().GetCallqueue().CallLater(Tick, 0, true);
		Tick();
	}

	//------------------------------------------------------------------------------------------------
	//! "[SlotLottery] Signed up" -> title "SLOTLOTTERY", message "Signed up"
	protected static void SplitTag(string text, out string title, out string message)
	{
		message = text;
		if (!text.StartsWith("["))
			return;

		int close = text.IndexOf("]");
		if (close < 2 || close > 32)
			return;

		string tag = text.Substring(1, close - 1);
		tag.ToUpper();
		title = tag;
		message = text.Substring(close + 1, text.Length() - close - 1).Trim();
	}

	//------------------------------------------------------------------------------------------------
	protected void Tick()
	{
		if (!m_wMainWidget || !m_wCard)
		{
			GetGame().GetCallqueue().Remove(Tick);
			return;
		}

		float elapsed = GetGame().GetWorld().GetWorldTime() - m_fStartTime;
		if (elapsed >= m_fDuration)
		{
			DestroyHint();
			return;
		}

		// Ease-out slide and fade in, linear fade out at the end
		float appear = Math.Clamp(elapsed / FADE_IN_MS, 0, 1);
		float eased = 1 - (1 - appear) * (1 - appear);
		float disappear = Math.Clamp((m_fDuration - elapsed) / FADE_OUT_MS, 0, 1);

		m_wCard.SetOpacity(eased * disappear);
		FrameSlot.SetPosX(m_wCard, m_fCardX + SLIDE_DISTANCE * (1 - eased));

		if (m_wProgressFill)
			FrameSlot.SetSizeX(m_wProgressFill, Math.Max(PROGRESS_WIDTH * (1 - elapsed / m_fDuration), 1));
	}

	//------------------------------------------------------------------------------------------------
	//! Stop the loop and destroy the widget exactly once
	void DestroyHint()
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(Tick);
		m_bLoopsRemoved = true;

		Widget widget = m_wMainWidget;
		m_wMainWidget = null;
		m_wCard = null;
		m_wText = null;
		m_wTitle = null;
		m_wProgressFill = null;

		COA_PlayerControllerManager playerControllerManager = COA_PlayerControllerManager.GetInstance();
		if (playerControllerManager && playerControllerManager.m_wSavedHintWidget == widget)
			playerControllerManager.m_wSavedHintWidget = null;

		if (widget)
			widget.RemoveFromHierarchy();
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_Hint()
	{
		// When destroyed through DestroyHint() we are running inside Tick, i.e. inside the callqueue
		// tick (game.c OnUpdate) - touching the callqueue from the destructor then throws a
		// NullPointerError. DestroyHint() already removed the loop, so only clean up for other teardown paths.
		if (!m_bLoopsRemoved && GetGame() && GetGame().GetCallqueue())
			GetGame().GetCallqueue().Remove(Tick);

		m_wMainWidget = null;
		m_wCard = null;
		m_wText = null;
		m_wTitle = null;
		m_wProgressFill = null;
	}
}
