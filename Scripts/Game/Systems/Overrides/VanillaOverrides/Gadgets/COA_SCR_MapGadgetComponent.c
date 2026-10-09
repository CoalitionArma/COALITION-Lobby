//------------------------------------------------------------------------------------------------
//! While a leader is in the Tactical Camera their character holds the map up (see
//! COA_TacticalHintEditorComponent) purely as a visual for other players. Focusing the map gadget
//! would normally open the 2D map screen on top of the camera, so the focus is swallowed then.
modded class SCR_MapGadgetComponent
{
	override void ToggleFocused(bool enable)
	{
		if (enable && COA_TacticalCamera.IsLocalPlayerInTacticalCamera())
			return;

		super.ToggleFocused(enable);
	}
}
