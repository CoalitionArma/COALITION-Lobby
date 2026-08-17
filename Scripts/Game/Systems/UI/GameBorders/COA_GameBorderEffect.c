class COA_GameBorderEffect : SCR_ScriptedWidgetComponent
{
	TextWidget m_wCounter;
	bool m_bTriggerd = false;
	float m_fTime;
	string m_sString;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wCounter = TextWidget.Cast(w.FindAnyWidget("Counter"));
		m_bTriggerd = false;
	}
	
	void SetTime(float time)
	{
		m_fTime = time;
		Update(0.0);
	}
	
	void SetString(string str)
	{
		m_sString = str;
	}
	
	bool ShowVignette()
	{
		return true;
	}
	
	void Update(float timeSlice)
	{
		m_fTime -= timeSlice;
		if (m_fTime < 0) m_fTime = 0;
		int miliseconds = Math.Mod(m_fTime, 1) * 100;
		int seconds = Math.Mod(m_fTime, 60);
		int minutes = (m_fTime / 60);
		m_wCounter.SetTextFormat("%1:%2.%3", minutes.ToString(2), seconds.ToString(2), miliseconds.ToString(2));
		
		if (m_fTime <= 0 && !m_bTriggerd)
		{
			DamageManagerComponent damageManager = DamageManagerComponent.Cast(SCR_PlayerController.GetLocalMainEntity().FindComponent(DamageManagerComponent));
			if (!damageManager || damageManager.GetState() == EDamageState.DESTROYED)
				return;
			damageManager.SetHealthScaled(0);
			m_bTriggerd = true;
		}
	}
}