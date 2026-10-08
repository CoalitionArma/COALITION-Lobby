//------------------------------------------------------------------------------------------------
//! Shown to a player for a few seconds when they die: who killed them, with what, from how far and
//! where they were hit. Built from the spectator damage report (COA_SpectatorDamageReportStore),
//! which every client already receives. Lives on the workspace root, above the respawn/spectator
//! menu that opens on death.
class COA_KilledByCard
{
	protected static const ResourceName LAYOUT = "{7C0D52A8E91F3C17}UI/layouts/COA_KilledByCard.layout";
	protected static const int DISPLAY_MS = 5000;
	protected static const float FADE_SPEED = 4;

	protected static Widget s_wRoot;
	protected static int s_iShownForWorldTime = -1;

	//------------------------------------------------------------------------------------------------
	//! A damage report event was marked fatal - show the card if the victim is the local player.
	//! Both the fatal broadcast and the later "mark fatal" fix-up call this; the event's world time
	//! keeps it to one card per death.
	static void OnFatalEvent(int victimPlayerId)
	{
		if (RplSession.Mode() == RplMode.Dedicated)
			return;

		if (victimPlayerId <= 0 || victimPlayerId != SCR_PlayerController.GetLocalPlayerId())
			return;

		COA_SpectatorDamageReportEntry entry = COA_SpectatorDamageReportStore.GetLatestFatalEntry(victimPlayerId);
		if (!entry || entry.m_iWorldTime == s_iShownForWorldTime)
			return;

		s_iShownForWorldTime = entry.m_iWorldTime;
		Show(entry);
	}

	//------------------------------------------------------------------------------------------------
	protected static void Show(COA_SpectatorDamageReportEntry entry)
	{
		if (!s_wRoot)
			s_wRoot = GetGame().GetWorkspace().CreateWidgets(LAYOUT);

		if (!s_wRoot)
			return;

		TextWidget heading = TextWidget.Cast(s_wRoot.FindAnyWidget("Heading"));
		TextWidget killerName = TextWidget.Cast(s_wRoot.FindAnyWidget("KillerName"));
		Widget friendlyFire = s_wRoot.FindAnyWidget("FriendlyFire");
		TextWidget details = TextWidget.Cast(s_wRoot.FindAnyWidget("Details"));
		if (!heading || !killerName || !details)
			return;

		bool selfInflicted = entry.m_iAttackerPlayerId == entry.m_iVictimPlayerId;
		if (selfInflicted)
		{
			heading.SetText("YOU DIED");
			killerName.SetText("Self-inflicted");
		}
		else
		{
			heading.SetText("KILLED BY");
			killerName.SetText(GetKillerLabel(entry));
		}

		if (friendlyFire)
			friendlyFire.SetVisible(!selfInflicted && IsFriendlyFire(entry));

		string detailText = BuildDetails(entry);
		details.SetText(detailText);
		details.SetVisible(!detailText.IsEmpty());

		s_wRoot.SetOpacity(0);
		s_wRoot.SetVisible(true);
		AnimateWidget.Opacity(s_wRoot, 1, FADE_SPEED);

		GetGame().GetCallqueue().Remove(FadeOut);
		GetGame().GetCallqueue().CallLater(FadeOut, DISPLAY_MS, false);
	}

	//------------------------------------------------------------------------------------------------
	protected static void FadeOut()
	{
		if (s_wRoot)
			AnimateWidget.Opacity(s_wRoot, 0, FADE_SPEED);
	}

	//------------------------------------------------------------------------------------------------
	protected static string GetKillerLabel(COA_SpectatorDamageReportEntry entry)
	{
		if (entry.m_iAttackerPlayerId > 0)
			return entry.m_sAttackerName;

		// No player instigator: AI, or genuinely the environment (falls, fire, vehicles...)
		if (entry.m_sDamageType == "Gunshot")
			return "Unknown shooter";

		return entry.m_sDamageType;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsFriendlyFire(COA_SpectatorDamageReportEntry entry)
	{
		if (entry.m_iAttackerPlayerId <= 0)
			return false;

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return false;

		Faction victimFaction = slottingManager.GetPlayerSlotFaction(entry.m_iVictimPlayerId, true);
		Faction attackerFaction = slottingManager.GetPlayerSlotFaction(entry.m_iAttackerPlayerId, true);
		return victimFaction && victimFaction == attackerFaction;
	}

	//------------------------------------------------------------------------------------------------
	//! "M16A2  ·  142 m  ·  Head" - each part only when known
	protected static string BuildDetails(COA_SpectatorDamageReportEntry entry)
	{
		array<string> parts = {};

		if (!entry.m_sWeaponName.IsEmpty())
			parts.Insert(entry.m_sWeaponName);
		else if (entry.m_iAttackerPlayerId > 0 && entry.m_sDamageType != "Unknown")
			parts.Insert(entry.m_sDamageType);

		if (entry.m_fRangeMeters >= 0 && entry.m_iAttackerPlayerId != entry.m_iVictimPlayerId)
		{
			int meters = Math.Round(entry.m_fRangeMeters);
			parts.Insert(string.Format("%1 m", meters));
		}

		if (entry.m_sBodyRegion != "Unknown")
			parts.Insert(entry.m_sBodyRegion);

		return SCR_StringHelper.Join("  ·  ", parts);
	}
}
