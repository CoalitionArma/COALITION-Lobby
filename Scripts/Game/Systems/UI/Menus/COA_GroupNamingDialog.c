//------------------------------------------------------------------------------------------------
//! Opened by COA_MarkGroupJoinableAction.PerformOwner once a GM chooses to mark a placed group
//! joinable. Lets the GM type a name for the group before it's actually registered; on confirm,
//! sends the typed name (plus the group's RplId/faction, stashed via Init() right after opening)
//! to COA_PlayerRplToAuthorityManager.RequestMarkGroupJoinable, which is what actually creates the
//! slots server-side (see COA_SlottingManager.RegisterGMPossessionGroup) - this dialog only
//! collects the name, it never touches slotting state directly.
//!
//! Deliberately a plain ChimeraMenuBase rather than vanilla's DialogUI: DialogUI expects "Confirm"/
//! "Cancel" widgets wired through SCR_InputButtonComponent, a pattern with no working local example
//! to copy. This instead follows COA_AdminMenuUI's own proven button idiom - SCR_ButtonTextComponent
//! + manual m_OnClicked wiring, and a plain EditBoxWidget (no handler component needed) - both
//! confirmed working in COA_GroupNamingDialog.layout's admin-menu-derived structure.
class COA_GroupNamingDialog : ChimeraMenuBase
{
	// Hover/press feedback and entrance animation - see COA_UIPolish
	protected ref COA_MenuPolish m_UIPolish;

	protected Widget m_wRoot;
	protected EditBoxWidget m_wNameBox;
	protected RplId m_GroupId;
	protected FactionKey m_sFactionKey;

	//------------------------------------------------------------------------------------------------
	//! \param[in] groupId RplId of the group to be marked joinable once a name is confirmed
	//! \param[in] factionKey the group's faction, forwarded unchanged to RegisterGMPossessionGroup
	void Init(RplId groupId, FactionKey factionKey)
	{
		m_GroupId = groupId;
		m_sFactionKey = factionKey;
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		m_wRoot = GetRootWidget();
		if (!m_wRoot)
			return;

		m_wNameBox = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("Name"));

		SCR_ButtonTextComponent confirmButton = SCR_ButtonTextComponent.GetButtonText("Confirm", m_wRoot);
		if (confirmButton)
			confirmButton.m_OnClicked.Insert(OnConfirm);

		SCR_ButtonTextComponent cancelButton = SCR_ButtonTextComponent.GetButtonText("Cancel", m_wRoot);
		if (cancelButton)
			cancelButton.m_OnClicked.Insert(OnCancel);

		// Hover feedback on buttons without their own, and a quick staggered fade-in (COA_UIPolish)
		m_UIPolish = new COA_MenuPolish(GetRootWidget(), true);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnConfirm()
	{
		string typedName;
		if (m_wNameBox)
			typedName = m_wNameBox.GetText();

		COA_PlayerRplToAuthorityManager rplManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (rplManager)
			rplManager.RequestMarkGroupJoinable(m_GroupId, m_sFactionKey, typedName);

		Close();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCancel()
	{
		Close();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);

		if (m_UIPolish)
			m_UIPolish.Update(tDelta);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		super.OnMenuClose();

		if (m_UIPolish)
		{
			m_UIPolish.Cleanup();
			m_UIPolish = null;
		}
	}
};
