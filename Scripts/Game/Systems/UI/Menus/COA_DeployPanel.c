//------------------------------------------------------------------------------------------------
//! Tone of a status line or list row detail in the deploy screens
enum COA_EDeployTone
{
	NEUTRAL,
	GOOD,
	WARNING,
	BAD
}

//------------------------------------------------------------------------------------------------
//! A scripted map marker a deploy screen placed, kept so exactly the same marker can be removed
class COA_DeployMarker
{
	string m_sName;
	vector m_vPosition;
	int m_iColor;
}

//------------------------------------------------------------------------------------------------
//! The deploy screens' map markers (spawn points, friendly units), via COA_PlayerScriptedMarkerManager
class COA_DeployMarkers
{
	static const ResourceName MARKER_ICON = "{302979C3EAF01D2E}UI/Textures/Editor/ContentBrowser/ContentBrowser_Trait_SpawnPoint.edds";
	protected static const int MARKER_Z_ORDER = 50;

	protected ref array<ref COA_DeployMarker> m_aMarkers = {};

	//------------------------------------------------------------------------------------------------
	void Add(string name, vector position, int color)
	{
		COA_PlayerScriptedMarkerManager markerManager = COA_PlayerScriptedMarkerManager.GetInstance();
		if (!markerManager)
			return;

		markerManager.AddScriptedMarker("Static Marker", FormatPosition(position), 1, name, MARKER_ICON, MARKER_Z_ORDER, color);

		COA_DeployMarker marker = new COA_DeployMarker();
		marker.m_sName = name;
		marker.m_vPosition = position;
		marker.m_iColor = color;
		m_aMarkers.Insert(marker);
	}

	//------------------------------------------------------------------------------------------------
	void Clear()
	{
		COA_PlayerScriptedMarkerManager markerManager = COA_PlayerScriptedMarkerManager.GetInstance();
		if (markerManager)
		{
			foreach (COA_DeployMarker marker : m_aMarkers)
				markerManager.RemoveScriptedMarker("Static Marker", FormatPosition(marker.m_vPosition), 1, marker.m_sName, MARKER_ICON, MARKER_Z_ORDER, marker.m_iColor);
		}

		m_aMarkers.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected static string FormatPosition(vector position)
	{
		return string.Format("%1 %2 %3", position[0], position[1], position[2]);
	}
}

//------------------------------------------------------------------------------------------------
// The shared header + right-hand panel of the map-based deploy screens (COA_RespawnMenu,
// CRF_JIPForwardDeployMenu), in the lobby phase screens' style. The layouts carry the same widget
// names (UI/Respawn/COA_Respawn.layout, CRF's UI/JIPForwardDeploy/CRF_JIPForwardDeploy.layout):
//   Header:  HeaderTitle, timerTitle, timerCountDown, HeaderInfo
//   Panel:   PanelTitle, PanelCount, List1Box, EmptyText, StatusText,
//            ActionButton(+BG/Text), SecondaryButton(+BG/Text), HintText
// Menus own the behaviour; this only presents it, so both screens read and behave alike.
//------------------------------------------------------------------------------------------------
class COA_DeployPanel
{
	//! Row layout for the panel's list - name on the left, detail on the right
	static const ResourceName ROW_LAYOUT = "{7D3E1A0000000001}UI/Listbox/COA_DeployListRow.layout";

	// Same palette as the phase screens / admin menu (sRGB, converted like COA_PhaseUI)
	static const ref Color TEXT = Color.FromSRGBA(239, 242, 247, 255);
	static const ref Color TEXT_MUTED = Color.FromSRGBA(170, 181, 204, 255);
	static const ref Color TONE_GOOD = Color.FromSRGBA(110, 205, 135, 255);
	static const ref Color TONE_WARNING = Color.FromSRGBA(232, 178, 70, 255);
	static const ref Color TONE_BAD = Color.FromSRGBA(232, 96, 96, 255);
	protected static const ref Color PRIMARY_BG = Color.FromSRGBA(201, 54, 54, 255);
	protected static const ref Color PRIMARY_BG_DISABLED = Color.FromSRGBA(28, 33, 41, 255);

	// Button row geometry (layout units): full width when the secondary button is hidden
	protected static const float BUTTON_PRIMARY_WIDTH = 246;
	protected static const float BUTTON_FULL_WIDTH = 396;

	protected Widget m_wRoot;
	protected TextWidget m_wHeaderTitle;
	protected TextWidget m_wTimerTitle;
	protected TextWidget m_wTimer;
	protected TextWidget m_wHeaderInfo;
	protected TextWidget m_wPanelTitle;
	protected TextWidget m_wPanelCount;
	protected TextWidget m_wEmptyText;
	protected RichTextWidget m_wStatus;
	protected RichTextWidget m_wHints;
	protected Widget m_wPrimary;
	protected Widget m_wPrimaryBG;
	protected TextWidget m_wPrimaryText;
	protected Widget m_wSecondary;
	protected Widget m_wSecondaryBG;

	protected SCR_ButtonTextComponent m_PrimaryButton;
	protected SCR_ButtonTextComponent m_SecondaryButton;
	protected SCR_ListBoxComponent m_List;

	//------------------------------------------------------------------------------------------------
	void COA_DeployPanel(Widget root)
	{
		m_wRoot = root;
		if (!root)
			return;

		m_wHeaderTitle = TextWidget.Cast(root.FindAnyWidget("HeaderTitle"));
		m_wTimerTitle = TextWidget.Cast(root.FindAnyWidget("timerTitle"));
		m_wTimer = TextWidget.Cast(root.FindAnyWidget("timerCountDown"));
		m_wHeaderInfo = TextWidget.Cast(root.FindAnyWidget("HeaderInfo"));
		m_wPanelTitle = TextWidget.Cast(root.FindAnyWidget("PanelTitle"));
		m_wPanelCount = TextWidget.Cast(root.FindAnyWidget("PanelCount"));
		m_wEmptyText = TextWidget.Cast(root.FindAnyWidget("EmptyText"));
		m_wStatus = RichTextWidget.Cast(root.FindAnyWidget("StatusText"));
		m_wHints = RichTextWidget.Cast(root.FindAnyWidget("HintText"));

		m_wPrimary = root.FindAnyWidget("ActionButton");
		m_wPrimaryBG = root.FindAnyWidget("ActionButtonBG");
		m_wSecondary = root.FindAnyWidget("SecondaryButton");
		m_wSecondaryBG = root.FindAnyWidget("SecondaryButtonBG");

		if (m_wPrimary)
		{
			m_PrimaryButton = SCR_ButtonTextComponent.Cast(m_wPrimary.FindHandler(SCR_ButtonTextComponent));
			m_wPrimaryText = TextWidget.Cast(m_wPrimary.FindAnyWidget("ActionButtonText"));
		}

		if (m_wSecondary)
			m_SecondaryButton = SCR_ButtonTextComponent.Cast(m_wSecondary.FindHandler(SCR_ButtonTextComponent));

		Widget listRoot = root.FindAnyWidget("List1Box");
		if (listRoot)
			m_List = SCR_ListBoxComponent.Cast(listRoot.FindHandler(SCR_ListBoxComponent));
	}

	//------------------------------------------------------------------------------------------------
	SCR_ListBoxComponent GetList()
	{
		return m_List;
	}

	//------------------------------------------------------------------------------------------------
	SCR_ButtonTextComponent GetPrimaryButton()
	{
		return m_PrimaryButton;
	}

	//------------------------------------------------------------------------------------------------
	SCR_ButtonTextComponent GetSecondaryButton()
	{
		return m_SecondaryButton;
	}

//------------------------------------------------------------------------------------------------
//	HEADER
//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	void SetTitle(string title)
	{
		if (m_wHeaderTitle)
			m_wHeaderTitle.SetText(title);
	}

	//------------------------------------------------------------------------------------------------
	//! Centre of the header: a small label over a large value (e.g. "Next wave in" / "00:42").
	//! An empty label hides both.
	void SetTimer(string label, string value)
	{
		bool visible = !label.IsEmpty();

		if (m_wTimerTitle)
		{
			m_wTimerTitle.SetVisible(visible);
			m_wTimerTitle.SetText(label);
		}

		if (m_wTimer)
		{
			m_wTimer.SetVisible(visible);
			m_wTimer.SetText(value);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Right side of the header (tickets, unit count...)
	void SetHeaderInfo(string text)
	{
		if (m_wHeaderInfo)
			m_wHeaderInfo.SetText(text);
	}

//------------------------------------------------------------------------------------------------
//	PANEL
//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	void SetListTitle(string title, string count = string.Empty)
	{
		if (m_wPanelTitle)
			m_wPanelTitle.SetText(title);

		if (m_wPanelCount)
			m_wPanelCount.SetText(count);
	}

	//------------------------------------------------------------------------------------------------
	//! Text shown in place of the list when it has no rows; empty hides it
	void SetEmptyText(string text)
	{
		if (!m_wEmptyText)
			return;

		m_wEmptyText.SetText(text);
		m_wEmptyText.SetVisible(!text.IsEmpty());
	}

	//------------------------------------------------------------------------------------------------
	void SetStatus(string text, COA_EDeployTone tone = COA_EDeployTone.NEUTRAL)
	{
		if (!m_wStatus)
			return;

		m_wStatus.SetText(text);
		m_wStatus.SetColor(GetToneColor(tone, TEXT));
	}

	//------------------------------------------------------------------------------------------------
	//! Key hints along the bottom; rich text, so <action name='...'/> shows the bound key
	void SetHints(string richText)
	{
		if (m_wHints)
			m_wHints.SetText(richText);
	}

	//------------------------------------------------------------------------------------------------
	//! The primary (red) button. Disabled it greys out and ignores clicks.
	void SetPrimary(string text, bool enabled = true)
	{
		if (m_wPrimaryText)
		{
			m_wPrimaryText.SetText(text);
			if (enabled)
				m_wPrimaryText.SetColor(TEXT);
			else
				m_wPrimaryText.SetColor(TEXT_MUTED);
		}

		if (m_wPrimary)
			m_wPrimary.SetEnabled(enabled);

		if (m_wPrimaryBG)
		{
			if (enabled)
				m_wPrimaryBG.SetColor(PRIMARY_BG);
			else
				m_wPrimaryBG.SetColor(PRIMARY_BG_DISABLED);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The secondary (dark) button; hidden, the primary button takes the whole row
	void SetSecondaryVisible(bool visible)
	{
		if (m_wSecondary && m_wSecondary.IsVisible() == visible)
			return;

		if (m_wSecondary)
			m_wSecondary.SetVisible(visible);

		if (m_wSecondaryBG)
			m_wSecondaryBG.SetVisible(visible);

		float width = BUTTON_FULL_WIDTH;
		if (visible)
			width = BUTTON_PRIMARY_WIDTH;

		if (m_wPrimary)
			FrameSlot.SetSizeX(m_wPrimary, width);

		if (m_wPrimaryBG)
			FrameSlot.SetSizeX(m_wPrimaryBG, width);
	}

	//------------------------------------------------------------------------------------------------
	void SetSecondaryText(string text)
	{
		if (!m_wSecondary)
			return;

		TextWidget label = TextWidget.Cast(m_wSecondary.FindAnyWidget("SecondaryButtonText"));
		if (label)
			label.SetText(text);
	}

//------------------------------------------------------------------------------------------------
//	LIST ROWS
//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Add a row: name on the left, a short detail on the right (distance, alive count, "Blocked").
	//! \param[in] tone colours the detail (NEUTRAL = muted)
	//! \return the row's index
	int AddRow(string name, string detail, COA_EDeployTone tone = COA_EDeployTone.NEUTRAL, Managed data = null)
	{
		if (!m_List)
			return -1;

		int index = m_List.AddItem(name, data, ROW_LAYOUT);
		SetRowDetail(index, detail, tone);
		return index;
	}

	//------------------------------------------------------------------------------------------------
	void SetRowDetail(int index, string detail, COA_EDeployTone tone = COA_EDeployTone.NEUTRAL)
	{
		Widget row = GetRowWidget(index);
		if (!row)
			return;

		TextWidget detailText = TextWidget.Cast(row.FindAnyWidget("SubText"));
		if (!detailText)
			return;

		detailText.SetText(detail);
		detailText.SetColor(GetToneColor(tone, TEXT_MUTED));
	}

	//------------------------------------------------------------------------------------------------
	//! Dim a row's name (e.g. a blocked spawn point) without hiding it
	void SetRowDimmed(int index, bool dimmed)
	{
		Widget row = GetRowWidget(index);
		if (!row)
			return;

		TextWidget nameText = TextWidget.Cast(row.FindAnyWidget("Text"));
		if (!nameText)
			return;

		if (dimmed)
			nameText.SetColor(TEXT_MUTED);
		else
			nameText.SetColor(TEXT);
	}

	//------------------------------------------------------------------------------------------------
	protected Widget GetRowWidget(int index)
	{
		if (!m_List || index < 0 || index >= m_List.GetItemCount())
			return null;

		SCR_ListBoxElementComponent element = m_List.COA_GetElementComponent(index);
		if (!element)
			return null;

		return element.GetRootWidget();
	}

//------------------------------------------------------------------------------------------------
//	HELPERS
//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	static Color GetToneColor(COA_EDeployTone tone, Color neutral)
	{
		switch (tone)
		{
			case COA_EDeployTone.GOOD: return TONE_GOOD;
			case COA_EDeployTone.WARNING: return TONE_WARNING;
			case COA_EDeployTone.BAD: return TONE_BAD;
		}

		return neutral;
	}

	//------------------------------------------------------------------------------------------------
	//! "350 m" / "1.4 km"
	static string FormatDistance(float meters)
	{
		if (meters < 1000)
			return string.Format("%1 m", (int)(Math.Round(meters / 10) * 10));

		return string.Format("%1 km", (Math.Round(meters / 100) / 10).ToString(-1, 1));
	}

	//------------------------------------------------------------------------------------------------
	//! "1 unit" / "3 units"
	static string Plural(int count, string singular, string plural)
	{
		if (count == 1)
			return string.Format("%1 %2", count, singular);

		return string.Format("%1 %2", count, plural);
	}
}
