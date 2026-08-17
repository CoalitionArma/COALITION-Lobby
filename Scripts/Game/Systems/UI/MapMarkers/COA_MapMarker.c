//------------------------------------------------------------------------------------------------
// Holds the widgets for a single faction-colored player icon shown on the spectator map.
// Built from the same layout/component as the 3D spectator icons (COA_SpectatorLabelIconCharacter)
// so it gets proven faction coloring, character state and a name label for free.
class COA_SpectatorPlayerIconData
{
	Widget m_wRoot;
	Widget m_wLabel;
	COA_SpectatorLabelIconCharacter m_Icon;
}

modded class SCR_MapMarkersUI
{
	protected const int COA_DOUBLE_CLICK_MS = 350;
	protected const float COA_DOUBLE_CLICK_WORLD_TOLERANCE = 12.5;

	protected COA_PlayerScriptedMarkerManager m_PlayerScriptedMarkerManager;
	protected float m_fLastMapSelectTime = -1;
	protected float m_fLastMapClickWorldX;
	protected float m_fLastMapClickWorldY;

	// Map entity reference
	static SCR_MapEntity m_MapEntity;
	// Flag to track if the map is currently open
	static bool m_bIsMapOpen = false;

	// Array to store time values for marker updates
	protected ref array<float> m_aMarkerUpdateTimes = {};
	// Array to store cached positions for markers
	protected ref array<vector> m_aCachedPositions = {};
	// Map storing marker widgets and their associated data
	protected ref map<Widget, string> m_mMarkerWidgetData = new map<Widget, string>;

	protected const float COA_PLAYER_ICON_SIZE = 28.0;
	protected ref map<RplId, ref COA_SpectatorPlayerIconData> m_mSpectatorPlayerIcons = new map<RplId, ref COA_SpectatorPlayerIconData>();
	protected ref array<RplId> m_aSpectatorPlayerIconsLive = {};
	protected ref array<RplId> m_aSpectatorPlayerIconsToRemove = {};

	//------------------------------------------------------------------------------------------------
	// Called when the map is opened
	override void OnMapOpen(MapConfiguration config)
	{
		super.OnMapOpen(config);
		ResetDoubleClickState();
		
		m_PlayerScriptedMarkerManager = COA_PlayerScriptedMarkerManager.GetInstance();
		
		if (!COA_Gamemode.GetInstance() || !m_PlayerScriptedMarkerManager)
			return;
		
		// Update map open status
		if (!m_bIsMapOpen)
			m_bIsMapOpen = true;
		
		// Schedule marker initialization
		GetGame().GetCallqueue().Call(LoadStoredMarkers);
		m_PlayerScriptedMarkerManager.GetOnMarkerUpdate().Insert(LoadStoredMarkers);
	}
	
	//------------------------------------------------------------------------------------------------
	// Loads and creates markers from player's stored data
	void LoadStoredMarkers()
	{
		// Do not create widgets when the map is not open
		if (!m_bIsMapOpen)
			return;
		
		// Get stored marker data from player controller
		array<string> markerDataArray = m_PlayerScriptedMarkerManager.GetScriptedMarkersArray();
		
		// Return if no markers to display
		if(!markerDataArray || markerDataArray.IsEmpty())
			return;
		
		// Clean up any existing markers
		CleanupExistingMarkers();
		
		// Create each marker from the stored data
		foreach(int index, string markerData : markerDataArray)
		{	
			CreateMarkerFromData(markerData);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Cleans up existing marker widgets
	protected void CleanupExistingMarkers()
	{
		foreach(Widget markerWidget, string data : m_mMarkerWidgetData)
		{
			delete markerWidget;
		}
		
		m_mMarkerWidgetData.Clear();
	}
	
	//------------------------------------------------------------------------------------------------
	// Creates a single marker from the provided data string
	protected void CreateMarkerFromData(string markerData)
	{
		// Parse marker data
		TStringArray markerProperties = {};
		markerData.Split("||", markerProperties, false);
		if (markerProperties.Count() < 7)
			return;
		
		// Create marker widget
		Widget markerWidget = GetGame().GetWorkspace().CreateWidgets("{DD15734EB89D74E2}UI/layouts/Map/MapMarkerBase.layout", m_RootWidget);
		
		if (!markerWidget)
			return;
			
		// Set z-order from data
		markerWidget.SetZOrder(markerProperties[5].ToInt());

		// Setup marker icon
		ImageWidget markerIcon = ImageWidget.Cast(markerWidget.FindAnyWidget("MarkerIcon"));		
		if(markerIcon)
		{
			markerIcon.LoadImageTexture(0, markerProperties[4]);
			markerIcon.SetColorInt(markerProperties[6].ToInt());
			markerIcon.SetVisible(true);
		}
	
		// Setup marker text
		TextWidget markerText = TextWidget.Cast(markerWidget.FindAnyWidget("MarkerText"));
		if(markerText)
		{
			markerText.SetText(markerProperties[3]);
			markerText.SetVisible(true);
		}
	
		// Store widget with its data
		m_mMarkerWidgetData.Set(markerWidget, markerData);
	}
	
	//------------------------------------------------------------------------------------------------
	// Called when the map is closed
	override protected void OnMapClose(MapConfiguration config)
	{
		m_bIsMapOpen = false;
		ResetDoubleClickState();
		
		// Remove the marker update listener so updates don't create widgets after map close
		if (m_PlayerScriptedMarkerManager)
			m_PlayerScriptedMarkerManager.GetOnMarkerUpdate().Remove(LoadStoredMarkers);

		// Cancel any pending deferred LoadStoredMarkers call
		GetGame().GetCallqueue().Remove(LoadStoredMarkers);

		// Clean up marker widgets before parent is destroyed
		CleanupExistingMarkers();

		// Clean up spectator player icons before parent is destroyed
		COA_ClearSpectatorPlayerIcons();

		super.OnMapClose(config);
	}
	
	//------------------------------------------------------------------------------------------------
	// Updates marker positions on the map
	override void Update(float timeSlice)
	{
		super.Update(timeSlice);

		// Skip update if map is closed
		if (!m_bIsMapOpen)
			return;
		
		// Get map entity
		m_MapEntity = SCR_MapEntity.GetMapInstance();
		if (!m_MapEntity) 
			return;
		
		int markerIndex = 0;
		
		// Update each marker's position
		foreach(Widget markerWidget, string markerData : m_mMarkerWidgetData)
		{
			// Skip invalid markers
			if(!markerWidget)
			{
				markerIndex++;
				continue;
			}
				
			// Parse marker data
			TStringArray markerProperties = {};
			markerData.Split("||", markerProperties, false);
			
			if(!markerProperties || markerProperties.Count() < 7)
			{
				markerIndex++;
				continue;
			}
			
			// Calculate marker position
			vector markerPosition = GetMarkerPosition(markerProperties, markerIndex);
			
			// Update marker position on screen
			UpdateMarkerScreenPosition(markerWidget, markerPosition);

			markerIndex++;
		}

		// Show faction-colored player icons on the map while the local player is spectating
		COA_UpdateSpectatorPlayerIcons();
	}

	//------------------------------------------------------------------------------------------------
	// Creates/updates faction-colored icons for every player-controlled character, but only while
	// the local player is spectating - regular players must never see enemy positions on the map.
	protected void COA_UpdateSpectatorPlayerIcons()
	{
		if (!COA_EntityHelper.IsSpectator())
		{
			COA_ClearSpectatorPlayerIcons();
			return;
		}

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return;

		map<int, ref COA_SlotData> slotMap = slottingManager.GetSlotMap();
		if (!slotMap)
			return;

		m_aSpectatorPlayerIconsLive.Clear();

		foreach (int slotId, COA_SlotData slotData : slotMap)
		{
			RplId charRplId = slotData.GetSlotCurrentCharacter();
			if (charRplId == RplId.Invalid() || !Replication.FindItem(charRplId))
				continue;

			RplComponent charRpl = RplComponent.Cast(Replication.FindItem(charRplId));
			if (!charRpl)
				continue;

			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(charRpl.GetEntity());
			if (!character)
				continue;

			SCR_Faction faction = SCR_Faction.Cast(character.GetFaction());
			if (!faction || !COA_ShouldShowSpectatorPlayerIcon(faction))
				continue;

			m_aSpectatorPlayerIconsLive.Insert(charRplId);

			COA_SpectatorPlayerIconData iconData = m_mSpectatorPlayerIcons.Get(charRplId);
			if (!iconData)
			{
				iconData = COA_CreateSpectatorPlayerIcon(character, faction);
				if (!iconData)
					continue;

				m_mSpectatorPlayerIcons.Set(charRplId, iconData);
			}

			// Refreshes the name label plus wounded/dead state using the same logic as the 3D icons
			if (iconData.m_Icon)
				iconData.m_Icon.UpdateLabel();

			COA_PositionSpectatorPlayerIcon(iconData, character);
		}

		// Remove icons for characters that no longer qualify (disconnected, respawned, faction hidden, etc.)
		m_aSpectatorPlayerIconsToRemove.Clear();
		foreach (RplId rplId, COA_SpectatorPlayerIconData data : m_mSpectatorPlayerIcons)
		{
			if (m_aSpectatorPlayerIconsLive.Find(rplId) == -1)
				m_aSpectatorPlayerIconsToRemove.Insert(rplId);
		}

		foreach (RplId rplId : m_aSpectatorPlayerIconsToRemove)
		{
			COA_SpectatorPlayerIconData data = m_mSpectatorPlayerIcons.Get(rplId);
			if (data && data.m_wRoot)
				data.m_wRoot.RemoveFromHierarchy();

			m_mSpectatorPlayerIcons.Remove(rplId);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Respects the gamemode's "hide other spectator factions" setting, matching COA_SpectatorMenu's
	// 3D floating icons so the map overlay can't leak intel that setting is meant to hide.
	protected bool COA_ShouldShowSpectatorPlayerIcon(Faction faction)
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode || !gamemode.m_bHideOtherSpectatorFactions)
			return true;

		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		Faction localPlayerFaction = COA_SlottingManager.GetInstance().GetPlayerSlotFaction(localPlayerId);
		if (!localPlayerFaction)
			return true;

		return faction == localPlayerFaction;
	}

	//------------------------------------------------------------------------------------------------
	// Instantiates the same layout/component used by the 3D spectator icons (proven to render
	// correctly - faction colors, character icon, wounded/dead state) and repurposes it as a flat
	// map pin. A hand-built ImageWidget rendered as an untextured black square, so this reuses a
	// real widget resource instead, same as PlayableSelector's marker icons do.
	protected COA_SpectatorPlayerIconData COA_CreateSpectatorPlayerIcon(SCR_ChimeraCharacter character, SCR_Faction faction)
	{
		if (!m_RootWidget)
			return null;

		Widget root = GetGame().GetWorkspace().CreateWidgets("{68625BAD23CEE68F}UI/Spectator/SpectatorLabelIconCharacter.layout", m_RootWidget);
		if (!root)
			return null;

		COA_SpectatorLabelIconCharacter icon = COA_SpectatorLabelIconCharacter.Cast(root.FindHandler(COA_SpectatorLabelIconCharacter));
		if (!icon)
		{
			root.RemoveFromHierarchy();
			return null;
		}

		// Sets up faction colors, character portrait icon and label text via existing logic
		icon.SetEntity(character, "Spine3");

		// Clicking the icon follows that player in third-person orbit and closes the map. Reuses
		// the icon's existing MMB-follow handler (COA_SpectatorLabelIconCharacter.OnMMBClicked)
		// as the click action, so this needs the active spectator menu wired in first.
		COA_SpectatorMenu specMenu = COA_SpectatorMenu.Cast(GetGame().GetMenuManager().GetTopMenu());
		if (specMenu)
			icon.SetSpectatorMenu(specMenu);

		Widget labelButton = root.FindAnyWidget("LabelButton");
		if (labelButton)
		{
			// Recenter the colored circle (LabelButton) around the root's local origin
			FrameSlot.SetSize(labelButton, COA_PLAYER_ICON_SIZE, COA_PLAYER_ICON_SIZE);
			FrameSlot.SetPos(labelButton, -COA_PLAYER_ICON_SIZE * 0.5, -COA_PLAYER_ICON_SIZE * 0.5);

			SCR_ButtonTextComponent button = icon.GetButton();
			if (specMenu && button)
			{
				button.m_OnClicked.Insert(icon.OnMMBClicked);
				button.m_OnClicked.Insert(COA_OnSpectatorPlayerIconClicked);
			}
		}

		// Recenter the character portrait to the same size/position as the circle so it renders
		// directly over it, instead of sitting at its static top-left layout offset.
		Widget portraitIcon = root.FindAnyWidget("SpectatorLabelIcon");
		if (portraitIcon)
		{
			FrameSlot.SetSize(portraitIcon, COA_PLAYER_ICON_SIZE, COA_PLAYER_ICON_SIZE);
			FrameSlot.SetPos(portraitIcon, -COA_PLAYER_ICON_SIZE * 0.5, -COA_PLAYER_ICON_SIZE * 0.5);
			portraitIcon.SetZOrder(10);
		}

		// HandlerAttached() hides the icon until its own Update() runs; we position/refresh it
		// manually instead (its Update() projects via the 3D free camera), so show it now.
		root.SetOpacity(1.0);
		root.SetEnabled(true);

		COA_SpectatorPlayerIconData data = new COA_SpectatorPlayerIconData();
		data.m_wRoot = root;
		data.m_wLabel = root.FindAnyWidget("SpectatorLabel");
		data.m_Icon = icon;

		// Match the name text color to the icon's own faction fill color
		if (faction)
		{
			RichTextWidget nameText = RichTextWidget.Cast(root.FindAnyWidget("SpectatorLabelText"));
			if (nameText)
				nameText.SetColor(faction.GetFactionColor());
		}

		return data;
	}

	//------------------------------------------------------------------------------------------------
	// Fires alongside COA_SpectatorLabelIconCharacter.OnMMBClicked (which moves the camera onto the
	// clicked player in third-person orbit) to also close the map, mirroring the map toggle key.
	protected void COA_OnSpectatorPlayerIconClicked()
	{
		COA_SpectatorMenu specMenu = COA_SpectatorMenu.Cast(GetGame().GetMenuManager().GetTopMenu());
		if (specMenu)
			specMenu.CloseMap();
	}

	//------------------------------------------------------------------------------------------------
	// Converts the character's world position to map screen space, matching
	// UpdateMarkerScreenPosition's approach, and keeps the player's name label centered above the icon.
	protected void COA_PositionSpectatorPlayerIcon(COA_SpectatorPlayerIconData data, SCR_ChimeraCharacter character)
	{
		if (!data || !data.m_wRoot || !m_MapEntity || !character)
			return;

		vector worldPos = character.GetOrigin();

		int screenX, screenY;
		m_MapEntity.WorldToScreen(worldPos[0], worldPos[2], screenX, screenY, true);

		float screenXD = GetGame().GetWorkspace().DPIUnscale(screenX);
		float screenYD = GetGame().GetWorkspace().DPIUnscale(screenY);

		// The circle/portrait are already centered on the root's local origin (see
		// COA_CreateSpectatorPlayerIcon), so the root itself sits directly on the screen point.
		FrameSlot.SetPos(data.m_wRoot, screenXD, screenYD);

		if (!data.m_wLabel)
			return;

		float labelSizeX, labelSizeY;
		data.m_wLabel.GetScreenSize(labelSizeX, labelSizeY);
		float labelSizeXD = GetGame().GetWorkspace().DPIUnscale(labelSizeX);
		float labelSizeYD = GetGame().GetWorkspace().DPIUnscale(labelSizeY);

		// Sit the label's bottom edge just above the icon's top edge (icon top = -half its size)
		const float COA_PLAYER_LABEL_GAP = 2.0;
		FrameSlot.SetPos(data.m_wLabel, -labelSizeXD * 0.5, -(COA_PLAYER_ICON_SIZE * 0.5) - COA_PLAYER_LABEL_GAP - labelSizeYD);
	}

	//------------------------------------------------------------------------------------------------
	// Removes all spectator player icon widgets, e.g. when the map closes or spectating ends.
	protected void COA_ClearSpectatorPlayerIcons()
	{
		foreach (RplId rplId, COA_SpectatorPlayerIconData data : m_mSpectatorPlayerIcons)
		{
			if (data && data.m_wRoot)
				data.m_wRoot.RemoveFromHierarchy();
		}

		m_mSpectatorPlayerIcons.Clear();
	}
	
	//------------------------------------------------------------------------------------------------
	// Calculates marker position based on marker properties
	protected vector GetMarkerPosition(array<string> markerProperties, int markerIndex)
	{
		vector position;
		
		// Static markers use direct position
		if(markerProperties[0] == "Static Marker") 
		{
			position = markerProperties[1].ToVector();
			return position;
		}
		
		// Entity-based markers
		IEntity entity = GetGame().GetWorld().FindEntityByName(markerProperties[0]);
		if (!entity)
			return vector.Zero;
		
		float updateInterval = markerProperties[2].ToFloat();
		vector offset = markerProperties[1].ToVector();
		
		// If marker has an update interval, handle caching logic
		if(updateInterval > 0) 
		{
			float currentTime = GetGame().GetWorld().GetWorldTime();
			
			// Check if it's time to update the cached position
			if (!m_aMarkerUpdateTimes.IsIndexValid(markerIndex) || currentTime >= m_aMarkerUpdateTimes.Get(markerIndex)) 
			{
				float nextUpdateTime = currentTime + (updateInterval * 1000);
				
				// Store next update time
				if (m_aMarkerUpdateTimes.IsIndexValid(markerIndex))
					m_aMarkerUpdateTimes.Set(markerIndex, nextUpdateTime);
				else
					m_aMarkerUpdateTimes.Insert(nextUpdateTime);
				
				// Calculate and cache new position
				position = entity.GetOrigin() + offset;
				
				if (m_aCachedPositions.IsIndexValid(markerIndex))
					m_aCachedPositions.Set(markerIndex, position);
				else
					m_aCachedPositions.Insert(position);
			} 
			else 
			{
				// Use cached position if available
				if (m_aCachedPositions.IsIndexValid(markerIndex))
					position = m_aCachedPositions.Get(markerIndex);
				else
					position = entity.GetOrigin() + offset;
			}
		} 
		else 
		{
			// No update interval, get current position
			position = entity.GetOrigin() + offset;
		}
		
		return position;
	}
	
	//------------------------------------------------------------------------------------------------
	// Updates marker widget position on screen based on world position
	protected void UpdateMarkerScreenPosition(Widget marker, vector worldPos)
	{
		int screenX;
		int screenY;
		
		// Convert world position to screen coordinates
		m_MapEntity.WorldToScreen(worldPos[0], worldPos[2], screenX, screenY, true);
		
		// Apply DPI scaling
		screenX = GetGame().GetWorkspace().DPIUnscale(screenX);
		screenY = GetGame().GetWorkspace().DPIUnscale(screenY);
			
		// Set widget position
		FrameSlot.SetPos(marker, screenX, screenY);
	}

	//------------------------------------------------------------------------------------------------
	// Disable quick radial marker menu opening. Marker placement is handled by double-left-click.
	override protected void OnInputQuickMarkerMenu(float value, EActionTrigger reason)
	{
		return;
	}

	//------------------------------------------------------------------------------------------------
	// Double-left-click on empty map opens marker placement dialog at click position.
	override protected void OnInputMapSelect(float value, EActionTrigger reason)
	{
		super.OnInputMapSelect(value, reason);

		if (m_MarkerEditRoot)
		{
			ResetDoubleClickState();
			return;
		}

		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager || !inputManager.IsUsingMouseAndKeyboard())
			return;

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity)
			return;

		if (!m_CursorModule)
			return;

		if ((m_CursorModule.GetCursorState() & SCR_MapCursorModule.STATE_POPUP_RESTRICTED) != 0)
			return;

		if ((m_CursorModule.GetCursorState() & EMapCursorState.CS_PAN) != 0)
			return;

		if (IsMarkerUnderCursor())
		{
			ResetDoubleClickState();
			return;
		}

		float worldX, worldY;
		mapEntity.GetMapCursorWorldPosition(worldX, worldY);

		ChimeraWorld world = GetGame().GetWorld();
		if (!world)
			return;

		float now = world.GetWorldTime();
		bool isDoubleClick = false;

		if (m_fLastMapSelectTime >= 0)
		{
			float deltaTime = now - m_fLastMapSelectTime;
			float dx = worldX - m_fLastMapClickWorldX;
			float dy = worldY - m_fLastMapClickWorldY;
			float distSq = dx * dx + dy * dy;
			float toleranceSq = COA_DOUBLE_CLICK_WORLD_TOLERANCE * COA_DOUBLE_CLICK_WORLD_TOLERANCE;

			isDoubleClick = deltaTime <= COA_DOUBLE_CLICK_MS && distSq <= toleranceSq;
		}

		if (!isDoubleClick)
		{
			m_fLastMapSelectTime = now;
			m_fLastMapClickWorldX = worldX;
			m_fLastMapClickWorldY = worldY;
			return;
		}

		ResetDoubleClickState();

		float screenX, screenY;
		mapEntity.WorldToScreen(worldX, worldY, screenX, screenY);
		mapEntity.PanSmooth(screenX, screenY, 0.05);

		if (!m_PlacedMarkerConfig)
		{
			if (!m_MarkerMgr)
				return;

			SCR_MapMarkerConfig markerConfig = m_MarkerMgr.GetMarkerConfig();
			if (!markerConfig)
				return;

			m_PlacedMarkerConfig = SCR_MapMarkerEntryPlaced.Cast(markerConfig.GetMarkerEntryConfigByType(SCR_EMapMarkerType.PLACED_CUSTOM));
			if (!m_PlacedMarkerConfig)
				return;
		}

		CreateMarkerEditDialog();
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsMarkerUnderCursor()
	{
		array<Widget> widgets = SCR_MapCursorModule.GetMapWidgetsUnderCursor();
		foreach (Widget widget : widgets)
		{
			SCR_MapMarkerWidgetComponent markerComp = SCR_MapMarkerWidgetComponent.Cast(widget.FindHandler(SCR_MapMarkerWidgetComponent));
			if (markerComp)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected void ResetDoubleClickState()
	{
		m_fLastMapSelectTime = -1;
		m_fLastMapClickWorldX = 0;
		m_fLastMapClickWorldY = 0;
	}

	//------------------------------------------------------------------------------------------------
	// True while a marker edit dialog (custom or military) is open. Other widgets use this to
	// avoid stealing focus off the marker text field just because the mouse hovers over them -
	// see COA_SCR_ButtonBaseComponent.c / COA_SCR_SliderComponent.c / COA_SCR_EditBoxComponent.c.
	static bool COA_IsMarkerEditDialogOpen()
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity)
			return false;

		SCR_MapMarkersUI markersUI = SCR_MapMarkersUI.Cast(mapEntity.GetMapUIComponent(SCR_MapMarkersUI));
		return markersUI && markersUI.m_MarkerEditRoot != null;
	}
}
