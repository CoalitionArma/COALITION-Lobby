[BaseContainerProps()]
class COA_PolyZoneEffectRestricted : COA_PolyZoneEffect
{
	[Attribute("10")]
	float m_fKillTime;
	
	bool m_bTriggerd = false;
	
	override void OnFrame(COA_PolyZoneEffectHandler handler, IEntity ent, float timeSlice)
	{
		m_fKillTime -= timeSlice;
		if (m_fKillTime <= 0 && !m_bTriggerd)
		{
			DamageManagerComponent damageManager = DamageManagerComponent.Cast(ent.FindComponent(DamageManagerComponent));
			if (!damageManager || damageManager.GetState() == EDamageState.DESTROYED)
				return;
			damageManager.SetHealthScaled(0);
		}
	}
	
	override void OnActivate(COA_PolyZoneEffectHandler handler, IEntity ent)
	{
		
	}
	
	override void OnDeactivate(COA_PolyZoneEffectHandler handler, IEntity ent)
	{
		
	}
	
	override COA_EffectContainer GetEffectContainer()
	{
		COA_EffectContainer effect = new COA_EffectContainer();
		effect.m_iId = m_iId;
		effect.m_fTime = m_fKillTime;
		effect.m_iType = COA_EPolyZoneEffectHUDType.RestrictedZone;
		return effect;
	}
	
	override COA_PolyZoneEffect CreateCopyObject()
	{
		return new COA_PolyZoneEffectRestricted();
	}
	
	override void CopyFields(COA_PolyZoneEffect effect)
	{
		COA_PolyZoneEffectRestricted effectCurrent = COA_PolyZoneEffectRestricted.Cast(effect);
		effectCurrent.m_fKillTime = m_fKillTime;
	}
}