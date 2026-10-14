
//------------------------------------------------------------------------------------------------
//! Tactical camera context action: give the selected friendly squad(s) an order at the cursor.
//! One instance per order type in COA_EditorModeTactical.et's context action list. Select a squad
//! first, then right-click the ground where the order should go (COA_TacticalContextActionsEditorComponent
//! keeps the selection for that right-click). Right-clicking a squad member orders that squad at
//! its own position. Runs on the leader's client and asks the server through
//! COA_PlayerRplToAuthorityManager.RequestTacticalOrder, which checks the leader's role and faction.
[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class COA_TacticalOrderAction : SCR_SelectedEntitiesContextAction
{
	[Attribute(SCR_Enum.GetDefault(COA_ETacticalOrder.MOVE), UIWidgets.ComboBox, "Order this action gives", "", ParamEnumArray.FromEnum(COA_ETacticalOrder))]
	protected COA_ETacticalOrder m_eOrder;

	// Squads already ordered during one Perform, so a squad selected through several of its
	// members is told once
	protected ref array<RplId> m_aOrderedGroups = {};

	//------------------------------------------------------------------------------------------------
	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		if (!COA_TacticalCamera.IsEnabled())
			return false;

		array<SCR_AIGroup> squads = {};
		return COA_TacticalCamera.GetPlayerSquads(selectedEntity, squads) > 0;
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
		COA_PlayerRplToAuthorityManager authorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (!authorityManager)
			return;

		array<SCR_AIGroup> squads = {};
		COA_TacticalCamera.GetPlayerSquads(selectedEntity, squads);
		foreach (SCR_AIGroup squad : squads)
		{
			RplId groupId = Replication.FindItemId(squad);
			if (groupId == RplId.Invalid() || m_aOrderedGroups.Contains(groupId))
				continue;

			m_aOrderedGroups.Insert(groupId);
			authorityManager.RequestTacticalOrder(groupId, m_eOrder, cursorWorldPosition);
		}
	}
}
