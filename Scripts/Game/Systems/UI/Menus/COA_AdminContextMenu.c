//------------------------------------------------------------------------------------------------
// Admin menu right-click menu
//
// Right-clicking a player in any of the admin menu's player lists, or a ticket in the Tickets tab,
// opens a small menu at the cursor with quick fixes for that player (heal, unstuck, teleport, reset
// gear, respawn) and, on ticket rows, the ticket actions. COA_AdminMenu owns it: AttachContextMenu
// hooks a list's rows after every repopulation, OpenContextMenu builds the menu
// (AdminContextMenu.layout plus one AdminContextMenuRow.layout per action), and the menu closes on
// tab change and menu close.
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
//! Actions offered on the right-click menu
enum COA_EAdminContextAction
{
	CLOSE = -1,
	HEAL = 0,
	REPAIR_VEHICLE,
	UNSTUCK,
	BRING_TO_ME,
	GO_TO_PLAYER,
	RESET_GEAR,
	RESPAWN_ON_GROUP,
	OPEN_TICKET,
	ASSIGN_TICKET,
	CLOSE_TICKET,
	COPY_NAME,
}

//------------------------------------------------------------------------------------------------
//! Added to each list row by COA_AdminMenu.AttachContextMenu: right click opens the menu for the row
class COA_AdminListRowContextHandler : ScriptedWidgetComponent
{
	COA_AdminMenu m_Menu;
	SCR_ListBoxComponent m_List;
	Widget m_wRow;

	//------------------------------------------------------------------------------------------------
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (button != 1 || !m_Menu || !m_List)
			return false;

		m_Menu.OpenContextMenu(m_List, m_wRow, x, y);
		return true;
	}
}

//------------------------------------------------------------------------------------------------
//! Button in the right-click menu - attached in AdminContextMenu.layout (the backdrop, which closes
//! the menu) and AdminContextMenuRow.layout (one per action)
class COA_AdminContextActionButton : ScriptedWidgetComponent
{
	[Attribute("0", desc: "COA_EAdminContextAction this button performs; -1 closes the menu")]
	int m_iAction;

	ref ScriptInvoker m_OnClicked = new ScriptInvoker(); // (int action)

	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button == 0)
			m_OnClicked.Invoke(m_iAction);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! A right click outside the menu closes it too
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (button == 1 && m_iAction == COA_EAdminContextAction.CLOSE)
		{
			m_OnClicked.Invoke(m_iAction);
			return true;
		}

		return false;
	}
}
