//------------------------------------------------------------------------------------------------
//! Results an admin can end a mission with (admin menu > Game Mode > End with result): a winning
//! faction key or a draw, plus an optional short result line ("Survivors extracted").
//! Shared by the admin menu, the server handler (COA_PlayerRplToAuthorityManager.RpcAsk_RequestEndMission),
//! the outro and CRF's logging/AAR so they all validate and word a result the same way.
class COA_MissionResultHelper
{
	//! Outcome key for a draw. CRF_LoggingManager writes it as the side ("mission_winner,DRAW,..."),
	//! which is what the bot's isDrawResult() checks for. No player gets the winner flag/XP bonus.
	static const string DRAW = "DRAW";

	//! The result line is shown in the outro title and the AAR header, and written to the mission log
	static const int MAX_NOTE_LENGTH = 40;

	//------------------------------------------------------------------------------------------------
	//! \return true for the faction keys CRF_LoggingManager records, or DRAW
	static bool IsValidOutcome(string outcome)
	{
		return outcome == "BLUFOR" || outcome == "OPFOR" || outcome == "INDFOR" || outcome == "CIV" || outcome == DRAW;
	}

	//------------------------------------------------------------------------------------------------
	//! \return "BLUFOR VICTORY", "DRAW"..., or empty for no/unknown outcome
	static string GetOutcomeText(string outcome)
	{
		switch (outcome)
		{
			case "BLUFOR": return "BLUFOR VICTORY";
			case "OPFOR": return "OPFOR VICTORY";
			case "INDFOR": return "INDFOR VICTORY";
			case "CIV": return "CIVILIAN VICTORY";
			case "DRAW": return "DRAW";
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! Outcome text with the result line appended: "BLUFOR VICTORY - Survivors extracted"
	static string GetResultLine(string outcome, string note)
	{
		string text = GetOutcomeText(outcome);
		if (note.IsEmpty())
			return text;

		if (text.IsEmpty())
			return note;

		return text + " - " + note;
	}

	//------------------------------------------------------------------------------------------------
	//! One trimmed line without commas (the mission log is comma separated), capped at MAX_NOTE_LENGTH
	static string SanitizeNote(string note)
	{
		note.Replace("\r", " ");
		note.Replace("\n", " ");
		note.Replace("\t", " ");
		note.Replace(",", ";");
		note = note.Trim();

		if (note.Length() > MAX_NOTE_LENGTH)
			note = note.Substring(0, MAX_NOTE_LENGTH).Trim();

		return note;
	}
}
