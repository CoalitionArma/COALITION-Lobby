modded class CVON_RadioComponent
{
	//Handles assigning 
	//==========================================================================================================================================================================
	override void InitializeRadios()
	{
		//Have this ifdef here cause in the workshop its a listen server, I explain the code in the non workshop version.
		#ifdef WORKBENCH
		if (m_sFactionKey != "")
			return;
	
		// The character carrying the radio (GetRootParent() is the vehicle for a seated player)
		IEntity carrier = GetCarrier();
		if (!SCR_ChimeraCharacter.Cast(carrier))
			return;
		
		FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(carrier.FindComponent(FactionAffiliationComponent));
		if (!factionComp)
			return;
		m_sFactionKey = factionComp.GetAffiliatedFactionKey();
		m_sOriginalFactionKey = factionComp.GetAffiliatedFactionKey();
		SCR_FactionManager factionMan = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		SCR_Faction faction = SCR_Faction.Cast(factionMan.GetFactionByKey(m_sFactionKey));
		m_aChannels.InsertAll(factionMan.GetFactionActiveChannelSR(faction.GetFactionKey()));
		m_aChannels.InsertAll(factionMan.GetFactionActiveChannelLR(faction.GetFactionKey()));
		if (m_aChannels.Count() == 0)
		{
			m_aChannels.Insert("55500");
			m_iCurrentChannel = 1;
			m_sFrequency = m_aChannels.Get(0);
		}
		Replication.BumpMe();
		#else
		if (System.IsConsoleApp())
		{
			//Faction found no longer needed.
			if (m_sFactionKey != "")
				return;

			//Radio is in the inventory of a player (GetRootParent() is the vehicle for a seated player)
			IEntity carrier = GetCarrier();
			if (!SCR_ChimeraCharacter.Cast(carrier))
				return;

			FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(carrier.FindComponent(FactionAffiliationComponent));
			if (!factionComp)
				return;

			m_sFactionKey = factionComp.GetAffiliatedFactionKey();
			m_sOriginalFactionKey = factionComp.GetAffiliatedFactionKey();
			//Add that faction to this radio and get the faction to prep to load the factions frequencies
			SCR_FactionManager factionMan = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			SCR_Faction faction = SCR_Faction.Cast(factionMan.GetFactionByKey(m_sFactionKey));
			m_aChannels.InsertAll(factionMan.GetFactionActiveChannelSR(faction.GetFactionKey()));
			m_aChannels.InsertAll(factionMan.GetFactionActiveChannelLR(faction.GetFactionKey()));
			//If there are no frequencies added, just add a default channel.
			if (m_aChannels.Count() == 0)
			{
				m_aChannels.Insert("55500");
				m_iCurrentChannel = 1;
				m_sFrequency = m_aChannels.Get(0);
			}
			//hehe
			Replication.BumpMe();
		}
		#endif
	}
}