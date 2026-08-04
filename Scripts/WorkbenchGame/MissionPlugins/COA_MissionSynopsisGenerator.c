#ifdef WORKBENCH
//------------------------------------------------------------------------------------------------
//! Callsign-inferred ORBAT tier for the mission synopsis' Slotting section (see GetCallsignTier()).
//! Internal to the synopsis generator - not a runtime/gameplay concept, so it's kept out of COA_Enums.c.
enum COA_ESlottingCallsignTier
{
	COMPANY,
	PLATOON,
	SQUAD,
	OTHER   // Unrecognized/custom callsign - always rendered top-level, un-nested
}

//------------------------------------------------------------------------------------------------
//! Builds the markdown mission synopsis a mission maker pastes into their PR description, from the
//! mission's actual configured data.
//!
//! This is a plain script class rather than part of COA_MissionConfigurationPlugin so that mods
//! depending on the lobby can extend it - Workbench instantiates plugins from the class carrying
//! WorkbenchPluginAttribute, so a `modded class` of the plugin itself is never instantiated, while
//! a `modded class` of this one works normally. CRF extends GetActiveGameModeComponents() that way.
class COA_MissionSynopsisGenerator
{
	//! Set by the plugin before building - the rest is read off the mission itself.
	string m_sMissionAuthor;
	string m_sMissionDescription;

	//------------------------------------------------------------------------------------------------
	//! Builds the synopsis, copies it to the clipboard, and opens the read-only preview dialog.
	//! Nothing is written to disk - a file here would get committed and bundled into the mod itself,
	//! which we don't want. The mission maker pastes this into the PR description instead.
	void ShowMissionSynopsis(COA_Gamemode gamemode, IEntitySource entitySource, string missionDisplayName, string missionTerrain, string missionMode, int missionPlayercount)
	{
		array<string> synopsisLines = BuildMissionSynopsis(gamemode, entitySource, missionDisplayName, missionTerrain, missionMode, missionPlayercount);
		string synopsisText = SCR_StringHelper.Join("\n", synopsisLines);

		System.ExportToClipboard(synopsisText);

		COA_MissionSynopsisDialog synopsisDialog = new COA_MissionSynopsisDialog();
		synopsisDialog.m_sSynopsis = synopsisText;
		Workbench.ScriptDialog(
			"Mission Synopsis (Copied to Clipboard)",
			"This synopsis has already been copied to your clipboard - create your PR and paste it into your PR description now. \n\n You may open the synopsis again via the 'Show Mission Synopsis' button in the Workbench plugin, without regenerating the mission config.",
			synopsisDialog);
	}

	//------------------------------------------------------------------------------------------------
	//! Builds the full markdown mission synopsis from the mission's actual configured data.
	array<string> BuildMissionSynopsis(COA_Gamemode gamemode, IEntitySource entitySource, string missionDisplayName, string missionTerrain, string missionMode, int missionPlayercount)
	{
		array<string> lines = {};

		if (!gamemode)
			return lines;

		// Markers so Coalition_Bot can find this block wherever it's pasted in the PR description,
		// without needing a committed file (this text never touches disk/the mod itself).
		// Coalition_Bot matches these exact strings - do not rename them.
		lines.Insert("<!-- CRF_SYNOPSIS_START -->");
		lines.Insert(string.Format("# %1", missionDisplayName));
		lines.Insert("");
		lines.Insert(string.Format("**Author:** %1  **Terrain:** %2  **Game Mode:** %3  **Player Count:** %4", m_sMissionAuthor, missionTerrain, missionMode, missionPlayercount));
		lines.Insert("");

		if (!m_sMissionDescription.IsEmpty() && m_sMissionDescription != "<Description>")
		{
			lines.Insert(m_sMissionDescription);
			lines.Insert("");
		}

		AppendGeneralSection(lines, gamemode);
		AppendRespawnSection(lines, gamemode);
		AppendGearscriptSection(lines, gamemode);
		AppendGameModeComponentSection(lines, gamemode, entitySource, missionMode);
		AppendWeatherSection(lines, gamemode);
		AppendSlottingSection(lines, gamemode);

		lines.Insert("<!-- CRF_SYNOPSIS_END -->");

		return lines;
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendGeneralSection(array<string> lines, COA_Gamemode gamemode)
	{
		lines.Insert("## General");

		if (!gamemode.m_sFactionOneKey.IsEmpty() && !gamemode.m_sFactionTwoKey.IsEmpty())
			lines.Insert(string.Format("- Slotting Ratio: %1 %2:%3 %4", gamemode.m_sFactionOneKey, gamemode.m_iFactionOneRatio, gamemode.m_iFactionTwoRatio, gamemode.m_sFactionTwoKey));

		string timeLimit = "No limit";
		if (gamemode.m_iTimeLimitMinutes > 0)
			timeLimit = string.Format("%1 minutes", gamemode.m_iTimeLimitMinutes);
		lines.Insert(string.Format("- Time Limit: %1", timeLimit));

		lines.Insert(string.Format("- Disable JIP: %1", BoolToYesNo(gamemode.m_bLockUnusedSlots)));

		string safestartLimit = "Disabled";
		if (gamemode.m_bUseSafestartTimeLimit)
			safestartLimit = string.Format("%1 minutes", gamemode.m_iSafestartTimeLimit);
		lines.Insert(string.Format("- Safestart Time Limit: %1", safestartLimit));

		lines.Insert(string.Format("- Mission Time Scale: %1x", gamemode.m_fMissionTimeScale));
		lines.Insert(string.Format("- Coalition VON (CVON): %1", BoolToYesNo(gamemode.m_bUseCVON)));
		lines.Insert("");
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendRespawnSection(array<string> lines, COA_Gamemode gamemode)
	{
		lines.Insert("## Respawn");
		lines.Insert(string.Format("- Respawn Enabled: %1", BoolToYesNo(gamemode.m_bRespawnEnabled)));

		if (!gamemode.m_bRespawnEnabled)
		{
			lines.Insert("");
			return;
		}

		bool slotBased = (gamemode.m_eRespawnMode == COA_ERespawnMode.SLOT);

		string respawnModeName = "Team-Based";
		if (slotBased)
			respawnModeName = "Slot-Based";
		lines.Insert(string.Format("- Mode: %1", respawnModeName));

		if (!slotBased)
		{
			if (!gamemode.GetSlots("BLUFOR").IsEmpty())
				lines.Insert(string.Format("- BLUFOR Tickets: %1", TicketToString(gamemode.m_iBLUFORTickets)));
			if (!gamemode.GetSlots("OPFOR").IsEmpty())
				lines.Insert(string.Format("- OPFOR Tickets: %1", TicketToString(gamemode.m_iOPFORTickets)));
			if (!gamemode.GetSlots("INDFOR").IsEmpty())
				lines.Insert(string.Format("- INDFOR Tickets: %1", TicketToString(gamemode.m_iINDFORTickets)));
			if (!gamemode.GetSlots("CIV").IsEmpty())
				lines.Insert(string.Format("- CIV Tickets: %1", TicketToString(gamemode.m_iCIVTickets)));
		}
		else
		{
			lines.Insert("- Per-squad respawn pools are listed under Slotting below.");
		}

		lines.Insert(string.Format("- Wave Respawn: %1", BoolToYesNo(gamemode.m_bWaveRespawn)));
		lines.Insert(string.Format("- Time To Respawn: %1s", gamemode.m_iTimeToRespawn));

		string cutoff = "Never disables";
		if (gamemode.m_iRespawnCutoffMinutes > 0)
			cutoff = string.Format("Disables %1 minutes before mission end", gamemode.m_iRespawnCutoffMinutes);
		lines.Insert(string.Format("- Respawn Cutoff: %1", cutoff));

		lines.Insert(string.Format("- Rally Points Enabled: %1", BoolToYesNo(gamemode.m_bRallyPointsEnabled)));
		lines.Insert("");
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendGearscriptSection(array<string> lines, COA_Gamemode gamemode)
	{
		lines.Insert("## Gearscripts");

		if (!gamemode.GetSlots("BLUFOR").IsEmpty())
			lines.Insert(string.Format("- BLUFOR: %1", GearScriptToString(gamemode.GetGearScriptSettings("BLUFOR"))));
		if (!gamemode.GetSlots("OPFOR").IsEmpty())
			lines.Insert(string.Format("- OPFOR: %1", GearScriptToString(gamemode.GetGearScriptSettings("OPFOR"))));
		if (!gamemode.GetSlots("INDFOR").IsEmpty())
			lines.Insert(string.Format("- INDFOR: %1", GearScriptToString(gamemode.GetGearScriptSettings("INDFOR"))));
		if (!gamemode.GetSlots("CIV").IsEmpty())
			lines.Insert(string.Format("- CIV: %1", GearScriptToString(gamemode.GetGearScriptSettings("CIV"))));

		lines.Insert("");
	}

	//------------------------------------------------------------------------------------------------
	protected string GearScriptToString(COA_GearScriptContainer gearScriptSettings)
	{
		if (!gearScriptSettings || gearScriptSettings.m_rGearScript.IsEmpty())
			return "None";

		return string.Format("%1", gearScriptSettings.m_rGearScript);
	}

	//------------------------------------------------------------------------------------------------
	protected string TicketToString(int tickets)
	{
		if (tickets == -1)
			return "Unlimited";

		return tickets.ToString();
	}

	//------------------------------------------------------------------------------------------------
	protected string BoolToYesNo(bool value)
	{
		if (value)
			return "Yes";

		return "No";
	}

	//------------------------------------------------------------------------------------------------
	//! Lists the gamemode logic components attached to COA_Lobby, on top of the single authored
	//! COA_EGamemode label. A mission can stack more than one (e.g. Rally + Attrition).
	protected void AppendGameModeComponentSection(array<string> lines, COA_Gamemode gamemode, IEntitySource entitySource, string missionMode)
	{
		lines.Insert("## Game Mode Components");
		lines.Insert(string.Format("- Selected Mode: %1", missionMode));

		array<string> activeComponents = {};
		GetActiveGameModeComponents(entitySource, activeComponents);

		string componentList = "None detected";
		if (!activeComponents.IsEmpty())
		{
			componentList = activeComponents.Get(0);
			for (int i = 1; i < activeComponents.Count(); i++)
				componentList = componentList + ", " + activeComponents.Get(i);
		}

		lines.Insert(string.Format("- Active Components: %1", componentList));
		lines.Insert("");
	}

	//------------------------------------------------------------------------------------------------
	//! Extension point: the lobby ships no gamemode logic components of its own, so mods that add
	//! them override this and insert a display name per component found on entitySource. See
	//! CRF_COA_MissionSynopsisGenerator.c in the Coalition Reforger Framework for an example.
	protected void GetActiveGameModeComponents(IEntitySource entitySource, notnull array<string> activeComponents)
	{
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendWeatherSection(array<string> lines, COA_Gamemode gamemode)
	{
		lines.Insert("## Weather");

		SCR_TimeAndWeatherHandlerComponent timeAndWeatherComp = SCR_TimeAndWeatherHandlerComponent.Cast(gamemode.FindComponent(SCR_TimeAndWeatherHandlerComponent));
		if (!timeAndWeatherComp || timeAndWeatherComp.GetStartingWeatherAndTime().IsEmpty())
		{
			lines.Insert("- Not configured");
			lines.Insert("");
			return;
		}

		string startingWeather = timeAndWeatherComp.GetStartingWeatherAndTime()[0].GetWeatherPresetName();
		int startingHour = timeAndWeatherComp.GetStartingWeatherAndTime()[0].GetStartingHour();
		int startingMinutes = timeAndWeatherComp.GetStartingWeatherAndTime()[0].GetStartingMinutes();

		lines.Insert(string.Format("- Starting Weather: %1", startingWeather));
		lines.Insert(string.Format("- Starting Time: %1:%2", startingHour.ToString(), startingMinutes.ToString()));
		lines.Insert(string.Format("- Random Starting Weather: %1", BoolToYesNo(timeAndWeatherComp.GetRandomStartingWeather())));
		lines.Insert(string.Format("- Random Weather Changes: %1", BoolToYesNo(timeAndWeatherComp.GetRandomWeatherChanges())));
		lines.Insert("");
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendSlottingSection(array<string> lines, COA_Gamemode gamemode)
	{
		//--- Load the global roles config directly as a resource (not via COA_GearscriptManager.GetRolesConfig(),
		//--- which depends on a live runtime manager singleton that isn't guaranteed to exist in Workbench)
		COA_RolesConfig rolesConfig = COA_RolesConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer("{4388548E9F600148}Configs/Gearscripts/COA_Global_Roles_Config.conf").GetResource().ToBaseContainer()));

		AppendFactionSlotting(lines, "BLUFOR", gamemode.GetSlots("BLUFOR"), gamemode.m_eRespawnMode, rolesConfig);
		AppendFactionSlotting(lines, "OPFOR", gamemode.GetSlots("OPFOR"), gamemode.m_eRespawnMode, rolesConfig);
		AppendFactionSlotting(lines, "INDFOR", gamemode.GetSlots("INDFOR"), gamemode.m_eRespawnMode, rolesConfig);
		AppendFactionSlotting(lines, "CIV", gamemode.GetSlots("CIV"), gamemode.m_eRespawnMode, rolesConfig);
	}

	//------------------------------------------------------------------------------------------------
	//! Classifies a squad/platoon/company's tier from its callsign, using the same COY / NPLT / N-M
	//! naming convention the Quick Slot Setup tool's auto-numbering already produces (SetPluginQuickSlots
	//! in COA_MissionSlottingPlugin.c). Custom-renamed callsigns that don't match any pattern fall back
	//! to COA_ESlottingCallsignTier.OTHER, which is always rendered top-level/un-nested.
	protected COA_ESlottingCallsignTier GetCallsignTier(string callsign)
	{
		string upper = callsign;
		upper.ToUpper();

		if (upper == "COY")
			return COA_ESlottingCallsignTier.COMPANY;

		if (upper.IndexOf("PLT") != -1)
			return COA_ESlottingCallsignTier.PLATOON;

		if (upper.IndexOf("-") != -1)
			return COA_ESlottingCallsignTier.SQUAD;

		return COA_ESlottingCallsignTier.OTHER;
	}

	//------------------------------------------------------------------------------------------------
	//! Renders a faction's slotting groups nested by tier (Company > Platoon > Squad) inferred from
	//! callsign, using array order to track which company/platoon is "current" as we walk the list -
	//! matching how the Quick Slot Setup tool orders company/platoon/squad entries.
	protected void AppendFactionSlotting(array<string> lines, string factionName, array<ref COA_SlottingGroup> factionSlots, COA_ERespawnMode respawnMode, COA_RolesConfig rolesConfig)
	{
		if (factionSlots.IsEmpty())
			return;

		bool slotBased = (respawnMode == COA_ERespawnMode.SLOT);

		lines.Insert(string.Format("## Slotting - %1 (%2 slots)", factionName, GetPlayerCount(factionSlots)));

		bool hasActiveCompany = false;
		bool hasActivePlatoon = false;

		foreach (ref COA_SlottingGroup slotGroup : factionSlots)
		{
			COA_ESlottingCallsignTier tier = GetCallsignTier(slotGroup.m_sCallsign);
			int depth = 0;

			switch (tier)
			{
				case COA_ESlottingCallsignTier.COMPANY:
					depth = 0;
					hasActiveCompany = true;
					hasActivePlatoon = false;
					break;

				case COA_ESlottingCallsignTier.PLATOON:
					if (hasActiveCompany)
						depth = 1;
					hasActivePlatoon = true;
					break;

				case COA_ESlottingCallsignTier.SQUAD:
					if (hasActivePlatoon)
					{
						depth = 1;
						if (hasActiveCompany)
							depth = 2;
					}
					break;

				// COA_ESlottingCallsignTier.OTHER: unrecognized/custom callsign, always top-level -
				// doesn't reset hasActiveCompany/hasActivePlatoon, so it doesn't break the chain
				// (e.g. a standalone vehicle crew group listed between two squads of the same platoon).
			}

			AppendSlottingGroup(lines, slotGroup, depth, slotBased, rolesConfig);
		}

		lines.Insert("");
	}

	//------------------------------------------------------------------------------------------------
	//! Prints one squad/platoon/company as a nested bullet list entry at the given indent depth.
	//! Uses indented "-" bullets (not heading levels) throughout so the hierarchy renders correctly
	//! both on GitHub (PR file diff) and in Discord (which only supports up to H3 headings).
	protected void AppendSlottingGroup(array<string> lines, COA_SlottingGroup slotGroup, int depth, bool slotBased, COA_RolesConfig rolesConfig)
	{
		string indent = "";
		for (int i = 0; i < depth; i++)
			indent = indent + "  ";
		string roleIndent = indent + "  ";

		string flagTypeName = SCR_Enum.GetEnumName(COA_EFlagType, slotGroup.m_FlagType);
		lines.Insert(string.Format("%1- **%2** (%3)", indent, slotGroup.m_sCallsign, flagTypeName));

		if (slotBased && slotGroup.m_eRespawnPoolType == COA_ERespawnPoolType.PER_GROUP)
			lines.Insert(string.Format("%1- Respawn Pool (shared): %2", roleIndent, TicketToString(slotGroup.m_iGroupRespawns)));

		// Tally role counts within this squad
		map<COA_EGearRole, int> roleTally = new map<COA_EGearRole, int>();
		array<COA_EGearRole> roleOrder = {};
		foreach (COA_EGearRole role : slotGroup.m_aSlots)
		{
			if (!roleTally.Contains(role))
			{
				roleTally.Set(role, 0);
				roleOrder.Insert(role);
			}

			roleTally.Set(role, roleTally.Get(role) + 1);
		}

		foreach (COA_EGearRole role : roleOrder)
		{
			string roleName = SCR_Enum.GetEnumName(COA_EGearRole, role);
			if (rolesConfig)
			{
				COA_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
				if (roleConfig && !roleConfig.m_sRoleName.IsEmpty())
					roleName = roleConfig.m_sRoleName;
			}

			string roleLine = string.Format("%1- %2 x%3", roleIndent, roleName, roleTally.Get(role));

			if (slotBased && slotGroup.m_eRespawnPoolType == COA_ERespawnPoolType.PER_SLOT)
				roleLine = roleLine + string.Format(" (%1 respawns each)", TicketToString(slotGroup.GetRoleRespawnCount(role)));

			lines.Insert(roleLine);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected int GetPlayerCount(array<ref COA_SlottingGroup> factionSlots)
	{
		int missionPlayercount;

		foreach (ref COA_SlottingGroup slotGroup : factionSlots)
			foreach (COA_EGearRole role : slotGroup.m_aSlots)
				missionPlayercount++;

		return missionPlayercount;
	}
}

//------------------------------------------------------------------------------------------------
//! Read-only preview window for the generated mission synopsis. The text is already on the
//! clipboard by the time this opens - this just lets the mission maker see/re-select it.
class COA_MissionSynopsisDialog
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBoxMultiline, desc: "Mission synopsis - already copied to clipboard. Paste this into your PR description.")]
	string m_sSynopsis;

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Close", true)]
	protected bool ButtonClose()
	{
		return true;
	}
}
#endif
