//----------------------------------------------------------------
// Armavision properties open empty when BundeswehrMod is loaded.
//
// BWAR_AttributesManagerEditorComponent.c mods the constructor of
// SCR_AttributesManagerEditorComponentClass - the component's prefab data - and
// appends BWAR_Edit.conf to m_aAttributes there. That constructor runs for EVERY
// SCR_AttributesManagerEditorComponent on EVERY editor mode entity, so its five
// attributes land on Armavision as well as Edit mode. All five are m_bIsServer 1.
//
// SCR_AttributesManagerEditorComponent.StartEditing(items) defaults onlyServer to
// true, and takes a server round trip whenever any attribute in the list is a server
// attribute. That path resolves each item with Replication.FindItemId(). The manual
// camera is a purely local entity, so it returns -1, the camera is discarded, and the
// server is asked to edit an empty set - the dialog opens with "No properties".
//
// Every genuine camera attribute in Photo.conf is client-side, so the round trip is
// pointless for this target. Ask for local-only editing and the camera survives.
//
// Remove this once BundeswehrMod scopes its attributes to Edit mode.
//----------------------------------------------------------------
modded class SCR_AttributesButtonEditorUIComponent
{
	// Mirrors the ParamEnum values on SCR_AttributesButtonEditorUIComponent.m_iTarget.
	protected const int COA_TARGET_CAMERA = 1;

	//----------------------------------------------------------------
	override protected void OnButtonAction()
	{
		if (m_iTarget != COA_TARGET_CAMERA)
		{
			super.OnButtonAction();
			return;
		}

		SCR_ManualCamera camera = SCR_CameraEditorComponent.GetCameraInstance();
		if (!camera)
		{
			// Photo mode was locked, so its camera was never spawned.
			// See COA_SCR_CameraLimitedEditorComponent.
			Print("[COA] Armavision properties requested but no manual camera exists.", LogLevel.ERROR);
			return;
		}

		SCR_AttributesManagerEditorComponent manager = SCR_AttributesManagerEditorComponent.Cast(
			SCR_AttributesManagerEditorComponent.GetInstance(SCR_AttributesManagerEditorComponent));

		if (!manager)
		{
			super.OnButtonAction();
			return;
		}

		if (m_Category)
			manager.SetCurrentCategory(m_Category);

		// Explicitly array<Managed> - StartEditing rejects any other array type.
		array<Managed> items = {camera};
		manager.StartEditing(items, false);
	}
}
