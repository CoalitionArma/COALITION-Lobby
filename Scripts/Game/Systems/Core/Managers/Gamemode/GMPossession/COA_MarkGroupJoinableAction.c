//------------------------------------------------------------------------------------------------
//! GM-only editor context action: opens a naming dialog for a placed AI group, then (once the GM
//! confirms a name) marks it joinable - turning every member (including the leader) into an
//! individually pickable slot in the normal player-facing slotting menu. See
//! COA_SlottingManager.RegisterGMPossessionGroup for the actual slot creation and
//! COA_PlayerRplToAuthorityManager.RpcAsk_MarkGroupJoinable for the server-side call this dialog's
//! confirm button ultimately makes.
//!
//! IsServer() is true so CanBeShown/CanBePerformed (including the GAME_MASTER role check) are
//! re-validated server-side before anything happens, matching the framework's own
//! ActionPerformServer flow - but Perform() itself does nothing: the actual group registration
//! can't happen until the GM has typed a name, so it's deferred entirely to PerformOwner(), which
//! runs back on the initiating client once server validation passes, and just opens the dialog.
//!
//! Opt-in per group (not automatic for every group placed during safestart), since GMs regularly
//! place AI that's never meant to be player-joinable (scripted enemies, scenery, ambushes).
//!
//! NOTE: registering this action so it actually appears in the Game Master context menu is a
//! Workbench Attribute-Editor step (adding it to the GM editor mode's
//! SCR_ContextActionsEditorComponent.m_ActionsLists prefab attribute) - it cannot be done from
//! this script file alone.
[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class COA_MarkGroupJoinableAction : SCR_BaseEditorAction
{
	//------------------------------------------------------------------------------------------------
	override bool CanBeShown(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags)
	{
		if (!hoveredEntity)
			return false;

		COA_SafestartManager safestart = COA_SafestartManager.GetInstance();
		if (!safestart || !safestart.GetSafestartStatus())
			return false;

		if (hoveredEntity.GetEntityType() != EEditableEntityType.GROUP)
			return false;

		SCR_EditableGroupComponent groupEntity = SCR_EditableGroupComponent.Cast(hoveredEntity);
		if (!groupEntity)
			return false;

		SCR_AIGroup group = groupEntity.GetAIGroupComponent();
		if (!group || group.GetPlayerAndAgentCount() <= 0)
			return false;

		Faction faction = groupEntity.GetFaction();
		if (!faction || faction.GetFactionKey() == "CIV")
			return false;

		if (COA_GMPossessionManager.GetInstance().IsGroupAlreadyRegistered(group))
			return false;

		PlayerController localController = GetGame().GetPlayerController();
		if (!localController || !GetGame().GetPlayerManager().HasPlayerRole(localController.GetPlayerId(), EPlayerRole.GAME_MASTER))
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformed(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags)
	{
		return CanBeShown(hoveredEntity, selectedEntities, cursorWorldPosition, flags);
	}

	//------------------------------------------------------------------------------------------------
	//! Runs server-side once CanBePerformed re-validates - intentionally does nothing. Actual
	//! registration is deferred until the GM has typed a name in the dialog PerformOwner opens.
	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
	}

	//------------------------------------------------------------------------------------------------
	//! Runs back on the initiating GM's own client after server validation succeeds. Opens the
	//! naming dialog with enough context (the group's RplId + faction key) for its confirm button to
	//! ask the server to actually register the slots.
	override void PerformOwner(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		if (!hoveredEntity)
			return;

		SCR_EditableGroupComponent groupEntity = SCR_EditableGroupComponent.Cast(hoveredEntity);
		if (!groupEntity)
			return;

		SCR_AIGroup group = groupEntity.GetAIGroupComponent();
		Faction faction = groupEntity.GetFaction();
		if (!group || !faction)
			return;

		RplComponent groupRplComp = RplComponent.Cast(group.FindComponent(RplComponent));
		if (!groupRplComp)
			return;

		COA_GroupNamingDialog dialog = COA_GroupNamingDialog.Cast(GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.COA_GroupNamingDialog));
		if (dialog)
			dialog.Init(groupRplComp.Id(), faction.GetFactionKey());
	}

	//------------------------------------------------------------------------------------------------
	override bool IsServer()
	{
		return true;
	}
};
