//! Safe start HUD card (UI/layouts/COA_SafeStartDisplay.layout): timer, status line, progress bar,
//! per-side READY / NOT READY pills and a ready-up hint, plus the "safe start ended" banner.
class COA_SafeStartDisplay : SCR_InfoDisplayExtended
{
	//------------------------------------------------------------------------------------------------
	// Layout constants
	//------------------------------------------------------------------------------------------------
	protected static const float PROGRESS_WIDTH = 232;	// CardWidth in the layout
	protected static const float REFRESH_INTERVAL = 0.25;
	protected static const float BANNER_SECONDS = 5;
	protected static const float BANNER_FADE = 1.5;
	protected static const float DOCK_GAP = 6;				// below the mission brief card
	protected static const float DOCK_SEARCH_INTERVAL = 1;

	// Status array order from COA_SafestartManager.GetWhosReady()
	protected static const ref array<string> FACTION_KEYS = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
	protected static const ref array<string> FACTION_WIDGETS = {"Blufor", "Opfor", "Indfor", "Civ"};

	//------------------------------------------------------------------------------------------------
	// UI widget references
	//------------------------------------------------------------------------------------------------
	protected Widget m_wCard;
	protected ImageWidget m_wSafeDot;
	protected TextWidget m_wTimerCaption;
	protected TextWidget m_wTimerText;
	protected TextWidget m_wTimerDescription;
	protected Widget m_wProgressRow;
	protected Widget m_wProgressFill;
	protected Widget m_wFactionsDivider;
	protected RichTextWidget m_wReadyHint;
	protected TextWidget m_wMissionStart;
	protected TextWidget m_wMissionStart2;

	protected ref array<Widget> m_aFactionRows = {};
	protected ref array<Widget> m_aFactionTicks = {};
	protected ref array<Widget> m_aFactionPills = {};
	protected ref array<TextWidget> m_aFactionStatus = {};
	protected ref array<TextWidget> m_aFactionReadyBy = {};

	//------------------------------------------------------------------------------------------------
	// Manager references
	//------------------------------------------------------------------------------------------------
	protected COA_SafestartManager m_SafestartManager;
	protected SCR_FactionManager m_FactionManager;
	protected COA_Gamemode m_Gamemode;

	//------------------------------------------------------------------------------------------------
	// State
	//------------------------------------------------------------------------------------------------
	protected bool m_bInitialized;
	protected bool m_bAlreadyActivated;
	protected float m_fRefreshTimer;
	protected float m_fPulse;
	protected float m_fBannerTime;
	protected Widget m_wMissionBrief;		// COA_SafeStartInfoDisplay's card, this one docks under it
	protected float m_fDockSearchTimer = DOCK_SEARCH_INTERVAL;

	// Palette (sRGB, converted once in InitializeReferences)
	protected ref Color m_Amber;
	protected ref Color m_Green;
	protected ref Color m_GreenBG;
	protected ref Color m_Red;
	protected ref Color m_RedBG;

	//------------------------------------------------------------------------------------------------
	override protected void DisplayUpdate(IEntity owner, float timeSlice)
	{
		super.DisplayUpdate(owner, timeSlice);

		if (!m_bInitialized)
		{
			m_bInitialized = InitializeReferences();
			if (!m_bInitialized)
				return;
		}

		bool hudVisible = COA_PlayerControllerManager.GetInstance().m_bHUDVisible;
		bool safestart = m_SafestartManager.GetSafestartStatus();

		// Safe start started / ended
		if (safestart && !m_bAlreadyActivated)
		{
			StopMission();
			m_bAlreadyActivated = true;
		}
		else if (!safestart && m_bAlreadyActivated)
		{
			StartMission();
			m_bAlreadyActivated = false;
		}

		m_wCard.SetVisible(safestart && hudVisible);
		UpdateBanner(timeSlice, hudVisible);

		if (!safestart || !hudVisible)
			return;

		DockUnderMissionBrief(timeSlice);
		UpdatePulse(timeSlice);

		m_fRefreshTimer += timeSlice;
		if (m_fRefreshTimer < REFRESH_INTERVAL)
			return;

		m_fRefreshTimer = 0;
		UpdateTimer();
		UpdatePlayedFactions();
	}

	//------------------------------------------------------------------------------------------------
	protected bool InitializeReferences()
	{
		m_SafestartManager = COA_SafestartManager.GetInstance();
		m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		m_Gamemode = COA_Gamemode.GetInstance();
		if (!m_SafestartManager || !m_wRoot)
			return false;

		m_Amber = Color.FromSRGBA(232, 150, 60, 255);
		m_Green = Color.FromSRGBA(92, 196, 128, 255);
		m_GreenBG = Color.FromSRGBA(92, 196, 128, 46);
		m_Red = Color.FromSRGBA(232, 96, 96, 255);
		m_RedBG = Color.FromSRGBA(232, 96, 96, 40);

		m_wCard = m_wRoot.FindAnyWidget("SafeStartCard");
		m_wSafeDot = ImageWidget.Cast(m_wRoot.FindAnyWidget("SafeDot"));
		m_wTimerCaption = TextWidget.Cast(m_wRoot.FindAnyWidget("TimerCaption"));
		m_wTimerText = TextWidget.Cast(m_wRoot.FindAnyWidget("TimerText"));
		m_wTimerDescription = TextWidget.Cast(m_wRoot.FindAnyWidget("TimerDescription"));
		m_wProgressRow = m_wRoot.FindAnyWidget("ProgressRow");
		m_wProgressFill = m_wRoot.FindAnyWidget("ProgressFill");
		m_wFactionsDivider = m_wRoot.FindAnyWidget("FactionsDivider");
		m_wReadyHint = RichTextWidget.Cast(m_wRoot.FindAnyWidget("ReadyHint"));
		m_wMissionStart = TextWidget.Cast(m_wRoot.FindAnyWidget("MissionStart"));
		m_wMissionStart2 = TextWidget.Cast(m_wRoot.FindAnyWidget("MissionStart2"));

		m_aFactionRows.Clear();
		m_aFactionTicks.Clear();
		m_aFactionPills.Clear();
		m_aFactionStatus.Clear();
		m_aFactionReadyBy.Clear();
foreach (int i, string name : FACTION_WIDGETS)
		{
			m_aFactionRows.Insert(m_wRoot.FindAnyWidget(name + "Frame"));
			m_aFactionTicks.Insert(m_wRoot.FindAnyWidget(name + "Tick"));
			m_aFactionPills.Insert(m_wRoot.FindAnyWidget(name + "PillBG"));
			m_aFactionStatus.Insert(TextWidget.Cast(m_wRoot.FindAnyWidget(name + "Ready")));
			m_aFactionReadyBy.Insert(TextWidget.Cast(m_wRoot.FindAnyWidget(name + "ReadyBy")));

			Widget tick = m_aFactionTicks[i];
			if (tick)
				tick.SetColor(FactionColor(FACTION_KEYS[i]));
		}

		return m_wCard && m_wTimerText;
	}

	//------------------------------------------------------------------------------------------------
	//! Keeps this card directly under the mission brief card (top-left corner), following it as its
	//! content changes height. Without that card the layout's own position is used.
	protected void DockUnderMissionBrief(float timeSlice)
	{
		// A card removed from the UI keeps no parent: look it up again (throttled, it searches the whole UI)
		if (!m_wMissionBrief || !m_wMissionBrief.GetParent())
		{
			m_wMissionBrief = null;
			m_fDockSearchTimer += timeSlice;
			if (m_fDockSearchTimer < DOCK_SEARCH_INTERVAL)
				return;

			m_fDockSearchTimer = 0;
			m_wMissionBrief = GetGame().GetWorkspace().FindAnyWidget("SafeStartInfoPanel");
			if (!m_wMissionBrief)
				return;
		}

		if (!m_wMissionBrief.IsVisible())
			return;

		Widget parent = m_wCard.GetParent();
		if (!parent)
			return;

		float briefX, briefY, briefW, briefH, parentX, parentY;
		m_wMissionBrief.GetScreenPos(briefX, briefY);
		m_wMissionBrief.GetScreenSize(briefW, briefH);
		parent.GetScreenPos(parentX, parentY);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		FrameSlot.SetPos(m_wCard, workspace.DPIUnscale(briefX - parentX), workspace.DPIUnscale(briefY + briefH - parentY) + DOCK_GAP);
	}

	//------------------------------------------------------------------------------------------------
	//! Amber dot breathes during safe start; green and faster while going live
	protected void UpdatePulse(float timeSlice)
	{
		if (!m_wSafeDot)
			return;

		bool goingLive = m_SafestartManager.GetGoingLive();
		float speed = 3;
		if (goingLive)
			speed = 7;

		m_fPulse += timeSlice * speed;
		m_wSafeDot.SetOpacity(0.35 + 0.65 * (0.5 + 0.5 * Math.Sin(m_fPulse)));

		if (goingLive)
			m_wSafeDot.SetColor(m_Green);
		else
			m_wSafeDot.SetColor(m_Amber);
	}

	//------------------------------------------------------------------------------------------------
	//! Timer, caption, status line and progress bar for the three phases: waiting for ready (elapsed
	//! time), time-limited safe start (ends in) and the go-live countdown once every side is ready
	protected void UpdateTimer()
	{
		int ready, playing;
		CountSides(ready, playing);
		float remaining = m_SafestartManager.GetSafeStartTimeRemaining();	// float: fractions below

		string sides;
		if (playing == 0)
			sides = "Waiting for players to slot in";
		else
			sides = string.Format("%1 of %2 sides ready", ready, playing);

		if (m_SafestartManager.GetGoingLive())
		{
			m_wTimerCaption.SetText("LIVE IN");
			m_wTimerText.SetText(m_SafestartManager.GetFormattedSafeStartTimeRemaining());
			m_wTimerDescription.SetText("All sides ready, going live");
			SetProgress(remaining / COA_SafestartManager.GO_LIVE_SECONDS, m_Green);
		}
		else if (m_SafestartManager.GetCountdownMode())
		{
			m_wTimerCaption.SetText("ENDS IN");
			m_wTimerText.SetText(m_SafestartManager.GetFormattedSafeStartTimeRemaining());
			m_wTimerDescription.SetText(sides + "  ·  ends automatically");

			float total = 1;
			if (m_Gamemode && m_Gamemode.m_iSafestartTimeLimit > 0)
				total = m_Gamemode.m_iSafestartTimeLimit * 60;
			SetProgress(remaining / total, m_Amber);
		}
		else
		{
			m_wTimerCaption.SetText("ELAPSED");
			m_wTimerText.SetText(ShortTime(COA_GameTimerManager.GetInstance().GetServerWorldTime()));
			m_wTimerDescription.SetText(sides);
			SetProgress(-1, m_Amber);
		}

		UpdateHint(ready, playing);
	}

	//------------------------------------------------------------------------------------------------
	//! fraction < 0 hides the bar
	protected void SetProgress(float fraction, Color color)
	{
		if (!m_wProgressRow || !m_wProgressFill)
			return;

		m_wProgressRow.SetVisible(fraction >= 0);
		if (fraction < 0)
			return;

		fraction = Math.Clamp(fraction, 0, 1);
		FrameSlot.SetSizeX(m_wProgressFill, Math.Max(PROGRESS_WIDTH * fraction, 1));
		m_wProgressFill.SetColor(color);
	}

	//------------------------------------------------------------------------------------------------
	//! Group leaders get the key to press; everyone else is told who readies their side
	protected void UpdateHint(int ready, int playing)
	{
		if (!m_wReadyHint)
			return;

		int playerId = SCR_PlayerController.GetLocalPlayerId();
		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		SCR_AIGroup group;
		if (groupsManager)
			group = groupsManager.GetPlayerGroup(playerId);

		if (group && group.IsPlayerLeader(playerId))
			m_wReadyHint.SetText("<action name='COA_ToggleSideReady'/>  Toggle your side ready");
		else if (playing > 0)
			m_wReadyHint.SetText("Group leaders ready up for their side");
		else
			m_wReadyHint.SetText("");

		m_wReadyHint.SetVisible(playing > 0);
	}

	//------------------------------------------------------------------------------------------------
	//! One row per playing side with a READY / NOT READY pill
	protected void UpdatePlayedFactions()
	{
		array<string> statuses = m_SafestartManager.GetWhosReady();
		array<string> readyBy = m_SafestartManager.GetReadyBy();
		bool anyShown;

		foreach (int i, Widget row : m_aFactionRows)
		{
			string status = "N/A";
			if (statuses && statuses.IsIndexValid(i))
				status = statuses[i];

			bool shown = status != "N/A";
			if (row)
				row.SetVisible(shown);
			if (!shown)
				continue;

			anyShown = true;
			bool isReady = status == "Ready";

			TextWidget statusText = m_aFactionStatus[i];
			if (statusText)
			{
				if (isReady)
				{
					statusText.SetText("READY");
					statusText.SetColor(m_Green);
				}
				else
				{
					statusText.SetText("NOT READY");
					statusText.SetColor(m_Red);
				}
			}

			// "by Name" next to the pill, so people know who to ask when a side un-readies
			TextWidget readyByText = m_aFactionReadyBy[i];
			if (readyByText)
			{
				string by;
				if (isReady && readyBy && readyBy.IsIndexValid(i) && !readyBy[i].IsEmpty())
					by = "by " + readyBy[i];
				readyByText.SetText(by);
				readyByText.SetVisible(!by.IsEmpty());
			}

			Widget pill = m_aFactionPills[i];
			if (pill)
			{
				if (isReady)
					pill.SetColor(m_GreenBG);
				else
					pill.SetColor(m_RedBG);
			}
		}

		if (m_wFactionsDivider)
			m_wFactionsDivider.SetVisible(anyShown);
	}

	//------------------------------------------------------------------------------------------------
	protected void CountSides(out int ready, out int playing)
	{
		ready = 0;
		playing = 0;
		array<string> statuses = m_SafestartManager.GetWhosReady();
		if (!statuses)
			return;

		foreach (string status : statuses)
		{
			if (status == "N/A")
				continue;

			playing++;
			if (status == "Ready")
				ready++;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! "00:01:23" -> "01:23" while under an hour
	protected static string ShortTime(string time)
	{
		if (time.Length() == 8 && time.StartsWith("00:"))
			return time.Substring(3, 5);

		return time;
	}

	//------------------------------------------------------------------------------------------------
	protected Color FactionColor(string factionKey)
	{
		if (m_FactionManager)
		{
			Faction faction = m_FactionManager.GetFactionByKey(factionKey);
			if (faction)
				return faction.GetFactionColor();
		}

		return Color.FromSRGBA(169, 180, 204, 255);
	}

	//------------------------------------------------------------------------------------------------
	//! Banner holds, then fades over its last BANNER_FADE seconds (frame-rate independent)
	protected void UpdateBanner(float timeSlice, bool hudVisible)
	{
		if (!m_wMissionStart || !m_wMissionStart2)
			return;

		m_wMissionStart.SetVisible(hudVisible);
		m_wMissionStart2.SetVisible(hudVisible);

		if (m_fBannerTime <= 0)
			return;

		m_fBannerTime = Math.Max(m_fBannerTime - timeSlice, 0);
		float opacity = Math.Clamp(m_fBannerTime / BANNER_FADE, 0, 1);
		m_wMissionStart.SetOpacity(opacity);
		m_wMissionStart2.SetOpacity(opacity);
	}

	//------------------------------------------------------------------------------------------------
	//! Safe start began: show the card, clear any banner
	protected void StopMission()
	{
		m_fBannerTime = 0;
		m_fRefreshTimer = REFRESH_INTERVAL;		// fill the card on the first frame
		if (m_wMissionStart)
			m_wMissionStart.SetOpacity(0);
		if (m_wMissionStart2)
			m_wMissionStart2.SetOpacity(0);
	}

	//------------------------------------------------------------------------------------------------
	//! Safe start ended: title card plus the "weapons live" banner
	protected void StartMission()
	{
		COA_PlayerMenuManager.GetInstance().DisplayTitleCard();

		m_fBannerTime = BANNER_SECONDS;
		if (m_wMissionStart)
			m_wMissionStart.SetOpacity(1);
		if (m_wMissionStart2)
			m_wMissionStart2.SetOpacity(1);
	}
}
