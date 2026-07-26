[BaseContainerProps()]
class COA_PolyZoneEffectScreenBlure : COA_PolyZoneEffect
{
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
		effect.m_fTime = 10000;
		effect.m_iType = COA_EPolyZoneEffectHUDType.ScreenBlure;
		return effect;
	}
	
	override COA_PolyZoneEffect CreateCopyObject()
	{
		return new COA_PolyZoneEffectScreenBlure();
	}
	
	override void CopyFields(COA_PolyZoneEffect effect)
	{
		COA_PolyZoneEffectScreenBlure effectCurrent = COA_PolyZoneEffectScreenBlure.Cast(effect);
	}
}