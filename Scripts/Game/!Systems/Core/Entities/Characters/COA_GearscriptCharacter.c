class COA_GearscriptCharacterClass : COA_PlayerCharacterClass
{
}

class COA_GearscriptCharacter : COA_PlayerCharacter
{
	protected COA_EGearRole m_GearRole;

	//! Set once a gearscript has been applied to this character, so the deferred pass queued in
	//! EOnInit does not re-run ClearEntityGear() + a full re-equip over gear that was already
	//! applied synchronously during player initialization.
	protected bool m_bGearApplied;

//=============================================================================================================================================================================================================================================================================================================================================================
//	 CHARACTER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		COA_GearscriptManager gearscriptManager = COA_GearscriptManager.GetInstance();
		
		if (!GetGame().InPlayMode() || !gearscriptManager)
			return;
		
		// Schedule gearscript identity setup with appropriate delay
		GetGame().GetCallqueue().Call(
			gearscriptManager.SetEntityIdentity, 
			owner
		);
	
		// Apply gearscript if not on client
		// Deferred: this covers characters that are NOT spawned through player initialization (AI,
		// editor-placed, previews). For player characters, COA_GamemodeManager applies the gearscript
		// synchronously before handing the character to the player, and the m_bGearApplied guard in
		// ApplyDeferredGear() then makes this a no-op.
		if (RplSession.Mode() != RplMode.Client)
			GetGame().GetCallqueue().Call(ApplyDeferredGear);
	}

	//------------------------------------------------------------------------------------------------
	//! End-of-frame gearscript pass, skipped if the gear was already applied synchronously.
	protected void ApplyDeferredGear()
	{
		if (m_bGearApplied)
			return;

		COA_GearscriptManager gearscriptManager = COA_GearscriptManager.GetInstance();
		if (!gearscriptManager)
			return;

		gearscriptManager.SetEntityGear(this, GetPrefabData().GetPrefabName());
	}

	//------------------------------------------------------------------------------------------------
	//! Called by COA_GearscriptManager.SetEntityGear once a loadout has actually been applied.
	void MarkGearApplied()
	{
		m_bGearApplied = true;
	}

	//------------------------------------------------------------------------------------------------
	bool IsGearApplied()
	{
		return m_bGearApplied;
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 GEAR ROLE GETTER/SETTER METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	void SetGearRole(COA_EGearRole role)
	{
		m_GearRole = role;
	}
	
	//------------------------------------------------------------------------------------------------
	COA_EGearRole GetGearRole()
	{
		return m_GearRole;
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 WEAPON SELECTION METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! RPC up to the owner to select the primary weapon (should only be executed on characters that a player is controlling)
	void SelectPrimaryWeapon()
	{
		Rpc(RpcDo_SelectPrimaryWeapon);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RpcDo_SelectPrimaryWeapon()
	{
		IEntity entity = SCR_PlayerController.GetLocalControlledEntity();
		if (!entity)
			return;
		
		// Use the shared weapon selection logic from helper
		COA_WeaponHelper.SelectPrimaryWeaponForEntity(entity);
	}
}