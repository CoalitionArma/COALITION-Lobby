//------------------------------------------------------------------------------------------------
//! On-screen countdown started from the admin menu (Hints > Countdown), e.g. "MISSION ENDING  4:59".
//! The state lives in COA_AdminMenuManager's RplProps (label, target faction, server end time); this
//! only reads it and counts down locally against the replicated server clock. When it reaches zero
//! it shows TIME for a few seconds and removes itself. Client only - nothing is created on a
//! dedicated server.
class COA_AdminCountdownDisplay
{
	protected static const ResourceName LAYOUT = "{F17444D1F2BAEDD9}UI/layouts/Hint/AdminCountdown.layout";
	protected static const int TICK_MS = 200;
	protected static const float DONE_HOLD_MS = 5000;		// TIME stays up this long after zero
	protected static const float WARNING_MS = 10000;		// the time turns red for the last 10 seconds

	protected static ref COA_AdminCountdownDisplay s_Instance;

	protected Widget m_wRoot;
	protected TextWidget m_wLabel;
	protected TextWidget m_wTime;
	protected Widget m_wAccent;

	protected bool m_bRunning;
	protected int m_iSerial = -1;
	protected bool m_bDoneCuePlayed;

	//------------------------------------------------------------------------------------------------
	//! Re-read the countdown state: show, update or hide. Called when it replicates.
	static void Refresh()
	{
		if (System.IsConsoleApp())
			return;

		if (!s_Instance)
			s_Instance = new COA_AdminCountdownDisplay();

		s_Instance.Start();
	}

	//------------------------------------------------------------------------------------------------
	static bool IsRunning()
	{
		return s_Instance != null && s_Instance.m_bRunning;
	}

	//------------------------------------------------------------------------------------------------
	//! "4:59", "1:02:03" - whole seconds rounded up so it reads 0:01 right until it ends
	static string FormatRemaining(float remainingMs)
	{
		if (remainingMs < 0)
			remainingMs = 0;

		int totalSeconds = Math.Ceil(remainingMs * 0.001);
		int hours = totalSeconds / 3600;
		int minutes = (totalSeconds % 3600) / 60;
		int seconds = totalSeconds % 60;

		if (hours > 0)
			return string.Format("%1:%2:%3", hours, minutes.ToString(2), seconds.ToString(2));

		return string.Format("%1:%2", minutes, seconds.ToString(2));
	}

	//------------------------------------------------------------------------------------------------
	protected void Start()
	{
		GetGame().GetCallqueue().Remove(Tick);
		m_bRunning = true;
		Tick();

		if (m_bRunning)
			GetGame().GetCallqueue().CallLater(Tick, TICK_MS, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void Stop()
	{
		m_bRunning = false;
		GetGame().GetCallqueue().Remove(Tick);

		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();

		m_wRoot = null;
		m_wLabel = null;
		m_wTime = null;
		m_wAccent = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void Tick()
	{
		COA_AdminMenuManager manager = COA_AdminMenuManager.GetInstance();
		if (!manager || !manager.IsCountdownActive() || !IsForLocalPlayer(manager.GetCountdownFaction()))
		{
			Stop();
			return;
		}

		float remainingMs = manager.GetCountdownRemainingMs();
		if (remainingMs < -DONE_HOLD_MS)
		{
			Stop();
			return;
		}

		// A new countdown (or a restart): play the end cue again when this one finishes
		if (manager.GetCountdownSerial() != m_iSerial)
		{
			m_iSerial = manager.GetCountdownSerial();
			m_bDoneCuePlayed = false;
		}

		if (!m_wRoot && !CreateWidgets())
		{
			Stop();
			return;
		}

		string label = manager.GetCountdownLabel();
		label.ToUpper();
		if (m_wLabel)
			m_wLabel.SetText(label);

		if (!m_wTime)
			return;

		if (remainingMs > 0)
		{
			m_wTime.SetText(FormatRemaining(remainingMs));

			if (remainingMs <= WARNING_MS)
				m_wTime.SetColor(Color.FromSRGBA(232, 96, 96, 255));
			else
				m_wTime.SetColor(Color.FromSRGBA(239, 242, 247, 255));

			return;
		}

		m_wTime.SetText("TIME");
		m_wTime.SetColor(Color.FromSRGBA(232, 96, 96, 255));

		if (!m_bDoneCuePlayed)
		{
			m_bDoneCuePlayed = true;
			// Only when it ends while being watched - not for a JIP player arriving in the TIME window
			if (remainingMs > -1000)
				SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_RESPAWN_COUNTDOWN_END);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool CreateWidgets()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return false;

		m_wRoot = workspace.CreateWidgets(LAYOUT);
		if (!m_wRoot)
			return false;

		m_wLabel = TextWidget.Cast(m_wRoot.FindAnyWidget("CountdownLabel"));
		m_wTime = TextWidget.Cast(m_wRoot.FindAnyWidget("CountdownTime"));
		m_wAccent = m_wRoot.FindAnyWidget("CountdownAccent");
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Same targeting as faction hints (COA_RplBroadcastManager.RpcDo_SendHint)
	protected static bool IsForLocalPlayer(string factionKey)
	{
		if (factionKey.IsEmpty())
			return true;

		Faction localFaction = SCR_FactionManager.SGetLocalPlayerFaction();
		return localFaction != null && localFaction.GetFactionKey() == factionKey;
	}
}
