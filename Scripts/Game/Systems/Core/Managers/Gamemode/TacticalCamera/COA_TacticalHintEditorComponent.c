[ComponentEditorProps(category: "GameScripted/Editor", description: "Shows the Tactical Camera how-to hint when the mode is opened", icon: "WBData/ComponentEditorProps/componentEditor.png")]
class COA_TacticalHintEditorComponentClass : SCR_BaseEditorComponentClass
{
}

//------------------------------------------------------------------------------------------------
//! On COA_EditorModeTactical.et: tells a leader what the Tactical Camera can do the first time they
//! open it in a session, and logs on the server when a player enters or leaves it
//! (see COA_TacticalCamera).
class COA_TacticalHintEditorComponent : SCR_BaseEditorComponent
{
	protected static const float HINT_DURATION = 25;

	protected static bool s_bHintShown;

	//------------------------------------------------------------------------------------------------
	//! Server: the tactical mode became the player's active mode (editor opened in it, or switched to it)
	override protected void EOnEditorActivateServer()
	{
		LogServer("entered");
	}

	//------------------------------------------------------------------------------------------------
	//! Server: the player closed the editor or switched to another mode
	override protected void EOnEditorDeactivateServer()
	{
		LogServer("left");
	}

	//------------------------------------------------------------------------------------------------
	protected void LogServer(string action)
	{
		SCR_EditorManagerEntity editorManager = GetManager();
		if (!editorManager)
			return;

		int playerId = editorManager.GetPlayerID();
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);

		string where;
		IEntity body = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (body)
			where = " at " + SCR_MapEntity.GetGridLabel(body.GetOrigin());

		string message = string.Format("%1 (ID %2) %3 the Tactical Camera%4", playerName, playerId, action, where);
		Print("[Coalition Lobby] Tactical camera: " + message, LogLevel.NORMAL);

		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.LogAdminAction(message, playerId, false, COA_EAdminLogLevel.Low);
	}

	protected static const int MAP_HOLD_DELAY_MS = 250;

	//------------------------------------------------------------------------------------------------
	//! Client: the leader's character takes out and looks at their map while the camera is open, so
	//! to everyone else it reads as "checking the map" rather than standing idle. The map screen
	//! itself is kept closed by the Lobby's SCR_MapGadgetComponent mod.
	override protected void EOnEditorActivate()
	{
		GetGame().GetCallqueue().Remove(HoldMap);
		GetGame().GetCallqueue().CallLater(HoldMap, MAP_HOLD_DELAY_MS, false, true);

		if (s_bHintShown)
			return;

		s_bHintShown = true;

		string text = string.Format(
			"Overhead view of friendly units within %1 m of your body (up to %2 m high).\n"
			+ "Left-click selects a squad, right-click gives it an order: Move, Attack, Defend, Hold, Regroup or Cancel.\n"
			+ "Every member of that squad gets the order as a hint and a marker on their map.\n"
			+ "Nothing can be placed or edited here. Press the Game Master key again to return to your body.",
			Math.Round(COA_TacticalCamera.GetRange()), Math.Round(COA_TacticalCamera.GetMaxHeight()));

		SCR_HintManagerComponent.ShowCustomHint(text, "TACTICAL CAMERA", HINT_DURATION);
	}

	//------------------------------------------------------------------------------------------------
	override protected void EOnEditorDeactivate()
	{
		GetGame().GetCallqueue().Remove(HoldMap);
		HoldMap(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void HoldMap(bool hold)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(SCR_PlayerController.GetLocalMainEntity());
		if (!character)
			return;

		SCR_GadgetManagerComponent gadgetManager = SCR_GadgetManagerComponent.GetGadgetManager(character);
		if (!gadgetManager)
			return;

		IEntity mapGadget = gadgetManager.GetGadgetByType(EGadgetType.MAP);
		if (!mapGadget)
			return;

		bool holdingMap = gadgetManager.GetHeldGadget() == mapGadget;
		if (hold && !holdingMap)
			gadgetManager.SetGadgetMode(mapGadget, EGadgetMode.IN_HAND, true);
		else if (!hold && holdingMap)
			gadgetManager.RemoveHeldGadget();
	}
}
