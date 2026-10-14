[ComponentEditorProps(category: "GameScripted/Editor", description: "Tactical Camera context actions: right-clicking open ground keeps the selected squad", icon: "WBData/ComponentEditorProps/componentEditor.png")]
class COA_TacticalContextActionsEditorComponentClass : SCR_ContextActionsEditorComponentClass
{
}

//------------------------------------------------------------------------------------------------
//! Context actions of the Tactical Camera (COA_EditorModeTactical.et). The vanilla component clears
//! the selection when the menu is opened over open ground, which would leave nothing to order -
//! here the selection is kept, so "select a squad, right-click where it should go" works.
//! Right-clicking a unit outside the selection still selects that unit first, like Game Master.
class COA_TacticalContextActionsEditorComponent : SCR_ContextActionsEditorComponent
{
	//------------------------------------------------------------------------------------------------
	override protected int ValidateSelection(bool isInstant)
	{
		if (m_StateManager && m_StateManager.GetState() != EEditorState.SELECTING)
			return super.ValidateSelection(isInstant);

		m_HoveredEntity = null;
		if (m_HoverManager)
			m_HoveredEntity = m_HoverManager.GetFirstEntity();

		if (m_HoveredEntity && m_SelectedManager && !m_SelectedManager.Contains(m_HoveredEntity))
			m_SelectedManager.Replace(m_HoveredEntity);

		m_SelectedEntities.Clear();
		if (m_SelectedManager)
			m_SelectedManager.GetEntities(m_SelectedEntities);

		if (m_HoveredEntity && m_SelectedEntities.IsEmpty())
			m_SelectedEntities.Insert(m_HoveredEntity);

		EEditorContextActionFlags flags;
		if (GetManager().IsLimited())
			flags |= EEditorContextActionFlags.LIMITED;

		if (GetGame().GetInputManager().IsUsingMouseAndKeyboard())
			flags |= EEditorContextActionFlags.USING_MOUSE_AND_KEYBOARD;

		return flags;
	}
}
