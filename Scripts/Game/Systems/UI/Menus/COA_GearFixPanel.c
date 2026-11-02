//------------------------------------------------------------------------------------------------
//! A row of the Gear Fix target list: a weapon slot (slot >= 0) or a role's items (slot -1)
class COA_GearFixTarget
{
	int m_iRole = -1;
	int m_iSlot = -1;

	//------------------------------------------------------------------------------------------------
	void COA_GearFixTarget(int role, int slot)
	{
		m_iRole = role;
		m_iSlot = slot;
	}
}

//------------------------------------------------------------------------------------------------
//! Admin menu > Gear Fix (AdminMenuWidgets/GearFixMenu.layout), created and driven by COA_AdminMenu:
//! pick a faction and a slot or role, pick the replacement, apply it for the rest of the session,
//! see and revert active overrides, and read the gear check's results. The server does all the work
//! and re-checks everything (COA_GearFixManager); this only reads gearscripts to fill in the lists.
class COA_GearFixPanel
{
	protected static const int MAX_RESULTS = 100;
	protected static const int MAX_MODPACK_WEAPONS = 300;	// listed before a search narrows it down
	protected static const int MAX_MAGAZINES = 60;
	protected static const int DEFAULT_LAUNCHER_ROUNDS = 6;	// a weapon gaining a launcher has no old count to start from

	protected Widget m_wRoot;

	protected SCR_ListBoxComponent m_FactionList;
	protected SCR_ListBoxComponent m_TargetList;
	protected SCR_ListBoxComponent m_ResultList;
	protected SCR_ListBoxComponent m_SecondaryList;
	protected SCR_ListBoxComponent m_LauncherList;
	protected SCR_ListBoxComponent m_OverrideList;
	protected SCR_ListBoxComponent m_IssueList;

	protected EditBoxWidget m_wSearchBox;
	protected EditBoxWidget m_wCountInput;
	protected EditBoxWidget m_wLauncherCountInput;
	protected EditBoxWidget m_wAssistantCountInput;
	protected TextWidget m_wCurrentText;
	protected TextWidget m_wResultCaption;
	protected TextWidget m_wSecondaryCaption;
	protected TextWidget m_wCountCaption;
	protected TextWidget m_wAssistantCountCaption;
	protected TextWidget m_wStatusText;
	protected TextWidget m_wCheckSummary;
	protected TextWidget m_wIssueDetail;
	protected Widget m_wRegearCheck;
	protected SCR_ButtonTextComponent m_RegearButton;

	protected ref array<string> m_aFactions = {};
	protected ref array<ref COA_GearFixTarget> m_aTargets = {};
	protected ref array<ResourceName> m_aResults = {};
	protected ref array<ResourceName> m_aSecondary = {};
	protected ref array<ResourceName> m_aLauncher = {};
	protected ref array<int> m_aOverrideIds = {};
	protected ref array<ref COA_GearCheckIssue> m_aListedIssues = {};

	//! Attachments of each weapon in the faction's gearscript, carried over when one is picked
	protected ref map<ResourceName, ref array<ResourceName>> m_mWeaponAttachments = new map<ResourceName, ref array<ResourceName>>();

	protected int m_iMode = COA_EGearFixAction.WEAPON;
	protected bool m_bArsenalSource;
	protected string m_sLastSearch;

//=============================================================================================================================================================================================================================================================================================================================================================
//	 SETUP
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	void Init(notnull Widget root)
	{
		m_wRoot = root;

		m_FactionList = FindList("FactionListBox0");
		m_TargetList = FindList("TargetListBox0");
		m_ResultList = FindList("ResultListBox0");
		m_SecondaryList = FindList("SecondaryListBox0");
		m_LauncherList = FindList("LauncherListBox0");
		m_OverrideList = FindList("OverrideListBox0");
		m_IssueList = FindList("IssueListBox0");

		m_wSearchBox = EditBoxWidget.Cast(root.FindAnyWidget("SearchBox0"));
		m_wCountInput = EditBoxWidget.Cast(root.FindAnyWidget("CountInput"));
		m_wLauncherCountInput = EditBoxWidget.Cast(root.FindAnyWidget("LauncherCountInput"));
		m_wAssistantCountInput = EditBoxWidget.Cast(root.FindAnyWidget("AssistantCountInput"));
		m_wCurrentText = TextWidget.Cast(root.FindAnyWidget("CurrentText"));
		m_wResultCaption = TextWidget.Cast(root.FindAnyWidget("ResultCaption"));
		m_wSecondaryCaption = TextWidget.Cast(root.FindAnyWidget("SecondaryCaption"));
		m_wCountCaption = TextWidget.Cast(root.FindAnyWidget("CountCaption"));
		m_wAssistantCountCaption = TextWidget.Cast(root.FindAnyWidget("AssistantCountCaption"));
		m_wStatusText = TextWidget.Cast(root.FindAnyWidget("StatusText"));
		m_wCheckSummary = TextWidget.Cast(root.FindAnyWidget("CheckSummaryText"));
		m_wIssueDetail = TextWidget.Cast(root.FindAnyWidget("IssueDetailText"));
		m_wRegearCheck = root.FindAnyWidget("RegearButtonImage");

		if (!m_FactionList || !m_TargetList || !m_ResultList || !m_SecondaryList || !m_LauncherList || !m_OverrideList || !m_IssueList)
		{
			Print("COA_GearFixPanel: GearFixMenu.layout is missing a list box", LogLevel.WARNING);
			return;
		}

		m_FactionList.m_OnChanged.Insert(OnFactionChanged);
		m_TargetList.m_OnChanged.Insert(OnTargetChanged);
		m_ResultList.m_OnChanged.Insert(OnResultChanged);
		m_IssueList.m_OnChanged.Insert(OnIssueChanged);

		SCR_ButtonTextComponent button;
		button = SCR_ButtonTextComponent.GetButtonText("ModeWeaponButton", root);
		if (button)
			button.m_OnClicked.Insert(OnModeWeapon);

		button = SCR_ButtonTextComponent.GetButtonText("ModeSwapButton", root);
		if (button)
			button.m_OnClicked.Insert(OnModeSwap);

		button = SCR_ButtonTextComponent.GetButtonText("ModeRemoveButton", root);
		if (button)
			button.m_OnClicked.Insert(OnModeRemove);

		button = SCR_ButtonTextComponent.GetButtonText("ModeAddButton", root);
		if (button)
			button.m_OnClicked.Insert(OnModeAdd);

		button = SCR_ButtonTextComponent.GetButtonText("SourceGearscriptButton", root);
		if (button)
			button.m_OnClicked.Insert(OnSourceGearscript);

		button = SCR_ButtonTextComponent.GetButtonText("SourceArsenalButton", root);
		if (button)
			button.m_OnClicked.Insert(OnSourceArsenal);

		button = SCR_ButtonTextComponent.GetButtonText("SearchButton0", root);
		if (button)
			button.m_OnClicked.Insert(RefreshResults);

		button = SCR_ButtonTextComponent.GetButtonText("ApplyButton", root);
		if (button)
			button.m_OnClicked.Insert(OnApply);

		button = SCR_ButtonTextComponent.GetButtonText("RevertButton", root);
		if (button)
			button.m_OnClicked.Insert(OnRevert);

		button = SCR_ButtonTextComponent.GetButtonText("RevertRegearButton", root);
		if (button)
			button.m_OnClicked.Insert(OnRevertRegear);

		button = SCR_ButtonTextComponent.GetButtonText("RunCheckButton", root);
		if (button)
			button.m_OnClicked.Insert(OnRunCheck);

		button = SCR_ButtonTextComponent.GetButtonText("FixIssueButton", root);
		if (button)
			button.m_OnClicked.Insert(OnFixIssue);


		m_RegearButton = SCR_ButtonTextComponent.GetButtonText("RegearButton", root);
		if (m_RegearButton)
			m_RegearButton.m_OnToggled.Insert(OnRegearToggled);

		COA_GearFixClient.s_OnChanged.Insert(RefreshServerData);
		COA_GearFixClient.s_OnResult.Insert(OnResult);

		PopulateFactions();
		UpdateModeLook();
		UpdateSourceLook();
		RefreshServerData();

		// What's already here shows at once; this brings it up to date
		COA_PlayerRplToAuthorityManager authorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (authorityManager)
			authorityManager.RequestGearFixData();

		string faction;
		int role;
		int slot;
		ResourceName resource;
		if (COA_GearFixClient.ConsumePendingOpen(faction, role, slot, resource))
			Prefill(faction, role, slot, resource);
		else if (!m_aFactions.IsEmpty())
			m_FactionList.SetItemSelected(0, true);
	}

	//------------------------------------------------------------------------------------------------
	void Cleanup()
	{
		COA_GearFixClient.s_OnChanged.Remove(RefreshServerData);
		COA_GearFixClient.s_OnResult.Remove(OnResult);
		m_wRoot = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Once a second while the tab is open: the search follows what's typed
	void Update()
	{
		if (!m_wSearchBox)
			return;

		string search = m_wSearchBox.GetText();
		if (search != m_sLastSearch)
			RefreshResults();
	}

	//------------------------------------------------------------------------------------------------
	protected SCR_ListBoxComponent FindList(string name)
	{
		Widget listRoot = m_wRoot.FindAnyWidget(name);
		if (!listRoot)
			return null;

		return SCR_ListBoxComponent.Cast(listRoot.FindHandler(SCR_ListBoxComponent));
	}

	//------------------------------------------------------------------------------------------------
	protected void SetText(TextWidget widget, string text)
	{
		if (widget)
			widget.SetText(text);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetStatus(string text, bool success)
	{
		if (!m_wStatusText)
			return;

		m_wStatusText.SetText(text);
		if (success)
			m_wStatusText.SetColor(Color.FromSRGBA(110, 200, 130, 255));
		else
			m_wStatusText.SetColor(Color.FromSRGBA(232, 96, 96, 255));
	}

	//------------------------------------------------------------------------------------------------
	protected string GetSelectedFaction()
	{
		int index = m_FactionList.GetSelectedItem();
		if (!m_aFactions.IsIndexValid(index))
			return string.Empty;

		return m_aFactions[index];
	}

	//------------------------------------------------------------------------------------------------
	protected COA_GearFixTarget GetSelectedTarget()
	{
		int index = m_TargetList.GetSelectedItem();
		if (!m_aTargets.IsIndexValid(index))
			return null;

		return m_aTargets[index];
	}

	//------------------------------------------------------------------------------------------------
	protected ResourceName GetSelectedResult()
	{
		int index = m_ResultList.GetSelectedItem();
		if (!m_aResults.IsIndexValid(index))
			return ResourceName.Empty;

		return m_aResults[index];
	}

	//------------------------------------------------------------------------------------------------
	protected ResourceName GetSelectedLauncherAmmo()
	{
		int index = m_LauncherList.GetSelectedItem();
		if (!m_aLauncher.IsIndexValid(index))
			return ResourceName.Empty;

		return m_aLauncher[index];
	}

	//------------------------------------------------------------------------------------------------
	protected ResourceName GetSelectedSecondary()
	{
		int index = m_SecondaryList.GetSelectedItem();
		if (!m_aSecondary.IsIndexValid(index))
			return ResourceName.Empty;

		return m_aSecondary[index];
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 TARGET COLUMN
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected void PopulateFactions()
	{
		m_FactionList.Clear();
		COA_GearFixSlotHelper.GetPlayableFactions(m_aFactions);
		foreach (string faction : m_aFactions)
			m_FactionList.AddItem(faction);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFactionChanged()
	{
		PopulateTargets();
		CollectWeaponAttachments();
		OnTargetChanged();
	}

	//------------------------------------------------------------------------------------------------
	//! Faction-wide slots and items first, then what each slotted role can have changed on its own
	protected void PopulateTargets()
	{
		m_TargetList.Clear();
		m_aTargets.Clear();

		string faction = GetSelectedFaction();
		COA_GearScriptConfig gearConfig = COA_GearFixSlotHelper.LoadGearscript(faction);
		COA_RolesConfig rolesConfig = COA_GearscriptManager.GetRolesConfig();
		if (!gearConfig || !rolesConfig)
			return;

		for (int slot = COA_EGearFixSlot.RIFLE; slot <= COA_EGearFixSlot.AA; slot++)
		{
			array<ref COA_GearFixSlotEntry> entries = {};
			COA_GearFixSlotHelper.GetSlotEntries(gearConfig, -1, slot, entries);
			if (!entries.IsEmpty())
				AddTarget(-1, slot, string.Format("All / %1: %2", COA_GearFixText.SlotLabel(slot), COA_GearFixText.Name(entries[0].m_rWeapon)));
		}

		AddTarget(-1, -1, "All / Items");

		array<int> roles = {};
		COA_GearFixSlotHelper.GetSlottedRoles(faction, roles);
		foreach (int role : roles)
		{
			COA_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
			if (!roleConfig)
				continue;

			string roleName = COA_GearFixText.RoleName(role);

			array<int> roleSlots = {};
			COA_GearFixSlotHelper.GetRoleSlots(gearConfig, roleConfig, role, roleSlots);
			foreach (int roleSlot : roleSlots)
			{
				// Specialty weapons are shared with their assistants: those are only under All
				if (!COA_GearFixSlotHelper.IsPoolSlot(roleSlot) && !COA_GearFixSlotHelper.IsCustomSlot(roleSlot))
					continue;

				array<ref COA_GearFixSlotEntry> roleEntries = {};
				COA_GearFixSlotHelper.GetSlotEntries(gearConfig, role, roleSlot, roleEntries);
				if (!roleEntries.IsEmpty())
					AddTarget(role, roleSlot, string.Format("%1 / %2: %3", roleName, COA_GearFixText.SlotLabel(roleSlot), COA_GearFixText.Name(roleEntries[0].m_rWeapon)));
			}

			AddTarget(role, -1, string.Format("%1 / Items", roleName));
		}

		if (!m_aTargets.IsEmpty())
			m_TargetList.SetItemSelected(0, true, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddTarget(int role, int slot, string label)
	{
		m_aTargets.Insert(new COA_GearFixTarget(role, slot));
		m_TargetList.AddItem(label);
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectWeaponAttachments()
	{
		m_mWeaponAttachments.Clear();

		string faction = GetSelectedFaction();
		COA_GearScriptConfig gearConfig = COA_GearFixSlotHelper.LoadGearscript(faction);
		if (!gearConfig)
			return;

		array<ref COA_GearFixSlotEntry> entries = {};
		CollectAllWeaponEntries(gearConfig, faction, entries);
		foreach (COA_GearFixSlotEntry entry : entries)
		{
			if (!m_mWeaponAttachments.Contains(entry.m_rWeapon))
				m_mWeaponAttachments.Set(entry.m_rWeapon, entry.m_aAttachments);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectAllWeaponEntries(COA_GearScriptConfig gearConfig, string faction, notnull array<ref COA_GearFixSlotEntry> entries)
	{
		for (int slot = COA_EGearFixSlot.RIFLE; slot <= COA_EGearFixSlot.AA; slot++)
		{
			array<ref COA_GearFixSlotEntry> slotEntries = {};
			COA_GearFixSlotHelper.GetSlotEntries(gearConfig, -1, slot, slotEntries);
			foreach (COA_GearFixSlotEntry slotEntry : slotEntries)
				entries.Insert(slotEntry);
		}

		if (!gearConfig.m_RolesToSetCustomSettings)
			return;

		foreach (COA_Role_Custom_Gear customGear : gearConfig.m_RolesToSetCustomSettings)
		{
			if (!customGear)
				continue;

			for (int customSlot = COA_EGearFixSlot.CUSTOM_PRIMARY; customSlot <= COA_EGearFixSlot.CUSTOM_PISTOL; customSlot++)
			{
				array<ref COA_GearFixSlotEntry> customEntries = {};
				COA_GearFixSlotHelper.GetSlotEntries(gearConfig, customGear.m_Role, customSlot, customEntries);
				foreach (COA_GearFixSlotEntry customEntry : customEntries)
					entries.Insert(customEntry);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Picking a slot means changing a weapon, picking items means an item change
	protected void OnTargetChanged()
	{
		COA_GearFixTarget target = GetSelectedTarget();
		if (target)
		{
			if (target.m_iSlot >= 0)
				m_iMode = COA_EGearFixAction.WEAPON;
			else if (m_iMode == COA_EGearFixAction.WEAPON)
				m_iMode = COA_EGearFixAction.ITEM_SWAP;
		}

		UpdateModeLook();
		UpdateCurrentText();
		FillDefaultCounts();
		RefreshResults();
		RefreshSecondary();
	}

	//------------------------------------------------------------------------------------------------
	//! "Now: ..." - what the target gives today
	protected void UpdateCurrentText()
	{
		COA_GearFixTarget target = GetSelectedTarget();
		if (!target)
		{
			SetText(m_wCurrentText, "Pick a faction and a slot or role.");
			return;
		}

		if (target.m_iSlot < 0)
		{
			SetText(m_wCurrentText, string.Format("Items of %1. Swap, remove or add below.", COA_GearFixText.RoleName(target.m_iRole)));
			return;
		}

		array<ref COA_GearFixSlotEntry> entries = {};
		COA_GearFixSlotHelper.GetSlotEntries(COA_GearFixSlotHelper.LoadGearscript(GetSelectedFaction()), target.m_iRole, target.m_iSlot, entries);
		if (entries.IsEmpty())
		{
			SetText(m_wCurrentText, "Nothing in the gearscript for this slot.");
			return;
		}

		array<string> names = {};
		foreach (COA_GearFixSlotEntry entry : entries)
			names.Insert(COA_GearFixText.Name(entry.m_rWeapon));

		string text = string.Format("Now: %1 (%2 mags", SCR_StringHelper.Join(" / ", names), entries[0].GetMagazineTotal(false));
		if (COA_GearFixSlotHelper.IsSpecSlot(target.m_iSlot))
			text += string.Format(", assistants %1", entries[0].GetMagazineTotal(true));

		text += ")";

		COA_GearOverride active = FindActiveOverride(target);
		if (active)
			text += string.Format(". Active override: %1", COA_GearFixText.Name(active.m_rNewWeapon));

		SetText(m_wCurrentText, text);
	}

	//------------------------------------------------------------------------------------------------
	protected COA_GearOverride FindActiveOverride(COA_GearFixTarget target)
	{
		string faction = GetSelectedFaction();
		foreach (COA_GearOverride gearOverride : COA_GearFixClient.s_aOverrides)
		{
			if (gearOverride.m_iAction == COA_EGearFixAction.WEAPON && gearOverride.m_sFaction == faction && gearOverride.m_iRole == target.m_iRole && gearOverride.m_iSlot == target.m_iSlot)
				return gearOverride;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Start the counts at what the target has now
	protected void FillDefaultCounts()
	{
		if (!m_wCountInput || !m_wAssistantCountInput)
			return;

		if (m_wLauncherCountInput)
			m_wLauncherCountInput.SetText(DEFAULT_LAUNCHER_ROUNDS.ToString());

		if (m_iMode != COA_EGearFixAction.WEAPON)
		{
			m_wCountInput.SetText("1");
			return;
		}

		COA_GearFixTarget target = GetSelectedTarget();
		if (!target)
			return;

		array<ref COA_GearFixSlotEntry> entries = {};
		COA_GearFixSlotHelper.GetSlotEntries(COA_GearFixSlotHelper.LoadGearscript(GetSelectedFaction()), target.m_iRole, target.m_iSlot, entries);
		if (entries.IsEmpty())
			return;

		m_wCountInput.SetText(entries[0].GetPoolTotal(false, false).ToString());
		m_wAssistantCountInput.SetText(entries[0].GetPoolTotal(true, false).ToString());

		int launcherRounds = entries[0].GetPoolTotal(false, true);
		if (m_wLauncherCountInput && launcherRounds > 0)
			m_wLauncherCountInput.SetText(launcherRounds.ToString());
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 REPLACEMENT COLUMN
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected void OnModeWeapon()
	{
		SetMode(COA_EGearFixAction.WEAPON);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnModeSwap()
	{
		SetMode(COA_EGearFixAction.ITEM_SWAP);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnModeRemove()
	{
		SetMode(COA_EGearFixAction.ITEM_REMOVE);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnModeAdd()
	{
		SetMode(COA_EGearFixAction.ITEM_ADD);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetMode(int mode)
	{
		m_iMode = mode;
		UpdateModeLook();
		FillDefaultCounts();
		RefreshResults();
		RefreshSecondary();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateModeLook()
	{
		SetActiveLook("ModeWeaponButtonBG", m_iMode == COA_EGearFixAction.WEAPON);
		SetActiveLook("ModeSwapButtonBG", m_iMode == COA_EGearFixAction.ITEM_SWAP);
		SetActiveLook("ModeRemoveButtonBG", m_iMode == COA_EGearFixAction.ITEM_REMOVE);
		SetActiveLook("ModeAddButtonBG", m_iMode == COA_EGearFixAction.ITEM_ADD);

		switch (m_iMode)
		{
			case COA_EGearFixAction.WEAPON:
				SetText(m_wResultCaption, "New weapon");
				SetText(m_wSecondaryCaption, "Its magazine");
				SetText(m_wCountCaption, "Mags");
				SetText(m_wAssistantCountCaption, "Assistant mags");
				break;

			case COA_EGearFixAction.ITEM_SWAP:
				SetText(m_wResultCaption, "New item");
				SetText(m_wSecondaryCaption, "Item to replace");
				SetText(m_wCountCaption, "");
				SetText(m_wAssistantCountCaption, "");
				break;

			case COA_EGearFixAction.ITEM_REMOVE:
				SetText(m_wResultCaption, "");
				SetText(m_wSecondaryCaption, "Item to remove");
				SetText(m_wCountCaption, "");
				SetText(m_wAssistantCountCaption, "");
				break;

			case COA_EGearFixAction.ITEM_ADD:
				SetText(m_wResultCaption, "Item to add");
				SetText(m_wSecondaryCaption, "");
				SetText(m_wCountCaption, "How many");
				SetText(m_wAssistantCountCaption, "");
				break;
		}

		bool showCount = m_iMode == COA_EGearFixAction.WEAPON || m_iMode == COA_EGearFixAction.ITEM_ADD;
		SetVisible("CountInput", showCount);
		SetVisible("CountInputBG", showCount);
		SetVisible("AssistantCountInput", m_iMode == COA_EGearFixAction.WEAPON);
		SetVisible("AssistantCountInputBG", m_iMode == COA_EGearFixAction.WEAPON);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetVisible(string name, bool visible)
	{
		Widget widget = m_wRoot.FindAnyWidget(name);
		if (widget)
			widget.SetVisible(visible);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetActiveLook(string backgroundName, bool active)
	{
		Widget background = m_wRoot.FindAnyWidget(backgroundName);
		if (!background)
			return;

		if (active)
			background.SetColor(Color.FromSRGBA(201, 54, 54, 255));
		else
			background.SetColor(Color.FromSRGBA(28, 31, 40, 255));
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSourceGearscript()
	{
		m_bArsenalSource = false;
		UpdateSourceLook();
		RefreshResults();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSourceArsenal()
	{
		m_bArsenalSource = true;
		COA_GearFixCatalog.Build();
		UpdateSourceLook();
		RefreshResults();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateSourceLook()
	{
		SetActiveLook("SourceGearscriptButtonBG", !m_bArsenalSource);
		SetActiveLook("SourceArsenalButtonBG", m_bArsenalSource);
	}

	//------------------------------------------------------------------------------------------------
	//! The list of replacements, by display name but keeping the ResourceName underneath
	protected void RefreshResults()
	{
		string search;
		if (m_wSearchBox)
			search = m_wSearchBox.GetText();

		m_sLastSearch = search;
		string searchLower = search;
		searchLower.ToLower();

		ResourceName previous = GetSelectedResult();

		m_ResultList.Clear();
		m_aResults.Clear();

		if (m_iMode == COA_EGearFixAction.ITEM_REMOVE)
			return;

		array<ResourceName> candidates = {};
		bool modpackWeapons = m_bArsenalSource && m_iMode == COA_EGearFixAction.WEAPON;
		int limit = MAX_RESULTS;
		if (modpackWeapons)
		{
			limit = MAX_MODPACK_WEAPONS;
			CollectModpackWeapons(candidates);
		}
		else if (m_bArsenalSource)
		{
			// Thousands of entries: not listed until there's something to look for
			if (searchLower.IsEmpty())
				return;

			candidates = COA_GearFixCatalog.s_aItems;
		}
		else
		{
			CollectGearscriptCandidates(candidates);
		}

		int matches;
		foreach (ResourceName candidate : candidates)
		{
			if (!modpackWeapons && m_aResults.Count() >= limit)
				break;

			// The file name first, so a big catalogue search doesn't load every prefab for its display name
			string fileName = FilePath.StripPath(candidate);
			fileName.ToLower();
			if (!searchLower.IsEmpty() && !fileName.Contains(searchLower))
			{
				if (!m_bArsenalSource)
				{
					string displayName = COA_GearFixText.Name(candidate);
					displayName.ToLower();
					if (!displayName.Contains(searchLower))
						continue;
				}
				else
				{
					continue;
				}
			}

			matches++;
			if (m_aResults.Count() >= limit)
				continue;

			m_aResults.Insert(candidate);

			// By file name, so listing the modpack doesn't load hundreds of prefabs for their display names
			if (modpackWeapons)
				m_ResultList.AddItem(FilePath.StripExtension(FilePath.StripPath(candidate)));
			else
				m_ResultList.AddItem(COA_GearFixText.Name(candidate));
		}

		if (modpackWeapons && matches > m_aResults.Count())
			SetText(m_wResultCaption, string.Format("New weapon (first %1 of %2, search to narrow)", m_aResults.Count(), matches));
		else if (modpackWeapons)
			SetText(m_wResultCaption, string.Format("New weapon (%1 not in this gearscript)", matches));
		else if (m_iMode == COA_EGearFixAction.WEAPON)
			SetText(m_wResultCaption, "New weapon");

		int previousIndex = m_aResults.Find(previous);
		if (previousIndex >= 0)
			m_ResultList.SetItemSelected(previousIndex, true, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Every weapon loaded in the modpack that this faction's gearscript doesn't already have
	protected void CollectModpackWeapons(notnull array<ResourceName> candidates)
	{
		array<ResourceName> gearscriptWeapons = {};
		CollectGearscriptCandidates(gearscriptWeapons);

		set<string> gearscriptGuids = new set<string>();
		foreach (ResourceName gearscriptWeapon : gearscriptWeapons)
			gearscriptGuids.Insert(SCR_ResourceNameUtils.GetPrefabGUID(gearscriptWeapon));

		foreach (ResourceName weapon : COA_GearFixCatalog.s_aWeapons)
		{
			if (!gearscriptGuids.Contains(SCR_ResourceNameUtils.GetPrefabGUID(weapon)))
				candidates.Insert(weapon);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! This faction's gearscript: all its weapons, or all its items, weapons' magazines and clothing
	protected void CollectGearscriptCandidates(notnull array<ResourceName> candidates)
	{
		string faction = GetSelectedFaction();
		COA_GearScriptConfig gearConfig = COA_GearFixSlotHelper.LoadGearscript(faction);
		if (!gearConfig)
			return;

		if (m_iMode == COA_EGearFixAction.WEAPON)
		{
			array<ref COA_GearFixSlotEntry> entries = {};
			CollectAllWeaponEntries(gearConfig, faction, entries);
			foreach (COA_GearFixSlotEntry entry : entries)
			{
				if (!candidates.Contains(entry.m_rWeapon))
					candidates.Insert(entry.m_rWeapon);
			}

			return;
		}

		COA_GearFixTarget all = new COA_GearFixTarget(-1, -1);
		CollectKit(all, candidates);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnResultChanged()
	{
		if (m_iMode == COA_EGearFixAction.WEAPON)
			RefreshSecondary();
	}

	//------------------------------------------------------------------------------------------------
	//! Weapon mode: magazines fitting the picked weapon. Swap/remove: what the target has to pick from.
	protected void RefreshSecondary()
	{
		ResourceName previous = GetSelectedSecondary();

		m_SecondaryList.Clear();
		m_aSecondary.Clear();

		array<ResourceName> launcherAmmo = {};
		if (m_iMode == COA_EGearFixAction.ITEM_ADD)
		{
			RefreshLauncherAmmo(launcherAmmo);
			return;
		}

		if (m_iMode == COA_EGearFixAction.WEAPON)
		{
			CollectFittingMagazines(GetSelectedResult(), m_aSecondary, launcherAmmo);
		}
		else
		{
			COA_GearFixTarget target = GetSelectedTarget();
			if (target)
				CollectKit(target, m_aSecondary);
		}

		foreach (ResourceName prefab : m_aSecondary)
			m_SecondaryList.AddItem(COA_GearFixText.Name(prefab));

		int previousIndex = m_aSecondary.Find(previous);
		if (previousIndex < 0 && m_iMode == COA_EGearFixAction.WEAPON && !m_aSecondary.IsEmpty())
			previousIndex = 0;

		if (previousIndex >= 0)
			m_SecondaryList.SetItemSelected(previousIndex, true, false);

		RefreshLauncherAmmo(launcherAmmo);
	}

	//------------------------------------------------------------------------------------------------
	//! The new weapon's launcher rounds beside its magazines; only there when it has a launcher
	protected void RefreshLauncherAmmo(notnull array<ResourceName> launcherAmmo)
	{
		ResourceName previous = GetSelectedLauncherAmmo();

		m_LauncherList.Clear();
		m_aLauncher.Clear();
		m_aLauncher.InsertAll(launcherAmmo);

		foreach (ResourceName round : m_aLauncher)
			m_LauncherList.AddItem(COA_GearFixText.Name(round));

		int selected = m_aLauncher.Find(previous);
		if (selected < 0)
			selected = PickDefaultLauncherAmmo();

		if (selected >= 0)
			m_LauncherList.SetItemSelected(selected, true, false);

		bool show = !m_aLauncher.IsEmpty();
		SetVisible("LauncherCaption", show);
		SetVisible("LauncherListBox0BG", show);
		SetVisible("LauncherListBox0", show);
		SetVisible("LauncherCountCaption", show);
		SetVisible("LauncherCountInputBG", show);
		SetVisible("LauncherCountInput", show);

		// The magazine list takes the full width when there's nothing beside it
		float backgroundWidth = 368;
		float listWidth = 360;
		if (show)
		{
			backgroundWidth = 180;
			listWidth = 172;
		}

		Widget background = m_wRoot.FindAnyWidget("SecondaryListBox0BG");
		if (background)
			FrameSlot.SetSize(background, backgroundWidth, 150);

		Widget list = m_wRoot.FindAnyWidget("SecondaryListBox0");
		if (list)
			FrameSlot.SetSize(list, listWidth, 142);
	}

	//------------------------------------------------------------------------------------------------
	//! UGLs don't come with a default round: high explosive if there is one, else the first. The faction's
	//! own rounds are listed first, so its HE wins over the catalogue's.
	protected int PickDefaultLauncherAmmo()
	{
		foreach (int index, ResourceName round : m_aLauncher)
		{
			string fileName = FilePath.StripPath(round);
			fileName.ToLower();
			if (fileName.Contains("smoke") || fileName.Contains("flare") || fileName.Contains("illum") || fileName.Contains("marker"))
				continue;

			if (fileName.Contains("m433") || fileName.Contains("m406") || fileName.Contains("vog") || fileName.Contains("_he"))
				return index;
		}

		if (m_aLauncher.IsEmpty())
			return -1;

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	//! The weapon's own magazine, this faction's magazines that fit, then the arsenals' (cached per prefab)
	protected void CollectFittingMagazines(ResourceName weapon, notnull array<ResourceName> magazines, notnull array<ResourceName> launcherAmmo)
	{
		if (weapon.IsEmpty())
			return;

		array<ResourceName> attachments = m_mWeaponAttachments.Get(weapon);

		ResourceName defaultMagazine = COA_GearPrefabHelper.GetDefaultMagazine(weapon);
		if (!defaultMagazine.IsEmpty())
			magazines.Insert(defaultMagazine);

		string faction = GetSelectedFaction();
		COA_GearScriptConfig gearConfig = COA_GearFixSlotHelper.LoadGearscript(faction);
		if (gearConfig)
		{
			array<ref COA_GearFixSlotEntry> entries = {};
			CollectAllWeaponEntries(gearConfig, faction, entries);
			foreach (COA_GearFixSlotEntry entry : entries)
			{
				foreach (ResourceName magazine : entry.m_aMagazines)
					AddIfFits(weapon, magazine, attachments, magazines, launcherAmmo);
			}
		}

		COA_GearFixCatalog.Build();
		foreach (ResourceName catalogMagazine : COA_GearFixCatalog.s_aMagazines)
			AddIfFits(weapon, catalogMagazine, attachments, magazines, launcherAmmo);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddIfFits(ResourceName weapon, ResourceName magazine, array<ResourceName> attachments, notnull array<ResourceName> magazines, notnull array<ResourceName> launcherAmmo)
	{
		array<ResourceName> list = magazines;
		if (COA_GearPrefabHelper.IsLauncherMagazine(weapon, magazine))
			list = launcherAmmo;

		if (list.Count() >= MAX_MAGAZINES || list.Contains(magazine))
			return;

		if (COA_GearPrefabHelper.MagazineFits(weapon, magazine, attachments) == 1)
			list.Insert(magazine);
	}

	//------------------------------------------------------------------------------------------------
	//! Non-weapon items and clothing a target's role(s) get, plus what active overrides hand out
	protected void CollectKit(COA_GearFixTarget target, notnull array<ResourceName> items)
	{
		string faction = GetSelectedFaction();
		COA_GearScriptConfig gearConfig = COA_GearFixSlotHelper.LoadGearscript(faction);
		COA_RolesConfig rolesConfig = COA_GearscriptManager.GetRolesConfig();
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gearConfig || !rolesConfig || !gamemode)
			return;

		COA_GearScriptContainer gearSettings = gamemode.GetGearScriptSettings(faction);

		array<int> roles = {};
		if (target.m_iRole >= 0)
			roles.Insert(target.m_iRole);
		else
			COA_GearFixSlotHelper.GetSlottedRoles(faction, roles);

		foreach (int role : roles)
		{
			map<ResourceName, int> kit = new map<ResourceName, int>();
			COA_GearFixSlotHelper.GetKitItems(gearConfig, gearSettings, rolesConfig.FindRoleConfig(role), role, kit);
			foreach (ResourceName prefab, int count : kit)
			{
				if (!items.Contains(prefab))
					items.Insert(prefab);
			}

			COA_Role_Custom_Gear customGear = COA_LoadoutPreviewHelper.FindCustomGear(gearConfig, role);
			if (customGear)
				AddClothing(customGear.m_Clothing, items);
		}

		AddClothing(gearConfig.m_DefaultClothing, items);

		foreach (COA_GearOverride gearOverride : COA_GearFixClient.s_aOverrides)
		{
			if (gearOverride.m_sFaction == faction && !gearOverride.m_rNewItem.IsEmpty() && (gearOverride.m_iRole < 0 || gearOverride.m_iRole == target.m_iRole) && !items.Contains(gearOverride.m_rNewItem))
				items.Insert(gearOverride.m_rNewItem);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void AddClothing(array<ref COA_Clothing> clothingList, notnull array<ResourceName> items)
	{
		if (!clothingList)
			return;

		foreach (COA_Clothing clothing : clothingList)
		{
			if (!clothing || !clothing.m_ClothingPrefabs)
				continue;

			foreach (ResourceName prefab : clothing.m_ClothingPrefabs)
			{
				if (!prefab.IsEmpty() && !items.Contains(prefab))
					items.Insert(prefab);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRegearToggled()
	{
		if (m_wRegearCheck != null && m_RegearButton != null)
			m_wRegearCheck.SetVisible(m_RegearButton.IsToggled());
	}

	//------------------------------------------------------------------------------------------------
	protected void OnApply()
	{
		string faction = GetSelectedFaction();
		COA_GearFixTarget target = GetSelectedTarget();
		COA_PlayerRplToAuthorityManager authorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (faction.IsEmpty() || !target || !authorityManager)
		{
			SetStatus("Pick a faction and a slot or role.", false);
			return;
		}

		ResourceName result = GetSelectedResult();
		ResourceName secondary = GetSelectedSecondary();
		array<string> attachments = {};
		int count;
		int assistantCount;
		if (m_wCountInput)
			count = m_wCountInput.GetText().ToInt();

		if (m_wAssistantCountInput)
			assistantCount = m_wAssistantCountInput.GetText().ToInt();

		switch (m_iMode)
		{
			case COA_EGearFixAction.WEAPON:
			{
				if (target.m_iSlot < 0)
				{
					SetStatus("Pick a weapon slot on the left.", false);
					return;
				}

				if (result.IsEmpty())
				{
					SetStatus("Pick the new weapon.", false);
					return;
				}

				array<ResourceName> weaponAttachments = m_mWeaponAttachments.Get(result);
				if (weaponAttachments)
				{
					foreach (ResourceName attachment : weaponAttachments)
						attachments.Insert(attachment);
				}

				break;
			}

			case COA_EGearFixAction.ITEM_SWAP:
				if (result.IsEmpty() || secondary.IsEmpty())
				{
					SetStatus("Pick the item to replace and the new item.", false);
					return;
				}
				break;

			case COA_EGearFixAction.ITEM_REMOVE:
				if (secondary.IsEmpty())
				{
					SetStatus("Pick the item to remove.", false);
					return;
				}
				break;

			case COA_EGearFixAction.ITEM_ADD:
				if (result.IsEmpty() || count <= 0)
				{
					SetStatus("Pick the item and how many to add.", false);
					return;
				}
				break;
		}

		bool regear = true;
		if (m_RegearButton)
			regear = m_RegearButton.IsToggled();

		// Items have no slot; the server only reads what the action uses
		int slot = target.m_iSlot;
		if (m_iMode != COA_EGearFixAction.WEAPON)
			slot = -1;

		ResourceName launcherAmmo;
		int launcherCount;
		if (m_iMode == COA_EGearFixAction.WEAPON)
		{
			launcherAmmo = GetSelectedLauncherAmmo();
			if (m_wLauncherCountInput)
				launcherCount = m_wLauncherCountInput.GetText().ToInt();
		}

		authorityManager.RequestGearOverride(faction, target.m_iRole, slot, m_iMode, result, secondary, count, assistantCount, regear, attachments, launcherAmmo, launcherCount);
		SetStatus("Sent to the server...", true);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnResult(bool success, string message)
	{
		SetStatus(message, success);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 OVERRIDES AND GEAR CHECK COLUMN
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! New data from the server
	protected void RefreshServerData()
	{
		if (!m_wRoot)
			return;

		m_OverrideList.Clear();
		m_aOverrideIds.Clear();
		foreach (COA_GearOverride gearOverride : COA_GearFixClient.s_aOverrides)
		{
			m_aOverrideIds.Insert(gearOverride.m_iId);
			m_OverrideList.AddItem(gearOverride.Describe());
		}

		m_IssueList.Clear();
		m_aListedIssues.Clear();
		foreach (COA_GearCheckIssue issue : COA_GearFixClient.s_aIssues)
		{
			m_aListedIssues.Insert(issue);
			m_IssueList.AddItem(issue.ToListLine());
		}

		SetText(m_wCheckSummary, string.Format("%1 errors, %2 warnings", COA_GearFixClient.s_iErrors, COA_GearFixClient.s_iWarnings));
		SetText(m_wIssueDetail, "");

		UpdateCurrentText();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRevert()
	{
		RevertSelected(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRevertRegear()
	{
		RevertSelected(true);
	}

	//------------------------------------------------------------------------------------------------
	protected void RevertSelected(bool regear)
	{
		int index = m_OverrideList.GetSelectedItem();
		COA_PlayerRplToAuthorityManager authorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (!m_aOverrideIds.IsIndexValid(index) || !authorityManager)
		{
			SetStatus("Pick an active override to revert.", false);
			return;
		}

		authorityManager.RequestGearOverrideRevert(m_aOverrideIds[index], regear);
		SetStatus("Sent to the server...", true);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRunCheck()
	{
		COA_PlayerRplToAuthorityManager authorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (authorityManager)
			authorityManager.RequestGearCheckRun();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnIssueChanged()
	{
		int index = m_IssueList.GetSelectedItem();
		if (!m_aListedIssues.IsIndexValid(index))
			return;

		COA_GearCheckIssue issue = m_aListedIssues[index];
		string detail = issue.ToListLine();
		if (!issue.m_rResource.IsEmpty())
			detail += "\n" + issue.m_rResource;

		SetText(m_wIssueDetail, detail);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFixIssue()
	{
		int index = m_IssueList.GetSelectedItem();
		if (!m_aListedIssues.IsIndexValid(index))
		{
			SetStatus("Pick an issue from the list first.", false);
			return;
		}

		COA_GearCheckIssue issue = m_aListedIssues[index];
		Prefill(issue.m_sFaction, issue.m_iRole, issue.m_iSlot, issue.m_rResource);
	}

	//------------------------------------------------------------------------------------------------
	//! Show a faction + slot/role, as asked for by the warning card or an issue
	protected void Prefill(string faction, int role, int slot, ResourceName resource)
	{
		int factionIndex = m_aFactions.Find(faction);
		if (factionIndex < 0)
			factionIndex = 0;

		if (!m_aFactions.IsIndexValid(factionIndex))
			return;

		m_FactionList.SetItemSelected(factionIndex, true, false);
		PopulateTargets();
		CollectWeaponAttachments();

		int targetIndex = FindTargetIndex(role, slot);
		if (targetIndex >= 0)
			m_TargetList.SetItemSelected(targetIndex, true, false);

		OnTargetChanged();

		// An item problem: have the item ready to swap
		if (slot < 0 && !resource.IsEmpty())
		{
			int secondaryIndex = m_aSecondary.Find(resource);
			if (secondaryIndex >= 0)
				m_SecondaryList.SetItemSelected(secondaryIndex, true, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The exact row, else the same slot for all roles (specialty slots), else the role's items
	protected int FindTargetIndex(int role, int slot)
	{
		int allRolesSlot = -1;
		int roleItems = -1;
		for (int i = 0; i < m_aTargets.Count(); i++)
		{
			COA_GearFixTarget target = m_aTargets[i];
			if (target.m_iRole == role && target.m_iSlot == slot)
				return i;

			if (target.m_iRole < 0 && target.m_iSlot == slot && slot >= 0)
				allRolesSlot = i;

			if (target.m_iRole == role && target.m_iSlot < 0)
				roleItems = i;
		}

		if (allRolesSlot >= 0)
			return allRolesSlot;

		return roleItems;
	}
}
