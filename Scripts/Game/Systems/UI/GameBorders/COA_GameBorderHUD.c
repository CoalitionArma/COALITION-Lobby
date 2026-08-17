class COA_GameBorderHUD : SCR_InfoDisplay
{
	OverlayWidget m_wVignette;
	VerticalLayoutWidget m_wEffectsVerticalLayout;
	
	ref map<COA_EGameBorderEffectType, ResourceName> m_mEffectLayouts = new map<COA_EGameBorderEffectType, ResourceName>();
	ref map<int, COA_GameBorderEffect> m_mEffects = new map<COA_EGameBorderEffectType, COA_GameBorderEffect>();
	
	bool m_bShowVignette = false;
	
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
		
		// TODO: config
		m_mEffectLayouts.Insert(COA_EGameBorderEffectType.RestrictedZone, "{934EEEE4F36CE31E}UI/Map/HUD/GameBorderEffects/GameBorderRestrictedZoneEffect.layout");
		
		COA_BorderCheckSystem borderCheck = COA_BorderCheckSystem.GetInstance();
		if (borderCheck)
			borderCheck.RegisterHUD(this);
	}
	
	override protected void UpdateValues(IEntity owner, float timeSlice)
	{
		super.UpdateValues(owner, timeSlice);
		foreach (int id, COA_GameBorderEffect GameBorderEffect : m_mEffects)
		{
			GameBorderEffect.Update(timeSlice);
		}
		
		UpdateOverlayVisibility(m_wVignette, m_bShowVignette, timeSlice);
	}

	protected void UpdateOverlayVisibility(Widget overlay, bool show, float timeSlice)
	{
		if (!overlay)
			return;

		if (show)
		{
			overlay.SetEnabled(true);
			overlay.SetOpacity(Math.Clamp(overlay.GetOpacity() + timeSlice * 5.0, 0, 1));
			return;
		}

		float opacity = Math.Clamp(overlay.GetOpacity() - timeSlice * 5.0, 0, 1);
		overlay.SetOpacity(opacity);
		if (opacity <= 0)
			overlay.SetEnabled(false);
	}
	
	void HideAll()
	{
		foreach (int id, COA_GameBorderEffect effectHUD : m_mEffects)
		{
			HideEffect(id);
		}
	}
	
	void ShowEffect(COA_BorderSettingsContainer effect)
	{
		if (!effect || !m_wEffectsVerticalLayout || !m_mEffectLayouts.Contains(effect.m_iType))
			return;

		if (m_mEffects.Contains(effect.m_iId))
			HideEffect(effect.m_iId);
		
		ResourceName effectLayout = m_mEffectLayouts.Get(effect.m_iType);
		Widget effectWidget = GetGame().GetWorkspace().CreateWidgets(effectLayout, m_wEffectsVerticalLayout);
		if (!effectWidget)
			return;

		COA_GameBorderEffect GameBorderEffect = COA_GameBorderEffect.Cast(effectWidget.FindHandler(COA_GameBorderEffect));
		if (!GameBorderEffect)
		{
			delete effectWidget;
			return;
		}

		GameBorderEffect.SetString(effect.m_sString);
		GameBorderEffect.SetTime(effect.m_fTime);
		if (!m_bShowVignette) m_bShowVignette = GameBorderEffect.ShowVignette();
		m_mEffects.Insert(effect.m_iId, GameBorderEffect);
	}
	
	void HideEffect(int id)
	{
		if (!m_mEffects.Contains(id))
			return;
		
		COA_GameBorderEffect effectHUD = m_mEffects.Get(id);
		if (effectHUD && effectHUD.GetRootWidget())
			effectHUD.GetRootWidget().RemoveFromHierarchy();

		m_mEffects.Remove(id);
		m_bShowVignette = false;
		foreach (int idT, COA_GameBorderEffect GameBorderEffect : m_mEffects)
		{
			m_bShowVignette = GameBorderEffect.ShowVignette();
			if (m_bShowVignette) break;
		}
	}
}
