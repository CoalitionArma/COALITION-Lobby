#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "6 | Generate Config File",
	description: "Generate Mission Configuration File", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Lobby Mission Plugins",
	awesomeFontCode: 0xF0C7)
] 
class COA_MissionConfigurationPlugin : WorkbenchPlugin
{	
	//------------------------------------------------------------------------------------
	[Attribute("<Author>", "auto", "", category: "Mission Config - Mission Info")]
	protected string m_sMissionAuthor;

	//! Written to the mission header as m_sAuthorGUID, which CRF's modded SCR_MissionHeader declares
	//! and CRF's COA_Gamemode reads to hand the mission maker admin privileges automatically.
	//! Harmless when CRF isn't loaded - the header simply has no such field to set.
	[Attribute("", "auto", "Your BI account GUID for automatic admin privileges (auto-filled from workbench)", category: "Mission Config - Mission Info")]
	protected string m_sMissionAuthorGUID;

	[Attribute(uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(COA_EGamemode), category: "Mission Config - Mission Info")]
	COA_EGamemode m_MissionMode;
	
	[Attribute("<Name>", "auto", "", category: "Mission Config - Mission Info")]
	protected string m_sMissionName;
	
	[Attribute("<Description>", "auto", "", category: "Mission Config - Mission Info")]
	protected string m_sMissionDescription;

	protected const string SCENARIOS_PATH = "Missions";

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		m_sMissionAuthor = "<Author>";

		// Auto-fill GUID from currently logged-in Workbench user
		BackendApi backendApi = GetGame().GetBackendApi();
		if (backendApi)
		{
			UUID identityId = BackendAuthenticatorApi.GetIdentityId();
			if (identityId && !identityId.IsNull())
				m_sMissionAuthorGUID = identityId; // UUID extends string, can be assigned directly
			else
				m_sMissionAuthorGUID = "<AuthorGUID - Not logged in to BI account>";
		}
		else
		{
			m_sMissionAuthorGUID = "<AuthorGUID - Backend not available>";
		}

		m_MissionMode = COA_EGamemode.TVT;
		m_sMissionName = "<Name>";
		m_sMissionDescription = "<Description>";

		if (!Workbench.ScriptDialog(
		"Mission Config Generator", 
		"This will automatically generate and sort the mission configuration file. \n\n WARNING: DO NOT RUN THIS TWICE FOR ONE MISSION, SIMPLY GO TO THE ALREADY CREATED CONFIG AND MANUALLY UPDATE IT.", 
		this))
		
			return;
	}
	
	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Cancel")]
	protected bool ButtonCancel()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Re-shows the synopsis preview (and re-copies it to the clipboard) without regenerating the
	//! mission config - lets a mission maker re-open the synopsis after closing the window, without
	//! risking a second .conf being generated (see the "DO NOT RUN THIS TWICE" warning above).
	[ButtonAttribute("Show Mission Synopsis")]
	protected bool ButtonShowSynopsis()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return false;

		WorldEditorAPI api = worldEditor.GetApi();

		IEntitySource entitySource = api.FindEntityByName("COA_Lobby");
		if (!entitySource)
			return false;

		COA_Gamemode gamemode = COA_Gamemode.Cast(api.SourceToEntity(entitySource));
		if (!gamemode)
			return false;

		string missionMode = SCR_Enum.GetEnumName(COA_EGamemode, m_MissionMode);

		int missionPlayercount = GetPlayerCount(gamemode.GetSlots("BLUFOR"));
		missionPlayercount = missionPlayercount + GetPlayerCount(gamemode.GetSlots("OPFOR"));
		missionPlayercount = missionPlayercount + GetPlayerCount(gamemode.GetSlots("INDFOR"));
		missionPlayercount = missionPlayercount + GetPlayerCount(gamemode.GetSlots("CIV"));

		string worldPath;
		api.GetWorldPath(worldPath);

		array<string> strArray = {};
		worldPath.Split("/", strArray, false);
		string missionTerrain = strArray.Get(strArray.Count() - 2);

		string missionDisplayName = string.Format("COA %1%2 %3", missionMode, missionPlayercount, m_sMissionName);

		ShowMissionSynopsis(gamemode, entitySource, missionDisplayName, missionTerrain, missionMode, missionPlayercount);

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Hands the synopsis to the mission maker via clipboard + a preview window. Mods extend the
	//! synopsis itself by modding COA_MissionSynopsisGenerator, not this plugin - see the note on
	//! that class for why a modded plugin never runs.
	protected void ShowMissionSynopsis(COA_Gamemode gamemode, IEntitySource entitySource, string missionDisplayName, string missionTerrain, string missionMode, int missionPlayercount)
	{
		COA_MissionSynopsisGenerator synopsisGenerator = new COA_MissionSynopsisGenerator();
		synopsisGenerator.m_sMissionAuthor = m_sMissionAuthor;
		synopsisGenerator.m_sMissionDescription = m_sMissionDescription;

		synopsisGenerator.ShowMissionSynopsis(gamemode, entitySource, missionDisplayName, missionTerrain, missionMode, missionPlayercount);
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Generate Mission Config", true)]
	protected bool ButtonNext()
	{
		string missionMode = SCR_Enum.GetEnumName(COA_EGamemode, m_MissionMode);
		int missionPlayercount;
		string worldPath;

		//--- Get mission header from the template config (can't use the class directly, it's engine-controlled class that cannot have reference in script)
		Resource templateResource = Resource.Load("{3D094352621EA88C}Missions/COA_BaseMissionConfig.conf");
		BaseContainer missionHeaderContainer = templateResource.GetResource().ToBaseContainer();

		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		WorldEditorAPI api = worldEditor.GetApi();

		api.GetWorldPath(worldPath);

		//--- Get world path with GUID and save it to the header
		ResourceManager resourceManager = Workbench.GetModule(ResourceManager);
		string absWorldPath;
		Workbench.GetAbsolutePath(worldPath, absWorldPath);
		MetaFile worldMeta = resourceManager.GetMetaFile(absWorldPath);
		string fullWorldPath = worldMeta.GetResourceID();
		missionHeaderContainer.Set("World", fullWorldPath);
		missionHeaderContainer.Set("m_sAuthor", m_sMissionAuthor);
		missionHeaderContainer.Set("m_sAuthorGUID", m_sMissionAuthorGUID);
		missionHeaderContainer.Set("m_sGameMode", missionMode);
		missionHeaderContainer.Set("m_sDescription", m_sMissionDescription);
		missionHeaderContainer.Set("m_iMapMarkerLimitPerPlayer", 256);
		missionHeaderContainer.Set("m_iPlayerCount", 128);

		IEntitySource entitySource = api.FindEntityByName("COA_Lobby");

		if (!entitySource)
			return false;

		COA_Gamemode gamemode = COA_Gamemode.Cast(api.SourceToEntity(entitySource));

		if (gamemode)
		{
			missionPlayercount = GetPlayerCount(gamemode.GetSlots("BLUFOR"));
			missionPlayercount = missionPlayercount + GetPlayerCount(gamemode.GetSlots("OPFOR"));
			missionPlayercount = missionPlayercount + GetPlayerCount(gamemode.GetSlots("INDFOR"));
			missionPlayercount = missionPlayercount + GetPlayerCount(gamemode.GetSlots("CIV"));
		};

		missionHeaderContainer.Set("m_sName", string.Format("COA %1%2 %3", missionMode, missionPlayercount, m_sMissionName));

		//--- Get target config path
		string fileSystem = FilePath.FileSystemNameFromFileName(worldPath);
		fileSystem = SCR_AddonTool.ToFileSystem(fileSystem);

		array<string> strArray = {};
		worldPath.Split("/", strArray, false);

		string missionTerrain = strArray.Get(strArray.Count() - 2);
		missionHeaderContainer.Set("m_sTerrainName", missionTerrain);

		string relativeDirPath = fileSystem + SCENARIOS_PATH + "/" + missionTerrain;
		string absoluteDirPath;
		if (!Workbench.GetAbsolutePath(relativeDirPath, absoluteDirPath, true)) // the Missions directory does not exist
		{
			if (!Workbench.GetAbsolutePath(relativeDirPath, absoluteDirPath, false))
			{
				Print("Unable to obtain the " + SCENARIOS_PATH + " directory path at " + relativeDirPath, LogLevel.ERROR);
				return false;
			}

			if (!FileIO.MakeDirectory(absoluteDirPath))
			{
				Print("Unable to create the " + SCENARIOS_PATH + " directory at " + absoluteDirPath, LogLevel.ERROR);
				return false;
			}

			Print("Successfully created the " + SCENARIOS_PATH + " directory at " + absoluteDirPath, LogLevel.NORMAL);
		}

		DateTimeUtcAsInt time = Workbench.GetPackedUtcTime();
		string monthFinal;
		int month = time.GetMonth();
		if (month < 10)
			monthFinal = "0";

		monthFinal = monthFinal + month.ToString();

		string dayFinal;
		int day = time.GetDay();
		if (day < 10)
			dayFinal = "0";

		dayFinal = dayFinal + day.ToString();

		string missionBasePath = FilePath.Concat(relativeDirPath, string.Format("%1_%2%3_%4%5_%6", SCR_StringHelper.Filter(m_sMissionAuthor, SCR_StringHelper.ALPHANUMERICAL), monthFinal, dayFinal, missionMode, missionPlayercount, SCR_StringHelper.Filter(m_sMissionName, SCR_StringHelper.ALPHANUMERICAL)));
		string missionHeaderPath = FilePath.AppendExtension(missionBasePath, "conf");

		//--- Create the config
		if (!BaseContainerTools.SaveContainer(missionHeaderContainer, ResourceName.Empty, missionHeaderPath))
		{
			Print(string.Format("Unable to create mission header at %1!", missionHeaderPath), LogLevel.ERROR);
			return false;
		}

		string missionHeaderAbsPath;
		Workbench.GetAbsolutePath(missionHeaderPath, missionHeaderAbsPath, false);
		resourceManager.RegisterResourceFile(missionHeaderAbsPath, false);

		//--- Build the mission synopsis and hand it to the mission maker via clipboard + a preview window
		//--- (NOT written to disk - a file here would get committed and bundled into the mod itself, which
		//--- we don't want. The mission maker pastes this into the PR description instead.)
		string missionDisplayName = string.Format("COA %1%2 %3", missionMode, missionPlayercount, m_sMissionName);
		ShowMissionSynopsis(gamemode, entitySource, missionDisplayName, missionTerrain, missionMode, missionPlayercount);

		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected int GetPlayerCount(array<ref COA_SlottingGroup> factionSlots)
	{
		int missionPlayercount;

		foreach (ref COA_SlottingGroup slotGroup : factionSlots)
			foreach(COA_EGearRole role : slotGroup.m_aSlots)
				missionPlayercount++;

		return missionPlayercount;
	}
}
#endif