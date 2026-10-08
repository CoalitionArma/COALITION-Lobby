//------------------------------------------------------------------------------------------------
// Builds the auto-generated "SIGNAL" briefing entry (COA_MissionDescriptionUI) once the round is
// past slotting: the player's element and who fills it, each friendly element's leader, the radio
// nets each element is assigned, and the radios the player is carrying.
//
// Nets follow the same rules COA_CVON_GroupsManagerComponent.TuneFreqDelayWithPresets uses to tune
// radios on spawn (faction overrides, then the CVON preset config, matched on the group's squad
// callsign), so they read correctly for elements whose players aren't streamed in. Players can
// retune afterwards - YOUR RADIOS shows what the local player is actually on.
//------------------------------------------------------------------------------------------------
class COA_SignalBriefing
{
	static const string TITLE = "SIGNAL";

	//------------------------------------------------------------------------------------------------
	//! Whether there is anything to brief: the round is past slotting and the player has a slot
	static bool IsAvailable(COA_Gamemode gamemode)
	{
		if (!gamemode || gamemode.m_GamemodeState < COA_EGamemodeState.GAME)
			return false;

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		return slottingManager && slottingManager.GetPlayerSlotGroup(SCR_PlayerController.GetLocalPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	static string BuildText()
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return string.Empty;

		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		SCR_AIGroup myGroup = slottingManager.GetPlayerSlotGroup(localPlayerId);
		Faction myFaction = slottingManager.GetPlayerSlotFaction(localPlayerId, true);
		if (!myGroup || !myFaction)
			return string.Empty;

		array<string> lines = {};
		AppendElementSection(lines, slottingManager, myGroup, localPlayerId);
		AppendCommandSection(lines, slottingManager, myFaction.GetFactionKey(), myGroup);

		if (CVON_VONGameModeComponent.GetInstance())
		{
			AppendNetsSection(lines, slottingManager, SCR_Faction.Cast(myFaction), myGroup);

			lines.Insert("YOUR RADIOS");
			array<string> radioLines = {};
			GetRadioLines(SCR_PlayerController.GetLocalControlledEntity(), radioLines);
			if (radioLines.IsEmpty())
				lines.Insert("- None carried");
			foreach (string radioLine : radioLines)
				lines.Insert("- " + radioLine);
		}

		return SCR_StringHelper.Join("\n", lines);
	}

	//------------------------------------------------------------------------------------------------
	//! "AN/PRC-152: CH 1 - 41000" per CVON radio the character carries (also used by /freq)
	static void GetRadioLines(IEntity character, notnull array<string> outLines)
	{
		outLines.Clear();

		CVON_VONGameModeComponent cvon = CVON_VONGameModeComponent.GetInstance();
		if (!character || !cvon || COA_EntityHelper.IsSpectator(character))
			return;

		array<IEntity> radios = cvon.GetRadioEntities(character);
		if (!radios)
			return;

		foreach (IEntity radio : radios)
		{
			CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radio.FindComponent(CVON_RadioComponent));
			if (!radioComp)
				continue;

			string power;
			if (!radioComp.m_bPower)
				power = " (off)";

			outLines.Insert(string.Format("%1: CH %2 - %3%4", radioComp.m_sRadioName, radioComp.m_iCurrentChannel, radioComp.m_sFrequency, power));
		}
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 SECTIONS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! The player's own group, one line per filled slot in slot order
	protected static void AppendElementSection(notnull array<string> lines, COA_SlottingManager slottingManager, SCR_AIGroup group, int localPlayerId)
	{
		lines.Insert(string.Format("YOUR ELEMENT: %1", group.GetCustomNameWithOriginal()));

		RplId groupId;
		if (COA_ReplicationHelper.GetRplId(group, groupId))
		{
			PlayerManager playerManager = GetGame().GetPlayerManager();
			foreach (int slotId : slottingManager.GetAllSlotIDsForGroup(groupId))
			{
				COA_SlotData slotData = slottingManager.GetSlotData(slotId);
				if (!slotData || slotData.GetSlotCurrentPlayerId() <= 0)
					continue;

				string playerName = playerManager.GetPlayerName(slotData.GetSlotCurrentPlayerId());
				if (slotData.GetSlotCurrentPlayerId() == localPlayerId)
					playerName += " (you)";
				if (slotData.GetIsDeadSlot())
					playerName += " (KIA)";

				lines.Insert(string.Format("- %1: %2", slotData.GetSlotName(), playerName));
			}
		}

		lines.Insert("");
	}

	//------------------------------------------------------------------------------------------------
	//! Who leads every other friendly element
	protected static void AppendCommandSection(notnull array<string> lines, COA_SlottingManager slottingManager, FactionKey factionKey, SCR_AIGroup myGroup)
	{
		array<SCR_AIGroup> groups = slottingManager.GetAllGroups(factionKey);
		if (!groups)
			return;

		array<string> commandLines = {};
		foreach (SCR_AIGroup group : groups)
		{
			if (!group || group == myGroup)
				continue;

			COA_SlotData leaderSlot = FindLeaderSlot(slottingManager, group);
			if (!leaderSlot)
				continue;

			string leaderName = GetGame().GetPlayerManager().GetPlayerName(leaderSlot.GetSlotCurrentPlayerId());
			commandLines.Insert(string.Format("- %1: %2 (%3)", group.GetCustomNameWithOriginal(), leaderName, leaderSlot.GetSlotName()));
		}

		if (commandLines.IsEmpty())
			return;

		lines.Insert("COMMAND");
		lines.InsertAll(commandLines);
		lines.Insert("");
	}

	//------------------------------------------------------------------------------------------------
	//! Assigned short/long range net per friendly element, the player's own first
	protected static void AppendNetsSection(notnull array<string> lines, COA_SlottingManager slottingManager, SCR_Faction faction, SCR_AIGroup myGroup)
	{
		if (!faction)
			return;

		array<SCR_AIGroup> groups = slottingManager.GetAllGroups(faction.GetFactionKey());
		if (!groups)
			return;

		lines.Insert("NETS");
		lines.Insert(BuildNetLine(faction, myGroup, true));

		foreach (SCR_AIGroup group : groups)
		{
			if (group && group != myGroup && FindLeaderSlot(slottingManager, group))
				lines.Insert(BuildNetLine(faction, group, false));
		}

		lines.Insert("");
	}

	//------------------------------------------------------------------------------------------------
	protected static string BuildNetLine(SCR_Faction faction, SCR_AIGroup group, bool isMine)
	{
		string shortRange, longRange;
		GetAssignedNets(faction, group, shortRange, longRange);

		string line = string.Format("- %1: SR %2", group.GetCustomNameWithOriginal(), shortRange);
		if (!longRange.IsEmpty())
			line += string.Format("  /  LR %1", longRange);
		if (isMine)
			line += "  (yours)";

		return line;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 HELPERS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! First SR and LR frequency CVON assigns to members of this group on spawn
	protected static void GetAssignedNets(SCR_Faction faction, SCR_AIGroup group, out string shortRange, out string longRange)
	{
		string company, platoon, squad, character, format;
		group.GetCallsigns(company, platoon, squad, character, format);

		CVON_GroupFrequencyContainer container;
		if (faction.GetCallsignInfo())
			container = FindFrequencyContainer(faction.GetCallsignInfo().m_aGroupFrequencyOverrides, squad);

		CVON_VONGameModeComponent cvon = CVON_VONGameModeComponent.GetInstance();
		if (!container && cvon && cvon.m_FreqConfig)
			container = FindFrequencyContainer(cvon.m_FreqConfig.m_aPresetGroupFrequencyContainers, squad);

		// Same fallbacks as the radio tuning: the callsign itself, and the faction's first LR channel
		shortRange = squad;
		if (container && container.m_aSRFrequencies && !container.m_aSRFrequencies.IsEmpty())
			shortRange = container.m_aSRFrequencies[0];

		if (container && container.m_aLRFrequencies && !container.m_aLRFrequencies.IsEmpty())
		{
			longRange = container.m_aLRFrequencies[0];
			return;
		}

		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return;

		array<string> lrChannels = factionManager.GetFactionActiveChannelLR(faction.GetFactionKey());
		if (lrChannels && !lrChannels.IsEmpty())
			longRange = lrChannels[0];
	}

	//------------------------------------------------------------------------------------------------
	protected static CVON_GroupFrequencyContainer FindFrequencyContainer(array<ref CVON_GroupFrequencyContainer> containers, string squadCallsign)
	{
		if (!containers)
			return null;

		string normalizedSquad = SCR_Faction.NormalizeCallsign(squadCallsign);
		foreach (CVON_GroupFrequencyContainer container : containers)
		{
			if (!container || !container.m_aGroupNames)
				continue;

			foreach (string groupName : container.m_aGroupNames)
			{
				string normalizedName = SCR_Faction.NormalizeCallsign(groupName);
				if (normalizedSquad.Contains(normalizedName) || normalizedName.Contains(normalizedSquad))
					return container;
			}
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Filled squad lead slot, else team lead, else the first filled slot; null if the group is empty
	protected static COA_SlotData FindLeaderSlot(COA_SlottingManager slottingManager, SCR_AIGroup group)
	{
		RplId groupId;
		if (!COA_ReplicationHelper.GetRplId(group, groupId))
			return null;

		COA_SlotData squadLead, teamLead, anyone;
		foreach (int slotId : slottingManager.GetAllSlotIDsForGroup(groupId))
		{
			COA_SlotData slotData = slottingManager.GetSlotData(slotId);
			if (!slotData || slotData.GetSlotCurrentPlayerId() <= 0)
				continue;

			COA_ESlotType slotType = slotData.GetSlotType();
			if (slotType == COA_ESlotType.SQUAD_LEADER && !squadLead)
				squadLead = slotData;
			else if (slotType == COA_ESlotType.TEAM_LEADER && !teamLead)
				teamLead = slotData;
			else if (!anyone)
				anyone = slotData;
		}

		if (squadLead)
			return squadLead;
		if (teamLead)
			return teamLead;
		return anyone;
	}
}
