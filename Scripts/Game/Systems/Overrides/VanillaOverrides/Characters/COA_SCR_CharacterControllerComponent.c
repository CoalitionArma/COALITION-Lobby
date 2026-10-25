//------------------------------------------------------------------------------------------------
//! Dead bodies for players who join, or stream a body in, after the death.
//! A death ragdoll's start is replicated only to the clients that have the body at that moment. A
//! client that receives the character once it is already dead (joined later, or the body left its
//! replication bubble and came back) never gets a ragdoll, so the body stands in the skeleton's bind
//! ("A") pose.
//!
//! Ragdoll() only works on the body's owner (the server, for a corpse), so a client can't fix this
//! itself. Instead the client asks the server (COA_PlayerRplToAuthorityManager.RequestBodyRagdoll),
//! and the server restarts the ragdoll on the owner. The engine then replicates the ragdoll start to
//! every client that has the body, the new one included. The server's body is already lying down,
//! so it settles where it lies.
//!
//! Two client triggers, since it's not certain which the engine raises for a body that arrives dead:
//! the JIP life state change, and a check shortly after the character is created on this client
//! (retried while the dead state replicates).
modded class SCR_CharacterControllerComponent
{
	protected static const bool COA_JIP_RAGDOLL_LOG = false;		// diagnostics: "[Coalition Lobby] Dead body pose" lines
	protected static const int COA_JIP_RAGDOLL_FIRST_CHECK_MS = 1500;
	protected static const int COA_JIP_RAGDOLL_RETRY_MS = 1000;
	protected static const int COA_JIP_RAGDOLL_MAX_CHECKS = 5;
	// Client: how long to wait for the server's ragdoll before asking again, and how often to ask
	protected static const int COA_JIP_RAGDOLL_VERIFY_MS = 3000;
	protected static const int COA_JIP_RAGDOLL_MAX_REQUESTS = 2;
	// Server: requests for the same body within this window share one restart (several clients
	// streaming it in together), and a body isn't restarted again sooner than the cooldown
	protected static const int COA_BODY_RAGDOLL_BATCH_MS = 500;
	protected static const float COA_BODY_RAGDOLL_COOLDOWN_MS = 2000;

	protected int m_iCOA_RagdollChecks;
	protected int m_iCOA_RagdollRequests;
	protected bool m_bCOA_RagdollRestartPending;
	protected float m_fCOA_LastRagdollRestart = -1; // world ms, -1 = never

	//------------------------------------------------------------------------------------------------
	override void OnInit(IEntity owner)
	{
		super.OnInit(owner);

		if (!Replication.IsServer())
			COA_ScheduleBodyCheck(COA_JIP_RAGDOLL_FIRST_CHECK_MS);
	}

	//------------------------------------------------------------------------------------------------
	override void OnLifeStateChanged(ECharacterLifeState previousLifeState, ECharacterLifeState newLifeState, bool isJIP)
	{
		super.OnLifeStateChanged(previousLifeState, newLifeState, isJIP);

		// Clients only: anyone who saw the death already has the ragdoll
		if (!isJIP || newLifeState != ECharacterLifeState.DEAD || Replication.IsServer())
			return;

		COA_Log("JIP death event");
		m_iCOA_RagdollChecks = 0;
		COA_ScheduleBodyCheck(100);
	}

	//------------------------------------------------------------------------------------------------
	protected void COA_ScheduleBodyCheck(int delayMs)
	{
		GetGame().GetCallqueue().Remove(COA_CheckBodyPose);
		GetGame().GetCallqueue().CallLater(COA_CheckBodyPose, delayMs);
	}

	//------------------------------------------------------------------------------------------------
	//! Client: ask for a ragdoll for a dead body that isn't ragdolling here
	protected void COA_CheckBodyPose()
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(GetOwner());
		if (!character)
			return;

		m_iCOA_RagdollChecks++;

		// Not dead (yet, as far as this client knows): look again while the state may still be replicating
		if (GetLifeState() != ECharacterLifeState.DEAD)
		{
			if (m_iCOA_RagdollChecks < COA_JIP_RAGDOLL_MAX_CHECKS && m_iCOA_RagdollRequests == 0)
				COA_ScheduleBodyCheck(COA_JIP_RAGDOLL_RETRY_MS);
			return;
		}

		// Dead in a vehicle seat: stays seated, as it does for everyone else
		if (CompartmentAccessComponent.GetVehicleIn(character))
			return;

		CharacterAnimationComponent animation = GetAnimationComponent();
		if (animation && animation.IsRagdollActive())
		{
			if (m_iCOA_RagdollRequests > 0)
				COA_Log("ragdoll running");
			return;
		}

		if (m_iCOA_RagdollRequests >= COA_JIP_RAGDOLL_MAX_REQUESTS)
		{
			COA_Log("still not ragdolling after asking the server");
			return;
		}

		m_iCOA_RagdollRequests++;

		RplComponent rpl = RplComponent.Cast(character.FindComponent(RplComponent));
		if (rpl && rpl.IsOwner())
		{
			// This client owns the body, so it can ragdoll it itself
			COA_Log("owner - ragdolling locally");
			Ragdoll();
		}
		else
		{
			COA_PlayerRplToAuthorityManager authorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
			if (!rpl || !authorityManager)
				return;

			COA_Log("asking the server for a ragdoll");
			authorityManager.RequestBodyRagdoll(rpl.Id());
		}

		COA_ScheduleBodyCheck(COA_JIP_RAGDOLL_VERIFY_MS);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: restart this dead body's ragdoll so clients that streamed it in after the death get
	//! one too. Requested by clients (COA_PlayerRplToAuthorityManager.RpcAsk_RequestBodyRagdoll).
	void COA_RestartBodyRagdoll()
	{
		if (!Replication.IsServer() || m_bCOA_RagdollRestartPending || GetLifeState() != ECharacterLifeState.DEAD)
			return;

		float now = GetGame().GetWorld().GetWorldTime();
		if (m_fCOA_LastRagdollRestart >= 0 && now - m_fCOA_LastRagdollRestart < COA_BODY_RAGDOLL_COOLDOWN_MS)
			return;

		m_bCOA_RagdollRestartPending = true;
		GetGame().GetCallqueue().CallLater(COA_DoRestartBodyRagdoll, COA_BODY_RAGDOLL_BATCH_MS);
	}

	//------------------------------------------------------------------------------------------------
	protected void COA_DoRestartBodyRagdoll()
	{
		m_bCOA_RagdollRestartPending = false;

		IEntity owner = GetOwner();
		if (!owner || GetLifeState() != ECharacterLifeState.DEAD || CompartmentAccessComponent.GetVehicleIn(owner))
			return;

		m_fCOA_LastRagdollRestart = GetGame().GetWorld().GetWorldTime();

		// Ragdoll() only runs on the owner - normally the server for a corpse
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (rpl && !rpl.IsOwner())
		{
			COA_Log("server: ragdoll restart sent to the owner");
			MakeOwnerRagdoll_S(0);
			return;
		}

		COA_Log("server: ragdoll restarted");
		Ragdoll();
	}

	//------------------------------------------------------------------------------------------------
	protected void COA_Log(string message)
	{
		if (!COA_JIP_RAGDOLL_LOG)
			return;

		IEntity owner = GetOwner();
		string where = "?";
		if (owner)
			where = owner.GetOrigin().ToString();

		Print(string.Format("[Coalition Lobby] Dead body pose: %1 (%2 at %3)", message, owner, where), LogLevel.NORMAL);
	}
}
