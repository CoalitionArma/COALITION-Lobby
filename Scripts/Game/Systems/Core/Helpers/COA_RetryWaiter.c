//! Shared base for "poll until a condition is true, then act" waits.
//! Subclass and override IsConditionMet()/OnReady() (and optionally OnTimeout()).
//! Hold the subclass instance in a `ref` member on the owner so it isn't collected mid-wait.
class COA_RetryWaiter
{
	protected int m_iIntervalMs;
	protected int m_iMaxAttempts;
	protected int m_iAttempt = 0;
	protected string m_sDebugLabel;
	protected bool m_bRunning = false;

	//------------------------------------------------------------------------------------------------
	//! Begin polling. Calls OnReady() as soon as IsConditionMet() is true, or OnTimeout() after maxAttempts.
	void Start(int intervalMs, int maxAttempts, string debugLabel = "")
	{
		m_iIntervalMs = intervalMs;
		m_iMaxAttempts = maxAttempts;
		m_sDebugLabel = debugLabel;
		m_iAttempt = 0;
		m_bRunning = true;

		Tick();
	}

	//------------------------------------------------------------------------------------------------
	//! Stop polling without firing OnReady() or OnTimeout().
	void Cancel()
	{
		if (!m_bRunning)
			return;

		m_bRunning = false;
		GetGame().GetCallqueue().Remove(Tick);
	}

	//------------------------------------------------------------------------------------------------
	//! Override: return true once the awaited condition holds.
	protected bool IsConditionMet()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Override: react to the condition becoming true.
	protected void OnReady()
	{
	}

	//------------------------------------------------------------------------------------------------
	//! Override: react to maxAttempts being exhausted. Default logs a single, clearly labeled error.
	protected void OnTimeout()
	{
		Print(string.Format("[COA_RetryWaiter] ERROR: '%1' timed out after %2 attempts (%3ms interval)", m_sDebugLabel, m_iMaxAttempts, m_iIntervalMs), LogLevel.ERROR);
	}

	//------------------------------------------------------------------------------------------------
	protected void Tick()
	{
		if (!m_bRunning)
			return;

		if (IsConditionMet())
		{
			m_bRunning = false;
			OnReady();
			return;
		}

		// IsConditionMet() may have called Cancel() itself to abandon the wait early (e.g. the thing
		// being waited on no longer applies) - that's neither success nor a timeout, so stop quietly.
		if (!m_bRunning)
			return;

		m_iAttempt++;
		if (m_iAttempt >= m_iMaxAttempts)
		{
			m_bRunning = false;
			OnTimeout();
			return;
		}

		GetGame().GetCallqueue().CallLater(Tick, m_iIntervalMs, false);
	}
}
