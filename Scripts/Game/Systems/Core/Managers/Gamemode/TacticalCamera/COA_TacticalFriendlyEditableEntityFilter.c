//------------------------------------------------------------------------------------------------
//! The Tactical Camera's ACTIVE filter (COA_EditorModeTactical.et): only entities in the local player's
//! faction get through, always - allied factions don't (COA_TacticalCamera.IsSameSide). The vanilla SCR_FriendlyEditableEntityFilter only filters while the whole
//! editor is limited, and it isn't for staff - SCR_EditorManagerEntity.CanOpen (Lobby mod) unlimits
//! admins, moderators and spectators, and any Game Master mode unlimits the editor too - so they saw every
//! faction's players. It also took the side from whatever the player controlled when the editor opened;
//! this one uses the slot faction, like the rest of the tactical camera (COA_TacticalCamera).
//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(EEditableEntityState, "m_State")]
class COA_TacticalFriendlyEditableEntityFilter : SCR_BaseEditableEntityFilter
{
	protected Faction m_LocalFaction;

	//------------------------------------------------------------------------------------------------
	override bool CanAdd(SCR_EditableEntityComponent entity)
	{
		if (!entity)
			return false;

		Faction localFaction = GetLocalFaction();
		IEntity owner = entity.GetOwner();
		if (!localFaction || !owner)
			return false;

		FactionAffiliationComponent factionAffiliation = FactionAffiliationComponent.Cast(owner.FindComponent(FactionAffiliationComponent));
		if (!factionAffiliation)
			return false;

		return COA_TacticalCamera.IsSameSide(localFaction, factionAffiliation.GetAffiliatedFaction());
	}

	//------------------------------------------------------------------------------------------------
	//! The side is looked up again every time the editor opens (before the entities are filled)
	override void EOnEditorActivate()
	{
		m_LocalFaction = null;
		GetLocalFaction();
	}

	//------------------------------------------------------------------------------------------------
	//! The local player's slot faction - looked up until the faction manager knows it
	protected Faction GetLocalFaction()
	{
		if (!m_LocalFaction)
			m_LocalFaction = SCR_FactionManager.SGetLocalPlayerFaction();

		return m_LocalFaction;
	}
}
