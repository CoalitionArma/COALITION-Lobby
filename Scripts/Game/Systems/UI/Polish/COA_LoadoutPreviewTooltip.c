//------------------------------------------------------------------------------------------------
//! Card next to the cursor (or the focused row on a controller) showing what the slot under it
//! spawns with: the character dressed in its gear (COA_LoadoutModelPreview) and the item list
//! (COA_LoadoutPreviewHelper). The owning menu calls Update() every frame with the
//! list it should watch; rows are found by walking up from the hovered widget, so nothing has to be
//! attached when rows are created or rebuilt.
class COA_LoadoutPreviewTooltip
{
	protected static const ResourceName LAYOUT = "{7C0A51E3B4D29F60}UI/Phases/COA_LoadoutPreview.layout";

	// Wait this long on a row before showing, so sweeping the cursor down the list doesn't flicker
	protected static const float SHOW_DELAY = 0.25;
	protected static const float CURSOR_OFFSET_X = 24;
	protected static const float CURSOR_OFFSET_Y = 12;
	protected static const float SCREEN_MARGIN = 12;

	protected Widget m_wRoot;
	protected Widget m_wCard;
	protected TextWidget m_wRoleName;
	protected TextWidget m_wSlotContext;
	protected RichTextWidget m_wSummary;
	protected ItemPreviewWidget m_wModelPreview;
	protected Widget m_wModelSizer;

	protected int m_iShownSlotId = -1;
	protected int m_iHoveredSlotId = -1;
	protected float m_fHoverTime;

	//------------------------------------------------------------------------------------------------
	void COA_LoadoutPreviewTooltip(Widget parent)
	{
		if (!parent)
			return;

		m_wRoot = GetGame().GetWorkspace().CreateWidgets(LAYOUT, parent);
		if (!m_wRoot)
			return;

		m_wCard = m_wRoot.FindAnyWidget("LoadoutPreviewCard");
		m_wRoleName = TextWidget.Cast(m_wRoot.FindAnyWidget("RoleName"));
		m_wSlotContext = TextWidget.Cast(m_wRoot.FindAnyWidget("SlotContext"));
		m_wSummary = RichTextWidget.Cast(m_wRoot.FindAnyWidget("Summary"));
		m_wModelPreview = ItemPreviewWidget.Cast(m_wRoot.FindAnyWidget("ModelPreview"));
		m_wModelSizer = m_wRoot.FindAnyWidget("ModelSizer");
		m_wRoot.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_LoadoutPreviewTooltip()
	{
		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();
	}

	//------------------------------------------------------------------------------------------------
	//! \param[in] tDelta Frame time
	//! \param[in] listRoot Root widget of the list whose slot rows get a preview
	void Update(float tDelta, Widget listRoot)
	{
		if (!m_wRoot || !m_wCard)
			return;

		bool usingMouse = GetGame().GetInputManager().IsUsingMouseAndKeyboard();
		Widget target;
		if (usingMouse)
			target = WidgetManager.GetWidgetUnderCursor();
		else
			target = GetGame().GetWorkspace().GetFocusedWidget();

		Widget rowWidget;
		COA_ListBoxElementComponent row = FindSlotRow(target, listRoot, rowWidget);
		int slotId = -1;
		if (row)
			slotId = row.m_iSlotId;

		if (slotId != m_iHoveredSlotId)
		{
			m_iHoveredSlotId = slotId;
			m_fHoverTime = 0;
			Hide();
		}

		if (slotId < 0)
			return;

		m_fHoverTime += tDelta;
		if (m_fHoverTime < SHOW_DELAY)
			return;

		if (m_iShownSlotId != slotId && !Populate(slotId))
			return;

		Position(usingMouse, rowWidget);
	}

	//------------------------------------------------------------------------------------------------
	void Hide()
	{
		m_iShownSlotId = -1;
		if (m_wRoot)
			m_wRoot.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	//! Walks up from widget to the slot row containing it, if that row is inside listRoot.
	//! Group header rows carry a group and no slot, so they are skipped.
	protected COA_ListBoxElementComponent FindSlotRow(Widget widget, Widget listRoot, out Widget rowWidget)
	{
		if (!listRoot)
			return null;

		COA_ListBoxElementComponent row;
		while (widget)
		{
			if (widget == listRoot)
				break;

			if (!row)
			{
				row = COA_ListBoxElementComponent.Cast(widget.FindHandler(COA_ListBoxElementComponent));
				if (row)
					rowWidget = widget;
			}

			widget = widget.GetParent();
		}

		// Only rows actually under listRoot count
		if (!row || widget != listRoot || row.group || !COA_SlottingManager.GetInstance().GetSlotData(row.m_iSlotId))
		{
			rowWidget = null;
			return null;
		}

		return row;
	}

	//------------------------------------------------------------------------------------------------
	protected bool Populate(int slotId)
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		COA_SlotData slotData = slottingManager.GetSlotData(slotId);
		if (!slotData)
			return false;

		string summary = COA_LoadoutPreviewHelper.BuildSummary(slotData.GetSlotFactionKey(), slotData.GetSlotRole());
		if (summary.IsEmpty())
			return false;

		m_wRoleName.SetText(slotData.GetSlotName());

		string context;
		SCR_AIGroup group = COA_EntityHelper.GetGroupFromRplId(slotData.GetSlotCurrentGroup());
		if (group)
			context = group.GetCustomNameWithOriginal();

		int occupant = slotData.GetSlotCurrentPlayerId();
		if (occupant > 0)
		{
			string occupantName = GetGame().GetPlayerManager().GetPlayerName(occupant);
			if (context.IsEmpty())
				context = occupantName;
			else
				context = string.Format("%1  ·  %2", context, occupantName);
		}

		m_wSlotContext.SetText(context);
		m_wSlotContext.SetVisible(!context.IsEmpty());
		m_wSummary.SetText(summary);

		// Only redressed when the hovered slot changes, not every frame
		bool modelShown = COA_LoadoutModelPreview.Show(m_wModelPreview, slotData.GetSlotFactionKey(), slotData.GetSlotRole(), slotData.GetSlotResource());
		if (m_wModelSizer)
			m_wModelSizer.SetVisible(modelShown);

		m_iShownSlotId = slotId;
		m_wRoot.SetVisible(true);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Beside the cursor (mouse) or to the right of the focused row (controller), kept on screen
	protected void Position(bool usingMouse, Widget rowWidget)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();

		float x, y;
		if (usingMouse || !rowWidget)
		{
			int mouseX, mouseY;
			WidgetManager.GetMousePos(mouseX, mouseY);
			x = workspace.DPIUnscale(mouseX) + CURSOR_OFFSET_X;
			y = workspace.DPIUnscale(mouseY) + CURSOR_OFFSET_Y;
		}
		else
		{
			float rowX, rowY, rowW, rowH;
			rowWidget.GetScreenPos(rowX, rowY);
			rowWidget.GetScreenSize(rowW, rowH);
			x = workspace.DPIUnscale(rowX + rowW) + CURSOR_OFFSET_X;
			y = workspace.DPIUnscale(rowY);
		}

		float cardW, cardH, screenW, screenH;
		m_wCard.GetScreenSize(cardW, cardH);
		m_wRoot.GetScreenSize(screenW, screenH);
		cardW = workspace.DPIUnscale(cardW);
		cardH = workspace.DPIUnscale(cardH);
		screenW = workspace.DPIUnscale(screenW);
		screenH = workspace.DPIUnscale(screenH);

		// Flip to the left of the cursor near the right edge, and clamp vertically
		if (x + cardW > screenW - SCREEN_MARGIN)
			x = x - cardW - CURSOR_OFFSET_X * 2;

		x = Math.Clamp(x, SCREEN_MARGIN, Math.Max(SCREEN_MARGIN, screenW - cardW - SCREEN_MARGIN));
		y = Math.Clamp(y, SCREEN_MARGIN, Math.Max(SCREEN_MARGIN, screenH - cardH - SCREEN_MARGIN));

		FrameSlot.SetPos(m_wCard, x, y);
	}
}
