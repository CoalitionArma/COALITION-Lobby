//------------------------------------------------------------------------------------------------
//! "Exit" button on the Tactical Camera HUD (COA_ModeTactical.layout): closes the editor.
class COA_TacticalExitButton : ScriptedWidgetComponent
{
	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button != 0)
			return false;

		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (editorManager)
			GetGame().GetCallqueue().CallLater(editorManager.Close, 1, false, false); // cannot close the UI from its own handler

		return true;
	}
}
