//----------------------------------------------------------------
// ArmaVision (editor photo mode) camera.
//
// Vanilla locks the ArmaVision camera whenever the editor is limited, the session is
// multiplayer and the mission header does not set m_bIsArmavisionAllowedInMP - none of
// ours do. While locked, SCR_CameraEditorComponent.CreateCamera() deletes the camera
// instead of spawning it, so SCR_CameraEditorComponent.GetCameraInstance() stays null.
//
// The "Edit ArmaVision properties" button (UI/layouts/Editor/Buttons/Button_CameraSettings,
// action "EditorAttributes", V by default) passes that camera to the attributes manager,
// and every attribute in Configs/Editor/AttributeLists/Photo.conf starts with
// "if (!item.IsInherited(SCR_CameraBase)) return null;". With a null camera they all bail
// out, so the properties window opens with nothing in it - no overlays, frames, filters
// or lens flare.
//
// Spectators fly on their own spectator camera, so ArmaVision otherwise looks like it
// works; only the properties are dead. Unlock the camera for the players who are meant to
// have full editor access so the attributes resolve against a real camera entity.
//----------------------------------------------------------------
modded class SCR_CameraLimitedEditorComponent
{
	//----------------------------------------------------------------
	override protected void EOnEditorPreActivate()
	{
		super.EOnEditorPreActivate();

		if (!COA_Gamemode.GetInstance())
			return;

		if (!SCR_EditorManagerEntity.COA_HasUnlimitedEditorAccess())
			return;

		// Cleared before the camera is spawned on the next frame, so the unlimited
		// ArmaVision camera prefab is used and CreateCamera() is allowed to run.
		m_bIsLimited = false;
		m_bIsLocked = false;
	}
}
