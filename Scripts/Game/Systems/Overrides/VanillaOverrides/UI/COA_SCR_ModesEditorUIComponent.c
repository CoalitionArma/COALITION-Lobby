//------------------------------------------------------------------------------------------------
//! Lists the Lobby's Tactical Camera (see COA_TacticalCamera) in the editor's mode dropdown.
//! The dropdown only shows modes that are in the editor core config, and the vanilla config does
//! not list STRATEGY, so the entry is added here from the Lobby's own config instead. Access to
//! the mode itself is still decided server-side by the Lobby's SCR_EditorManagerEntity mod.
modded class SCR_ModesEditorUIComponent
{
	protected static const ResourceName COA_TACTICAL_MODE_CONF = "{6A6612AB7E3CA003}Configs/Editor/COA_TacticalEditorMode.conf";

	protected ref SCR_EditorModePrefab m_COA_TacticalModePrefab;

	//------------------------------------------------------------------------------------------------
	override protected void CreateDropdown()
	{
		COA_AddTacticalMode();
		super.CreateDropdown();
	}

	//------------------------------------------------------------------------------------------------
	protected void COA_AddTacticalMode()
	{
		if (!COA_Gamemode.GetInstance())
			return;

		// Already listed (another mod or a config override may provide STRATEGY)
		for (int i = 0, count = m_aOrderedEditorModePrefabs.Count(); i < count; i++)
		{
			SCR_EditorModePrefab existing = m_aOrderedEditorModePrefabs[i];
			if (existing && existing.GetMode() == EEditorMode.STRATEGY)
				return;
		}

		if (!m_COA_TacticalModePrefab)
		{
			Resource resource = Resource.Load(COA_TACTICAL_MODE_CONF);
			if (!resource || !resource.IsValid())
			{
				Print("[Coalition Lobby] Tactical camera: could not load " + COA_TACTICAL_MODE_CONF, LogLevel.ERROR);
				return;
			}

			m_COA_TacticalModePrefab = SCR_EditorModePrefab.Cast(BaseContainerTools.CreateInstanceFromContainer(resource.GetResource().ToBaseContainer()));
		}

		if (!m_COA_TacticalModePrefab)
			return;

		int order = 0;
		if (m_COA_TacticalModePrefab.GetInfo())
			order = m_COA_TacticalModePrefab.GetInfo().GetOrder();

		m_aOrderedEditorModePrefabs.Insert(order, m_COA_TacticalModePrefab);
	}
}
