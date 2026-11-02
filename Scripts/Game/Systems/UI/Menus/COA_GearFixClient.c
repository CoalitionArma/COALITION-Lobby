//------------------------------------------------------------------------------------------------
//! Client side of the Gear Fix tool: what the server last sent this staff member, and the request to
//! open the tab on a particular faction and slot. Only staff ever receive this data.
class COA_GearFixClient
{
	static ref array<ref COA_GearOverride> s_aOverrides = {};
	static ref array<ref COA_GearCheckIssue> s_aIssues = {};
	static int s_iErrors;
	static int s_iWarnings;

	//! Fired when overrides or issues changed
	static ref ScriptInvoker s_OnChanged = new ScriptInvoker();

	//! (bool success, string message) - outcome of this admin's own request
	static ref ScriptInvoker s_OnResult = new ScriptInvoker();

	protected static ref array<ref COA_GearCheckIssue> s_aIncomingIssues = {};

	protected static bool s_bPendingOpen;
	protected static string s_sPendingFaction;
	protected static int s_iPendingRole = -1;
	protected static int s_iPendingSlot = -1;
	protected static ResourceName s_rPendingResource;

	//------------------------------------------------------------------------------------------------
	static void ReceiveOverrides(array<string> overrides)
	{
		s_aOverrides.Clear();
		if (overrides)
		{
			foreach (string packed : overrides)
			{
				COA_GearOverride gearOverride = COA_GearOverride.Unpack(packed);
				if (gearOverride)
					s_aOverrides.Insert(gearOverride);
			}
		}

		s_OnChanged.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	//! The list arrives in batches; it only replaces the old one when the last batch is in
	static void ReceiveIssues(array<string> issues, bool first, bool last, int errors, int warnings, bool showCard)
	{
		if (first)
			s_aIncomingIssues.Clear();

		if (issues)
		{
			foreach (string packed : issues)
			{
				COA_GearCheckIssue issue = COA_GearCheckIssue.Unpack(packed);
				if (issue)
					s_aIncomingIssues.Insert(issue);
			}
		}

		if (!last)
			return;

		s_aIssues.Clear();
		foreach (COA_GearCheckIssue incoming : s_aIncomingIssues)
			s_aIssues.Insert(incoming);

		s_aIncomingIssues.Clear();
		s_iErrors = errors;
		s_iWarnings = warnings;
		s_OnChanged.Invoke();

		if (showCard)
			COA_GearCheckCard.Refresh();
	}

	//------------------------------------------------------------------------------------------------
	static void ReceiveResult(bool success, string message)
	{
		s_OnResult.Invoke(success, message);
	}

	//------------------------------------------------------------------------------------------------
	static bool IsStaff()
	{
		if (SCR_Global.IsAdmin())
			return true;

		COA_PermissionManager permissionManager = COA_PermissionManager.GetInstance();
		return permissionManager != null && permissionManager.IsModerator();
	}

	//------------------------------------------------------------------------------------------------
	//! Open the admin menu's Gear Fix tab on a faction and slot/role
	static void OpenTab(string faction, int role, int slot, ResourceName resource)
	{
		s_bPendingOpen = true;
		s_sPendingFaction = faction;
		s_iPendingRole = role;
		s_iPendingSlot = slot;
		s_rPendingResource = resource;

		MenuManager menuManager = GetGame().GetMenuManager();
		COA_AdminMenu adminMenu = COA_AdminMenu.Cast(menuManager.FindMenuByPreset(ChimeraMenuPreset.COA_AdminMenu));
		if (adminMenu)
			adminMenu.GearFixButton();
		else
			menuManager.OpenMenu(ChimeraMenuPreset.COA_AdminMenu);
	}

	//------------------------------------------------------------------------------------------------
	//! COA_AdminMenu: should it open on the Gear Fix tab?
	static bool HasPendingOpen()
	{
		return s_bPendingOpen;
	}

	//------------------------------------------------------------------------------------------------
	//! COA_GearFixPanel: take the requested faction and slot, once
	static bool ConsumePendingOpen(out string faction, out int role, out int slot, out ResourceName resource)
	{
		if (!s_bPendingOpen)
			return false;

		s_bPendingOpen = false;
		faction = s_sPendingFaction;
		role = s_iPendingRole;
		slot = s_iPendingSlot;
		resource = s_rPendingResource;
		return true;
	}
}

//------------------------------------------------------------------------------------------------
//! Compact warning shown to staff only, before the game goes live, when the gear check found
//! something: issue count, the worst few, and a button to the Gear Fix tab on the first one. Removes
//! itself when the game goes live. Buttons need the cursor, so it's also pointed at from the card's
//! text for when no menu is open. Nothing is created on a dedicated server.
class COA_GearCheckCard
{
	protected static const ResourceName LAYOUT = "{7C41E0A95D3B862F}UI/layouts/Hint/GearCheckCard.layout";
	protected static const int TICK_MS = 2000;
	protected static const int TOP_ISSUES = 3;
	protected static const int MAX_LINE_CHARS = 90;

	protected static ref COA_GearCheckCard s_Instance;
	protected static int s_iDismissedTotal = -1;	// dismissed at this many issues; a different count shows it again
	protected static COA_Gamemode s_DismissedIn;	// statics outlive the mission; a new mission shows it again

	protected Widget m_wRoot;

	//------------------------------------------------------------------------------------------------
	//! Show or update the card for the current results
	static void Refresh()
	{
		if (System.IsConsoleApp() || !COA_GearFixClient.IsStaff())
			return;

		int total = COA_GearFixClient.s_iErrors + COA_GearFixClient.s_iWarnings;
		if (total <= 0 || (total == s_iDismissedTotal && s_DismissedIn == COA_Gamemode.GetInstance()))
			return;

		if (!s_Instance)
			s_Instance = new COA_GearCheckCard();

		s_Instance.Show();
	}

	//------------------------------------------------------------------------------------------------
	protected void Show()
	{
		if (!m_wRoot)
		{
			m_wRoot = GetGame().GetWorkspace().CreateWidgets(LAYOUT);
			if (!m_wRoot)
				return;

			COA_UIPolish.AttachHoverEffects(m_wRoot);

			SCR_ButtonTextComponent openButton = SCR_ButtonTextComponent.GetButtonText("GearCardOpenButton", m_wRoot);
			if (openButton)
				openButton.m_OnClicked.Insert(OnOpenClicked);

			SCR_ButtonTextComponent dismissButton = SCR_ButtonTextComponent.GetButtonText("GearCardDismissButton", m_wRoot);
			if (dismissButton)
				dismissButton.m_OnClicked.Insert(OnDismissClicked);

			GetGame().GetCallqueue().CallLater(Tick, TICK_MS, true);
		}

		TextWidget title = TextWidget.Cast(m_wRoot.FindAnyWidget("GearCardTitle"));
		if (title)
			title.SetText(string.Format("GEAR CHECK: %1 ERRORS, %2 WARNINGS", COA_GearFixClient.s_iErrors, COA_GearFixClient.s_iWarnings));

		TextWidget body = TextWidget.Cast(m_wRoot.FindAnyWidget("GearCardBody"));
		if (body)
			body.SetText(BuildBody());
	}

	//------------------------------------------------------------------------------------------------
	protected string BuildBody()
	{
		array<string> lines = {};
		foreach (COA_GearCheckIssue issue : COA_GearFixClient.s_aIssues)
		{
			if (issue.m_iSeverity == COA_EGearCheckSeverity.INFO)
				continue;

			string line = issue.ToListLine();
			if (line.Length() > MAX_LINE_CHARS)
				line = line.Substring(0, MAX_LINE_CHARS) + "...";

			lines.Insert(line);
			if (lines.Count() >= TOP_ISSUES)
				break;
		}

		return SCR_StringHelper.Join("\n", lines);
	}

	//------------------------------------------------------------------------------------------------
	//! Gone once the game is live
	protected void Tick()
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode)
		{
			Close();
			return;
		}

		bool preGame = gamemode.m_GamemodeState == COA_EGamemodeState.BRIEFING || gamemode.m_GamemodeState == COA_EGamemodeState.SLOTTING;
		if (!preGame && gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
		{
			COA_SafestartManager safestartManager = COA_SafestartManager.GetInstance();
			preGame = safestartManager != null && safestartManager.GetSafestartStatus();
		}

		if (!preGame)
			Close();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnOpenClicked()
	{
		// The first (worst) issue with a faction decides where the tab opens
		string faction;
		int role = -1;
		int slot = -1;
		ResourceName resource;
		foreach (COA_GearCheckIssue issue : COA_GearFixClient.s_aIssues)
		{
			if (issue.m_iSeverity == COA_EGearCheckSeverity.INFO)
				continue;

			faction = issue.m_sFaction;
			role = issue.m_iRole;
			slot = issue.m_iSlot;
			resource = issue.m_rResource;
			break;
		}

		Close();
		COA_GearFixClient.OpenTab(faction, role, slot, resource);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnDismissClicked()
	{
		s_iDismissedTotal = COA_GearFixClient.s_iErrors + COA_GearFixClient.s_iWarnings;
		s_DismissedIn = COA_Gamemode.GetInstance();
		Close();
	}

	//------------------------------------------------------------------------------------------------
	protected void Close()
	{
		GetGame().GetCallqueue().Remove(Tick);

		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();

		m_wRoot = null;
	}
}

//------------------------------------------------------------------------------------------------
//! Everything the loaded arsenals list, for the Gear Fix search. Mission factions' own catalogues are
//! usually empty (CRF's arsenals are gearscript-based), so besides any faction's catalogue this reads
//! the vanilla inventory catalogues directly - config layers stack by GUID, so that includes what mods
//! add to them, such as CRF's generated "CRF loaded mods" lists. Built on first use, once per session.
class COA_GearFixCatalog
{
	protected static const ref array<string> CATALOGS = {
		"{5F7EC52FC40A03E2}Configs/EntityCatalog/US/InventoryItems_EntityCatalog_US.conf",
		"{C53421647C3D0D2E}Configs/EntityCatalog/USSR/InventoryItems_EntityCatalog_USSR.conf",
		"{9D7E5804BB2E9B28}Configs/EntityCatalog/CIV/InventoryItems_EntityCatalog_CIV.conf"
	};

	static ref array<ResourceName> s_aWeapons = {};
	static ref array<ResourceName> s_aMagazines = {};
	static ref array<ResourceName> s_aItems = {};

	protected static bool s_bBuilt;
	protected static ref set<string> s_Seen = new set<string>();

	//------------------------------------------------------------------------------------------------
	static void Build()
	{
		if (s_bBuilt)
			return;

		s_bBuilt = true;

		FactionManager factionManager = GetGame().GetFactionManager();
		if (factionManager)
		{
			array<Faction> factions = {};
			factionManager.GetFactionsList(factions);
			foreach (Faction faction : factions)
			{
				SCR_Faction scriptedFaction = SCR_Faction.Cast(faction);
				if (scriptedFaction)
					AddCatalog(scriptedFaction.GetFactionEntityCatalogOfType(EEntityCatalogType.ITEM, false));
			}
		}

		SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
		if (catalogManager)
			AddCatalog(catalogManager.GetEntityCatalogOfType(EEntityCatalogType.ITEM, false));

		foreach (string catalogResource : CATALOGS)
			LoadCatalog(catalogResource);

		s_Seen.Clear();
		SortByFileName(s_aWeapons);
	}

	//------------------------------------------------------------------------------------------------
	//! Catalogue order is by mod; by file name, weapons of a kind (MG_, Rifle_, Handgun_) sit together
	protected static void SortByFileName(notnull array<ResourceName> prefabs)
	{
		array<string> keys = {};
		foreach (ResourceName prefab : prefabs)
		{
			string fileName = FilePath.StripPath(prefab);
			fileName.ToLower();
			keys.Insert(fileName + "|" + prefab);
		}

		keys.Sort();
		prefabs.Clear();
		foreach (string key : keys)
		{
			int separator = key.IndexOf("|");
			ResourceName sorted = key.Substring(separator + 1, key.Length() - separator - 1);
			prefabs.Insert(sorted);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Read straight from the config, not initialised: InitCatalog would load every listed prefab
	protected static void LoadCatalog(ResourceName resourceName)
	{
		Resource resource = BaseContainerTools.LoadContainer(resourceName);
		if (resource == null || !resource.IsValid())
			return;

		BaseResourceObject resourceObject = resource.GetResource();
		if (!resourceObject)
			return;

		SCR_EntityCatalog catalog = SCR_EntityCatalog.Cast(BaseContainerTools.CreateInstanceFromContainer(resourceObject.ToBaseContainer()));
		if (!catalog)
			return;

		AddCatalog(catalog);

		SCR_EntityCatalogMultiList multiList = SCR_EntityCatalogMultiList.Cast(catalog);
		if (!multiList)
			return;

		array<SCR_EntityCatalogMultiListEntry> lists = {};
		multiList.GetMultiList(lists);
		foreach (SCR_EntityCatalogMultiListEntry list : lists)
		{
			if (!list.m_aEntities)
				continue;

			foreach (SCR_EntityCatalogEntry entry : list.m_aEntities)
				AddEntry(entry);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddCatalog(SCR_EntityCatalog catalog)
	{
		if (!catalog)
			return;

		array<SCR_EntityCatalogEntry> entries = {};
		catalog.GetEntityList(entries);
		foreach (SCR_EntityCatalogEntry entry : entries)
			AddEntry(entry);
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddEntry(SCR_EntityCatalogEntry entry)
	{
		if (!entry || !entry.IsEnabled())
			return;

		ResourceName prefab = entry.GetPrefab();
		if (prefab.IsEmpty())
			return;

		string guid = SCR_ResourceNameUtils.GetPrefabGUID(prefab);
		if (s_Seen.Contains(guid))
			return;

		s_Seen.Insert(guid);

		SCR_ArsenalItem arsenalItem = SCR_ArsenalItem.Cast(entry.GetEntityDataOfType(SCR_ArsenalItem));
		if (!arsenalItem)
		{
			s_aItems.Insert(prefab);
			return;
		}

		int weaponTypes = SCR_EArsenalItemType.RIFLE | SCR_EArsenalItemType.PISTOL | SCR_EArsenalItemType.ROCKET_LAUNCHER | SCR_EArsenalItemType.MACHINE_GUN | SCR_EArsenalItemType.SNIPER_RIFLE;

		SCR_EArsenalItemMode mode = arsenalItem.GetItemMode();
		if (mode == SCR_EArsenalItemMode.AMMUNITION)
			s_aMagazines.Insert(prefab);
		else if ((arsenalItem.GetItemType() & weaponTypes) != 0 && mode != SCR_EArsenalItemMode.ATTACHMENT)
			s_aWeapons.Insert(prefab);
		else
			s_aItems.Insert(prefab);
	}
}
