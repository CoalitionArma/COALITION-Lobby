[BaseContainerProps()]
class COA_PolyZoneEffect
{
	static int m_iLastId;
	int m_iId;
	
	void OnFrame(COA_PolyZoneEffectHandler handler, IEntity ent, float timeSlice)
	{
		
	}
	
	void OnActivate(COA_PolyZoneEffectHandler handler, IEntity ent)
	{
		
	}
	
	void OnDeactivate(COA_PolyZoneEffectHandler handler, IEntity ent)
	{
		
	}
	
	COA_PolyZoneEffect CreateCopyObject()
	{
		return new COA_PolyZoneEffect();
	}
	
	void CopyFields(COA_PolyZoneEffect effect)
	{
		
	}
	
	COA_EffectContainer GetEffectContainer()
	{
		COA_EffectContainer effect = new COA_EffectContainer();
		return effect;
	}
	
	COA_PolyZoneEffect Copy()
	{
		COA_PolyZoneEffect copy = CreateCopyObject();
		CopyFields(copy);
		return copy;	
	}
	
	void COA_PolyZoneEffect()
	{
		m_iLastId++;
		m_iId = m_iLastId;
	}
}
class COA_EffectContainer
{
	int m_iId;
	COA_EPolyZoneEffectHUDType m_iType;
	float m_fTime;
	string m_sString;
}