//------------------------------------------------------------------------------------------------
//! Tactical camera context action: give the selected friendly squad(s) an order at the cursor.
//! One instance per order type in COA_EditorModeTactical.et's context action list. Runs on the
//! leader's client and asks the server through COA_PlayerRplToAuthorityManager.RequestTacticalOrder,
//! which checks the leader's role and faction before telling the squad.
[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class COA_TacticalOrderAction : SCR_SelectedEntitiesContextAction
{
	[Attribute(SCR_Enum.GetDefault(COA_ETacticalOrder.MOVE), UIWidgets.ComboBox, "Order this action gives", "", ParamEnumArray.FromEnum(COA_ETacticalOrder))]
	protected COA_ETacticalOrder m_eOrder;

	// Groups already ordered during one Perform, so a squad selected through several of its
	// members is told once
	protected ref array<RplId> m_aOrderedGroups = {};

	//------------------------------------------------------------------------------------------------
	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return COA_TacticalCamera.IsEnabled() && GetPlayerGroup(selectedEntity) != null;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformed(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return CanBeShown(selectedEntity, cursorWorldPosition, flags);
	}

	//------------------------------------------------------------------------------------------------
	override bool InitPerform()
	{
		m_aOrderedGroups.Clear();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void Perform(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition)
	{
		SCR_AIGroup group = GetPlayerGroup(selectedEntity);
		if (!group)
			return;

		RplId groupId = Replication.FindItemId(group);
		if (groupId == RplId.Invalid() || m_aOrderedGroups.Contains(groupId))
			return;

		m_aOrderedGroups.Insert(groupId);

		COA_PlayerRplToAuthorityManager authorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (authorityManager)
			authorityManager.RequestTacticalOrder(groupId, m_eOrder, cursorWorldPosition);
	}

	//------------------------------------------------------------------------------------------------
	//! The player squad a selected group or character belongs to, if it is on the leader's side
	protected SCR_AIGroup GetPlayerGroup(SCR_EditableEntityComponent entity)
	{
		if (!entity)
			return null;

		SCR_EditableGroupComponent groupEntity = SCR_EditableGroupComponent.Cast(entity);
		if (!groupEntity)
			groupEntity = SCR_EditableGroupComponent.Cast(entity.GetAIGroup());

		if (!groupEntity)
			return null;

		SCR_AIGroup group = groupEntity.GetAIGroupComponent();
		if (!group || group.GetPlayerCount() <= 0)
			return null;

		Faction localFaction = SCR_FactionManager.SGetLocalPlayerFaction();
		Faction groupFaction = group.GetFaction();
		if (!localFaction || !groupFaction || !localFaction.IsFactionFriendly(groupFaction))
			return null;

		return group;
	}
}
