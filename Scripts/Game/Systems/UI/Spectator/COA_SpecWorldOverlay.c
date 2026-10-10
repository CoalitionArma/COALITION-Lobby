//------------------------------------------------------------------------------------------------
//! Hover card for the in-world spectator markers (player dots, squad markers, vehicle markers).
//! Uses the spectator map's card layout (COA_SpecMapTooltip.layout) so both look the same.
//! Owned by COA_SpectatorMenu; created inside IconsFrame so it shares the markers' coordinates.
class COA_SpecHoverCard
{
	protected static const ResourceName LAYOUT = "{6A75150000000003}UI/Spectator/COA_SpecMapTooltip.layout";
	protected static const float HEALTH_WIDTH = 240;		// TooltipWidth in the layout
	protected static const float REFRESH_MS = 250;
	protected static const float OFFSET_X = 22;
	protected static const float OFFSET_Y = -16;
	protected static const float SCREEN_MARGIN = 12;

	protected Widget m_wRoot;
	protected Widget m_wCard;
	protected TextWidget m_wTitle;
	protected TextWidget m_wSub;
	protected TextWidget m_wState;
	protected RichTextWidget m_wBody;
	protected Widget m_wHealthRow;
	protected Widget m_wHealthFill;
	protected Widget m_wAccent;

	protected Managed m_Subject;		// what the card currently describes
	protected float m_fNextRefresh;

	//------------------------------------------------------------------------------------------------
	void COA_SpecHoverCard(notnull Widget parent)
	{
		m_wRoot = GetGame().GetWorkspace().CreateWidgets(LAYOUT, parent);
		if (!m_wRoot)
			return;

		m_wRoot.SetZOrder(100000);		// above every marker (their z follows camera depth, all negative)
		m_wCard = m_wRoot.FindAnyWidget("Card");
		m_wTitle = TextWidget.Cast(m_wRoot.FindAnyWidget("Title"));
		m_wSub = TextWidget.Cast(m_wRoot.FindAnyWidget("Sub"));
		m_wState = TextWidget.Cast(m_wRoot.FindAnyWidget("State"));
		m_wBody = RichTextWidget.Cast(m_wRoot.FindAnyWidget("Body"));
		m_wHealthRow = m_wRoot.FindAnyWidget("HealthRow");
		m_wHealthFill = m_wRoot.FindAnyWidget("HealthFill");
		m_wAccent = m_wRoot.FindAnyWidget("Accent");
		m_wRoot.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	void Hide()
	{
		m_Subject = null;
		if (m_wRoot)
			m_wRoot.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	//! \param[in] anchorX / anchorY marker position (IconsFrame coordinates); the card sits to its right
	void ShowCharacter(SCR_ChimeraCharacter character, float anchorX, float anchorY)
	{
		if (!character || !m_wRoot)
			return;

		if (NeedsRefresh(character))
			FillCharacter(character);

		Place(anchorX, anchorY);
	}

	//------------------------------------------------------------------------------------------------
	void ShowSquad(SCR_AIGroup group, float anchorX, float anchorY)
	{
		if (!group || !m_wRoot)
			return;

		if (NeedsRefresh(group))
			FillSquad(group);

		Place(anchorX, anchorY);
	}

	//------------------------------------------------------------------------------------------------
	void ShowVehicle(IEntity vehicle, array<IEntity> crew, float anchorX, float anchorY)
	{
		if (!vehicle || !m_wRoot)
			return;

		if (NeedsRefresh(vehicle))
			FillVehicle(vehicle, crew);

		Place(anchorX, anchorY);
	}

	//------------------------------------------------------------------------------------------------
	//! Refill on a new subject right away, otherwise a few times a second
	protected bool NeedsRefresh(Managed subject)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (subject == m_Subject && now < m_fNextRefresh)
			return false;

		m_Subject = subject;
		m_fNextRefresh = now + REFRESH_MS;
		m_wRoot.SetVisible(true);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void FillCharacter(SCR_ChimeraCharacter character)
	{
		COA_SlotData slotData;
		RplComponent rpl = RplComponent.Cast(character.FindComponent(RplComponent));
		if (rpl && COA_SlottingManager.GetInstance())
			slotData = COA_SlottingManager.GetInstance().GetSlotDataFromCharacter(rpl.Id());

		string name, role;
		SCR_AIGroup group;
		if (slotData)
		{
			role = slotData.GetSlotName();
			int playerId = slotData.GetSlotCurrentPlayerId();
			if (playerId > 0)
				name = GetGame().GetPlayerManager().GetPlayerName(playerId);
			if (name.IsEmpty())
				name = role;
			group = COA_EntityHelper.GetGroupFromRplId(slotData.GetSlotCurrentGroup());
		}

		string sub = role;
		if (group)
			sub = SquadName(group) + "  ·  " + role;

		IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(character);
		if (vehicle)
			sub = sub + "  ·  " + VehicleName(vehicle);

		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
		bool dead = controller && controller.IsDead();
		bool uncon = !dead && controller && controller.IsUnconscious();

		string state = "Click to follow";
		if (dead)
		{
			state = "Killed in action";
			if (slotData && !slotData.GetKillerName().IsEmpty())
				state = "Killed by " + slotData.GetKillerName();
		}
		else if (uncon)
		{
			state = "Unconscious";
		}

		float health = 0;
		if (!dead)
			health = CharacterHealth(character);

		Fill(name, sub, health, true, state, "", FactionColor(character.GetFaction()));
	}

	//------------------------------------------------------------------------------------------------
	protected void FillSquad(SCR_AIGroup group)
	{
		string body;
		int alive, total;

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		RplComponent groupRpl = RplComponent.Cast(group.FindComponent(RplComponent));
		if (slottingManager && groupRpl && slottingManager.GetSlotMap())
		{
			RplId groupId = groupRpl.Id();
			foreach (int slotId, COA_SlotData slotData : slottingManager.GetSlotMap())
			{
				if (!slotData || slotData.GetSlotCurrentGroup() != groupId)
					continue;

				SCR_ChimeraCharacter character = COA_EntityHelper.GetCharacterFromRplId(slotData.GetSlotCurrentCharacter());
				if (!character)
					continue;

				int playerId = slotData.GetSlotCurrentPlayerId();
				string name;
				if (playerId > 0)
					name = GetGame().GetPlayerManager().GetPlayerName(playerId);
				if (name.IsEmpty())
					name = slotData.GetSlotName();

				total++;
				if (!body.IsEmpty())
					body += "<br/>";

				if (!COA_DamageHelper.CheckIfEntityAlive(character))
				{
					body += "<color rgba='120,128,148,255'>† " + name + "</color>";
					continue;
				}

				alive++;
				body += name;
				if (playerId > 0 && group.GetLeaderID() == playerId)
					body += "  <color rgba='120,128,148,255'>SL</color>";

				SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
				if (controller && controller.IsUnconscious())
					body += "  <color rgba='206,103,17,255'>unconscious</color>";
			}
		}

		string sub = string.Format("%1 of %2 alive", alive, total);
		Fill(SquadName(group), sub, 0, false, "", body, FactionColor(group.GetFaction()));
	}

	//------------------------------------------------------------------------------------------------
	protected void FillVehicle(IEntity vehicle, array<IEntity> crew)
	{
		string body;
		Faction faction;
		if (crew)
		{
			foreach (IEntity member : crew)
			{
				if (!member)
					continue;

				if (!faction)
				{
					SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(member);
					if (character)
						faction = character.GetFaction();
				}

				if (!body.IsEmpty())
					body += "<br/>";
				body += CharacterName(member);
			}
		}

		float health = 1;
		DamageManagerComponent damage = DamageManagerComponent.Cast(vehicle.FindComponent(DamageManagerComponent));
		if (damage)
			health = damage.GetHealthScaled();

		int count;
		if (crew)
			count = crew.Count();

		Fill(VehicleName(vehicle), string.Format("%1 aboard", count), health, true, "", body, FactionColor(faction));
	}

	//------------------------------------------------------------------------------------------------
	protected void Fill(string title, string sub, float health, bool showHealth, string state, string body, Color accent)
	{
		if (m_wTitle)
			m_wTitle.SetText(title);
		if (m_wSub)
			m_wSub.SetText(sub);
		if (m_wState)
		{
			m_wState.SetText(state);
			m_wState.SetVisible(!state.IsEmpty());
		}
		if (m_wAccent)
			m_wAccent.SetColor(accent);
		if (m_wBody)
		{
			m_wBody.SetText(body);
			m_wBody.SetVisible(!body.IsEmpty());
		}
		if (m_wHealthRow)
			m_wHealthRow.SetVisible(showHealth);

		if (showHealth && m_wHealthFill)
		{
			health = Math.Clamp(health, 0, 1);
			FrameSlot.SetSizeX(m_wHealthFill, Math.Max(HEALTH_WIDTH * health, 1));
			m_wHealthFill.SetVisible(health > 0);
			m_wHealthFill.SetColor(HealthColor(health));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Right of the marker, flipped left near the screen edge, kept on screen vertically
	protected void Place(float anchorX, float anchorY)
	{
		Widget parent = m_wRoot.GetParent();
		if (!parent || !m_wCard)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		float cardW, cardH, screenW, screenH;
		m_wCard.GetScreenSize(cardW, cardH);
		parent.GetScreenSize(screenW, screenH);
		cardW = workspace.DPIUnscale(cardW);
		cardH = workspace.DPIUnscale(cardH);
		screenW = workspace.DPIUnscale(screenW);
		screenH = workspace.DPIUnscale(screenH);

		float x = anchorX + OFFSET_X;
		if (x + cardW > screenW - SCREEN_MARGIN)
			x = anchorX - OFFSET_X - cardW;

		float y = Math.Clamp(anchorY + OFFSET_Y, SCREEN_MARGIN, Math.Max(SCREEN_MARGIN, screenH - cardH - SCREEN_MARGIN));
		FrameSlot.SetPos(m_wRoot, x, y);
	}

	//------------------------------------------------------------------------------------------------
	static string SquadName(SCR_AIGroup group)
	{
		if (!group)
			return "";

		string name = group.GetCustomName();
		if (name.IsEmpty())
			name = group.GetCustomNameWithOriginal();

		return name;
	}

	//------------------------------------------------------------------------------------------------
	static string VehicleName(IEntity vehicle)
	{
		SCR_EditableVehicleComponent editable = SCR_EditableVehicleComponent.Cast(vehicle.FindComponent(SCR_EditableVehicleComponent));
		if (editable && editable.GetInfo() && !editable.GetInfo().GetName().IsEmpty())
			return editable.GetInfo().GetName();

		return "Vehicle";
	}

	//------------------------------------------------------------------------------------------------
	//! Player name, or the slot name when nobody is in the slot
	static string CharacterName(IEntity character)
	{
		RplComponent rpl = RplComponent.Cast(character.FindComponent(RplComponent));
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!rpl || !slottingManager)
			return "Unknown";

		COA_SlotData slotData = slottingManager.GetSlotDataFromCharacter(rpl.Id());
		if (!slotData)
			return "Unknown";

		int playerId = slotData.GetSlotCurrentPlayerId();
		if (playerId > 0)
		{
			string name = GetGame().GetPlayerManager().GetPlayerName(playerId);
			if (!name.IsEmpty())
				return name;
		}

		return slotData.GetSlotName();
	}

	//------------------------------------------------------------------------------------------------
	static Color FactionColor(Faction faction)
	{
		if (faction)
			return faction.GetFactionColor();

		return Color.FromSRGBA(169, 180, 204, 255);
	}

	//------------------------------------------------------------------------------------------------
	//! Blood volume when ACE medical is running, vanilla health otherwise
	static float CharacterHealth(SCR_ChimeraCharacter character)
	{
		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!damage)
			return 1;

		SCR_CharacterBloodHitZone blood = damage.GetBloodHitZone();
		if (blood)
			return blood.GetHealthScaled();

		return damage.GetHealthScaled();
	}

	//------------------------------------------------------------------------------------------------
	//! Same scale as the "Now spectating" card
	static Color HealthColor(float health)
	{
		if (health > 0.75)
			return Color.FromSRGBA(92, 196, 128, 255);
		if (health > 0.5)
			return Color.FromSRGBA(232, 170, 72, 255);
		if (health > 0.25)
			return Color.FromSRGBA(232, 128, 64, 255);

		return Color.FromSRGBA(232, 96, 96, 255);
	}
}

//------------------------------------------------------------------------------------------------
//! An occupied vehicle's in-world marker
class COA_SpecWorldVehicle : Managed
{
	IEntity m_Vehicle;
	Widget m_wRoot;
	COA_SpecMapPin m_Pin;
	ref array<IEntity> m_aCrew = {};
	bool m_bSeen;
	bool m_bHovered;
	float m_fScreenX;
	float m_fScreenY;
}

//------------------------------------------------------------------------------------------------
//! A recent death's in-world marker
class COA_SpecWorldKill
{
	vector m_vPosition;
	ref Color m_Color;
	float m_fTime;		// world time, ms
	Widget m_wRoot;
	COA_SpecMapPin m_Pin;
}

//------------------------------------------------------------------------------------------------
//! In-world spectator markers besides the player / squad icons: occupied vehicles (vehicle icon,
//! crew count, name on hover) and recent kills (fading skull in the victim's faction colour).
//! Reuses the spectator map pin layout. Owned and driven by COA_SpectatorMenu.
class COA_SpecWorldMarkers
{
	protected static const ResourceName PIN_LAYOUT = "{6A75150000000001}UI/Spectator/COA_SpecMapPin.layout";
	protected static const ResourceName ATLAS = "{F3A9B47F55BE8D2B}UI/Images/SpecImages/Spec_Atlas_x64.imageset";

	protected static const float VEHICLE_REFRESH_MS = 500;
	protected static const float VEHICLE_MAX_DISTANCE = 1200;
	protected static const float VEHICLE_MIN_DISTANCE = 6;		// hidden once the camera is basically inside it
	protected static const float VEHICLE_SIZE_NEAR = 26;
	protected static const float VEHICLE_SIZE_FAR = 18;
	protected static const float VEHICLE_ABOVE_ROOF = 1.2;		// metres above the vehicle's bounding box

	protected static const float KILL_LIFETIME_MS = 20000;
	protected static const float KILL_MAX_DISTANCE = 800;
	protected static const int KILL_MAX = 20;

	protected static ref Color s_Ring = new Color(0.0033, 0.0037, 0.0052, 0.92);
	protected static ref Color s_White = new Color(0.8632, 0.8879, 0.9301, 1);

	protected Widget m_wParent;
	protected ref map<RplId, ref COA_SpecWorldVehicle> m_mVehicles = new map<RplId, ref COA_SpecWorldVehicle>();
	protected ref array<ref COA_SpecWorldKill> m_aKills = {};
	protected float m_fNextVehicleRefresh;
	protected COA_SpecWorldVehicle m_HoveredVehicle;

	//------------------------------------------------------------------------------------------------
	void COA_SpecWorldMarkers(notnull Widget parent)
	{
		m_wParent = parent;
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_SpecWorldMarkers()
	{
		Clear();
	}

	//------------------------------------------------------------------------------------------------
	void Clear()
	{
		foreach (RplId id, COA_SpecWorldVehicle vehicle : m_mVehicles)
		{
			if (vehicle.m_wRoot)
				vehicle.m_wRoot.RemoveFromHierarchy();
		}
		m_mVehicles.Clear();

		foreach (COA_SpecWorldKill kill : m_aKills)
		{
			if (kill.m_wRoot)
				kill.m_wRoot.RemoveFromHierarchy();
		}
		m_aKills.Clear();
		m_HoveredVehicle = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Called by the spectator kill feed
	void AddKill(vector position, Color color)
	{
		if (position == vector.Zero)
			return;

		COA_SpecWorldKill kill = new COA_SpecWorldKill();
		kill.m_vPosition = position;
		kill.m_Color = color;
		kill.m_fTime = GetGame().GetWorld().GetWorldTime();
		m_aKills.Insert(kill);

		while (m_aKills.Count() > KILL_MAX)
		{
			if (m_aKills[0].m_wRoot)
				m_aKills[0].m_wRoot.RemoveFromHierarchy();
			m_aKills.RemoveOrdered(0);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \param[in] characters every lobby character (the menu's active character list)
	//! \param[in] hideOtherFactions gamemode setting: only show the local spectator's faction
	//! \param[in] localFaction the local spectator's slot faction (may be null)
	void Update(array<COA_PlayerCharacter> characters, bool hideOtherFactions, Faction localFaction)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now >= m_fNextVehicleRefresh)
		{
			m_fNextVehicleRefresh = now + VEHICLE_REFRESH_MS;
			RefreshVehicles(characters, hideOtherFactions, localFaction);
		}

		m_HoveredVehicle = null;
		CameraBase camera = GetGame().GetCameraManager().CurrentCamera();
		if (!camera)
			return;

		Widget underCursor = WidgetManager.GetWidgetUnderCursor();
		vector cameraPos = camera.GetOrigin();
		foreach (RplId id, COA_SpecWorldVehicle vehicle : m_mVehicles)
		{
			PositionVehicle(vehicle, cameraPos, underCursor);
		}

		UpdateKills(now, cameraPos);
	}

	//------------------------------------------------------------------------------------------------
	//! The vehicle marker under the cursor this frame (for the hover card), or null
	COA_SpecWorldVehicle GetHoveredVehicle()
	{
		return m_HoveredVehicle;
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshVehicles(array<COA_PlayerCharacter> characters, bool hideOtherFactions, Faction localFaction)
	{
		foreach (RplId id, COA_SpecWorldVehicle vehicle : m_mVehicles)
		{
			vehicle.m_bSeen = false;
			vehicle.m_aCrew.Clear();
		}

		if (characters)
		{
			foreach (COA_PlayerCharacter character : characters)
			{
				if (!character || COA_EntityHelper.IsSpectator(character) || !COA_DamageHelper.CheckIfEntityAlive(character))
					continue;

				if (hideOtherFactions && localFaction && character.GetFaction() != localFaction)
					continue;

				IEntity vehicleEntity = CompartmentAccessComponent.GetVehicleIn(character);
				if (!vehicleEntity)
					continue;

				RplComponent rpl = RplComponent.Cast(vehicleEntity.FindComponent(RplComponent));
				if (!rpl)
					continue;

				COA_SpecWorldVehicle vehicle = m_mVehicles.Get(rpl.Id());
				if (!vehicle || vehicle.m_Vehicle != vehicleEntity)
				{
					if (vehicle && vehicle.m_wRoot)
						vehicle.m_wRoot.RemoveFromHierarchy();

					vehicle = CreateVehicle(rpl.Id(), vehicleEntity, character.GetFaction());
					if (!vehicle)
						continue;
				}

				vehicle.m_bSeen = true;
				vehicle.m_aCrew.Insert(character);
			}
		}

		array<RplId> stale = {};
		foreach (RplId vehicleId, COA_SpecWorldVehicle vehicle : m_mVehicles)
		{
			if (!vehicle.m_bSeen || !vehicle.m_Vehicle)
			{
				stale.Insert(vehicleId);
				continue;
			}

			vehicle.m_Pin.SetBadge(vehicle.m_aCrew.Count().ToString());
		}

		foreach (RplId staleId : stale)
		{
			COA_SpecWorldVehicle staleVehicle = m_mVehicles.Get(staleId);
			if (staleVehicle.m_wRoot)
				staleVehicle.m_wRoot.RemoveFromHierarchy();
			m_mVehicles.Remove(staleId);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected COA_SpecWorldVehicle CreateVehicle(RplId id, IEntity vehicleEntity, Faction faction)
	{
		COA_SpecMapPin pin;
		Widget root = CreatePin(pin);
		if (!root)
			return null;

		COA_SpecWorldVehicle vehicle = new COA_SpecWorldVehicle();
		vehicle.m_Vehicle = vehicleEntity;
		vehicle.m_wRoot = root;
		vehicle.m_Pin = pin;
		pin.Init(root, COA_ESpecMapPinKind.VEHICLE, vehicle, null);

		SCR_UIInfo info;
		SCR_EditableVehicleComponent editable = SCR_EditableVehicleComponent.Cast(vehicleEntity.FindComponent(SCR_EditableVehicleComponent));
		if (editable)
			info = editable.GetInfo();

		pin.SetColors(COA_SpecHoverCard.FactionColor(faction), s_Ring);
		pin.SetIconFromInfo(info, ATLAS, "Car");
		pin.SetIconColor(s_White);
		pin.SetLabel(COA_SpecHoverCard.VehicleName(vehicleEntity));
		pin.SetLabelShown(false);
		pin.Layout(VEHICLE_SIZE_NEAR, false, true);

		m_mVehicles.Set(id, vehicle);
		return vehicle;
	}

	//------------------------------------------------------------------------------------------------
	protected void PositionVehicle(COA_SpecWorldVehicle vehicle, vector cameraPos, Widget underCursor)
	{
		if (!vehicle.m_Vehicle || !vehicle.m_wRoot)
			return;

		vector boundsMin, boundsMax;
		vehicle.m_Vehicle.GetWorldBounds(boundsMin, boundsMax);
		vector top = vehicle.m_Vehicle.GetOrigin();
		top[1] = boundsMax[1] + VEHICLE_ABOVE_ROOF;

		float distance = vector.Distance(cameraPos, top);
		vector screen = GetGame().GetWorkspace().ProjWorldToScreen(top, GetGame().GetWorld());
		bool show = screen[2] > 0 && distance < VEHICLE_MAX_DISTANCE && distance > VEHICLE_MIN_DISTANCE;

		vehicle.m_Pin.SetShown(show);
		vehicle.m_wRoot.SetEnabled(show);
		vehicle.m_bHovered = false;
		if (!show)
			return;

		float t = Math.Clamp((distance - 20) / (VEHICLE_MAX_DISTANCE - 20), 0, 1);
		vehicle.m_Pin.Layout(VEHICLE_SIZE_NEAR + (VEHICLE_SIZE_FAR - VEHICLE_SIZE_NEAR) * t, false, true);

		vehicle.m_bHovered = IsUnder(underCursor, vehicle.m_wRoot);
		vehicle.m_Pin.SetLabelShown(vehicle.m_bHovered);
		if (vehicle.m_bHovered)
		{
			vehicle.m_Pin.SetColors(COA_SpecHoverCard.FactionColor(VehicleFaction(vehicle)), s_White);
			m_HoveredVehicle = vehicle;
		}
		else
		{
			vehicle.m_Pin.SetColors(COA_SpecHoverCard.FactionColor(VehicleFaction(vehicle)), s_Ring);
		}

		vehicle.m_fScreenX = screen[0];
		vehicle.m_fScreenY = screen[1];
		vehicle.m_Pin.SetScreenPos(screen[0], screen[1]);
		vehicle.m_wRoot.SetZOrder(screen[2] * -10000);
	}

	//------------------------------------------------------------------------------------------------
	protected Faction VehicleFaction(COA_SpecWorldVehicle vehicle)
	{
		foreach (IEntity member : vehicle.m_aCrew)
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(member);
			if (character && character.GetFaction())
				return character.GetFaction();
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! A pulse when new, then a skull fading out over KILL_LIFETIME_MS
	protected void UpdateKills(float now, vector cameraPos)
	{
		for (int i = m_aKills.Count() - 1; i >= 0; i--)
		{
			COA_SpecWorldKill kill = m_aKills[i];
			float age = now - kill.m_fTime;
			if (age > KILL_LIFETIME_MS || age < 0)
			{
				if (kill.m_wRoot)
					kill.m_wRoot.RemoveFromHierarchy();
				m_aKills.RemoveOrdered(i);
				continue;
			}

			if (!kill.m_wRoot)
			{
				COA_SpecMapPin pin;
				kill.m_wRoot = CreatePin(pin);
				if (!kill.m_wRoot)
					continue;

				kill.m_Pin = pin;
				pin.Init(kill.m_wRoot, COA_ESpecMapPinKind.KILL, null, null);
				pin.SetColors(kill.m_Color, s_Ring);
				pin.SetIconImage(ATLAS, "Dead");
				pin.SetIconColor(s_White);
				pin.SetLabel("");
				pin.SetLabelShown(false);
			}

			vector point = kill.m_vPosition;
			point[1] = point[1] + 1.0;
			vector screen = GetGame().GetWorkspace().ProjWorldToScreen(point, GetGame().GetWorld());
			bool show = screen[2] > 0 && vector.Distance(cameraPos, point) < KILL_MAX_DISTANCE;
			kill.m_Pin.SetShown(show);
			if (!show)
				continue;

			float pulse = Math.Max(0, 1 - age / 1000);
			kill.m_Pin.Layout(14 + 12 * pulse, false, true);
			kill.m_wRoot.SetOpacity(Math.Clamp(1.2 - age / KILL_LIFETIME_MS, 0.15, 1));
			kill.m_Pin.SetScreenPos(screen[0], screen[1]);
			kill.m_wRoot.SetZOrder(screen[2] * -10000 - 2);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected Widget CreatePin(out COA_SpecMapPin pin)
	{
		Widget root = GetGame().GetWorkspace().CreateWidgets(PIN_LAYOUT, m_wParent);
		if (!root)
			return null;

		Widget hitArea = root.FindAnyWidget("HitArea");
		if (hitArea)
			pin = COA_SpecMapPin.Cast(hitArea.FindHandler(COA_SpecMapPin));

		if (!pin)
		{
			root.RemoveFromHierarchy();
			return null;
		}

		return root;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsUnder(Widget underCursor, Widget root)
	{
		while (underCursor)
		{
			if (underCursor == root)
				return true;

			underCursor = underCursor.GetParent();
		}

		return false;
	}
}
