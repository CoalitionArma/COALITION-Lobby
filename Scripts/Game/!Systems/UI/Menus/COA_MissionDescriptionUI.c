//! Shared mission-description (briefing) list + text panel logic.
//! Used by both COA_PreviewMenu (lobby briefing screen) and CRF's modded SCR_MapMenuUI (in-game map briefing),
//! which previously each carried their own copy of this logic. Lives here because COALITION-Lobby is a
//! dependency of the Coalition-Reforger-Framework addon, so both can reference this class.
class COA_MissionDescriptionUI
{
	protected Widget m_wRoot;                                 // The "MissionDescription" container widget
	protected Widget m_wDescriptionListWidget;                // "DescriptionList" overlay (the selectable list)
	protected SCR_ListBoxComponent m_cListBoxComponent;
	protected ButtonWidget m_wBackButton;
	protected ScrollLayoutWidget m_wScrollLayout;
	protected RichTextWidget m_wDescriptionText;
	protected COA_Gamemode m_Gamemode;
	protected ref array<ref COA_MissionDescriptor> m_aActiveDescriptors = {};

	//------------------------------------------------------------------------------------------------
	//! Finds and caches all widgets under the given "MissionDescription" container.
	//! Call once per menu open. Returns false if a required widget is missing.
	bool Init(notnull Widget missionDescriptionWidget, COA_Gamemode gamemode)
	{
		m_wRoot = missionDescriptionWidget;
		m_Gamemode = gamemode;

		m_wDescriptionListWidget = m_wRoot.FindAnyWidget("DescriptionList");
		if (!m_wDescriptionListWidget)
			return false;

		m_cListBoxComponent = SCR_ListBoxComponent.Cast(m_wDescriptionListWidget.FindHandler(SCR_ListBoxComponent));
		if (!m_cListBoxComponent)
			return false;

		m_wBackButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("BackButton"));
		m_wScrollLayout = ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("ScrollLayout"));
		m_wDescriptionText = RichTextWidget.Cast(m_wRoot.FindAnyWidget("DescriptionInfo"));

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Populates the list with descriptors relevant to the local player's faction and shows the list view
	//! (hiding whatever description text was previously on screen).
	void ShowList()
	{
		if (!m_cListBoxComponent || !m_Gamemode)
			return;

		GetGame().GetCallqueue().Remove(RefreshScrollLayout);

		if (m_wScrollLayout)
		{
			m_wScrollLayout.SetEnabled(false);
			m_wScrollLayout.SetSliderPos(0, 0);
		}

		if (m_wDescriptionListWidget)
		{
			m_wDescriptionListWidget.SetVisible(true);
			m_wDescriptionListWidget.SetEnabled(true);
		}

		if (m_wBackButton)
		{
			m_wBackButton.SetOpacity(0);
			m_wBackButton.SetEnabled(false);
			SCR_ButtonTextComponent backButton = SCR_ButtonTextComponent.Cast(m_wBackButton.FindHandler(SCR_ButtonTextComponent));
			if (backButton)
				backButton.m_OnClicked.Clear();
		}

		if (m_wDescriptionText)
			m_wDescriptionText.SetText("");

		m_cListBoxComponent.Clear();
		m_aActiveDescriptors.Clear();

		SCR_PlayerFactionAffiliationComponent factionComponent = SCR_PlayerFactionAffiliationComponent.Cast(
			GetGame().GetPlayerController().FindComponent(SCR_PlayerFactionAffiliationComponent)
		);

		if (!factionComponent)
			return;

		string playerFaction = factionComponent.GetAffiliatedFactionKey();

		foreach (ref COA_MissionDescriptor description : m_Gamemode.m_aMissionDescriptors)
		{
			// Add description visible to all factions
			if (description.m_bShowForAnyFaction)
			{
				m_cListBoxComponent.AddItem(
					description.m_sTitle,
					null,
					"{A564FC959554A1B9}UI/Listbox/DescriptionListboxElementNoIcon.layout"
				);
				m_aActiveDescriptors.Insert(description);
				continue;
			}

			// Add description specific to player's faction
			foreach (string factionKey : description.m_aFactionKeys)
			{
				if (playerFaction == factionKey)
				{
					m_cListBoxComponent.AddItem(
						description.m_sTitle,
						null,
						"{A564FC959554A1B9}UI/Listbox/DescriptionListboxElementNoIcon.layout"
					);
					m_aActiveDescriptors.Insert(description);
					break;
				}
			}
		}

		m_cListBoxComponent.m_OnChanged.Insert(ShowSelectedDescription);
	}

	//------------------------------------------------------------------------------------------------
	//! Shows the text of the currently-selected description.
	protected void ShowSelectedDescription()
	{
		if (!m_cListBoxComponent || !m_aActiveDescriptors)
			return;

		if (m_wScrollLayout)
			m_wScrollLayout.SetEnabled(true);

		// Hide the list so it doesn't sit interactive underneath the description text
		if (m_wDescriptionListWidget)
		{
			m_wDescriptionListWidget.SetVisible(false);
			m_wDescriptionListWidget.SetEnabled(false);
		}

		int index = m_cListBoxComponent.GetSelectedItem();
		if (index < 0 || index >= m_aActiveDescriptors.Count())
			return;

		string description = m_aActiveDescriptors.Get(index).m_sTextData;

		if (m_wBackButton)
		{
			m_wBackButton.SetOpacity(1);
			m_wBackButton.SetEnabled(true);
			SCR_ButtonTextComponent backButton = SCR_ButtonTextComponent.Cast(m_wBackButton.FindHandler(SCR_ButtonTextComponent));
			if (backButton)
				backButton.m_OnClicked.Insert(ShowList);
		}

		m_cListBoxComponent.Clear();
		m_cListBoxComponent.m_OnChanged.Clear();

		if (m_wDescriptionText)
			m_wDescriptionText.SetText(description);

		// Scroll back to the top for the newly selected description
		if (m_wScrollLayout)
			m_wScrollLayout.SetSliderPos(0, 0);

		// Force a relayout once the RichText has had time to finish re-wrapping at its new content
		// length. Without this, ScrollLayoutWidget can compute its scroll range against a stale
		// (too-short) content height from before the reflow settled, clamping scrolling short of
		// the true bottom. Matches the delay vanilla itself uses for the same class of issue
		// (SCR_TaskListEntryDescriptionUIComponent.CheckOverflow).
		GetGame().GetCallqueue().Remove(RefreshScrollLayout);
		GetGame().GetCallqueue().CallLater(RefreshScrollLayout, 100);
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshScrollLayout()
	{
		if (m_wDescriptionText)
			m_wDescriptionText.Update();

		if (m_wScrollLayout)
			m_wScrollLayout.Update();
	}

	//------------------------------------------------------------------------------------------------
	//! Clears list state and any registered handlers. Call from the owning menu's OnMenuClose.
	void Clear()
	{
		GetGame().GetCallqueue().Remove(RefreshScrollLayout);

		if (m_cListBoxComponent)
		{
			m_cListBoxComponent.m_OnChanged.Clear();
			m_cListBoxComponent.Clear();
		}

		if (m_wBackButton)
		{
			SCR_ButtonTextComponent backButton = SCR_ButtonTextComponent.Cast(m_wBackButton.FindHandler(SCR_ButtonTextComponent));
			if (backButton)
				backButton.m_OnClicked.Clear();
		}

		if (m_aActiveDescriptors)
			m_aActiveDescriptors.Clear();
	}
}
