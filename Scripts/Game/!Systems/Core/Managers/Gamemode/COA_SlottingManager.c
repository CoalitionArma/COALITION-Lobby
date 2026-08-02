class COA_SlottingManagerClass : ScriptComponentClass {}

class COA_SlottingManager : ScriptComponent
{
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================

	// Slot data storage - uses ID-based system where IDs are generated in AddSlot
	protected ref map<int, ref COA_SlotData> m_mSlotsMap = new map<int, ref COA_SlotData>;
	
	// Latest Slot ID used
	protected int m_iLatestSlotID;
	
	// Invoker for slot updates (batch/structural: used by all consumers including AAR & spectator)
	protected ref ScriptInvoker m_OnSlottingUpdate = new ScriptInvoker;
	
	// Invoker for surgical per-slot player-ID changes (avoids full list rebuild on every click)
	protected int m_iLastChangedSlotId = -1;
	protected ref ScriptInvoker m_OnSlotChanged = new ScriptInvoker;
	
	// References to other managers
	protected COA_RplBroadcastManager m_RplBroadcastManager;
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 MANAGER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_RplBroadcastManager = COA_RplBroadcastManager.GetInstance();
		
		// Need to call next frame due to race conditions if the faction manager hasn't fully initilized.
		GetGame().GetCallqueue().Call(InitilizeSlots);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void InitilizeSlots()
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		
		InitilizeSlotsForFaction("BLUFOR", gamemode.m_BLUFORSlots);
		InitilizeSlotsForFaction("OPFOR", gamemode.m_OPFORSlots);
		InitilizeSlotsForFaction("INDFOR", gamemode.m_INDFORSlots);
		InitilizeSlotsForFaction("CIV", gamemode.m_CIVSlots);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 SLOTTING UPDATE METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	void UpdateSlotCharacter(int slotId, RplId charId)
	{
		COA_SlotData slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetSlotCurrentCharacter(charId);
			m_RplBroadcastManager.UpdateSlotCharacterDelta(slotId, charId);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotRole(int slotId, COA_EGearRole role)
	{
		COA_SlotData slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetSlotRole(role);
			m_RplBroadcastManager.UpdateSlotRoleDelta(slotId, role);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotGroup(int slotId, RplId group)
	{
		COA_SlotData slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetSlotCurrentGroup(group);
			m_RplBroadcastManager.UpdateSlotGroupDelta(slotId, group);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotPlayerID(int slotId, int playerId = -1)
	{	
		COA_SlotData slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			// Server-side first-come-first-served guard:
			// If a player (playerId > 0) is trying to claim a slot that is already occupied
			// by a DIFFERENT player, reject the request silently.
			// playerId == 0 (kick/vacate) always proceeds; own-slot re-claim always proceeds.
			int currentOccupant = slotData.GetSlotCurrentPlayerId();
			if (playerId > 0 && currentOccupant > 0 && currentOccupant != playerId)
				return;
			
			slotData.SetSlotCurrentPlayerId(playerId);
			m_RplBroadcastManager.UpdateSlotPlayerIdDelta(slotId, playerId);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	//! Force-assigns a player to a slot for reconnect restoration.
	//! Bypasses the first-come-first-served guard so a reconnecting player (who may have a
	//! new numeric ID on a dedicated server) can reclaim the slot they held before disconnecting.
	//! Safe to call with newPlayerId > 0 — never triggers CleanupCharacterFromSlot.
	//! \param[in] slotId      Slot to restore
	//! \param[in] newPlayerId The reconnecting player's current numeric player ID
	void ForceUpdateSlotPlayerID(int slotId, int newPlayerId)
	{
		if (newPlayerId <= 0)
			return;
		
		COA_SlotData slotData = GetSlotData(slotId);
		if (!slotData)
			return;
		
		// SetSlotCurrentPlayerId only triggers CleanupCharacterFromSlot when playerId <= 0,
		// so calling it with newPlayerId > 0 is safe and will not delete the character entity.
		slotData.SetSlotCurrentPlayerId(newPlayerId);
		m_RplBroadcastManager.UpdateSlotPlayerIdDelta(slotId, newPlayerId);
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotLockedState(int slotId, bool isLocked = false)
	{
		COA_SlotData slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetIsLockedSlot(isLocked);
			if (isLocked)
				slotData.SetSlotCurrentPlayerId(0);
			
			m_RplBroadcastManager.UpdateSlotLockedDelta(slotId, isLocked);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotDeathState(int slotId, bool input)
	{
		COA_SlotData slotData = GetSlotData(slotId);

		if (slotData)
		{
			slotData.SetIsDeadSlot(input);
			m_RplBroadcastManager.UpdateSlotDeathDelta(slotId, input);
		};
	}

	//------------------------------------------------------------------------------------------------
	//! Used by COA_RespawnManager when the mission is in COA_ERespawnMode.SLOT to update a slot's
	//! (or, for PER_GROUP pools, every slot in the group's) remaining respawn count.
	void UpdateSlotRespawnsRemaining(int slotId, int remaining)
	{
		COA_SlotData slotData = GetSlotData(slotId);

		if (slotData)
		{
			slotData.SetSlotRespawnsRemaining(remaining);
			m_RplBroadcastManager.UpdateSlotRespawnsRemainingDelta(slotId, remaining);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	//! Stores the name of the player who killed this slot's occupant and replicates it to all clients.
	//! Called server-side immediately after death, where GetInstigatorPlayerID() is reliable.
	//! \param[in] slotId  The slot of the player who was killed
	//! \param[in] killerName  Display name of the killer (empty string = killed by AI/environment)
	void UpdateSlotKillerName(int slotId, string killerName)
	{
		COA_SlotData slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetKillerName(killerName);
			m_RplBroadcastManager.UpdateSlotKillerNameDelta(slotId, killerName);
		}
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 MISC GETTERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnSlottingUpdate()
	{
		return m_OnSlottingUpdate;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Returns the invoker that fires when a single slot's player ID changes (surgical update path).
	ScriptInvoker GetOnSlotChanged()
	{
		return m_OnSlotChanged;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Returns the slot ID that was most recently changed via NotifySlotPlayerIdChanged().
	int GetLastChangedSlotId()
	{
		return m_iLastChangedSlotId;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Called by RpcDo_UpdateSlotPlayerIdDelta to fire the targeted slot-change invoker.
	//! Stores the slot ID so subscribers can retrieve it without invoice arguments.
	void NotifySlotPlayerIdChanged(int slotId)
	{
		m_iLastChangedSlotId = slotId;
		if (m_OnSlotChanged)
			m_OnSlotChanged.Invoke();
	}
	
	//------------------------------------------------------------------------------------------------
	COA_SlotData GetSlotData(int slotId)
	{
		return m_mSlotsMap.Get(slotId);
	}
	
	//------------------------------------------------------------------------------------------------
	map<int, ref COA_SlotData> GetSlotMap()
	{
		return m_mSlotsMap;
	}
	
	//------------------------------------------------------------------------------------------------
	array<int> GetAllSlotIds()
	{
		array<int> slotIds = {};
		
		foreach (int slotId, COA_SlotData slotData : m_mSlotsMap)
		{
			slotIds.Insert(slotId);
		}
		
		return slotIds;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 GROUP GETTERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	array<SCR_AIGroup> GetAllGroups(FactionKey factionKey = "")
	{
		map<int, SCR_AIGroup> groupMap = new map<int, SCR_AIGroup>;
		array<SCR_AIGroup> outputArray = {};
		array<int> sortingArray = {};
		
		// Pre-allocate based on slot count (worst case: every slot has unique group)
		int slotCount = m_mSlotsMap.Count();
		sortingArray.Reserve(slotCount);
		outputArray.Reserve(slotCount);
		
		// Collect all relevant groups
		foreach (int slotId, COA_SlotData slotData : m_mSlotsMap)
		{
			if (!IsValidGroupInSlot(slotData))
				continue;
			
			// Check faction filter if provided
			if (!factionKey.IsEmpty() && slotData.GetSlotFactionKey() != factionKey)
				continue;
			
			SCR_AIGroup group = COA_EntityHelper.GetGroupFromRplId(slotData.GetSlotCurrentGroup());
			if (!group)
				continue;
				
			if (!groupMap.Contains(group.GetGroupID()))
				groupMap.Set(group.GetGroupID(), group);
		}
		
		// Sort groups by ID
		foreach (int groupId, SCR_AIGroup group : groupMap)
			sortingArray.Insert(groupId);
		
		sortingArray.Sort(false);
		
		// Create output array in sorted order
		foreach (int groupId : sortingArray)
			outputArray.Insert(groupMap.Get(groupId));
		
		return outputArray;
	}
	
	//------------------------------------------------------------------------------------------------
	// Helper method to check if group in slot is valid
	protected bool IsValidGroupInSlot(COA_SlotData slotData)
	{
		if (!slotData)
			return false;
			
		RplId groupId = slotData.GetSlotCurrentGroup();
		if (groupId == RplId.Invalid())
			return false;
			
		if (!Replication.FindItem(groupId))
			return false;
			
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	array<int> GetAllSlotIDsForGroup(RplId rplId)
	{
		array<int> outputArray = {};
		
		foreach (int slotID, COA_SlotData slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentGroup() == rplId)
				outputArray.Insert(slotID);
		}
		
		outputArray.Sort();
		
		return outputArray;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 CHRARACTER GETTERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	COA_SlotData GetSlotDataFromCharacter(RplId rplId)
	{
		foreach (int slotID, COA_SlotData slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentCharacter() == rplId)
				return slotData;
		}
		
		return null;
	}

	//------------------------------------------------------------------------------------------------
	int GetCharacterSlotID(IEntity entity)
	{
		if (!entity)
			return -1;
			
		RplComponent rplComp = RplComponent.Cast(entity.FindComponent(RplComponent));
		if (!rplComp)
			return -1;
			
		RplId rplId = rplComp.Id();
		
		foreach (int slotID, COA_SlotData slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentCharacter() == rplId)
				return slotID;
		}
		
		return -1;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 PLAYER GETTERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	int GetPlayerSlotID(int playerId)
	{
		foreach (int slotID, COA_SlotData slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentPlayerId() == playerId)
				return slotID;
		}
		
		return -1;
	}
	
	//------------------------------------------------------------------------------------------------
	COA_SlotData GetPlayerSlotData(int playerId)
	{
		foreach (int slotID, COA_SlotData slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentPlayerId() == playerId)
				return slotData;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_AIGroup GetPlayerSlotGroup(int playerId)
	{
		COA_SlotData slotData = GetPlayerSlotData(playerId);
		if (!slotData)
			return null;
			
		RplId groupId = slotData.GetSlotCurrentGroup();
		return COA_EntityHelper.GetGroupFromRplId(groupId);
	}
	
	//------------------------------------------------------------------------------------------------
	COA_PlayerCharacter GetPlayerSlotCharacter(int playerId)
	{
		COA_SlotData slotData = GetPlayerSlotData(playerId);
		if (!slotData)
			return null;
			
		RplId charId = slotData.GetSlotCurrentCharacter();
		return COA_PlayerCharacter.Cast(COA_EntityHelper.GetCharacterFromRplId(charId));
	}
	
	//------------------------------------------------------------------------------------------------
	Faction GetPlayerSlotFaction(int playerId, bool returnNull = false)
	{
		FactionManager factionManager = GetGame().GetFactionManager();
		
		COA_SlotData slotData = GetPlayerSlotData(playerId);
		
		if (!slotData && !returnNull)
			return factionManager.GetFactionByKey("CIV");
		else if (!slotData)
			return null;
			
		FactionKey factionKey = slotData.GetSlotFactionKey();
		
		if (factionKey.IsEmpty() && !returnNull)
			return factionManager.GetFactionByKey("CIV");
		else if (factionKey.IsEmpty())
			return null;
			
		return factionManager.GetFactionByKey(factionKey);
	}
	
	//------------------------------------------------------------------------------------------------
	ResourceName GetPlayerSlotResource(int playerId)
	{
		COA_SlotData slotData = GetPlayerSlotData(playerId);
		if (!slotData)
			return ResourceName.Empty;
			
		return slotData.GetSlotResource();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATE CHECKING
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Count how many players are slotted in a specific faction
	//! This counts SLOTTED players, not spawned players
	//! \param[in] factionKey The faction key to count (e.g. "BLUFOR", "OPFOR", "INDFOR", "CIV")
	//! \return Number of players slotted in that faction
	int GetSlottedPlayerCountByFaction(FactionKey factionKey)
	{
		int count = 0;
		
		foreach (int slotID, COA_SlotData slotData : m_mSlotsMap)
		{
			// Check if slot has a player and matches the faction
			if (slotData.GetSlotCurrentPlayerId() > 0 && slotData.GetSlotFactionKey() == factionKey)
				count++;
		}
		
		return count;
	}
	
	
	//------------------------------------------------------------------------------------------------
	bool IsFactionValid(FactionKey factionKey)
	{
		foreach (int slotID, COA_SlotData slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotFactionKey() == factionKey)
				return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsPlayerInASlot(int playerId)
	{
		foreach (int slotID, COA_SlotData slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentPlayerId() == playerId)
				return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsPlayerConsideredDead(int playerId)
	{
		COA_SlotData slotData = GetPlayerSlotData(playerId);
		if (!slotData)
			return false;
			
		return slotData.GetIsDeadSlot();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Helper method to check if a group is empty
	protected bool IsEmptyGroup(SCR_AIGroup group)
	{
		if (!group)
			return true;
			
		RplComponent rplComp = RplComponent.Cast(group.FindComponent(RplComponent));
		if (!rplComp)
			return true;
			
		RplId groupId = rplComp.Id();
		
		foreach (int slotId, COA_SlotData slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentGroup() != groupId)
				continue;
				
			if (slotData.GetSlotCurrentPlayerId() > 0)
				return false;
		}
		
		return true;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 FACTION INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	void InitilizeSlotsForFaction(FactionKey factionKey, array <ref COA_SlottingGroup> factionSlots)
	{
		if (factionKey.IsEmpty() || factionSlots.IsEmpty())
			return;
		
		InitilizeGroupCallsignsForFaction(factionKey, factionSlots);
		
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		Faction faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
		SCR_Faction scrFaction = SCR_Faction.Cast(faction);
		
		foreach (ref COA_SlottingGroup slotGroup : factionSlots)
		{	
			COA_EFlagType flagType = slotGroup.m_FlagType;
			
			if(scrFaction && scrFaction.GetFlagName(0))
			{
				TStringArray flagArray = {};
				scrFaction.GetFlagNames(flagArray);
				if((flagArray.Count() - 1) < flagType)
					flagType = COA_EFlagType.INFANTRY;
			};
			
			SCR_AIGroup group = SCR_GroupsManagerComponent.GetInstance().CreateNewPlayableGroup(scrFaction);
			group.SetFaction(scrFaction);
			group.SetGroupFlag(flagType, true);
			group.SetCanDeleteIfNoPlayer(false);
			group.SetDeleteWhenEmpty(false);
			group.SetMaxMembers(16);
			
			foreach(COA_EGearRole role : slotGroup.m_aSlots)
			{
				COA_RolesConfig rolesConfig = COA_GearscriptManager.GetRolesConfig();
				COA_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
				
				if (!roleConfig || !rolesConfig)
					return;
					
				// Create and configure new slot data
				COA_SlotData slotData = new COA_SlotData;
				
				// Set group and faction
				RplComponent groupRplComp = RplComponent.Cast(group.FindComponent(RplComponent));
				slotData.SetSlotCurrentGroup(groupRplComp.Id());
				slotData.SetSlotFactionKey(factionKey);
				
				// Set resource and character ID
				slotData.SetSlotRole(role);

				// Seed the slot/group respawn pool when the mission is using Slot-Based respawns
				if (COA_Gamemode.GetInstance().m_eRespawnMode == COA_ERespawnMode.SLOT)
				{
					slotData.SetRespawnPoolType(slotGroup.m_eRespawnPoolType);

					if (slotGroup.m_eRespawnPoolType == COA_ERespawnPoolType.PER_GROUP)
						slotData.SetSlotRespawnsRemaining(slotGroup.m_iGroupRespawns);
					else
						slotData.SetSlotRespawnsRemaining(slotGroup.GetRoleRespawnCount(role));
				}

				// Add to slots map
				m_iLatestSlotID++;
				slotData.SetSlotId(m_iLatestSlotID);
				m_mSlotsMap.Set(m_iLatestSlotID, slotData);
				
				// Broadcast new slot to all clients
				m_RplBroadcastManager.UpdateSlotData(slotData);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void InitilizeGroupCallsignsForFaction(FactionKey factionKey, array <ref COA_SlottingGroup> factionSlots)
	{
		array<ref SCR_CallsignInfo> callsignArray = new array<ref SCR_CallsignInfo>;
		foreach (ref COA_SlottingGroup slotGroup : factionSlots)
		{
			ref SCR_CallsignInfo callsignInfo = new SCR_CallsignInfo;
			callsignInfo.SetCallsign(slotGroup.m_sCallsign);
			callsignArray.Insert(callsignInfo);
		}
		
		Faction faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
		SCR_Faction scrFaction = SCR_Faction.Cast(faction);
		
		scrFaction.GetCallsignInfo().SetSquadArray(callsignArray);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 HELPER METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	void LockAllOpenSlots()
	{
		// Lock all empty slots
		foreach (int slotID, COA_SlotData slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentPlayerId() <= 0)
				UpdateSlotLockedState(slotID, true);
		}
		
		// Process groups
		array<SCR_AIGroup> allGroups = GetAllGroups();
		foreach (SCR_AIGroup group : allGroups)
		{    
			// Skip already private groups
			if (group.IsPrivate())
				continue;
			
			if (IsEmptyGroup(group))
				group.SetPrivate(true);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void CleanupCharacterFromSlot(COA_SlotData slotData)
	{
		if (!slotData)
			return;
			
		RplId charId = slotData.GetSlotCurrentCharacter();
		if (charId == RplId.Invalid())
			return;
			
		RplComponent rplComp = RplComponent.Cast(Replication.FindItem(charId));
		if (!rplComp)
			return;
			
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(rplComp.GetEntity());
		if (character)
			SCR_EntityHelper.DeleteEntityAndChildren(character);
			
		slotData.SetSlotCurrentCharacter(RplId.Invalid());
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 CLIENT SIDE REPLICATION METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Client-side: Update single slot from RPC (called by COA_RplBroadcastManager)
	//! Only updates if data actually changed (prevents unnecessary UI rebuilds)
	void UpdateSlotDataClient(COA_SlotData slotData)
	{
		if (Replication.IsServer())
			return;  // Server doesn't receive these, only sends
		
		int slotId = slotData.GetSlotId();
		COA_SlotData oldSlotData = m_mSlotsMap.Get(slotId);

		if(!oldSlotData)
			m_mSlotsMap.Set(slotId, slotData);
		else
			oldSlotData.DataUpdate(slotData);
				
		// Trigger UI update
		if (m_OnSlottingUpdate)
			m_OnSlottingUpdate.Invoke();
		
		Print(string.Format("[COA_SlottingManager] Client received slot %1 update", slotId), LogLevel.VERBOSE);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Client-side: Remove slot from RPC (called by COA_RplBroadcastManager)
	void RemoveSlotClient(int slotId)
	{
		if (Replication.IsServer())
			return;
		
		m_mSlotsMap.Remove(slotId);
		
		if (m_OnSlottingUpdate)
			m_OnSlottingUpdate.Invoke();
		
		Print(string.Format("[COA_SlottingManager] Client removed slot %1", slotId), LogLevel.VERBOSE);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 REPLICATION
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	override protected bool RplSave(ScriptBitWriter writer)
	{
		// Save slotData
		int slotsCount = m_mSlotsMap.Count();
		writer.WriteInt(slotsCount);
		foreach (int slotId, COA_SlotData slotData : m_mSlotsMap)
		{
			slotData.Save(writer);
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override protected bool RplLoad(ScriptBitReader reader)
	{
		// Load slotData
		int slotsCount;
		reader.ReadInt(slotsCount);
		for (int i = 0; i < slotsCount; i++)
		{
			COA_SlotData slotData = new COA_SlotData();
			slotData.Load(reader);
			UpdateSlotDataClient(slotData);
		}

		return true;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static COA_SlottingManager m_sInstance;
	void COA_SlottingManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_SlottingManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static COA_SlottingManager GetInstance()
	{
		return m_sInstance;
	}
}