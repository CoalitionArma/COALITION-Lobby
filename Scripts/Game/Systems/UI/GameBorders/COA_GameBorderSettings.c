[BaseContainerProps()]
class COA_GameBorderSettings
{
	[Attribute("10")]
	float m_fKillTime;
	
	static int m_iLastId;
	int m_iId;
	
	COA_BorderSettingsContainer GetEffectContainer()
	{
		COA_BorderSettingsContainer effect = new COA_BorderSettingsContainer();
		effect.m_iId = m_iId;
		effect.m_fTime = m_fKillTime;
		effect.m_iType = COA_EGameBorderEffectType.RestrictedZone;
		return effect;
	}
	
	void COA_GameBorderSettings()
	{
		m_iLastId++;
		m_iId = m_iLastId;
	}
}

class COA_BorderSettingsContainer
{
	int m_iId;
	COA_EGameBorderEffectType m_iType;
	float m_fTime;
	string m_sString;
}