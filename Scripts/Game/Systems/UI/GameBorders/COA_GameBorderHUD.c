class COA_GameBorderHUD : SCR_InfoDisplay
{
	protected OverlayWidget m_wVignette;
	protected VerticalLayoutWidget m_wEffectsVerticalLayout;
	protected COA_GameBorderEffect m_GameBorderEffect;
	
	bool m_bActive = false;
	
	override event void OnStartDraw(IEntity owner)
	{
		super.OnStartDraw(owner);
		
		m_wVignette = OverlayWidget.Cast(m_wRoot.FindAnyWidget("Vignette"));
		m_wEffectsVerticalLayout = VerticalLayoutWidget.Cast(m_wContent.FindAnyWidget("EffectsVerticalLayout"));
		if (!m_wVignette || !m_wEffectsVerticalLayout)
			return;

		// Transparent widgets still participate in hit testing. Keep inactive
		// full-screen effects disabled so they cannot block UI below this HUD.
		m_wVignette.SetOpacity(0);
		m_wVignette.SetEnabled(false);
		
		COA_BorderCheckSystem borderCheck = COA_BorderCheckSystem.GetInstance();
		if (borderCheck)
			borderCheck.RegisterHUD(this);
	}
	
	//------------------------------------------------------------------------------------------------
	override protected void UpdateValues(IEntity owner, float timeSlice)
	{
		super.UpdateValues(owner, timeSlice);
		
		if (m_GameBorderEffect)
			m_GameBorderEffect.Update(timeSlice);
	}
	
	//------------------------------------------------------------------------------------------------
	void ShowEffect(COA_GameBorder border)
	{
		if (!border || !m_wEffectsVerticalLayout)
			return;
		
		ResourceName effectLayout = "{934EEEE4F36CE31E}UI/Map/HUD/GameBorderEffects/GameBorderRestrictedZoneEffect.layout";
		Widget effectWidget = GetGame().GetWorkspace().CreateWidgets(effectLayout, m_wEffectsVerticalLayout);
		if (!effectWidget)
			return;

		m_GameBorderEffect = COA_GameBorderEffect.Cast(effectWidget.FindHandler(COA_GameBorderEffect));
		if (!m_GameBorderEffect)
		{
			delete effectWidget;
			return;
		}
		
		m_GameBorderEffect.SetTime(border.m_iKillTime);
		m_wVignette.SetOpacity(1);
		m_wVignette.SetEnabled(true);
		m_bActive = true;
	}
	
	//------------------------------------------------------------------------------------------------
	void HideEffect()
	{
		if (m_GameBorderEffect && m_GameBorderEffect.GetRootWidget())
			m_GameBorderEffect.GetRootWidget().RemoveFromHierarchy();

		m_wVignette.SetOpacity(0);
		m_wVignette.SetEnabled(false);
		m_bActive = false;
	}
}
