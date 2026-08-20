//------------------------------------------------------------------------------------------------
//! Server-only, unreplicated bookkeeping for GM-marked "possession" slots (see
//! COA_SlottingManager.RegisterGMPossessionGroup and COA_MarkGroupJoinableAction). A slot ID only
//! appears in m_mPossessionTargets between the moment it's registered and the moment a player
//! actually possesses that body - COA_GamemodeManager consumes the entry the instant hand-off
//! happens, so afterwards the slot is indistinguishable from a normal one and this class has
//! nothing left to say about it. No client ever needs this data: COA_SlotData's replicated fields
//! (role/faction/group/character) already carry everything the slotting menu renders.
class COA_GMPossessionManager
{
	protected ref map<int, RplId> m_mPossessionTargets = new map<int, RplId>();
	protected ref set<RplId> m_sRegisteredGroups = new set<RplId>();

	protected static ref COA_GMPossessionManager s_Instance;

	//------------------------------------------------------------------------------------------------
	static COA_GMPossessionManager GetInstance()
	{
		if (!s_Instance)
			s_Instance = new COA_GMPossessionManager();

		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	bool IsGroupAlreadyRegistered(notnull SCR_AIGroup group)
	{
		RplComponent groupRplComp = RplComponent.Cast(group.FindComponent(RplComponent));
		if (!groupRplComp)
			return false;

		return m_sRegisteredGroups.Contains(groupRplComp.Id());
	}

	//------------------------------------------------------------------------------------------------
	//! \param[in] slotId the newly-allocated COA_SlotData slot ID
	//! \param[in] targetEntityId RplId of the pre-existing AI character to possess when this slot is claimed
	//! \param[in] groupId RplId of the GM-placed group, recorded so the group can't be marked twice
	void RegisterPossessionSlot(int slotId, RplId targetEntityId, RplId groupId)
	{
		m_mPossessionTargets.Set(slotId, targetEntityId);
		m_sRegisteredGroups.Insert(groupId);
	}

	//------------------------------------------------------------------------------------------------
	//! \return true if slotId is a not-yet-consumed possession slot, with the target entity's RplId in targetEntityId
	bool TryGetPossessionTarget(int slotId, out RplId targetEntityId)
	{
		return m_mPossessionTargets.Find(slotId, targetEntityId);
	}

	//------------------------------------------------------------------------------------------------
	//! One-shot: called the moment a player actually possesses the target body (or when the target
	//! is found dead and initialization falls back to a normal spawn instead). After this, the slot
	//! behaves exactly like any other slot for the rest of the mission.
	void ConsumePossessionSlot(int slotId)
	{
		m_mPossessionTargets.Remove(slotId);
	}

	//------------------------------------------------------------------------------------------------
	//! Called when a GM-marked group is deleted from the editor (see
	//! COA_SlottingManager.OnPossessionGroupDeleted). Consumes every slot the group had registered
	//! and forgets the group itself, so a re-placed group could in principle be marked again.
	//! \param[in] groupId RplId of the deleted group
	//! \param[in] slotIds every slot ID the group had registered, already resolved by the caller
	void UnregisterGroup(RplId groupId, array<int> slotIds)
	{
		foreach (int slotId : slotIds)
			ConsumePossessionSlot(slotId);

		m_sRegisteredGroups.RemoveItem(groupId);
	}
}
