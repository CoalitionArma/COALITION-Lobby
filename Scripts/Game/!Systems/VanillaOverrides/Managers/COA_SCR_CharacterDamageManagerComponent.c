 /*
//! COA_SCR_CharacterDamageManagerComponent
//! Tracks damage events for weapon logging to fix issues with incorrect weapons being reported.
//! Also broadcasts cause-of-death damage type to all clients when a player dies.
*/
modded class SCR_CharacterDamageManagerComponent
{
	protected ref BaseDamageEffect m_eFatalDamageEffect;
	
	//------------------------------------------------------------------------------------------------
	BaseDamageEffect GetFatalDamageEffect()
	{
		return m_eFatalDamageEffect;
	}
	
	//------------------------------------------------------------------------------------------------
	//!	Invoked when damage state changes.
	protected override void OnDamageStateChanged(EDamageState newState, EDamageState previousDamageState, bool isJIP)
	{
		super.OnDamageStateChanged(newState, previousDamageState, isJIP);
		
		if (newState == EDamageState.DESTROYED)
		{
			BaseDamageEffect lastValidDamageEffect;
			array<ref BaseDamageEffect> baseDamageEffects = {};
			GetDamageHistory(baseDamageEffects);
			
			if (!baseDamageEffects.IsEmpty())
			{
				// Iterate in reverse — history is oldest-first, so we walk backwards
				// to find the most recent meaningful damage (the killing blow).
				for (int i = baseDamageEffects.Count() - 1; i >= 0; i--)
				{
					BaseDamageEffect damageEffect = baseDamageEffects.Get(i);
					EDamageType dt = damageEffect.GetDamageType();
					if (dt != EDamageType.TRUE && dt != EDamageType.REGENERATION && dt != EDamageType.HEALING)
					{
						lastValidDamageEffect = damageEffect;
						break;
					}
				}
				
				// If every entry was TRUE/HEALING/REGEN, fall back to the most recent entry
				if (!lastValidDamageEffect)
					lastValidDamageEffect = baseDamageEffects.Get(baseDamageEffects.Count() - 1);
			};
			
			// Track damage for weapon logging (server-only, handled inside)
			if (lastValidDamageEffect)
			{
				m_eFatalDamageEffect = lastValidDamageEffect;
				COA_MarkLatestSpectatorDamageReportFatal();
			}
		}
	}
}
