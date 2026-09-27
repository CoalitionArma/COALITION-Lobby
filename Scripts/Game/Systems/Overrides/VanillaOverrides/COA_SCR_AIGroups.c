modded class SCR_AIGroup
{
	//------------------------------------------------------------------------------------------------
	//! Forces a fresh replication snapshot of this group to already-connected clients. m_iGroupID is
	//! not an [RplProp] - it only ships as part of the group's full RplSave/RplLoad snapshot, which
	//! normally only goes out once, the first time a client observes the entity. Needed after
	//! SCR_GroupsManagerComponent.AssignGroupID() is called on an already-replicated GM-placed group
	//! (see COA_SlottingManager.RegisterGMPossessionGroup), so already-connected clients pick up the
	//! new ID - and therefore sort correctly in COA_SlottingMenu - without needing to reconnect.
	void COA_ForceResync()
	{
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	//! Vanilla appends " ( <original callsign> )" to a group's custom name (see base
	//! SCR_AIGroup.GetCustomNameWithOriginal) so player-named squads still show their real callsign
	//! alongside a nickname. GM-placed groups never got a proper company/platoon/squad callsign
	//! assigned, so their "original" name is just a bare auto-numbered fallback (e.g. "13") - showing
	//! it after a GM-typed name is just noise, not a meaningful callsign. Skip the suffix entirely
	//! for groups COA_SlottingManager.RegisterGMPossessionGroup has registered.
	override string GetCustomNameWithOriginal()
	{
		if (COA_GMPossessionManager.GetInstance().IsGroupAlreadyRegistered(this))
			return GetCustomName();

		// See GetCustomName - skip vanilla's SocialComponent.IsRestricted check for script-named groups
		if (m_iNameAuthorID <= 0)
		{
			string company, platoon, squad, character, format;
			GetCallsigns(company, platoon, squad, character, format);
			string originalName = string.Format(format, company, platoon, squad, character);

			if (m_sCustomName.IsEmpty())
				return originalName;

			return m_sCustomName + " ( " + originalName + " )";
		}

		return super.GetCustomNameWithOriginal();
	}

	//------------------------------------------------------------------------------------------------
	//! Group names/descriptions set by script (slotting, GM possession) have no player author (ID <= 0).
	//! Vanilla still asks SocialComponent.IsRestricted about that author, which logs
	//! "SocialComponent::IsRestricted: Invalid otherPlayerID." on every call - and nametags and spectator
	//! group icons call these every frame (millions of client log lines per session). A non-player author
	//! can never be restricted, so return the text directly.
	override string GetCustomName()
	{
		if (m_iNameAuthorID <= 0)
			return m_sCustomName;

		return super.GetCustomName();
	}

	//------------------------------------------------------------------------------------------------
	override string GetCustomDescription()
	{
		if (m_iDescriptionAuthorID <= 0)
			return m_sCustomDescription;

		return super.GetCustomDescription();
	}
}
