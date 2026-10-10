/**
 * Floating squad marker above the group leader in spectator: the faction NATO symbol with the squad
 * name above ("alive/total" is added while the marker is hovered).
 * Layout: UI/Spectator/SpectatorLabelIconGroup.layout
 */
class COA_SpectatorLabelIconGroup : COA_SpectatorLabelIcon
{
	//------------------------------------------------------------------------------------------------
	// Class member variables
	//------------------------------------------------------------------------------------------------
	
	// Group reference
	protected SCR_AIGroup m_Group;
	
	// UI widgets
	protected ImageWidget m_wGroupNATOIcon;
	protected ImageWidget m_wGroupNATOBackground;

	// Strength (alive / slotted members), refreshed once a second, shown only while hovered
	protected int m_iAlive;
	protected int m_iTotal;
	protected float m_fNextLabelRefresh;
	protected bool m_bHovered;

	// Cached raw centroid (before height offset)
	protected vector m_vRawCentroid;
	
	// Group display settings
	protected static const float GROUP_ICON_HEIGHT_OFFSET_MIN = 6.0;
	protected static const float GROUP_ICON_HEIGHT_OFFSET_MAX = 65.0;
	
	/*
	// Maximum distance the group icon can be from the group's median position (core group)
	// If the centroid calculation results in a position further than this from the median,
	// the icon will be clamped to this distance to prevent it from appearing too far away
	protected static const float MAX_CENTROID_DISTANCE_FROM_MEDIAN = 150.0;
	*/
	
	// Fade-out when the camera is very close to the group — individual character icons
	// are clearly visible at short range, so the group symbol becomes redundant noise.
	protected static const float GROUP_ICON_FADE_START = 20.0; 
	protected static const float GROUP_ICON_FADE_END   = 10.0;
	
	// Label fade distances — the group name fades out when the camera gets close.
	// Label is fully visible beyond GROUP_LABEL_FADE_START and hidden below GROUP_LABEL_FADE_END.
	protected static const float GROUP_LABEL_FADE_START = 60.0;
	protected static const float GROUP_LABEL_FADE_END   = 30.0;
	
	// Icon sizing — group icons are larger than individual character icons so they are
	// easily visible from a distance but don't overwhelm individual unit icons.
	// scaledIconSize drives the HEIGHT; width is derived from the NATO 3:2 aspect ratio.
	protected static const float GROUP_MAX_ICON_SIZE = 60.0;
	protected static const float GROUP_MIN_ICON_SIZE = 42.0;
	// Every faction's group flag imageset (BLUFOR, OPFOR, INDFOR, Civilian) uses 32x20 cells, with the
	// frame shape drawn inside that cell - a different ratio squashes or stretches it (1.5 made the
	// hostile diamond look slim)
	protected static const float GROUP_ICON_ASPECT_RATIO = 1.6;
	protected static const float GROUP_LABEL_GAP = 3.0;		// between the squad name and the symbol

	//------------------------------------------------------------------------------------------------
	// Widget initialization
	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		// Find group-specific widget references
		m_wGroupNATOIcon = ImageWidget.Cast(w.FindAnyWidget("GroupNATOIcon"));
		m_wGroupNATOBackground = ImageWidget.Cast(w.FindAnyWidget("GroupNATOBackground"));

		super.HandlerAttached(w);
	}
	
	//------------------------------------------------------------------------------------------------
	// Set the group associated with this label
	//------------------------------------------------------------------------------------------------
	void SetGroup(SCR_AIGroup group)
	{
		m_Group = group;
		
		if (!group)
			return;
		
		// Apply group-specific icon size — larger than individual character icons
		m_fMaxIconSize = GROUP_MAX_ICON_SIZE;
		m_fMinIconSize = GROUP_MIN_ICON_SIZE;
		
		// Set faction colors
		SetupFactionColors();
		
		// Load group NATO icon
		LoadGroupIcon();
		
		// Set group name
		SetGroupName();
	}
	
	//------------------------------------------------------------------------------------------------
	//! True while the cursor is on this (visible) squad marker - drives the member list card
	bool IsHoveredNow()
	{
		return m_bHovered && m_wRoot && m_wRoot.IsEnabled() && m_wRoot.GetOpacity() > 0;
	}

	//------------------------------------------------------------------------------------------------
	// Get the group reference
	//------------------------------------------------------------------------------------------------
	SCR_AIGroup GetGroup()
	{
		return m_Group;
	}
	
	//------------------------------------------------------------------------------------------------
	// Setup faction-based colors for the icon
	//------------------------------------------------------------------------------------------------
	protected void SetupFactionColors()
	{
		if (!m_Group)
			return;
		
		SCR_Faction faction = SCR_Faction.Cast(m_Group.GetFaction());
		if (!faction)
			return;
		
		// Hide the base circle icon — the NATO overlay replaces it entirely
		if (m_wSpectatorLabelIcon)
			m_wSpectatorLabelIcon.SetVisible(false);
		
		// Match the character icon color scheme:
		// GetFactionColor()        = solid fill color (used for background/icon body)
		// GetOutlineFactionColor() = outline/highlight color
		if (m_wGroupNATOIcon)
			m_wGroupNATOIcon.SetColor(faction.GetFactionColor());


		// Hide the separate background box — the NATO imageset images already contain the
		// correct background shape (rectangle, circle, etc.) baked into each image
		if (m_wGroupNATOBackground)
			m_wGroupNATOBackground.SetVisible(false);
	}
	
	//------------------------------------------------------------------------------------------------
	// Load the group's NATO flag icon
	//------------------------------------------------------------------------------------------------
	protected void LoadGroupIcon()
	{
		if (!m_Group || !m_wGroupNATOIcon)
			return;
		
		SCR_Faction faction = SCR_Faction.Cast(m_Group.GetFaction());
		if (!faction)
			return;
		
		ResourceName imageSet = faction.GetGroupFlagImageSet();
		string imageName = m_Group.GetGroupFlag();
		
		if (imageSet == "" || imageName == "")
			return;

		m_wGroupNATOIcon.LoadImageFromSet(0, imageSet, imageName);
	}
	
	//------------------------------------------------------------------------------------------------
	// Set the group name text
	//------------------------------------------------------------------------------------------------
	protected void SetGroupName()
	{
		if (!m_Group || !m_wSpectatorLabelText)
			return;
		
		string groupName = m_Group.GetCustomName();
		if (groupName.IsEmpty())
			groupName = m_Group.GetCustomNameWithOriginal();

		if (m_bHovered && m_iTotal > 0)
			groupName += string.Format("  <color rgba='170,180,204,255'>%1/%2</color>", m_iAlive, m_iTotal);

		m_wSpectatorLabelText.SetText(groupName);
	}

	//------------------------------------------------------------------------------------------------
	//! True while the cursor is over any part of this marker (plate, symbol, pointer or name)
	protected bool IsHovered()
	{
		Widget underCursor = WidgetManager.GetWidgetUnderCursor();
		while (underCursor)
		{
			if (underCursor == m_wRoot)
				return true;

			underCursor = underCursor.GetParent();
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	// Alive / slotted members of this group, from the slot map (players are not in GetAgents())
	//------------------------------------------------------------------------------------------------
	protected void RefreshStrength()
	{
		m_iAlive = 0;
		m_iTotal = 0;

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager || !m_Group)
			return;

		RplComponent groupRpl = RplComponent.Cast(m_Group.FindComponent(RplComponent));
		map<int, ref COA_SlotData> slotMap = slottingManager.GetSlotMap();
		if (!groupRpl || !slotMap)
			return;

		RplId groupId = groupRpl.Id();
		foreach (int slotId, COA_SlotData slotData : slotMap)
		{
			if (!slotData || slotData.GetSlotCurrentGroup() != groupId)
				continue;

			SCR_ChimeraCharacter character = COA_EntityHelper.GetCharacterFromRplId(slotData.GetSlotCurrentCharacter());
			if (!character)
				continue;

			m_iTotal++;
			if (COA_DamageHelper.CheckIfEntityAlive(character))
				m_iAlive++;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Override SetEntity - groups track the leader entity
	//------------------------------------------------------------------------------------------------
	override void SetEntity(IEntity entity, string boneName)
	{
		// Icon position is calculated from the group leader each frame in Update()
	}
	
	//------------------------------------------------------------------------------------------------
	// Override Update to position icon at group leader
	//------------------------------------------------------------------------------------------------
	override void Update()
	{
		if (!m_Group)
			return;
		
		// Get the group leader's player ID
		int leaderID = m_Group.GetLeaderID();
		if (leaderID <= 0)
		{
			if (m_wRoot)
			{
				m_wRoot.SetOpacity(0.0);
				m_wRoot.SetEnabled(false);
			}
			return;
		}
		
		// Get the leader entity from the player manager
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
		{
			if (m_wRoot)
			{
				m_wRoot.SetOpacity(0.0);
				m_wRoot.SetEnabled(false);
			}
			return;
		}
		
		IEntity leaderEntity = playerManager.GetPlayerControlledEntity(leaderID);
		if (!leaderEntity)
		{
			if (m_wRoot)
			{
				m_wRoot.SetOpacity(0.0);
				m_wRoot.SetEnabled(false);
			}
			return;
		}
		
		// Check if leader is alive - hide icon if leader is dead
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(leaderEntity.FindComponent(SCR_CharacterControllerComponent));
		if (controller && controller.IsDead())
		{
			if (m_wRoot)
			{
				m_wRoot.SetOpacity(0.0);
				m_wRoot.SetEnabled(false);
			}
			return;
		}
		
		// Get leader position
		vector leaderPosition = leaderEntity.GetOrigin();
		
		// Calculate distance from camera to the leader position (before height offset)
		vector cameraPosition = GetGame().GetCameraManager().CurrentCamera().GetOrigin();
		m_fDistanceToIcon = vector.Distance(cameraPosition, leaderPosition);
		
		// Scale the height offset with distance so the icon floats higher when zoomed out
		float distanceFraction = Math.Clamp(m_fDistanceToIcon / m_fMaxIconDistance, 0.0, 1.0);
		float heightOffset = GROUP_ICON_HEIGHT_OFFSET_MIN + (GROUP_ICON_HEIGHT_OFFSET_MAX - GROUP_ICON_HEIGHT_OFFSET_MIN) * distanceFraction;
		
		// Cache the raw position for line drawing before applying the icon height offset
		m_vRawCentroid = leaderPosition;
		
		// Raise the icon above the leader by the distance-scaled offset
		leaderPosition[1] = leaderPosition[1] + heightOffset;
		m_vWorldPosition = leaderPosition;
		
		// Project 3D world position to 2D screen coordinates
		vector screenPosition = GetGame().GetWorkspace().ProjWorldToScreen(m_vWorldPosition, GetGame().GetWorld());
		
		// Hide if behind camera or too far away
		if (screenPosition[2] < 0 || m_fDistanceToIcon > m_fMaxIconDistance)
		{
			m_wRoot.SetOpacity(0.0);
			m_wRoot.SetEnabled(false); // Disable so the invisible widget cannot block player clicks
			return;
		}
		
		// Fade out when the camera is very close — individual character icons are
		// already clearly readable at short range so the group symbol adds clutter.
		if (m_fDistanceToIcon <= GROUP_ICON_FADE_END)
		{
			m_wRoot.SetOpacity(0.0);
			m_wRoot.SetEnabled(false); // Disable so the invisible widget cannot block player clicks
			return;
		}
		float closeOpacity = 1.0;
		if (m_fDistanceToIcon < GROUP_ICON_FADE_START)
			closeOpacity = (m_fDistanceToIcon - GROUP_ICON_FADE_END) / (GROUP_ICON_FADE_START - GROUP_ICON_FADE_END);
		
		// Icon is visible — ensure it is enabled before rendering
		m_wRoot.SetEnabled(true);
		m_wRoot.SetOpacity(closeOpacity);
		
		// Update label content
		UpdateLabel();
		
		// Update label visibility based on distance
		UpdateLabelVisibility();
		
		// Update icon size and position based on distance
		UpdateIconAppearance(screenPosition);
		
		// Handle forced name display (when mouse hovers)
		if (m_bForceShowName)
		{
			m_wSpectatorLabel.SetOpacity(1.0);
			m_wRoot.SetZOrder(100);
		}
	}
	
	/*
	//------------------------------------------------------------------------------------------------
	// Calculate the centroid position from alive group members.
	// Uses the slot map to find player-controlled characters, because human players are not
	// returned by SCR_AIGroup.GetAgents() (which only yields AI agents).
	// Also falls back to GetAgents() for any pure-AI members.
	// NOTE: THIS SHIT IS NOT PERFORMANT. THE BEST CASE SCENARIO WOULD BE TO SET IT TO THE LEADER EACH FRAME WHICH IS WHAT WE'RE DOING NOW.
	//------------------------------------------------------------------------------------------------
	protected bool CalculateGroupCentroid(out vector centroid, out int aliveCount)
	{
		centroid = vector.Zero;
		aliveCount = 0;
		
		// Get the group's replication ID so we can match it against slot data
		RplComponent groupRpl = RplComponent.Cast(m_Group.FindComponent(RplComponent));
		if (!groupRpl)
			return false;
		
		RplId groupRplId = groupRpl.Id();
		
		// --- Player-controlled characters via slot map ---
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (slottingManager)
		{
			map<int, ref COA_SlotData> slotMap = slottingManager.GetSlotMap();
			if (slotMap)
			{
				foreach (int slotId, COA_SlotData slotData : slotMap)
				{
					if (!slotData || slotData.GetSlotCurrentGroup() != groupRplId)
						continue;
					
					RplId charRplId = slotData.GetSlotCurrentCharacter();
					if (charRplId == RplId.Invalid())
						continue;
					
					RplComponent charRpl = RplComponent.Cast(Replication.FindItem(charRplId));
					if (!charRpl)
						continue;
					
					IEntity entity = charRpl.GetEntity();
					if (!entity)
						continue;
					
					// Check alive state
					SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(entity.FindComponent(SCR_CharacterControllerComponent));
					if (controller && controller.IsDead())
						continue;
					
					centroid = centroid + entity.GetOrigin();
					aliveCount++;
				}
			}
		}
		
		// --- AI-controlled characters via GetAgents() (pure-AI groups) ---
		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		
		foreach (AIAgent agent : agents)
		{
			IEntity entity = agent.GetControlledEntity();
			if (!entity)
				continue;
			
			ChimeraCharacter character = ChimeraCharacter.Cast(entity);
			if (!character)
				continue;
			
			CharacterControllerComponent controller = character.GetCharacterController();
			if (!controller || controller.GetLifeState() == ECharacterLifeState.DEAD)
				continue;
			
			// Avoid double-counting if this AI entity was already added via slot map
			bool alreadyCounted = false;
			RplComponent entityRpl = RplComponent.Cast(entity.FindComponent(RplComponent));
			if (entityRpl && slottingManager)
			{
				COA_SlotData slotData = slottingManager.GetSlotDataFromCharacter(entityRpl.Id());
				if (slotData && slotData.GetSlotCurrentGroup() == groupRplId)
					alreadyCounted = true;
			}
			
			if (!alreadyCounted)
			{
				centroid = centroid + entity.GetOrigin();
				aliveCount++;
			}
		}
		
		if (aliveCount == 0)
			return false;
		
		centroid = centroid * (1.0 / aliveCount);
		
		// Clamp outliers: if any individual member pulls the centroid too far from the main group,
		// limit the centroid's deviation. This prevents the icon from appearing in dead space when
		// the leader or a single member is separated from the bulk of the squad.
		//
		// Strategy: Calculate the median position to find where the "core group" actually is,
		// then clamp the centroid to stay within MAX_CENTROID_DISTANCE_FROM_LEADER of that core.
		
		vector medianPosition = CalculateMedianPosition(groupRplId);
		if (medianPosition != vector.Zero)
		{
			vector centroidToMedian = centroid - medianPosition;
			float distanceToMedian = centroidToMedian.Length();
			
			// If centroid is too far from the median (core group), clamp it
			if (distanceToMedian > MAX_CENTROID_DISTANCE_FROM_MEDIAN)
			{
				vector direction = centroidToMedian.Normalized();
				centroid = medianPosition + (direction * MAX_CENTROID_DISTANCE_FROM_MEDIAN);
			}
		}
		
		return true;
	}
	*/
	
	/*
	//------------------------------------------------------------------------------------------------
	// Calculate the median position of the group to identify where the "core" of the group is.
	// This is more robust than using just the leader's position, especially when the leader
	// is separated, dead, or respawning far from the main group.
	//------------------------------------------------------------------------------------------------
	protected vector CalculateMedianPosition(RplId groupRplId)
	{
		array<vector> positions = {};
		
		// Collect all alive member positions
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (slottingManager)
		{
			map<int, ref COA_SlotData> slotMap = slottingManager.GetSlotMap();
			if (slotMap)
			{
				foreach (int slotId, COA_SlotData slotData : slotMap)
				{
					if (!slotData || slotData.GetSlotCurrentGroup() != groupRplId)
						continue;
					
					RplId charRplId = slotData.GetSlotCurrentCharacter();
					if (charRplId == RplId.Invalid())
						continue;
					
					RplComponent charRpl = RplComponent.Cast(Replication.FindItem(charRplId));
					if (!charRpl)
						continue;
					
					IEntity entity = charRpl.GetEntity();
					if (!entity)
						continue;
					
					SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(entity.FindComponent(SCR_CharacterControllerComponent));
					if (controller && controller.IsDead())
						continue;
					
					positions.Insert(entity.GetOrigin());
				}
			}
		}
		
		// Add AI members
		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		
		foreach (AIAgent agent : agents)
		{
			IEntity entity = agent.GetControlledEntity();
			if (!entity)
				continue;
			
			ChimeraCharacter character = ChimeraCharacter.Cast(entity);
			if (!character)
				continue;
			
			CharacterControllerComponent controller = character.GetCharacterController();
			if (!controller || controller.GetLifeState() == ECharacterLifeState.DEAD)
				continue;
			
			// Avoid duplicates
			bool alreadyCounted = false;
			RplComponent entityRpl = RplComponent.Cast(entity.FindComponent(RplComponent));
			if (entityRpl && slottingManager)
			{
				COA_SlotData slotData = slottingManager.GetSlotDataFromCharacter(entityRpl.Id());
				if (slotData && slotData.GetSlotCurrentGroup() == groupRplId)
					alreadyCounted = true;
			}
			
			if (!alreadyCounted)
				positions.Insert(entity.GetOrigin());
		}
		
		if (positions.IsEmpty())
			return vector.Zero;
		
		// Calculate median by taking the middle value when sorted by distance from origin
		// For simplicity, we'll approximate by finding the position closest to the average
		// (a true median would require sorting in 3D space, which is complex)
		
		// Calculate the average first
		vector sum = vector.Zero;
		foreach (vector pos : positions)
		{
			sum = sum + pos;
		}
		vector average = sum * (1.0 / positions.Count());
		
		// Find the position closest to the average (this approximates the median/center of mass)
		vector closestToAverage = positions[0];
		float minDistance = vector.Distance(average, closestToAverage);
		
		foreach (vector pos : positions)
		{
			float distance = vector.Distance(average, pos);
			if (distance < minDistance)
			{
				minDistance = distance;
				closestToAverage = pos;
			}
		}
		
		return closestToAverage;
	}
	*/
	
	//------------------------------------------------------------------------------------------------
	// Update the label - refresh group name (may change during play)
	//------------------------------------------------------------------------------------------------
	override void UpdateLabel()
	{
		super.UpdateLabel();

		// Name and strength change rarely; once a second is plenty
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fNextLabelRefresh)
			return;

		m_fNextLabelRefresh = now + 1000;
		RefreshStrength();
		SetGroupName();
	}
	
	//------------------------------------------------------------------------------------------------
	// The flag and name share one rounded card, so there is no separate label to fade: Update()
	// already fades the whole icon out when the camera gets close to the group
	//------------------------------------------------------------------------------------------------
	override protected void UpdateLabelVisibility()
	{
		m_wSpectatorLabel.SetOpacity(1.0);
	}
	
	//------------------------------------------------------------------------------------------------
	// Symbol centred on the projected point (growing as the camera closes in), name above (strength
	// added while hovered)
	//------------------------------------------------------------------------------------------------
	override protected void UpdateIconAppearance(vector screenPosition)
	{
		float t = 1.0 - (m_fDistanceToIcon - m_fMinIconDistance) / (m_fMaxIconDistance - m_fMinIconDistance);
		t = Math.Clamp(t, 0.0, 1.0);
		float iconH = m_fMinIconSize + (m_fMaxIconSize - m_fMinIconSize) * t;
		float iconW = iconH * GROUP_ICON_ASPECT_RATIO;

		PlaceCentered(m_wGroupNATOIcon, 0, iconW, iconH);

		// Strength (alive / total) only shows while the marker is hovered
		bool hovered = IsHovered();
		if (hovered != m_bHovered)
		{
			m_bHovered = hovered;
			SetGroupName();
		}

		float labelSizeX, labelSizeY;
		m_wSpectatorLabel.GetScreenSize(labelSizeX, labelSizeY);
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		float labelW = workspace.DPIUnscale(labelSizeX);
		float labelH = workspace.DPIUnscale(labelSizeY);
		FrameSlot.SetPos(m_wSpectatorLabel, -labelW * 0.5, -iconH * 0.5 - GROUP_LABEL_GAP - labelH);

		FrameSlot.SetPos(m_wRoot, screenPosition[0], screenPosition[1]);

		// Closer objects on top, but behind character icons
		m_wRoot.SetZOrder(screenPosition[2] * -10000 - 1);
	}

	//------------------------------------------------------------------------------------------------
	//! Sizes a widget and centres it horizontally on the icon, vertically on centreY
	protected static void PlaceCentered(Widget w, float centreY, float width, float height)
	{
		if (!w)
			return;

		FrameSlot.SetSize(w, width, height);
		FrameSlot.SetPos(w, -width * 0.5, centreY - height * 0.5);
	}
}
