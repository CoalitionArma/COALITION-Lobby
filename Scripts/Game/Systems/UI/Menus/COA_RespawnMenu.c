/**
 * Respawn screen: fullscreen map, the respawn timer in the header and a panel to pick and confirm a
 * spawn point (header + panel shared with the JIP forward deploy screen - see COA_DeployPanel).
 *
 * The respawn itself is driven by COA_RespawnManager: once the timer runs out with a spawn point
 * confirmed (m_SelectedSpawnPoint + m_RespawnConfirmed) it respawns the player. Before the timer
 * ends the button confirms/changes the choice; after it, it deploys straight away.
 */
class COA_RespawnMenu: ChimeraMenuBase
{
	//! How long a one-off message (e.g. "that point is blocked") replaces the status line
	protected static const float NOTICE_SECONDS = 4;

	// Hover/press feedback and entrance animation - see COA_UIPolish
	protected ref COA_MenuPolish m_UIPolish;
	protected ref COA_DeployPanel m_Panel;
	protected ref COA_DeployMarkers m_Markers = new COA_DeployMarkers();

	// UI components
	protected SCR_MapEntity m_MapEntity;
	protected SCR_ChatPanel m_ChatPanel;
	protected SCR_ListBoxComponent m_wSpawnListBox;
	protected ref array<COA_SpawnPointData> m_aSpawnPoints = {};
	protected FactionKey m_factionKey;
	protected SCR_AIGroup m_group;

	// Last text pushed to the panel, so it's only rebuilt when something changed
	protected string m_sLastStateKey;
	protected bool m_bLastCanSpectate = true;
	protected string m_sNotice;
	protected float m_fNoticeTimeLeft;

	//------------------------------------------------------------------------------------------------
	override void OnMenuInit()
	{
		super.OnMenuInit();

		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		m_Panel = new COA_DeployPanel(GetRootWidget());
		m_Panel.SetTitle("Respawn");
		m_Panel.SetSecondaryText("Spectate");

		COA_RespawnManager.GetInstance().GetOnSpawnPointsUpdated().Insert(OnSpawnpointStateChanged);

		InitializeSpawnpointSelection();

		if (m_MapEntity)
			GetGame().GetCallqueue().CallLater(OpenMapWithConfig, 100);

		RegisterInputHandlers();
		InitializeChatPanel();
		UpdateHints();
		UpdatePanel();

		// Hover feedback on buttons without their own, and a quick staggered fade-in (COA_UIPolish)
		m_UIPolish = new COA_MenuPolish(GetRootWidget(), true);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		if (m_UIPolish)
			m_UIPolish.Update(tDelta);

		super.OnMenuUpdate(tDelta);

		if (m_MapEntity)
			GetGame().GetInputManager().ActivateContext("MapContext");

		if (m_fNoticeTimeLeft > 0)
		{
			m_fNoticeTimeLeft -= tDelta;
			if (m_fNoticeTimeLeft <= 0)
				m_sNotice = string.Empty;
		}

		UpdatePanel();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		if (m_UIPolish)
		{
			m_UIPolish.Cleanup();
			m_UIPolish = null;
		}

		super.OnMenuClose();

		// Pending calls into this menu must not outlive it
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(OpenMapWithConfig);
			GetGame().GetCallqueue().Remove(AdjustMapZoom);
			GetGame().GetCallqueue().Remove(PopulateListBox);
			GetGame().GetCallqueue().Remove(OpenChatPanel);

			// A push-to-talk release still pending would run on the freed menu - release now instead
			if (!CVON_VONGameModeComponent.GetInstance())
			{
				GetGame().GetCallqueue().Remove(LobbyVoNDisableDelayed);
				LobbyVoNDisableDelayed();
			}
		}

		COA_RespawnManager rm = COA_RespawnManager.GetInstance();
		if (rm)
			rm.GetOnSpawnPointsUpdated().Remove(OnSpawnpointStateChanged);

		UnregisterInputHandlers();
		m_Markers.Clear();

		if (m_MapEntity)
			m_MapEntity.CloseMap();
	}

//------------------------------------------------------------------------------------------------
//	SPAWN POINT LIST
//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void InitializeSpawnpointSelection()
	{
		m_wSpawnListBox = m_Panel.GetList();
		if (m_wSpawnListBox)
			m_wSpawnListBox.m_OnChanged.Insert(UpdateSpawnSelection);

		SCR_ButtonTextComponent confirmButton = m_Panel.GetPrimaryButton();
		if (confirmButton)
			confirmButton.m_OnClicked.Insert(OnPrimaryClicked);

		SCR_ButtonTextComponent spectateButton = m_Panel.GetSecondaryButton();
		if (spectateButton)
			spectateButton.m_OnClicked.Insert(OnSpectateClicked);

		PopulateListBox();
	}

	//------------------------------------------------------------------------------------------------
	//! Rebuild the list and map markers from the faction's active spawn points, keeping the
	//! player's pick when it's still open
	protected void PopulateListBox()
	{
		if (!m_wSpawnListBox)
			return;

		m_aSpawnPoints.Clear();
		m_wSpawnListBox.Clear();
		m_Markers.Clear();

		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!respawnManager || !slottingManager)
			return;

		int playerID = GetGame().GetPlayerController().GetPlayerId();
		Faction playerFaction = slottingManager.GetPlayerSlotFaction(playerID);
		if (!playerFaction)
			return;

		// Keep the player's pick when the list is rebuilt (menu reopened after spectating, or a spawn
		// point changed state) instead of jumping back to the first point
		int previousSpawnPointId = -1;
		string previousSpawnPointName;
		if (respawnManager.m_SelectedSpawnPoint)
		{
			previousSpawnPointId = respawnManager.m_SelectedSpawnPoint.GetSpawnPointId();
			previousSpawnPointName = respawnManager.m_SelectedSpawnPoint.GetSpawnPointName();
		}

		m_factionKey = playerFaction.GetFactionKey();
		m_group = slottingManager.GetPlayerSlotGroup(playerID);

		vector deathPosition = GetLastDeathPosition();

		int previousIndex = -1;
		int firstOpenIndex = -1;
		foreach (COA_SpawnPointData spawnPointData : respawnManager.GetFactionSpawnpoints(m_factionKey, m_group))
		{
			IEntity point = COA_EntityHelper.GetEntityFromRplId(spawnPointData.GetSpawnPointEntity());
			if (!point || !spawnPointData.GetIsActiveSpawnPoint())
				continue;

			m_aSpawnPoints.Insert(spawnPointData);
			bool blocked = spawnPointData.GetIsSpawnPointBlocked();
			vector worldPos = point.GetOrigin();

			if (blocked)
				m_Markers.Add(spawnPointData.GetSpawnPointName(), worldPos, ARGB(255, 200, 60, 60));
			else
				m_Markers.Add(spawnPointData.GetSpawnPointName(), worldPos, ARGB(255, 0, 0, 225));

			int index = m_Panel.AddRow(spawnPointData.GetSpawnPointName(), GetRowDetail(spawnPointData, worldPos, deathPosition), GetRowTone(spawnPointData));
			m_Panel.SetRowDimmed(index, blocked);

			if (blocked)
				continue;

			if (firstOpenIndex == -1)
				firstOpenIndex = index;

			if (spawnPointData.GetSpawnPointId() == previousSpawnPointId)
				previousIndex = index;
		}

		if (previousIndex != -1)
		{
			m_wSpawnListBox.SetItemSelected(previousIndex, true);
			return;
		}

		// The earlier pick is gone or blocked now. Don't carry a confirmation over to a point the
		// player never chose - drop it and say why.
		if (previousSpawnPointId != -1)
		{
			if (respawnManager.m_RespawnConfirmed)
			{
				respawnManager.m_RespawnConfirmed = false;
				ShowNotice(string.Format("%1 is no longer available - pick another spawn point.", previousSpawnPointName));
			}

			respawnManager.m_SelectedSpawnPoint = null;
		}

		if (firstOpenIndex != -1)
			m_wSpawnListBox.SetItemSelected(firstOpenIndex, true);
	}

	//------------------------------------------------------------------------------------------------
	//! "Squad · 450 m" / "Blocked"
	protected string GetRowDetail(COA_SpawnPointData spawnPointData, vector worldPos, vector deathPosition)
	{
		if (spawnPointData.GetIsSpawnPointBlocked())
			return "Blocked";

		string detail;
		if (!spawnPointData.GetRestrictedToGroup().IsEmpty())
			detail = "Squad";

		if (deathPosition != vector.Zero)
		{
			string distance = COA_DeployPanel.FormatDistance(vector.DistanceXZ(worldPos, deathPosition));
			if (detail.IsEmpty())
				detail = distance;
			else
				detail = string.Format("%1 · %2", detail, distance);
		}

		return detail;
	}

	//------------------------------------------------------------------------------------------------
	protected COA_EDeployTone GetRowTone(COA_SpawnPointData spawnPointData)
	{
		if (spawnPointData.GetIsSpawnPointBlocked())
			return COA_EDeployTone.BAD;

		return COA_EDeployTone.NEUTRAL;
	}

	//------------------------------------------------------------------------------------------------
	//! Where the local player last died (zero if unknown) - row distances are measured from here
	protected vector GetLastDeathPosition()
	{
		COA_PlayerControllerManager manager = COA_PlayerControllerManager.GetInstance();
		if (!manager)
			return vector.Zero;

		return manager.m_vPlayersLastDeath[3];
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSpawnpointStateChanged()
	{
		GetGame().GetCallqueue().Remove(PopulateListBox);
		GetGame().GetCallqueue().Call(PopulateListBox);
	}

	//------------------------------------------------------------------------------------------------
	//! Store the selected spawn point in the respawn manager and pan the map to it
	protected void UpdateSpawnSelection()
	{
		COA_RespawnManager rm = COA_RespawnManager.GetInstance();
		if (!rm || !m_wSpawnListBox)
			return;

		int selected = m_wSpawnListBox.GetSelectedItem();
		if (selected < 0 || selected >= m_aSpawnPoints.Count())
			return;

		COA_SpawnPointData spawnPointData = m_aSpawnPoints[selected];
		IEntity point = COA_EntityHelper.GetEntityFromRplId(spawnPointData.GetSpawnPointEntity());

		if (spawnPointData.GetIsSpawnPointBlocked())
		{
			// Can't spawn there - put the selection back on the current pick
			m_wSpawnListBox.SetItemSelected(selected, false, false);
			RestoreSelection();
			ShowNotice(string.Format("%1 is blocked - there are enemies nearby.", spawnPointData.GetSpawnPointName()));
			PanMapToSpawnPoint(point);
			return;
		}

		// Changing the pick after confirming keeps the confirmation, now for the new point
		rm.m_SelectedSpawnPoint = spawnPointData;
		PanMapToSpawnPoint(point);
	}

	//------------------------------------------------------------------------------------------------
	//! Re-select the respawn manager's current pick (after refusing a blocked one)
	protected void RestoreSelection()
	{
		COA_RespawnManager rm = COA_RespawnManager.GetInstance();
		if (!rm || !rm.m_SelectedSpawnPoint)
			return;

		foreach (int i, COA_SpawnPointData data : m_aSpawnPoints)
		{
			if (data.GetSpawnPointId() == rm.m_SelectedSpawnPoint.GetSpawnPointId())
			{
				m_wSpawnListBox.SetItemSelected(i, true, false);
				return;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void PanMapToSpawnPoint(IEntity spawnpoint)
	{
		if (!spawnpoint)
			return;

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.IsOpen())
			return;

		vector worldPos = spawnpoint.GetOrigin();
		mapEntity.ZoomPanSmooth(0.5, worldPos[0], worldPos[2]);
	}

//------------------------------------------------------------------------------------------------
//	PANEL STATE
//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Header timer, tickets, status line and button for the current respawn state
	protected void UpdatePanel()
	{
		COA_RespawnManager rm = COA_RespawnManager.GetInstance();
		if (!rm || !m_Panel)
			return;

		int secondsLeft = (int)Math.Ceil(rm.m_fRespawnTimer);
		bool timerDone = secondsLeft <= 0;
		bool wave = rm.m_bCurrentWaveRespawn;
		bool confirmed = rm.m_RespawnConfirmed && rm.m_SelectedSpawnPoint != null;
		bool canSpectate = rm.CanSpectateWhileWaiting();

		// Header
		if (timerDone)
			m_Panel.SetTimer("Ready to deploy", "Now");
		else if (wave)
			m_Panel.SetTimer("Next wave in", SCR_FormatHelper.FormatTime(secondsLeft));
		else
			m_Panel.SetTimer("Respawn in", SCR_FormatHelper.FormatTime(secondsLeft));

		m_Panel.SetHeaderInfo(GetRespawnsText(rm));

		// Spectating is only offered while there's time for it (see COA_RespawnManager.CanSpectateWhileWaiting)
		m_Panel.SetSecondaryVisible(canSpectate);
		if (canSpectate != m_bLastCanSpectate)
		{
			m_bLastCanSpectate = canSpectate;
			UpdateHints();
		}

		// Count
		int open;
		foreach (COA_SpawnPointData data : m_aSpawnPoints)
		{
			if (!data.GetIsSpawnPointBlocked())
				open++;
		}

		string spawnName;
		if (rm.m_SelectedSpawnPoint)
			spawnName = rm.m_SelectedSpawnPoint.GetSpawnPointName();

		// Only rebuild the text when something it shows changed
		string stateKey = string.Format("%1|%2|%3|%4|%5|%6|%7|%8", timerDone, wave, confirmed, canSpectate, open, m_aSpawnPoints.Count(), spawnName, m_sNotice);
		if (stateKey == m_sLastStateKey)
			return;

		m_sLastStateKey = stateKey;

		if (m_aSpawnPoints.Count() == open)
			m_Panel.SetListTitle("Spawn points", COA_DeployPanel.Plural(open, "open", "open"));
		else
			m_Panel.SetListTitle("Spawn points", string.Format("%1 of %2 open", open, m_aSpawnPoints.Count()));

		if (m_aSpawnPoints.IsEmpty())
			m_Panel.SetEmptyText("No spawn points are open to your side right now.");
		else
			m_Panel.SetEmptyText(string.Empty);

		UpdateStatusAndButton(timerDone, wave, confirmed, canSpectate, open, spawnName);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateStatusAndButton(bool timerDone, bool wave, bool confirmed, bool canSpectate, int open, string spawnName)
	{
		string when = "when the timer ends";
		if (wave)
			when = "with the next wave";

		// The button: confirm / change before the timer ends, deploy after it
		if (open == 0 && !confirmed)
			m_Panel.SetPrimary("Confirm spawn", false);
		else if (spawnName.IsEmpty())
			m_Panel.SetPrimary("Confirm spawn", false);
		else if (timerDone && confirmed)
			m_Panel.SetPrimary("Deploying...", false);
		else if (timerDone)
			m_Panel.SetPrimary("Deploy now");
		else if (confirmed)
			m_Panel.SetPrimary("Change spawn");
		else
			m_Panel.SetPrimary("Confirm spawn");

		// The status line - a one-off notice wins
		if (!m_sNotice.IsEmpty())
		{
			m_Panel.SetStatus(m_sNotice, COA_EDeployTone.WARNING);
			return;
		}

		if (m_aSpawnPoints.IsEmpty())
		{
			m_Panel.SetStatus("Waiting for a spawn point to open.", COA_EDeployTone.WARNING);
		}
		else if (open == 0)
		{
			m_Panel.SetStatus("Every spawn point is blocked by enemies nearby. Waiting for one to clear.", COA_EDeployTone.WARNING);
		}
		else if (spawnName.IsEmpty())
		{
			m_Panel.SetStatus("Pick a spawn point from the list.");
		}
		else if (confirmed && !timerDone)
		{
			string text = string.Format("You'll deploy at %1 %2.", spawnName, when);
			if (canSpectate)
				text += " You can spectate until then.";

			m_Panel.SetStatus(text, COA_EDeployTone.GOOD);
		}
		else if (timerDone && !confirmed)
		{
			m_Panel.SetStatus(string.Format("Ready. Deploy at %1?", spawnName), COA_EDeployTone.GOOD);
		}
		else if (!timerDone)
		{
			string pickText = string.Format("Confirm %1 to deploy there %2.", spawnName, when);
			if (!wave)
				pickText += " You won't respawn until you confirm.";

			m_Panel.SetStatus(pickText);
		}
		else
		{
			m_Panel.SetStatus(string.Format("Deploying at %1...", spawnName), COA_EDeployTone.GOOD);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! "12 tickets left" (team pool) / "2 respawns left" (slot or squad pool) / "Unlimited respawns"
	protected string GetRespawnsText(COA_RespawnManager rm)
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode)
			return string.Empty;

		if (gamemode.m_eRespawnMode == COA_ERespawnMode.SLOT)
		{
			COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
			if (!slottingManager)
				return string.Empty;

			COA_SlotData slotData = slottingManager.GetPlayerSlotData(GetGame().GetPlayerController().GetPlayerId());
			if (!slotData)
				return string.Empty;

			int respawns = slotData.GetSlotRespawnsRemaining();
			if (respawns == -1)
				return "Unlimited respawns";

			return COA_DeployPanel.Plural(respawns, "respawn left", "respawns left");
		}

		int tickets = rm.GetFactionTickets(m_factionKey);
		if (tickets == -1)
			return "Unlimited respawns";

		return COA_DeployPanel.Plural(tickets, "ticket left", "tickets left");
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateHints()
	{
		string separator = "     ";
		string hints = "<action name='MenuBack'/> Menu" + separator + "<action name='ChatToggle'/> Chat";

		if (!CVON_VONGameModeComponent.GetInstance())
			hints += separator + "<action name='VONDirect'/> Talk";

		if (m_bLastCanSpectate)
			hints += separator + "<action name='COA_SpecRespawnMenu'/> Spectate";

		m_Panel.SetHints(hints);
	}

	//------------------------------------------------------------------------------------------------
	//! Replace the status line with a message for a few seconds
	protected void ShowNotice(string text)
	{
		m_sNotice = text;
		m_fNoticeTimeLeft = NOTICE_SECONDS;
	}

	//------------------------------------------------------------------------------------------------
	//! Before the timer ends: confirm / un-confirm the pick. After it: confirm, which the respawn
	//! manager acts on straight away.
	protected void OnPrimaryClicked()
	{
		COA_RespawnManager rm = COA_RespawnManager.GetInstance();
		if (!rm || !rm.m_SelectedSpawnPoint)
			return;

		if (rm.m_SelectedSpawnPoint.GetIsSpawnPointBlocked())
		{
			ShowNotice(string.Format("%1 is blocked - there are enemies nearby.", rm.m_SelectedSpawnPoint.GetSpawnPointName()));
			return;
		}

		m_sNotice = string.Empty;

		if (rm.m_fRespawnTimer <= 0)
			rm.m_RespawnConfirmed = true;
		else
			rm.m_RespawnConfirmed = !rm.m_RespawnConfirmed;
	}

//------------------------------------------------------------------------------------------------
//	SPECTATE
//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Leave the respawn menu for the spectator while the timer runs. The respawn manager brings the
	//! menu back when it's needed: when the timer ends with a confirmed spawn (respawn goes through at
	//! once), or COA_RespawnManager.SPECTATE_RETURN_SECONDS early when no spawn is confirmed yet.
	protected void OnSpectateClicked()
	{
		COA_RespawnManager rm = COA_RespawnManager.GetInstance();
		if (!rm || !rm.CanSpectateWhileWaiting())
			return;

		// Only when the spectator is underneath (it isn't for a moment right after death)
		if (!GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.COA_SpectatorMenu))
			return;

		rm.SetSpectateWhileWaiting(true);
		Close();
	}

	//------------------------------------------------------------------------------------------------
	//! COA_SpecRespawnMenu key while this menu is on top
	protected void Action_Spectate()
	{
		if (GetGame().GetMenuManager().GetTopMenu() != this)
			return;

		COA_RespawnManager rm = COA_RespawnManager.GetInstance();
		if (!rm || !rm.CanSpectateWhileWaiting() || !rm.ConsumeRespawnMenuToggle())
			return;

		OnSpectateClicked();
	}

//------------------------------------------------------------------------------------------------
//	INPUT, MAP, CHAT, VOICE
//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void RegisterInputHandlers()
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			inputManager.AddActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
			inputManager.AddActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		}
		inputManager.AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		inputManager.AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnCOA_ChatToggleAction);
		inputManager.AddActionListener("COA_SpecRespawnMenu", EActionTrigger.DOWN, Action_Spectate);
	}

	//------------------------------------------------------------------------------------------------
	protected void UnregisterInputHandlers()
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			inputManager.RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
			inputManager.RemoveActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		}
		inputManager.RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		inputManager.RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnCOA_ChatToggleAction);
		inputManager.RemoveActionListener("COA_SpecRespawnMenu", EActionTrigger.DOWN, Action_Spectate);
	}

	//------------------------------------------------------------------------------------------------
	protected void OpenMapWithConfig()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode || !m_MapEntity)
			return;

		SCR_MapConfigComponent configComp = SCR_MapConfigComponent.Cast(gameMode.FindComponent(SCR_MapConfigComponent));
		if (!configComp)
			return;

		MapConfiguration mapConfigFullscreen = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, "{1B8AC767E06A0ACD}Configs/Map/MapFullscreen.conf", GetRootWidget());
		m_MapEntity.OpenMap(mapConfigFullscreen);

		// Apply zoom out after short delay to ensure map is ready
		GetGame().GetCallqueue().CallLater(AdjustMapZoom, 100);
	}

	//------------------------------------------------------------------------------------------------
	//! Zoom out once the map is open, then centre on the current pick
	protected void AdjustMapZoom()
	{
		if (!m_MapEntity)
			return;

		m_MapEntity.ZoomOut();

		COA_RespawnManager rm = COA_RespawnManager.GetInstance();
		if (rm && rm.m_SelectedSpawnPoint)
			PanMapToSpawnPoint(COA_EntityHelper.GetEntityFromRplId(rm.m_SelectedSpawnPoint.GetSpawnPointEntity()));
	}

	//------------------------------------------------------------------------------------------------
	protected void InitializeChatPanel()
	{
		Widget wChatPanel = GetRootWidget().FindAnyWidget("ChatPanel");
		if (wChatPanel)
			m_ChatPanel = SCR_ChatPanel.Cast(wChatPanel.FindHandler(SCR_ChatPanel));
	}

	//------------------------------------------------------------------------------------------------
	void Action_OnCOA_ChatToggleAction()
	{
		if (!m_ChatPanel)
			return;

		// Short delay to prevent conflicts with other UI actions
		GetGame().GetCallqueue().CallLater(OpenChatPanel, 5);
	}

	//------------------------------------------------------------------------------------------------
	protected void OpenChatPanel()
	{
		if (!m_ChatPanel || m_ChatPanel.IsOpen())
			return;

		SCR_ChatPanelManager chatManager = SCR_ChatPanelManager.GetInstance();
		if (chatManager)
			chatManager.OpenChatPanel(m_ChatPanel);
	}

	//------------------------------------------------------------------------------------------------
	//! Back opens the pause menu rather than leaving, so the respawn isn't lost by accident
	void Action_Exit()
	{
		GetGame().GetCallqueue().CallLater(OpenPauseMenu, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void OpenPauseMenu()
	{
		ArmaReforgerScripted.OpenPauseMenu();
	}

	//------------------------------------------------------------------------------------------------
	//! Push-to-talk on the squad radio while dead (when CVON isn't running)
	void Action_VONon()
	{
		GetGame().GetCallqueue().Remove(LobbyVoNDisableDelayed);

		IEntity controlled = GetGame().GetPlayerController().GetControlledEntity();
		if (!controlled)
			return;

		SCR_VoNComponent von = SCR_VoNComponent.Cast(controlled.FindComponent(SCR_VoNComponent));
		if (!von)
			return;

		von.SetTransmitRadio(GetVoNTransiver());
		von.SetCommMethod(ECommMethod.SQUAD_RADIO);
		von.SetCapture(true);
	}

	//------------------------------------------------------------------------------------------------
	//! The player's radio transceiver for voice communication
	RadioTransceiver GetVoNTransiver()
	{
		IEntity entity = GetGame().GetPlayerController().GetControlledEntity();
		if (!entity)
			return null;

		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventoryManager)
			return null;

		array<IEntity> items = {};
		inventoryManager.GetItems(items);

		IEntity radioEntity;
		foreach (IEntity item : items)
		{
			if (item.FindComponent(BaseRadioComponent))
			{
				radioEntity = item;
				break;
			}
		}

		if (!radioEntity)
			return null;

		BaseRadioComponent radio = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		radio.SetPower(true);

		RadioTransceiver transceiver = RadioTransceiver.Cast(radio.GetTransceiver(0));
		if (transceiver)
			transceiver.SetFrequency(10000);

		return transceiver;
	}

	//------------------------------------------------------------------------------------------------
	void Action_VONOff()
	{
		// Slight delay to avoid cutting off voice too abruptly
		GetGame().GetCallqueue().CallLater(LobbyVoNDisableDelayed, 400);
	}

	//------------------------------------------------------------------------------------------------
	void LobbyVoNDisableDelayed()
	{
		IEntity controlled = GetGame().GetPlayerController().GetControlledEntity();
		if (!controlled)
			return;

		SCR_VoNComponent von = SCR_VoNComponent.Cast(controlled.FindComponent(SCR_VoNComponent));
		if (!von)
			return;

		von.SetCommMethod(ECommMethod.DIRECT);
		von.SetCapture(false);
	}
}
