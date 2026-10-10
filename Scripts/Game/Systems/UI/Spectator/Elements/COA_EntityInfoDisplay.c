class COA_EntityInfoDisplay : SCR_ScriptedWidgetComponent
{		
	protected Widget        m_wEntityInfoDisplay;
	protected TextWidget    m_wEntityName;
	protected TextWidget    m_wEntityRole;
	protected TextWidget    m_wEntityDamage;  // ACE blood state label (left side of status row)
	protected TextWidget    m_wEntityDamageType;     // state chip text: UNCON / BLEEDING / cause of death
	protected Widget        m_wHealthFill;           // rounded bar, width = blood/health fraction
	protected Widget        m_wStateChip;            // rounded chip behind m_wEntityDamageType
	protected Widget        m_wFactionPill;          // accent pill in the spectated player's faction colour

	protected static const float HEALTH_BAR_WIDTH = 388;	// matches HealthTrack in EntityInfoDisplay.layout
	
	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wEntityInfoDisplay = w;
		m_wEntityName = TextWidget.Cast(w.FindAnyWidget("EntityName"));
		m_wEntityRole = TextWidget.Cast(w.FindAnyWidget("EntityRole"));
		m_wEntityDamage = TextWidget.Cast(w.FindAnyWidget("EntityDamage"));
		m_wEntityDamageType = TextWidget.Cast(w.FindAnyWidget("EntityDamageType"));
		m_wHealthFill = w.FindAnyWidget("EntityHealthFill");
		m_wStateChip = w.FindAnyWidget("StateChipBG");
		m_wFactionPill = w.FindAnyWidget("FactionPill");
	}

	/**
	 * Updates the follow-mode HUD overlay shown when latched onto a player in TPP or FPP mode.
	 * Displays: player name, role name, faction-colored health bar.
	 * Hidden automatically when not following anyone.
	 */
	void UpdateEntityInfoDisplay(IEntity specEntity)
	{
		if (!m_wEntityInfoDisplay)
			return;

		// Hide the HUD if we are not following anyone
		if (!specEntity)
		{
			m_wEntityInfoDisplay.SetVisible(false);
			return;
		}

		m_wEntityInfoDisplay.SetVisible(true);

		// Faction accent
		if (m_wFactionPill)
		{
			FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(specEntity.FindComponent(FactionAffiliationComponent));
			if (factionComponent && factionComponent.GetAffiliatedFaction())
				m_wFactionPill.SetColor(factionComponent.GetAffiliatedFaction().GetFactionColor());
			else
				m_wFactionPill.SetColor(Color.FromSRGBA(201, 54, 54, 255));
		}

		string playerName = "";
		RplComponent rpl = RplComponent.Cast(specEntity.FindComponent(RplComponent));
		if (rpl)
		{
			COA_SlotData slotData = COA_SlottingManager.GetInstance().GetSlotDataFromCharacter(rpl.Id());
			int playerId = 0;
			if (slotData)
				playerId = slotData.GetSlotCurrentPlayerId();
			if (playerId > 0)
				playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			if (playerName.IsEmpty() && slotData)
				playerName = slotData.GetSlotName();
		}

		// Truncate with ellipsis if the name is too long (~36 chars at bold font 18)
		const int NAME_MAX_CHARS = 36;
		if (playerName.Length() > NAME_MAX_CHARS)
			playerName = playerName.Substring(0, NAME_MAX_CHARS - 1) + "…";

		m_wEntityName.SetText(playerName);

		// --- Role name ---
		string roleName = "";
		if (rpl)
		{
			COA_SlotData slotData = COA_SlottingManager.GetInstance().GetSlotDataFromCharacter(rpl.Id());
			if (slotData)
			{
				roleName = slotData.GetSlotName();

				// Prepend group name if available
				int rolePlayerId = slotData.GetSlotCurrentPlayerId();
				if (rolePlayerId > 0)
				{
					SCR_AIGroup playerGroup = COA_SlottingManager.GetInstance().GetPlayerSlotGroup(rolePlayerId);
					if (playerGroup)
					{
						string groupName = playerGroup.GetCustomName();
						if (groupName.IsEmpty())
							groupName = playerGroup.GetCustomNameWithOriginal();
						if (!groupName.IsEmpty())
							roleName = groupName + "  ·  " + roleName;
					}
				}
			}
		}

		// Append vehicle info if the character is inside one
		IEntity vehicle = SCR_CompartmentAccessComponent.GetVehicleIn(specEntity);
		if (vehicle)
		{
			SCR_EditableVehicleComponent editableVehicle = SCR_EditableVehicleComponent.Cast(vehicle.FindComponent(SCR_EditableVehicleComponent));
			string vehicleName = "";
			if (editableVehicle)
			{
				SCR_UIInfo vehicleInfo = editableVehicle.GetInfo();
				if (vehicleInfo)
					vehicleName = vehicleInfo.GetName();
			}
			if (!vehicleName.IsEmpty())
				roleName = roleName + "  ·  " + vehicleName;
			else
				roleName = roleName + "  ·  In vehicle";
		}

		// Truncate with ellipsis if the string is too long to fit the role line (~48 chars at font 14)
		const int ROLE_MAX_CHARS = 48;
		if (roleName.Length() > ROLE_MAX_CHARS)
			roleName = roleName.Substring(0, ROLE_MAX_CHARS - 1) + "…";

		m_wEntityRole.SetText(roleName);

		// --- ACE Medical status (blood volume bar + blood state + bleeding indicator) ---
		SCR_CharacterDamageManagerComponent charDmg = SCR_CharacterDamageManagerComponent.Cast(
			specEntity.FindComponent(SCR_CharacterDamageManagerComponent));

		// Blood volume: prefer ACE blood hit zone (tracks bleeding), fall back to vanilla health
		float bloodScaled = 1.0;
		bool isBleeding = false;
		bool isDead = false;
		bool isUnconscious = false;
		string damageStateText = "";
		string bloodStateText = "";
		if (charDmg)
		{
			SCR_CharacterBloodHitZone bloodHZ = charDmg.GetBloodHitZone();
			if (bloodHZ)
			{
				bloodScaled = bloodHZ.GetHealthScaled();

				// Map blood damage state to a readable label.
				// ECharacterBloodState integer values per ACE-Anvil:
				//   0 = NORMAL, 1 = CLASS_1_HEMORRHAGE, 3 = CLASS_2_HEMORRHAGE,
				//   4 = CLASS_3_HEMORRHAGE, 5 = CLASS_4_HEMORRHAGE,
				//   2 = FATAL, UNCONSCIOUS maps to vanilla EDamageState.
				int bloodState = bloodHZ.GetDamageState();
				if (bloodState == 0)
					bloodStateText = "Healthy";
				else if (bloodState == 1)
					bloodStateText = "Class I Hemorrhage";
				else if (bloodState == 3)
					bloodStateText = "Class II Hemorrhage";
				else if (bloodState == 4)
					bloodStateText = "Class III Hemorrhage";
				else if (bloodState == 5)
					bloodStateText = "Class IV Hemorrhage";
				else if (bloodState == 2)
					bloodStateText = "Fatal Blood Loss";
				else
					bloodStateText = "Hemorrhagic Shock";
			}
			else
			{
				// ACE bleeding not available — fall back to vanilla health
				bloodScaled = charDmg.GetHealthScaled();
				bloodStateText = "Healthy";
			}
			
			isDead = !COA_DamageHelper.CheckIfEntityAlive(specEntity);
			
			if (isDead)
			{
				bloodScaled = 0;
				
				BaseDamageEffect fatalDamageEffect = charDmg.GetFatalDamageEffect();
				
				if (fatalDamageEffect)
				{
					damageStateText = COA_DamageHelper.GetCauseOfDeathString(fatalDamageEffect.GetDamageType());
					
					// Use the killer name cached in slot data at death time (set server-side in OnPlayerKilled).
					// Reliable even after the killer dies, respawns, or disconnects, unlike live entity lookups.
					if (rpl)
					{
						COA_SlotData victimSlotData = COA_SlottingManager.GetInstance().GetSlotDataFromCharacter(rpl.Id());
						if (victimSlotData)
						{
							string killerName = victimSlotData.GetKillerName();
							if (killerName.IsEmpty())
								bloodStateText = "KIA - Killed By: AI";
							else
								bloodStateText = string.Format("KIA - Killed By: %1", killerName);
						}
						else
							bloodStateText = "KIA";
					}
					else
						bloodStateText = "KIA";
				};
			} else
				isBleeding = charDmg.IsBleeding();
		}
		
		// --- Blood state label ---
		m_wEntityDamage.SetText(bloodStateText);

		// --- Blood bar fill ---
		// Drive the fill purely by anchors: AnchorMin.x = 0, AnchorMax.x = bloodScaled.
		// This is resolution/DPI-independent — no pixel math required.
		float clamped = Math.Clamp(bloodScaled, 0.0, 1.0);

		// Colour: green -> amber -> orange -> red as blood drops; grey when dead (palette, sRGB)
		Color barColor;
		if (bloodScaled > 0.75)
			barColor = Color.FromSRGBA(92, 196, 128, 255);
		else if (bloodScaled > 0.5)
			barColor = Color.FromSRGBA(232, 170, 72, 255);
		else if (bloodScaled > 0.25)
			barColor = Color.FromSRGBA(232, 128, 64, 255);
		else if (bloodScaled > 0)
			barColor = Color.FromSRGBA(232, 96, 96, 255);
		else
			barColor = Color.FromSRGBA(90, 96, 110, 255);

		if (m_wHealthFill)
		{
			FrameSlot.SetSizeX(m_wHealthFill, Math.Max(HEALTH_BAR_WIDTH * clamped, 1));
			m_wHealthFill.SetColor(barColor);
			m_wHealthFill.SetVisible(clamped > 0);
		}
		
		// --- Bleeding / unconscious indicator ---
		SCR_CharacterControllerComponent ctrl = SCR_CharacterControllerComponent.Cast(
			specEntity.FindComponent(SCR_CharacterControllerComponent));

		// Check if the spectated character is unconscious and get their resilience %
		if (ctrl && ctrl.IsUnconscious() && !isDead)
		{
			isUnconscious = true;
			if (charDmg)
			{
				SCR_CharacterResilienceHitZone resHz = charDmg.GetResilienceHitZone();
				if (resHz)
					damageStateText = "UNCON " + Math.Round(resHz.GetHealthScaled() * 100) + "%";
				else
					damageStateText = "UNCON";
			}
			else
				damageStateText = "UNCON";
		}
		
		// State chip: text + chip colour (palette, sRGB)
		string chipText;
		Color chipColor = Color.FromSRGBA(28, 31, 40, 255);
		if (isDead)
		{
			chipText = damageStateText;
			chipColor = Color.FromSRGBA(54, 58, 70, 255);
		}
		else if (isUnconscious && isBleeding)
		{
			chipText = damageStateText + "  ·  BLEEDING";
			chipColor = Color.FromSRGBA(168, 92, 32, 255);
		}
		else if (isUnconscious)
		{
			chipText = damageStateText;
			chipColor = Color.FromSRGBA(168, 112, 32, 255);
		}
		else if (isBleeding)
		{
			chipText = "BLEEDING";
			chipColor = Color.FromSRGBA(160, 40, 40, 255);
		}

		chipText.ToUpper();
		m_wEntityDamageType.SetText(chipText);
		m_wEntityDamageType.SetColor(Color.FromSRGBA(239, 242, 247, 255));
		if (m_wStateChip)
		{
			m_wStateChip.SetVisible(!chipText.IsEmpty());
			m_wStateChip.SetColor(chipColor);
		}
	}
};