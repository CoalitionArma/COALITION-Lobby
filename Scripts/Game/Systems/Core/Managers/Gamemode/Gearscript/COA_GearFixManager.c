//------------------------------------------------------------------------------------------------
// Server side of the admin "Gear Fix" tool and the automatic gearscript check. Owned by
// COA_GearscriptManager, so it lives exactly as long as the mission's world.
//
// Overrides: a session registry of changes applied on top of the gearscript. The gearscript config
// resources are never modified - after SetEntityGear finishes, OnGearApplied runs the same in-place
// swap on the fresh loadout that a live re-gear runs on a player mid-mission, so there is one code
// path for both. At spawn the character has its full gearscript ammo, so "scale what they still
// have" gives the override's full counts.
//
// Gear check: COA_GearCheckValidator runs once shortly after the mission loads and on demand; items
// that really failed to fit at spawn are added as they happen (RecordSpawnFailure). Staff get the
// list once - when the check finishes, when they join or log in, or when something new turns up.
//------------------------------------------------------------------------------------------------
class COA_GearFixSwapJob
{
	int m_iJobId;
	int m_iOverrideId;
	int m_iSlot;
	bool m_bSpawnPass;
	bool m_bNotify;
	COA_GearscriptCharacter m_Character;

	bool m_bWasCurrent;				//!< the old weapon was in the character's hands
	bool m_bHadWeapon;				//!< carried the old weapon, so gets the new one
	bool m_bAssistant;
	float m_fLoadedFraction;		//!< how full the old weapon's loaded magazine was
	int m_iSlotId = -1;
	BaseInventoryStorageComponent m_Storage;

	ref array<IEntity> m_aMagazinesToDelete = {};
	ref array<ResourceName> m_aGiveMagazines = {};
	ref array<int> m_aGiveCounts = {};
	int m_iGiveTotal;
	int m_iGiveLauncherTotal;
}

//------------------------------------------------------------------------------------------------
class COA_GearFixPending
{
	IEntity m_Entity;
	int m_iOverrideId;
	bool m_bNotify;
	float m_fExpiresAt;
}

//------------------------------------------------------------------------------------------------
class COA_GearFixManager
{
	protected static const int VALIDATION_START_DELAY_MS = 5000;
	protected static const int VALIDATION_FACTION_GAP_MS = 200;
	protected static const int TICK_MS = 3000;
	protected static const int STAFF_CHECK_TICKS = 5;			// every 15 s
	protected static const int SPAWN_PASS_DELAY_MS = 300;		// after SetEntityGear's next-frame attachments and item cleanup
	protected static const int HOLSTER_DELAY_MS = 1500;			// lets a weapon in hands be put away before it is deleted
	protected static const int REGEAR_STAGGER_MS = 80;
	protected static const int RUNTIME_PUSH_DELAY_MS = 5000;
	protected static const float PENDING_TIMEOUT_MS = 600000;	// a player unconscious or in a vehicle for 10 minutes keeps their gear
	protected static const int MAX_ISSUES_SENT = 60;
	protected static const int ISSUES_PER_RPC = 15;
	protected static const int MAX_COUNT = 30;

	protected ref array<ref COA_GearOverride> m_aOverrides = {};
	protected ref array<ref COA_GearOverride> m_aTransient = {};	// reverts and follow-up swaps, negative ids
	protected int m_iNextId = 1;
	protected int m_iNextTransientId = -1;

	protected ref map<int, ref COA_GearFixSwapJob> m_mJobs = new map<int, ref COA_GearFixSwapJob>();
	protected int m_iNextJobId = 1;
	protected ref array<ref COA_GearFixPending> m_aPending = {};

	protected ref array<ref COA_GearCheckIssue> m_aStaticIssues = {};
	protected ref array<ref COA_GearCheckIssue> m_aRuntimeIssues = {};
	protected ref array<string> m_aValidationQueue = {};
	protected bool m_bValidationRunning;
	protected bool m_bValidated;
	protected bool m_bRuntimePushQueued;
	protected bool m_bSuppressCard;		// a re-check an admin started from the Gear Fix tab: no warning card
	protected ref array<int> m_aStaffSent = {};
	protected int m_iTicks;

	//------------------------------------------------------------------------------------------------
	static COA_GearFixManager GetInstance()
	{
		COA_GearscriptManager gearscriptManager = COA_GearscriptManager.GetInstance();
		if (!gearscriptManager)
			return null;

		return gearscriptManager.GetGearFixManager();
	}

	//------------------------------------------------------------------------------------------------
	void Start()
	{
		GetGame().GetCallqueue().CallLater(DelayedStart, VALIDATION_START_DELAY_MS, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void DelayedStart()
	{
		if (!Replication.IsServer())
			return;

		RunValidation();
		GetGame().GetCallqueue().CallLater(ServerTick, TICK_MS, true);
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_GearFixManager()
	{
		if (!GetGame() || !GetGame().GetCallqueue())
			return;

		ScriptCallQueue queue = GetGame().GetCallqueue();
		queue.Remove(DelayedStart);
		queue.Remove(ServerTick);
		queue.Remove(ValidateNextFaction);
		queue.Remove(PushRuntimeIssues);
		queue.Remove(ApplySpawnOverrides);
		queue.Remove(RegearQueued);
		queue.Remove(WeaponSwap_Delete);
		queue.Remove(WeaponSwap_DeleteMagazines);
		queue.Remove(WeaponSwap_Spawn);
		queue.Remove(WeaponSwap_Finish);
		queue.Remove(GiveItems);
	}

	//------------------------------------------------------------------------------------------------
	protected void ServerTick()
	{
		ProcessPending();

		m_iTicks++;
		if (m_iTicks % STAFF_CHECK_TICKS == 0)
			SendToNewStaff();
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 GEAR CHECK
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Validate every playable faction, one per step so a big mission doesn't hitch for all of them at once
	void RunValidation()
	{
		if (m_bValidationRunning)
			return;

		m_bValidationRunning = true;
		m_aStaticIssues.Clear();
		COA_GearFixSlotHelper.GetPlayableFactions(m_aValidationQueue);
		GetGame().GetCallqueue().CallLater(ValidateNextFaction, VALIDATION_FACTION_GAP_MS, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void ValidateNextFaction()
	{
		if (!m_aValidationQueue.IsEmpty())
		{
			string faction = m_aValidationQueue[0];
			m_aValidationQueue.RemoveOrdered(0);
			COA_GearCheckValidator.ValidateFaction(faction, m_aStaticIssues);
		}

		if (!m_aValidationQueue.IsEmpty())
		{
			GetGame().GetCallqueue().CallLater(ValidateNextFaction, VALIDATION_FACTION_GAP_MS, false);
			return;
		}

		m_bValidationRunning = false;
		m_bValidated = true;
		PrintIssues();

		m_aStaffSent.Clear();
		SendToNewStaff();
		m_bSuppressCard = false;
	}

	//------------------------------------------------------------------------------------------------
	protected void PrintIssues()
	{
		array<ref COA_GearCheckIssue> issues = {};
		CollectIssues(issues);

		PrintFormat("[COA_GearCheck] Gearscript check finished: %1 error(s), %2 warning(s), %3 note(s)",
			COA_GearCheckValidator.CountSeverity(issues, COA_EGearCheckSeverity.ERROR),
			COA_GearCheckValidator.CountSeverity(issues, COA_EGearCheckSeverity.WARNING),
			COA_GearCheckValidator.CountSeverity(issues, COA_EGearCheckSeverity.INFO));

		foreach (COA_GearCheckIssue issue : issues)
		{
			if (issue.m_iSeverity == COA_EGearCheckSeverity.INFO)
				Print(issue.ToLogLine());
			else
				Print(issue.ToLogLine(), LogLevel.WARNING);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Static findings and what actually went wrong at spawn, errors first
	void CollectIssues(notnull array<ref COA_GearCheckIssue> issues)
	{
		issues.Clear();
		foreach (COA_GearCheckIssue staticIssue : m_aStaticIssues)
			issues.Insert(staticIssue);

		foreach (COA_GearCheckIssue runtimeIssue : m_aRuntimeIssues)
			issues.Insert(runtimeIssue);

		COA_GearCheckValidator.SortBySeverity(issues);
	}

	//------------------------------------------------------------------------------------------------
	//! COA_LoggingHelper: something could not be inserted while a character was being equipped. This
	//! is the real "did it fit" answer that the validator can only estimate.
	//! \param[in] entity The character, or a weapon it carries (attachments)
	static void RecordSpawnFailure(IEntity entity, ResourceName item, string itemType)
	{
		COA_GearFixManager manager = GetInstance();
		if (manager)
			manager.AddRuntimeIssue(entity, COA_EGearCheckSeverity.ERROR, string.Format("%1 could not be added at spawn (no space or invalid)", itemType), item);
	}

	//------------------------------------------------------------------------------------------------
	//! COA_LoggingHelper: a character came out over the weight limit
	static void RecordOverweight(IEntity entity, int kg)
	{
		COA_GearFixManager manager = GetInstance();
		if (manager)
			manager.AddRuntimeIssue(entity, COA_EGearCheckSeverity.WARNING, string.Format("Spawned at %1 kg, over the 60 kg limit", kg), ResourceName.Empty);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddRuntimeIssue(IEntity entity, int severity, string message, ResourceName resource)
	{
		if (!Replication.IsServer())
			return;

		// Attachment failures report the weapon: walk up to whoever is holding it
		COA_GearscriptCharacter character;
		IEntity current = entity;
		while (current && !character)
		{
			character = COA_GearscriptCharacter.Cast(current);
			current = current.GetParent();
		}

		if (!character)
			return;

		COA_GearCheckIssue issue = new COA_GearCheckIssue(severity, COA_EntityHelper.DetermineFactionKey(character), character.GetGearRole(), -1, message, resource);
		string key = issue.GetKey();
		foreach (COA_GearCheckIssue existing : m_aRuntimeIssues)
		{
			if (existing.GetKey() == key)
			{
				existing.m_iOccurrences++;
				return;
			}
		}

		m_aRuntimeIssues.Insert(issue);
		Print(issue.ToLogLine(), LogLevel.WARNING);

		if (m_bRuntimePushQueued)
			return;

		m_bRuntimePushQueued = true;
		GetGame().GetCallqueue().CallLater(PushRuntimeIssues, RUNTIME_PUSH_DELAY_MS, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void PushRuntimeIssues()
	{
		m_bRuntimePushQueued = false;
		m_aStaffSent.Clear();
		SendToNewStaff();
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsStaff(int playerId)
	{
		if (SCR_Global.IsAdmin(playerId))
			return true;

		COA_PermissionManager permissionManager = COA_PermissionManager.GetInstance();
		return permissionManager != null && permissionManager.IsModerator(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Briefing, slotting and safe start
	protected static bool IsPreGame()
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode)
			return false;

		if (gamemode.m_GamemodeState == COA_EGamemodeState.BRIEFING || gamemode.m_GamemodeState == COA_EGamemodeState.SLOTTING)
			return true;

		if (gamemode.m_GamemodeState != COA_EGamemodeState.GAME)
			return false;

		COA_SafestartManager safestartManager = COA_SafestartManager.GetInstance();
		return safestartManager != null && safestartManager.GetSafestartStatus();
	}

	//------------------------------------------------------------------------------------------------
	//! Sends everything to staff who haven't had it yet. Run periodically instead of hooking joins so
	//! it also catches a player who logs in as admin mid-session; players who left are forgotten so
	//! they get it again when they return.
	protected void SendToNewStaff()
	{
		if (!m_bValidated)
			return;

		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);

		for (int i = m_aStaffSent.Count() - 1; i >= 0; i--)
		{
			if (!players.Contains(m_aStaffSent[i]))
				m_aStaffSent.Remove(i);
		}

		foreach (int playerId : players)
		{
			if (m_aStaffSent.Contains(playerId) || !IsStaff(playerId))
				continue;

			m_aStaffSent.Insert(playerId);
			SendOverridesTo(playerId);
			SendIssuesTo(playerId, IsPreGame() && !m_bSuppressCard);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SendIssuesTo(int playerId, bool showCard)
	{
		COA_PlayerRplToOwnerManager ownerManager = COA_PlayerRplToOwnerManager.GetForPlayer(playerId);
		if (!ownerManager)
			return;

		array<ref COA_GearCheckIssue> issues = {};
		CollectIssues(issues);

		int errors = COA_GearCheckValidator.CountSeverity(issues, COA_EGearCheckSeverity.ERROR);
		int warnings = COA_GearCheckValidator.CountSeverity(issues, COA_EGearCheckSeverity.WARNING);
		if (errors + warnings == 0)
			showCard = false;

		int toSend = Math.Min(issues.Count(), MAX_ISSUES_SENT);
		int index = 0;
		bool first = true;
		while (first || index < toSend)
		{
			array<string> batch = {};
			while (index < toSend && batch.Count() < ISSUES_PER_RPC)
			{
				batch.Insert(issues[index].Pack());
				index++;
			}

			ownerManager.ReceiveGearCheckIssues(batch, first, index >= toSend, errors, warnings, showCard);
			first = false;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SendOverridesTo(int playerId)
	{
		COA_PlayerRplToOwnerManager ownerManager = COA_PlayerRplToOwnerManager.GetForPlayer(playerId);
		if (!ownerManager)
			return;

		array<string> packed = {};
		foreach (COA_GearOverride gearOverride : m_aOverrides)
			packed.Insert(gearOverride.Pack());

		ownerManager.ReceiveGearOverrides(packed);
	}

	//------------------------------------------------------------------------------------------------
	protected void SendOverridesToStaff()
	{
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int playerId : players)
		{
			if (IsStaff(playerId))
				SendOverridesTo(playerId);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SendResult(int playerId, bool success, string message)
	{
		COA_PlayerRplToOwnerManager ownerManager = COA_PlayerRplToOwnerManager.GetForPlayer(playerId);
		if (ownerManager)
			ownerManager.ReceiveGearFixResult(success, message);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 REQUESTS (staff-checked by COA_PlayerRplToAuthorityManager)
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! The Gear Fix tab was opened
	void HandleDataRequest(int playerId)
	{
		SendOverridesTo(playerId);
		SendIssuesTo(playerId, false);
	}

	//------------------------------------------------------------------------------------------------
	void HandleRunCheck(int playerId)
	{
		// What really failed at spawn stays listed: a static re-check can't tell whether it's fixed
		m_bSuppressCard = true;
		RunValidation();
		SendResult(playerId, true, "Gear check started.");
	}

	//------------------------------------------------------------------------------------------------
	//! \param[in] primary WEAPON: the new weapon. ITEM_SWAP/ITEM_ADD: the new item
	//! \param[in] secondary WEAPON: the new magazine. ITEM_SWAP/ITEM_REMOVE: the old item
	//! \param[in] count WEAPON: magazines for the weapon's holder. ITEM_ADD: how many
	//! \param[in] assistantCount WEAPON: magazines assistants of a specialty slot carry
	void HandleApply(int adminId, string faction, int role, int slot, int action, ResourceName primary, ResourceName secondary, int count, int assistantCount, bool regear, array<string> attachments, ResourceName launcherAmmo, int launcherCount)
	{
		COA_GearScriptConfig gearConfig = COA_GearFixSlotHelper.LoadGearscript(faction);
		if (!gearConfig)
		{
			SendResult(adminId, false, string.Format("%1 has no gearscript.", faction));
			return;
		}

		COA_GearOverride gearOverride = new COA_GearOverride();
		gearOverride.m_sFaction = faction;
		gearOverride.m_iRole = role;
		gearOverride.m_iAction = action;
		gearOverride.m_sAdminName = GetGame().GetPlayerManager().GetPlayerName(adminId);

		string error;
		switch (action)
		{
			case COA_EGearFixAction.WEAPON:
				error = BuildWeaponOverride(gearOverride, gearConfig, slot, primary, secondary, count, assistantCount, attachments, launcherAmmo, launcherCount);
				break;

			case COA_EGearFixAction.ITEM_SWAP:
				if (secondary.IsEmpty() || !COA_GearPrefabHelper.IsValidPrefab(primary))
					error = "Pick both the item to replace and a valid new item.";
				gearOverride.m_rOldItem = secondary;
				gearOverride.m_rNewItem = primary;
				break;

			case COA_EGearFixAction.ITEM_REMOVE:
				if (secondary.IsEmpty())
					error = "Pick the item to remove.";
				gearOverride.m_rOldItem = secondary;
				break;

			case COA_EGearFixAction.ITEM_ADD:
				if (!COA_GearPrefabHelper.IsValidPrefab(primary) || count <= 0)
					error = "Pick a valid item and a count of at least 1.";
				gearOverride.m_rNewItem = primary;
				gearOverride.m_iItemCount = Math.Min(count, MAX_COUNT);
				break;

			default:
				error = "Unknown action.";
		}

		if (!error.IsEmpty())
		{
			SendResult(adminId, false, error);
			return;
		}

		// A newer override for the same target replaces the older one; what the older one handed out
		// also counts as "the old gear" so current holders get swapped again
		array<ref COA_GearOverride> followUps = {};
		for (int i = m_aOverrides.Count() - 1; i >= 0; i--)
		{
			COA_GearOverride existing = m_aOverrides[i];
			if (!gearOverride.HasSameTarget(existing))
				continue;

			if (action == COA_EGearFixAction.WEAPON)
				InheritOldGear(gearOverride, existing);
			else if (action != COA_EGearFixAction.ITEM_ADD && existing.m_iAction == COA_EGearFixAction.ITEM_SWAP)
				followUps.Insert(BuildItemFollowUp(gearOverride, existing));

			m_aOverrides.Remove(i);
		}

		// A role-scoped weapon sits on top of a faction-wide override of the same slot
		if (action == COA_EGearFixAction.WEAPON && role >= 0)
		{
			COA_GearOverride factionWide = FindWeaponOverride(faction, -1, gearOverride.m_iSlot);
			if (factionWide)
				InheritOldGear(gearOverride, factionWide);
		}

		gearOverride.m_iId = m_iNextId;
		m_iNextId++;
		m_aOverrides.Insert(gearOverride);

		int regeared;
		if (regear)
		{
			regeared = Regear(gearOverride, true);
			foreach (COA_GearOverride followUp : followUps)
				Regear(followUp, true);
		}

		LogOverride(gearOverride, gearConfig);
		LogAdminAction(string.Format("%1 Set A Gear Override: %2 (%3 Characters Re-Geared)", gearOverride.m_sAdminName, gearOverride.Describe(), regeared));

		SendOverridesToStaff();
		SendResult(adminId, true, string.Format("Override set: %1. Re-gearing %2 character(s).", gearOverride.Describe(), regeared));
	}

	//------------------------------------------------------------------------------------------------
	//! \return error message, empty if valid
	protected string BuildWeaponOverride(COA_GearOverride gearOverride, COA_GearScriptConfig gearConfig, int slot, ResourceName weapon, ResourceName magazine, int count, int assistantCount, array<string> attachments, ResourceName launcherAmmo, int launcherCount)
	{
		if (slot < COA_EGearFixSlot.RIFLE || slot > COA_EGearFixSlot.CUSTOM_PISTOL)
			return "Pick a weapon slot.";

		gearOverride.m_iSlot = slot;

		if (COA_GearFixSlotHelper.IsCustomSlot(slot) && gearOverride.m_iRole < 0)
			return "A custom weapon slot belongs to one role.";

		if (gearOverride.m_iRole >= 0 && !COA_GearFixSlotHelper.IsCustomSlot(slot) && !COA_GearFixSlotHelper.IsPoolSlot(slot))
			return "Specialty weapons and their ammo are shared by a slot's assistants, so they can only be changed for all roles.";

		if (!COA_GearPrefabHelper.IsValidPrefab(weapon) || !COA_GearPrefabHelper.IsWeapon(weapon))
			return "Pick a valid weapon.";

		gearOverride.m_rNewWeapon = weapon;

		if (attachments)
		{
			foreach (string attachment : attachments)
			{
				if (COA_GearPrefabHelper.IsValidPrefab(attachment))
					gearOverride.m_aNewAttachments.Insert(attachment);
			}
		}

		// An UGL's rounds, which differ by launcher (VOGs for a GP-25, M433s for an M203)
		if (!launcherAmmo.IsEmpty() && launcherCount > 0)
		{
			if (!COA_GearPrefabHelper.IsValidPrefab(launcherAmmo) || !COA_GearPrefabHelper.IsLauncherMagazine(weapon, launcherAmmo))
				return "The launcher ammo prefab is not valid.";

			if (COA_GearPrefabHelper.MagazineFits(weapon, launcherAmmo, gearOverride.m_aNewAttachments) == 0)
				return string.Format("%1 does not fit %2.", COA_GearFixText.Name(launcherAmmo), COA_GearFixText.Name(weapon));

			gearOverride.m_aNewMagazines.Insert(launcherAmmo);
			gearOverride.m_aNewCounts.Insert(Math.ClampInt(launcherCount, 0, MAX_COUNT));
			gearOverride.m_aNewAssistantCounts.Insert(0);
		}

		if (magazine.IsEmpty())
			return string.Empty; // disposable launchers have nothing to reload with

		if (!COA_GearPrefabHelper.IsValidPrefab(magazine))
			return "The magazine prefab is not valid.";

		if (COA_GearPrefabHelper.MagazineFits(weapon, magazine, gearOverride.m_aNewAttachments) == 0)
			return string.Format("%1 does not fit %2.", COA_GearFixText.Name(magazine), COA_GearFixText.Name(weapon));

		gearOverride.m_aNewMagazines.Insert(magazine);
		gearOverride.m_aNewCounts.Insert(Math.ClampInt(count, 0, MAX_COUNT));
		gearOverride.m_aNewAssistantCounts.Insert(Math.ClampInt(assistantCount, 0, MAX_COUNT));
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected void InheritOldGear(notnull COA_GearOverride gearOverride, notnull COA_GearOverride previous)
	{
		if (!previous.m_rNewWeapon.IsEmpty())
			gearOverride.m_aExtraOldWeapons.Insert(previous.m_rNewWeapon);

		gearOverride.m_aExtraOldWeapons.InsertAll(previous.m_aExtraOldWeapons);
		gearOverride.m_aExtraOldMagazines.InsertAll(previous.m_aNewMagazines);
		gearOverride.m_aExtraOldMagazines.InsertAll(previous.m_aExtraOldMagazines);
		gearOverride.m_iExtraOldCount = previous.GetPoolCount(false, false);
		gearOverride.m_iExtraOldAssistantCount = previous.GetPoolCount(true, false);
		gearOverride.m_iExtraOldLauncherCount = previous.GetPoolCount(false, true);
		gearOverride.m_iExtraOldLauncherAssistantCount = previous.GetPoolCount(true, true);
	}

	//------------------------------------------------------------------------------------------------
	//! A -> B replaced by A -> C: whoever already holds B has to end up with C as well
	protected COA_GearOverride BuildItemFollowUp(notnull COA_GearOverride gearOverride, notnull COA_GearOverride previous)
	{
		COA_GearOverride followUp = new COA_GearOverride();
		followUp.m_iId = m_iNextTransientId;
		m_iNextTransientId--;
		followUp.m_sFaction = gearOverride.m_sFaction;
		followUp.m_iRole = gearOverride.m_iRole;
		followUp.m_iAction = gearOverride.m_iAction;
		followUp.m_rOldItem = previous.m_rNewItem;
		followUp.m_rNewItem = gearOverride.m_rNewItem;
		followUp.m_sAdminName = gearOverride.m_sAdminName;
		m_aTransient.Insert(followUp);
		return followUp;
	}

	//------------------------------------------------------------------------------------------------
	//! Future spawns go back to the gearscript; optionally current holders too
	void HandleRevert(int adminId, int overrideId, bool regear)
	{
		COA_GearOverride gearOverride = FindOverride(overrideId);
		if (!gearOverride || overrideId <= 0)
		{
			SendResult(adminId, false, "That override no longer exists.");
			return;
		}

		string adminName = GetGame().GetPlayerManager().GetPlayerName(adminId);
		string description = gearOverride.Describe();

		COA_GearOverride inverse;
		if (regear)
			inverse = BuildInverse(gearOverride);

		m_aOverrides.RemoveItem(gearOverride);

		int regeared;
		if (inverse)
			regeared = Regear(inverse, true);

		PrintFormat("[COA_GearFix] Override #%1 reverted by %2: %3", overrideId, adminName, description);
		LogAdminAction(string.Format("%1 Reverted A Gear Override: %2 (%3 Characters Re-Geared)", adminName, description, regeared));

		SendOverridesToStaff();
		SendResult(adminId, true, string.Format("Reverted: %1. Re-gearing %2 character(s).", description, regeared));
	}

	//------------------------------------------------------------------------------------------------
	//! The override that undoes another for current holders, kept only long enough to run
	protected COA_GearOverride BuildInverse(notnull COA_GearOverride gearOverride)
	{
		COA_GearOverride inverse = new COA_GearOverride();
		inverse.m_iId = m_iNextTransientId;
		m_iNextTransientId--;
		inverse.m_sFaction = gearOverride.m_sFaction;
		inverse.m_iRole = gearOverride.m_iRole;
		inverse.m_iSlot = gearOverride.m_iSlot;
		inverse.m_sAdminName = gearOverride.m_sAdminName;
		inverse.m_bRevertToGearscript = true;

		switch (gearOverride.m_iAction)
		{
			case COA_EGearFixAction.WEAPON:
			{
				inverse.m_iAction = COA_EGearFixAction.WEAPON;
				inverse.m_bExplicitOld = true;
				inverse.m_aExtraOldWeapons.Insert(gearOverride.m_rNewWeapon);
				inverse.m_aExtraOldMagazines.InsertAll(gearOverride.m_aNewMagazines);
				inverse.m_iExtraOldCount = gearOverride.GetPoolCount(false, false);
				inverse.m_iExtraOldAssistantCount = gearOverride.GetPoolCount(true, false);
				inverse.m_iExtraOldLauncherCount = gearOverride.GetPoolCount(false, true);
				inverse.m_iExtraOldLauncherAssistantCount = gearOverride.GetPoolCount(true, true);

				// Back to the faction-wide override this one sat on top of, else to the gearscript
				COA_GearOverride factionWide;
				if (gearOverride.m_iRole >= 0)
					factionWide = FindWeaponOverride(gearOverride.m_sFaction, -1, gearOverride.m_iSlot);

				if (factionWide)
				{
					inverse.m_rNewWeapon = factionWide.m_rNewWeapon;
					inverse.m_aNewAttachments.InsertAll(factionWide.m_aNewAttachments);
					inverse.m_aNewMagazines.InsertAll(factionWide.m_aNewMagazines);
					inverse.m_aNewCounts.InsertAll(factionWide.m_aNewCounts);
					inverse.m_aNewAssistantCounts.InsertAll(factionWide.m_aNewAssistantCounts);
					break;
				}

				array<ref COA_GearFixSlotEntry> entries = {};
				COA_GearFixSlotHelper.GetSlotEntries(COA_GearFixSlotHelper.LoadGearscript(gearOverride.m_sFaction), gearOverride.m_iRole, gearOverride.m_iSlot, entries);
				if (entries.IsEmpty())
					return null;

				inverse.m_rNewWeapon = entries[0].m_rWeapon;
				inverse.m_aNewAttachments.InsertAll(entries[0].m_aAttachments);
				inverse.m_aNewMagazines.InsertAll(entries[0].m_aMagazines);
				inverse.m_aNewCounts.InsertAll(entries[0].m_aCounts);
				inverse.m_aNewAssistantCounts.InsertAll(entries[0].m_aAssistantCounts);
				break;
			}

			case COA_EGearFixAction.ITEM_SWAP:
				inverse.m_iAction = COA_EGearFixAction.ITEM_SWAP;
				inverse.m_rOldItem = gearOverride.m_rNewItem;
				inverse.m_rNewItem = gearOverride.m_rOldItem;
				break;

			case COA_EGearFixAction.ITEM_REMOVE:
				// Hand back what the role's gearscript gives (count <= 0: per role)
				inverse.m_iAction = COA_EGearFixAction.ITEM_ADD;
				inverse.m_rNewItem = gearOverride.m_rOldItem;
				inverse.m_iItemCount = 0;
				break;

			case COA_EGearFixAction.ITEM_ADD:
				inverse.m_iAction = COA_EGearFixAction.ITEM_REMOVE;
				inverse.m_rOldItem = gearOverride.m_rNewItem;
				inverse.m_iItemCount = gearOverride.m_iItemCount;
				break;
		}

		m_aTransient.Insert(inverse);
		return inverse;
	}

	//------------------------------------------------------------------------------------------------
	protected COA_GearOverride FindOverride(int id)
	{
		foreach (COA_GearOverride gearOverride : m_aOverrides)
		{
			if (gearOverride.m_iId == id)
				return gearOverride;
		}

		foreach (COA_GearOverride transientOverride : m_aTransient)
		{
			if (transientOverride.m_iId == id)
				return transientOverride;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected COA_GearOverride FindWeaponOverride(string faction, int role, int slot)
	{
		foreach (COA_GearOverride gearOverride : m_aOverrides)
		{
			if (gearOverride.m_iAction == COA_EGearFixAction.WEAPON && gearOverride.m_sFaction == faction && gearOverride.m_iRole == role && gearOverride.m_iSlot == slot)
				return gearOverride;
		}

		return null;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 LOGGING
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected void LogAdminAction(string message)
	{
		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.LogAdminAction(message, -1, false, COA_EAdminLogLevel.High);
	}

	//------------------------------------------------------------------------------------------------
	//! What a mission maker has to change in the gearscript to make this override permanent
	protected void LogOverride(COA_GearOverride gearOverride, COA_GearScriptConfig gearConfig)
	{
		string resource = COA_GearFixSlotHelper.GetGearscriptResource(gearOverride.m_sFaction);
		PrintFormat("[COA_GearFix] Override #%1 by %2: %3", gearOverride.m_iId, gearOverride.m_sAdminName, gearOverride.Describe());
		PrintFormat("[COA_GearFix]   Gearscript: %1", resource);

		string roleName = "(all roles)";
		if (gearOverride.m_iRole >= 0)
			roleName = SCR_Enum.GetEnumName(COA_EGearRole, gearOverride.m_iRole);

		if (gearOverride.m_iAction != COA_EGearFixAction.WEAPON)
		{
			string listName = "m_DefaultInventoryItems / m_InfantryMedicalItems / m_MedicMedicalItems";
			if (gearOverride.m_iRole >= 0)
				listName = string.Format("m_RolesToSetCustomSettings [%1] m_AdditionalInventoryItems (or the faction list that gives it)", roleName);

			PrintFormat("[COA_GearFix]   Items, %1: %2 -> %3 x%4 in %5", roleName, gearOverride.m_rOldItem, gearOverride.m_rNewItem, gearOverride.m_iItemCount, listName);
			return;
		}

		string field = COA_GearFixText.SlotFieldName(gearOverride.m_iSlot);
		if (COA_GearFixSlotHelper.IsCustomSlot(gearOverride.m_iSlot))
			field = string.Format("m_RolesToSetCustomSettings [%1] %2", roleName, field);
		else if (gearOverride.m_iRole >= 0)
			field = string.Format("%1 for %2 only: needs a m_RolesToSetCustomSettings entry, which replaces ALL of that role's default weapons", field, roleName);

		PrintFormat("[COA_GearFix]   Field: %1", field);

		array<ref COA_GearFixSlotEntry> entries = {};
		COA_GearFixSlotHelper.GetSlotEntries(gearConfig, gearOverride.m_iRole, gearOverride.m_iSlot, entries);
		foreach (COA_GearFixSlotEntry entry : entries)
		{
			PrintFormat("[COA_GearFix]   Old: m_Weapon \"%1\"", entry.m_rWeapon);
			for (int i = 0; i < entry.m_aMagazines.Count(); i++)
				PrintFormat("[COA_GearFix]   Old:   m_Magazine \"%1\" m_MagazineCount %2 m_AssistantMagazineCount %3", entry.m_aMagazines[i], entry.m_aCounts[i], entry.m_aAssistantCounts[i]);
		}

		PrintFormat("[COA_GearFix]   New: m_Weapon \"%1\"", gearOverride.m_rNewWeapon);
		for (int m = 0; m < gearOverride.m_aNewMagazines.Count(); m++)
			PrintFormat("[COA_GearFix]   New:   m_Magazine \"%1\" m_MagazineCount %2 m_AssistantMagazineCount %3", gearOverride.m_aNewMagazines[m], gearOverride.m_aNewCounts[m], gearOverride.m_aNewAssistantCounts[m]);

		foreach (ResourceName attachment : gearOverride.m_aNewAttachments)
			PrintFormat("[COA_GearFix]   New:   m_Attachments \"%1\"", attachment);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 APPLYING OVERRIDES
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! COA_GearscriptManager.SetEntityGear finished a loadout
	void OnGearApplied(IEntity entity, FactionKey faction, int role)
	{
		foreach (COA_GearOverride gearOverride : m_aOverrides)
		{
			if (gearOverride.m_sFaction == faction && (gearOverride.m_iRole < 0 || gearOverride.m_iRole == role))
			{
				GetGame().GetCallqueue().CallLater(ApplySpawnOverrides, SPAWN_PASS_DELAY_MS, false, entity);
				return;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplySpawnOverrides(IEntity entity)
	{
		COA_GearscriptCharacter character = COA_GearscriptCharacter.Cast(entity);
		if (!character || IsDead(character))
			return;

		foreach (COA_GearOverride gearOverride : m_aOverrides)
		{
			if (Matches(character, gearOverride))
				ApplyTo(character, gearOverride, true, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Faction and role match, and no role-scoped override takes precedence over a faction-wide one
	protected bool Matches(COA_GearscriptCharacter character, COA_GearOverride gearOverride)
	{
		if (COA_EntityHelper.DetermineFactionKey(character) != gearOverride.m_sFaction)
			return false;

		int role = character.GetGearRole();
		if (gearOverride.m_iRole >= 0 && gearOverride.m_iRole != role)
			return false;

		if (gearOverride.m_iAction != COA_EGearFixAction.WEAPON)
			return true;

		if (gearOverride.m_iRole < 0 && FindWeaponOverride(gearOverride.m_sFaction, role, gearOverride.m_iSlot))
			return false;

		COA_RolesConfig rolesConfig = COA_GearscriptManager.GetRolesConfig();
		if (!rolesConfig)
			return false;

		COA_GearScriptConfig gearConfig = COA_GearFixSlotHelper.LoadGearscript(gearOverride.m_sFaction);
		COA_GearFixRoleUse use = COA_GearFixSlotHelper.GetRoleUse(gearConfig, rolesConfig.FindRoleConfig(role), role, gearOverride.m_iSlot);
		return use.m_bAffected;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsDead(COA_GearscriptCharacter character)
	{
		CharacterControllerComponent controller = character.GetCharacterController();
		return !controller || controller.GetLifeState() == ECharacterLifeState.DEAD;
	}

	//------------------------------------------------------------------------------------------------
	//! Unconscious, seated in a vehicle or mid-reload: changing their gear now could leave weapon
	//! state or animations stuck, so it waits until they're free
	protected bool IsBusy(COA_GearscriptCharacter character)
	{
		if (character.IsInVehicle())
			return true;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller)
			return true;

		return controller.GetLifeState() == ECharacterLifeState.INCAPACITATED || controller.IsReloading() || controller.IsUsingItem();
	}

	//------------------------------------------------------------------------------------------------
	//! Every living character (players and AI) the override applies to
	//! \return how many were re-geared or queued
	protected int Regear(COA_GearOverride gearOverride, bool notify)
	{
		array<COA_GearscriptCharacter> characters = {};
		CollectCharacters(characters);

		int count;
		foreach (COA_GearscriptCharacter character : characters)
		{
			if (IsDead(character) || !Matches(character, gearOverride))
				continue;

			count++;
			IEntity entity = character;
			GetGame().GetCallqueue().CallLater(RegearQueued, count * REGEAR_STAGGER_MS, false, entity, gearOverride.m_iId, notify);
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectCharacters(notnull array<COA_GearscriptCharacter> characters)
	{
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int playerId : players)
		{
			COA_GearscriptCharacter character = COA_GearscriptCharacter.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId));
			if (character)
				characters.Insert(character);
		}

		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld)
			return;

		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);
		foreach (AIAgent agent : agents)
		{
			COA_GearscriptCharacter aiCharacter = COA_GearscriptCharacter.Cast(agent.GetControlledEntity());
			if (aiCharacter && !characters.Contains(aiCharacter))
				characters.Insert(aiCharacter);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RegearQueued(IEntity entity, int overrideId, bool notify)
	{
		COA_GearscriptCharacter character = COA_GearscriptCharacter.Cast(entity);
		if (!character || IsDead(character))
			return;

		COA_GearOverride gearOverride = FindOverride(overrideId);
		if (!gearOverride)
			return;

		if (IsBusy(character))
		{
			QueuePending(character, overrideId, notify);
			return;
		}

		ApplyTo(character, gearOverride, false, notify);
	}

	//------------------------------------------------------------------------------------------------
	protected void QueuePending(IEntity entity, int overrideId, bool notify)
	{
		foreach (COA_GearFixPending existing : m_aPending)
		{
			if (existing.m_Entity == entity && existing.m_iOverrideId == overrideId)
				return;
		}

		COA_GearFixPending pending = new COA_GearFixPending();
		pending.m_Entity = entity;
		pending.m_iOverrideId = overrideId;
		pending.m_bNotify = notify;
		pending.m_fExpiresAt = GetGame().GetWorld().GetWorldTime() + PENDING_TIMEOUT_MS;
		m_aPending.Insert(pending);
	}

	//------------------------------------------------------------------------------------------------
	protected void ProcessPending()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		for (int i = m_aPending.Count() - 1; i >= 0; i--)
		{
			COA_GearFixPending pending = m_aPending[i];
			COA_GearscriptCharacter character = COA_GearscriptCharacter.Cast(pending.m_Entity);
			COA_GearOverride gearOverride = FindOverride(pending.m_iOverrideId);

			// Dead: the corpse keeps its gear and the respawn gets the override
			if (!character || !gearOverride || IsDead(character) || now > pending.m_fExpiresAt)
			{
				m_aPending.Remove(i);
				continue;
			}

			if (IsBusy(character))
				continue;

			bool notify = pending.m_bNotify;
			m_aPending.Remove(i);
			ApplyTo(character, gearOverride, false, notify);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyTo(COA_GearscriptCharacter character, COA_GearOverride gearOverride, bool spawnPass, bool notify)
	{
		if (gearOverride.m_iAction == COA_EGearFixAction.WEAPON)
			StartWeaponSwap(character, gearOverride, spawnPass, notify);
		else
			ApplyItemOverride(character, gearOverride, spawnPass, notify);
	}

	//------------------------------------------------------------------------------------------------
	protected void SendPlayerHint(COA_GearscriptCharacter character, string message)
	{
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(character);
		if (playerId <= 0)
			return;

		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.SendHint("[Gear Fix] " + message, playerId);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 WEAPON SWAP
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected void StartWeaponSwap(COA_GearscriptCharacter character, COA_GearOverride gearOverride, bool spawnPass, bool notify)
	{
		foreach (int existingId, COA_GearFixSwapJob existing : m_mJobs)
		{
			if (existing.m_Character != character || existing.m_iSlot != gearOverride.m_iSlot)
				continue;

			// A swap of this slot is still running on them: try again once it's done
			if (!spawnPass)
				QueuePending(character, gearOverride.m_iId, notify);

			return;
		}

		COA_GearFixSwapJob job = new COA_GearFixSwapJob();
		job.m_iJobId = m_iNextJobId;
		m_iNextJobId++;
		job.m_iOverrideId = gearOverride.m_iId;
		job.m_iSlot = gearOverride.m_iSlot;
		job.m_bSpawnPass = spawnPass;
		job.m_bNotify = notify;
		job.m_Character = character;
		m_mJobs.Set(job.m_iJobId, job);

		// Put a weapon in hands away first, or the player can end up holding an invisible gun
		int delay = 0;
		if (!spawnPass)
		{
			array<ResourceName> oldWeapons = {};
			array<ResourceName> oldMagazines = {};
			GetOldGear(gearOverride, character.GetGearRole(), oldWeapons, oldMagazines);
			IEntity oldWeapon = FindCarriedWeapon(character, oldWeapons);

			BaseWeaponManagerComponent weaponManager = character.GetWeaponManager();
			if (oldWeapon != null && weaponManager != null && weaponManager.GetCurrentWeapon() != null && weaponManager.GetCurrentWeapon().GetOwner() == oldWeapon)
			{
				job.m_bWasCurrent = true;
				delay = HOLSTER_DELAY_MS;

				int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(character);
				COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
				if (playerId > 0 && broadcastManager != null)
					broadcastManager.HolsterGun(playerId);
			}
		}

		GetGame().GetCallqueue().CallLater(WeaponSwap_Delete, delay, false, job.m_iJobId);
	}

	//------------------------------------------------------------------------------------------------
	//! Stage 1: work out what they have and scale the new ammo to it, then remove the old weapon
	//! first (deleting a magazine before its weapon can crash MuzzleInMagComponent, see ClearEntityGear)
	protected void WeaponSwap_Delete(int jobId)
	{
		COA_GearFixSwapJob job = m_mJobs.Get(jobId);
		if (!job)
			return;

		COA_GearOverride gearOverride = FindOverride(job.m_iOverrideId);
		COA_GearscriptCharacter character = job.m_Character;
		COA_RolesConfig rolesConfig = COA_GearscriptManager.GetRolesConfig();
		COA_GearScriptConfig gearConfig;
		if (gearOverride)
			gearConfig = COA_GearFixSlotHelper.LoadGearscript(gearOverride.m_sFaction);

		SCR_InventoryStorageManagerComponent inventoryManager;
		if (character)
			inventoryManager = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));

		if (!gearOverride || !character || !rolesConfig || !gearConfig || !inventoryManager || IsDead(character))
		{
			m_mJobs.Remove(jobId);
			return;
		}

		int role = character.GetGearRole();
		COA_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
		COA_GearFixRoleUse use = COA_GearFixSlotHelper.GetRoleUse(gearConfig, roleConfig, role, gearOverride.m_iSlot);
		job.m_bAssistant = use.m_bAssistant;

		array<ResourceName> oldWeapons = {};
		array<ResourceName> oldMagazines = {};
		GetOldGear(gearOverride, role, oldWeapons, oldMagazines);

		IEntity oldWeapon = FindCarriedWeapon(character, oldWeapons);
		int expected = GetExpectedMagazines(gearOverride, gearConfig, role, use, oldWeapon, false);
		int expectedLauncher = GetExpectedMagazines(gearOverride, gearConfig, role, use, oldWeapon, true);

		// Their spare magazines, minus what their other weapons are issued of the same magazine
		map<ResourceName, int> protectedMagazines = new map<ResourceName, int>();
		COA_GearFixSlotHelper.GetOtherSlotMagazines(gearConfig, roleConfig, role, gearOverride.m_iSlot, protectedMagazines);

		int had;
		int hadLauncher;
		array<IEntity> carried = {};
		GetCarriedItems(character, inventoryManager, carried);
		foreach (IEntity item : carried)
		{
			if (!item || !ContainsPrefab(oldMagazines, GetPrefab(item)) || IsInsideWeapon(item))
				continue;

			ResourceName prefab = GetPrefab(item);
			int keep = FindProtectedCount(protectedMagazines, prefab);
			if (keep > 0)
			{
				SetProtectedCount(protectedMagazines, prefab, keep - 1);
				continue;
			}

			// Rounds the new weapon's launcher also takes (M203 to M203, smoke included) stay with them
			bool oldLauncherRound = IsOldLauncherRound(oldWeapons, prefab);
			if (oldLauncherRound && COA_GearPrefabHelper.IsLauncherMagazine(gearOverride.m_rNewWeapon, prefab) && COA_GearPrefabHelper.MagazineFits(gearOverride.m_rNewWeapon, prefab, gearOverride.m_aNewAttachments) == 1)
				continue;

			if (oldLauncherRound)
				hadLauncher++;
			else
				had++;

			job.m_aMagazinesToDelete.Insert(item);
		}

		if (!oldWeapon && had + hadLauncher == 0)
		{
			m_mJobs.Remove(jobId); // nothing of this slot on them: dropped or never had it
			return;
		}

		int newConfigured = gearOverride.GetPoolCount(use.m_bAssistant, false);
		int newLauncherConfigured = gearOverride.GetPoolCount(use.m_bAssistant, true);
		if (!use.m_bGetsMagazines)
		{
			newConfigured = 0;
			newLauncherConfigured = 0;
		}

		int newTotal = ScaleMagazines(had, expected, newConfigured);
		job.m_iGiveTotal = DistributeMagazines(job, gearOverride, use.m_bAssistant, false, newConfigured, newTotal);

		// A rifle gaining a launcher has no old rounds to go by: same share as their magazines
		int newLauncherTotal = ScaleMagazines(had, expected, newLauncherConfigured);
		if (expectedLauncher > 0)
			newLauncherTotal = ScaleMagazines(hadLauncher, expectedLauncher, newLauncherConfigured);

		job.m_iGiveLauncherTotal = DistributeMagazines(job, gearOverride, use.m_bAssistant, true, newLauncherConfigured, newLauncherTotal);

		if (oldWeapon)
		{
			job.m_bHadWeapon = true;
			job.m_fLoadedFraction = GetLoadedFraction(oldWeapon);

			InventoryItemComponent itemComponent = InventoryItemComponent.Cast(oldWeapon.FindComponent(InventoryItemComponent));
			if (itemComponent != null && itemComponent.GetParentSlot() != null)
			{
				job.m_Storage = itemComponent.GetParentSlot().GetStorage();
				job.m_iSlotId = itemComponent.GetParentSlot().GetID();
			}

			SCR_EntityHelper.DeleteEntityAndChildren(oldWeapon);
		}

		GetGame().GetCallqueue().CallLater(WeaponSwap_DeleteMagazines, 100, false, jobId);
	}

	//------------------------------------------------------------------------------------------------
	//! Stage 2: remove the old magazines
	protected void WeaponSwap_DeleteMagazines(int jobId)
	{
		COA_GearFixSwapJob job = m_mJobs.Get(jobId);
		if (!job)
			return;

		foreach (IEntity magazine : job.m_aMagazinesToDelete)
		{
			if (magazine)
				SCR_EntityHelper.DeleteEntityAndChildren(magazine);
		}

		job.m_aMagazinesToDelete.Clear();
		GetGame().GetCallqueue().CallLater(WeaponSwap_Spawn, 150, false, jobId);
	}

	//------------------------------------------------------------------------------------------------
	//! Stage 3: new weapon into the same slot, new magazines into the inventory
	protected void WeaponSwap_Spawn(int jobId)
	{
		COA_GearFixSwapJob job = m_mJobs.Get(jobId);
		if (!job)
			return;

		COA_GearOverride gearOverride = FindOverride(job.m_iOverrideId);
		COA_GearscriptCharacter character = job.m_Character;
		COA_GearscriptManager gearscriptManager = COA_GearscriptManager.GetInstance();
		if (!gearOverride || !character || !gearscriptManager)
		{
			m_mJobs.Remove(jobId);
			return;
		}

		SCR_CharacterInventoryStorageComponent inventory = SCR_CharacterInventoryStorageComponent.Cast(character.FindComponent(SCR_CharacterInventoryStorageComponent));
		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory || !inventoryManager)
		{
			m_mJobs.Remove(jobId);
			return;
		}

		if (job.m_bHadWeapon)
		{
			bool spawned = false;
			if (job.m_Storage != null && EquipedWeaponStorageComponent.Cast(job.m_Storage) != null)
				spawned = inventoryManager.TrySpawnPrefabToStorage(gearOverride.m_rNewWeapon, job.m_Storage, job.m_iSlotId, EStoragePurpose.PURPOSE_WEAPON_PROXY);

			// Doesn't suit that slot (a launcher into the rifle slot...): any weapon slot, then any storage
			if (!spawned)
				spawned = inventoryManager.TrySpawnPrefabToStorage(gearOverride.m_rNewWeapon, null, -1, EStoragePurpose.PURPOSE_WEAPON_PROXY);

			if (!spawned)
				spawned = inventoryManager.TrySpawnPrefabToStorage(gearOverride.m_rNewWeapon);

			if (!spawned)
				COA_LoggingHelper.LogItemError(gearOverride.m_rNewWeapon, character, "WEAPON");
		}

		int role = character.GetGearRole();
		for (int i = 0; i < job.m_aGiveMagazines.Count(); i++)
			gearscriptManager.AddInventoryItem(job.m_aGiveMagazines[i], job.m_aGiveCounts[i], null, inventory, inventoryManager, role);

		GetGame().GetCallqueue().CallLater(WeaponSwap_Finish, 100, false, jobId);
	}

	//------------------------------------------------------------------------------------------------
	//! Stage 4: attachments, carry over how full the loaded magazine was, weapon back in hands, tell them
	protected void WeaponSwap_Finish(int jobId)
	{
		ref COA_GearFixSwapJob job = m_mJobs.Get(jobId);
		if (!job)
			return;

		COA_GearOverride gearOverride = FindOverride(job.m_iOverrideId);
		COA_GearscriptCharacter character = job.m_Character;
		m_mJobs.Remove(jobId);
		if (!gearOverride || !character)
			return;

		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		BaseWeaponManagerComponent weaponManager = character.GetWeaponManager();

		if (job.m_bHadWeapon && weaponManager != null && inventoryManager != null)
		{
			COA_WeaponHelper.AddAttachments(gearOverride.m_rNewWeapon, gearOverride.m_aNewAttachments, null, inventoryManager);

			array<ResourceName> newWeapons = {};
			newWeapons.Insert(gearOverride.m_rNewWeapon);
			IEntity newWeapon = FindCarriedWeapon(character, newWeapons);
			if (newWeapon)
			{
				// No free reload: the loaded magazine stays as full as the old one was
				if (!job.m_bSpawnPass)
					ApplyLoadedFraction(job, gearOverride, character, newWeapon);

				if (job.m_bSpawnPass)
				{
					COA_WeaponHelper.SelectWeapon(character);
				}
				else if (job.m_bWasCurrent)
				{
					array<WeaponSlotComponent> slots = {};
					weaponManager.GetWeaponsSlots(slots);
					foreach (WeaponSlotComponent slot : slots)
					{
						if (slot.GetWeaponEntity() == newWeapon)
						{
							character.SelectWeaponSlot(slot.GetWeaponSlotIndex());
							break;
						}
					}
				}
			}
		}

		if (job.m_bNotify)
			NotifyWeaponSwap(job, gearOverride, character);
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyLoadedFraction(COA_GearFixSwapJob job, COA_GearOverride gearOverride, COA_GearscriptCharacter character, IEntity newWeapon)
	{
		BaseWeaponComponent weaponComponent = BaseWeaponComponent.Cast(newWeapon.FindComponent(BaseWeaponComponent));
		if (!weaponComponent)
			return;

		BaseMagazineComponent loaded = weaponComponent.GetCurrentMagazine();
		if (loaded)
		{
			int rounds = Math.Round(loaded.GetMaxAmmoCount() * job.m_fLoadedFraction);
			loaded.SetAmmoCount(rounds);
			return;
		}

		// The new weapon comes unloaded, so what was in the old one becomes a spare magazine
		ResourceName magazine = gearOverride.GetFirstNewMagazine();
		COA_GearscriptManager gearscriptManager = COA_GearscriptManager.GetInstance();
		SCR_CharacterInventoryStorageComponent inventory = SCR_CharacterInventoryStorageComponent.Cast(character.FindComponent(SCR_CharacterInventoryStorageComponent));
		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (job.m_fLoadedFraction > 0 && !magazine.IsEmpty() && gearscriptManager != null && inventory != null && inventoryManager != null)
			gearscriptManager.AddInventoryItem(magazine, 1, null, inventory, inventoryManager, character.GetGearRole());
	}

	//------------------------------------------------------------------------------------------------
	protected void NotifyWeaponSwap(COA_GearFixSwapJob job, COA_GearOverride gearOverride, COA_GearscriptCharacter character)
	{
		string slotLabel = COA_GearFixText.SlotLabel(gearOverride.m_iSlot);
		string weaponName = COA_GearFixText.Name(gearOverride.m_rNewWeapon);
		ResourceName magazine = gearOverride.GetFirstNewMagazine();
		ResourceName launcherAmmo = gearOverride.GetNewLauncherAmmo();

		string ammo;
		if (job.m_iGiveTotal > 0 && !magazine.IsEmpty())
			ammo = string.Format(" with %1x %2", job.m_iGiveTotal, COA_GearFixText.Name(magazine));

		if (job.m_iGiveLauncherTotal > 0 && !launcherAmmo.IsEmpty())
		{
			if (ammo.IsEmpty())
				ammo = string.Format(" with %1x %2", job.m_iGiveLauncherTotal, COA_GearFixText.Name(launcherAmmo));
			else
				ammo += string.Format(" and %1x %2", job.m_iGiveLauncherTotal, COA_GearFixText.Name(launcherAmmo));
		}

		if (job.m_bHadWeapon)
		{
			if (gearOverride.m_bRevertToGearscript)
				SendPlayerHint(character, string.Format("An admin changed your %1 back to %2%3", slotLabel, weaponName, ammo));
			else
				SendPlayerHint(character, string.Format("An admin changed your %1 to %2%3", slotLabel, weaponName, ammo));

			return;
		}

		if (job.m_bAssistant)
			SendPlayerHint(character, string.Format("An admin changed the %1 to %2. The ammo you carry is now%3", slotLabel, weaponName, ammo));
		else
			SendPlayerHint(character, string.Format("An admin changed the %1 to %2. Your spare ammo was replaced%3; you weren't carrying the old weapon, so you weren't given the new one", slotLabel, weaponName, ammo));
	}

	//------------------------------------------------------------------------------------------------
	//! Weapons and magazines that count as the override's "old gear" for a role
	protected void GetOldGear(COA_GearOverride gearOverride, int role, notnull array<ResourceName> oldWeapons, notnull array<ResourceName> oldMagazines)
	{
		if (!gearOverride.m_bExplicitOld)
		{
			array<ref COA_GearFixSlotEntry> entries = {};
			COA_GearFixSlotHelper.GetSlotEntries(COA_GearFixSlotHelper.LoadGearscript(gearOverride.m_sFaction), role, gearOverride.m_iSlot, entries);
			foreach (COA_GearFixSlotEntry entry : entries)
			{
				oldWeapons.Insert(entry.m_rWeapon);
				oldMagazines.InsertAll(entry.m_aMagazines);
			}
		}

		oldWeapons.InsertAll(gearOverride.m_aExtraOldWeapons);
		oldMagazines.InsertAll(gearOverride.m_aExtraOldMagazines);
	}

	//------------------------------------------------------------------------------------------------
	//! A launcher round on every old weapon: a magazine is the main muzzle's if any of them takes it there
	protected bool IsOldLauncherRound(notnull array<ResourceName> oldWeapons, ResourceName magazine)
	{
		foreach (ResourceName weapon : oldWeapons)
		{
			if (!COA_GearPrefabHelper.IsLauncherMagazine(weapon, magazine))
				return false;
		}

		return !oldWeapons.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	//! How many spare magazines (or launcher rounds) a full loadout has of the old gear, for scaling
	protected int GetExpectedMagazines(COA_GearOverride gearOverride, COA_GearScriptConfig gearConfig, int role, COA_GearFixRoleUse use, IEntity oldWeapon, bool launcher)
	{
		if (!use.m_bGetsMagazines)
			return 0;

		array<ref COA_GearFixSlotEntry> entries = {};
		COA_GearFixSlotHelper.GetSlotEntries(gearConfig, role, gearOverride.m_iSlot, entries);

		if (oldWeapon)
		{
			ResourceName carried = GetPrefab(oldWeapon);
			foreach (COA_GearFixSlotEntry entry : entries)
			{
				if (SamePrefab(entry.m_rWeapon, carried))
					return entry.GetPoolTotal(use.m_bAssistant, launcher);
			}
		}

		// Carrying what an earlier override handed out
		if (gearOverride.m_iExtraOldCount >= 0)
		{
			if (launcher && use.m_bAssistant)
				return gearOverride.m_iExtraOldLauncherAssistantCount;

			if (launcher)
				return gearOverride.m_iExtraOldLauncherCount;

			if (use.m_bAssistant)
				return gearOverride.m_iExtraOldAssistantCount;

			return gearOverride.m_iExtraOldCount;
		}

		if (entries.IsEmpty())
			return 0;

		return entries[0].GetPoolTotal(use.m_bAssistant, launcher);
	}

	//------------------------------------------------------------------------------------------------
	//! Same share of the new ammo as they have left of the old, at least 1 if they had any, never
	//! more than the override's full count
	protected int ScaleMagazines(int had, int expected, int newConfigured)
	{
		if (had <= 0 || newConfigured <= 0)
			return 0;

		if (expected <= 0)
			return Math.Min(had, newConfigured);

		float share = had / (float)expected;
		int scaled = Math.Round(share * newConfigured);
		return Math.ClampInt(scaled, 1, newConfigured);
	}

	//------------------------------------------------------------------------------------------------
	//! Splits the scaled total across the override's magazine types of one kind (a revert can have several)
	//! \return how many were given
	protected int DistributeMagazines(COA_GearFixSwapJob job, COA_GearOverride gearOverride, bool assistant, bool launcher, int newConfigured, int newTotal)
	{
		if (newTotal <= 0 || newConfigured <= 0)
			return 0;

		int lastIndex = -1;
		for (int j = 0; j < gearOverride.m_aNewMagazines.Count(); j++)
		{
			if (gearOverride.IsLauncherAmmo(gearOverride.m_aNewMagazines[j]) == launcher)
				lastIndex = j;
		}

		int given;
		for (int i = 0; i < gearOverride.m_aNewMagazines.Count(); i++)
		{
			if (gearOverride.IsLauncherAmmo(gearOverride.m_aNewMagazines[i]) != launcher)
				continue;

			int configured = gearOverride.m_aNewCounts[i];
			if (assistant)
				configured = gearOverride.m_aNewAssistantCounts[i];

			int count = Math.Round(configured * newTotal / (float)newConfigured);

			// Rounding must not lose the last magazine they're owed
			if (count == 0 && configured > 0 && given == 0 && i == lastIndex)
				count = 1;

			if (count <= 0)
				continue;

			given += count;
			job.m_aGiveMagazines.Insert(gearOverride.m_aNewMagazines[i]);
			job.m_aGiveCounts.Insert(count);
		}

		return given;
	}

	//------------------------------------------------------------------------------------------------
	//! A weapon of this list in a weapon slot, or stowed in the inventory
	protected IEntity FindCarriedWeapon(COA_GearscriptCharacter character, array<ResourceName> weapons)
	{
		BaseWeaponManagerComponent weaponManager = character.GetWeaponManager();
		if (weaponManager)
		{
			array<IEntity> equipped = {};
			weaponManager.GetWeaponsList(equipped);
			foreach (IEntity weapon : equipped)
			{
				if (weapon != null && ContainsPrefab(weapons, GetPrefab(weapon)))
					return weapon;
			}
		}

		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventoryManager)
			return null;

		array<IEntity> items = {};
		GetCarriedItems(character, inventoryManager, items);
		foreach (IEntity item : items)
		{
			if (item != null && item.FindComponent(BaseWeaponComponent) != null && ContainsPrefab(weapons, GetPrefab(item)))
				return item;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! GetItems alone misses what's inside clothing (vest pouches, backpack compartments), so spare
	//! magazines there were never swapped. ClearEntityGear collects items the same way.
	protected void GetCarriedItems(IEntity character, SCR_InventoryStorageManagerComponent inventoryManager, notnull array<IEntity> items)
	{
		items.Clear();

		array<IEntity> found = {};
		inventoryManager.GetItems(found);

		SCR_CharacterInventoryStorageComponent storage = SCR_CharacterInventoryStorageComponent.Cast(character.FindComponent(SCR_CharacterInventoryStorageComponent));
		if (storage)
			inventoryManager.GetAllItems(found, storage);

		foreach (IEntity item : found)
		{
			if (item && !items.Contains(item))
				items.Insert(item);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected float GetLoadedFraction(IEntity weapon)
	{
		BaseWeaponComponent weaponComponent = BaseWeaponComponent.Cast(weapon.FindComponent(BaseWeaponComponent));
		if (!weaponComponent)
			return 0;

		BaseMagazineComponent loaded = weaponComponent.GetCurrentMagazine();
		if (!loaded || loaded.GetMaxAmmoCount() <= 0)
			return 0;

		return loaded.GetAmmoCount() / (float)loaded.GetMaxAmmoCount();
	}

	//------------------------------------------------------------------------------------------------
	//! A magazine in a weapon (or in an UGL on one) is not a spare
	protected bool IsInsideWeapon(IEntity item)
	{
		IEntity parent = item.GetParent();
		while (parent)
		{
			if (parent.FindComponent(BaseWeaponComponent))
				return true;

			parent = parent.GetParent();
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected ResourceName GetPrefab(IEntity entity)
	{
		if (!entity || !entity.GetPrefabData())
			return ResourceName.Empty;

		return entity.GetPrefabData().GetPrefabName();
	}

	//------------------------------------------------------------------------------------------------
	//! By GUID: mods can override a prefab from another path
	protected bool SamePrefab(ResourceName a, ResourceName b)
	{
		if (a.IsEmpty() || b.IsEmpty())
			return false;

		return SCR_ResourceNameUtils.GetPrefabGUID(a) == SCR_ResourceNameUtils.GetPrefabGUID(b);
	}

	//------------------------------------------------------------------------------------------------
	protected bool ContainsPrefab(array<ResourceName> prefabs, ResourceName prefab)
	{
		foreach (ResourceName candidate : prefabs)
		{
			if (SamePrefab(candidate, prefab))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected int FindProtectedCount(notnull map<ResourceName, int> protectedMagazines, ResourceName prefab)
	{
		foreach (ResourceName candidate, int count : protectedMagazines)
		{
			if (SamePrefab(candidate, prefab))
				return count;
		}

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	protected void SetProtectedCount(notnull map<ResourceName, int> protectedMagazines, ResourceName prefab, int count)
	{
		foreach (ResourceName candidate, int current : protectedMagazines)
		{
			if (SamePrefab(candidate, prefab))
			{
				protectedMagazines.Set(candidate, count);
				return;
			}
		}
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 ITEM OVERRIDES
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected void ApplyItemOverride(COA_GearscriptCharacter character, COA_GearOverride gearOverride, bool spawnPass, bool notify)
	{
		SCR_CharacterInventoryStorageComponent inventory = SCR_CharacterInventoryStorageComponent.Cast(character.FindComponent(SCR_CharacterInventoryStorageComponent));
		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory || !inventoryManager)
			return;

		int role = character.GetGearRole();

		if (gearOverride.m_iAction == COA_EGearFixAction.ITEM_ADD)
		{
			int count = gearOverride.m_iItemCount;
			if (count <= 0)
			{
				COA_RolesConfig rolesConfig = COA_GearscriptManager.GetRolesConfig();
				if (rolesConfig)
					count = COA_GearFixSlotHelper.CountKitItem(COA_GearFixSlotHelper.LoadGearscript(gearOverride.m_sFaction), COA_Gamemode.GetInstance().GetGearScriptSettings(gearOverride.m_sFaction), rolesConfig.FindRoleConfig(role), role, gearOverride.m_rNewItem);
			}

			if (count <= 0)
				return;

			GiveItems(character, gearOverride.m_rNewItem, count);
			if (notify)
				SendPlayerHint(character, string.Format("An admin gave you %1x %2", count, COA_GearFixText.Name(gearOverride.m_rNewItem)));

			return;
		}

		// Swap/remove: everything of the old item they have (at most m_iItemCount if that's set)
		array<IEntity> carried = {};
		GetCarriedItems(character, inventoryManager, carried);

		array<IEntity> matching = {};
		foreach (IEntity item : carried)
		{
			if (item != null && !matching.Contains(item) && SamePrefab(GetPrefab(item), gearOverride.m_rOldItem) && !IsInsideWeapon(item))
				matching.Insert(item);
		}

		if (gearOverride.m_iAction == COA_EGearFixAction.ITEM_REMOVE && gearOverride.m_iItemCount > 0)
		{
			while (matching.Count() > gearOverride.m_iItemCount)
				matching.Remove(matching.Count() - 1);
		}

		if (matching.IsEmpty())
			return;

		bool swapping = gearOverride.m_iAction == COA_EGearFixAction.ITEM_SWAP;
		int regular;
		foreach (IEntity match : matching)
		{
			// Worn clothing keeps its slot, and what's inside it stays in the inventory
			InventoryItemComponent itemComponent = InventoryItemComponent.Cast(match.FindComponent(InventoryItemComponent));
			if (itemComponent != null && itemComponent.GetParentSlot() != null && SCR_CharacterInventoryStorageComponent.Cast(itemComponent.GetParentSlot().GetStorage()) != null)
			{
				array<ResourceName> clothing = {};
				if (swapping)
					clothing.Insert(gearOverride.m_rNewItem);
				else
					clothing.Insert(ResourceName.Empty);

				COA_ClothingHelper.UpdateClothingSlot(clothing, itemComponent.GetParentSlot().GetID(), role, false, null, inventory, inventoryManager);
				continue;
			}

			regular++;
			SCR_EntityHelper.DeleteEntityAndChildren(match);
		}

		if (swapping && regular > 0)
		{
			IEntity entity = character;
			GetGame().GetCallqueue().CallLater(GiveItems, 100, false, entity, gearOverride.m_rNewItem, regular);
		}

		RefreshRadios(character, gearOverride.m_rOldItem, gearOverride.m_rNewItem);

		if (!notify)
			return;

		if (swapping)
			SendPlayerHint(character, string.Format("An admin changed your %1 to %2", COA_GearFixText.Name(gearOverride.m_rOldItem), COA_GearFixText.Name(gearOverride.m_rNewItem)));
		else
			SendPlayerHint(character, string.Format("An admin removed your %1", COA_GearFixText.Name(gearOverride.m_rOldItem)));
	}

	//------------------------------------------------------------------------------------------------
	protected void GiveItems(IEntity entity, ResourceName item, int count)
	{
		COA_GearscriptCharacter character = COA_GearscriptCharacter.Cast(entity);
		COA_GearscriptManager gearscriptManager = COA_GearscriptManager.GetInstance();
		if (!character || !gearscriptManager)
			return;

		SCR_CharacterInventoryStorageComponent inventory = SCR_CharacterInventoryStorageComponent.Cast(character.FindComponent(SCR_CharacterInventoryStorageComponent));
		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (inventory != null && inventoryManager != null)
			gearscriptManager.AddInventoryItem(item, count, null, inventory, inventoryManager, character.GetGearRole());

		RefreshRadios(character, ResourceName.Empty, item);
	}

	//------------------------------------------------------------------------------------------------
	//! Radios are listed per player, so changing one needs the list rebuilt and the new one tuned
	protected void RefreshRadios(COA_GearscriptCharacter character, ResourceName oldItem, ResourceName newItem)
	{
		if (!COA_GearPrefabHelper.IsRadio(oldItem) && !COA_GearPrefabHelper.IsRadio(newItem))
			return;

		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(character);
		if (playerId > 0)
			GetGame().GetCallqueue().CallLater(COA_PlayerRplToAuthorityManager.InitializePlayerRadiosDelayed, 500, false, playerId);
	}
}
