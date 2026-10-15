//------------------------------------------------------------------------------------------------
//! Dead bodies for players who join (or stream a body in) after the death.
//! A death ragdoll is simulated locally on each client that saw the death and is never replicated.
//! A client that only receives the character once it is already dead got no death event, so no
//! ragdoll ever ran there and the body stood in the skeleton's bind ("A") pose. Here such a body is
//! dropped into a ragdoll on that client, so it collapses where it lies. Its pose won't match other
//! clients' (ragdolls never do), but it reads as a body.
//!
//! Two triggers, since it's not certain which the engine raises for a body that arrives dead: the
//! JIP life state change, and a check shortly after the character is created on this client (retried
//! while the dead state replicates). The body's physics ragdoll (PhysicsRagdoll) is enabled first; the
//! controller's Ragdoll() is the fallback if there is none or it didn't take.
modded class SCR_CharacterControllerComponent
{
	protected static const bool COA_JIP_RAGDOLL_LOG = false;		// diagnostics: "[Coalition Lobby] Dead body pose" lines on the client
	protected static const int COA_JIP_RAGDOLL_FIRST_CHECK_MS = 1500;
	protected static const int COA_JIP_RAGDOLL_RETRY_MS = 1000;
	protected static const int COA_JIP_RAGDOLL_MAX_CHECKS = 5;

	protected int m_iCOA_RagdollChecks;
	protected bool m_bCOA_RagdollRequested;

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

		// Clients only: the server (and anyone who saw the death) already ragdolled the body
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
	//! Ragdoll a dead body that isn't ragdolling on this client
	protected void COA_CheckBodyPose()
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(GetOwner());
		if (!character)
			return;

		m_iCOA_RagdollChecks++;

		// Not dead (yet, as far as this client knows): look again while the state may still be replicating
		if (GetLifeState() != ECharacterLifeState.DEAD)
		{
			if (m_iCOA_RagdollChecks < COA_JIP_RAGDOLL_MAX_CHECKS && !m_bCOA_RagdollRequested)
				COA_ScheduleBodyCheck(COA_JIP_RAGDOLL_RETRY_MS);
			return;
		}

		// Dead in a vehicle seat: stays seated, as it does for everyone else
		if (CompartmentAccessComponent.GetVehicleIn(character))
			return;

		CharacterAnimationComponent animation = GetAnimationComponent();
		if (animation && animation.IsRagdollActive())
		{
			if (m_bCOA_RagdollRequested)
				COA_Log("ragdoll running");
			return;
		}

		if (!m_bCOA_RagdollRequested)
		{
			// First attempt: the body's physics ragdoll (what a death ragdoll is)
			m_bCOA_RagdollRequested = true;
			PhysicsRagdoll physicsRagdoll = PhysicsRagdoll.GetRagdoll(character);
			if (physicsRagdoll && physicsRagdoll.Enable())
			{
				COA_Log("enabled physics ragdoll");
			}
			else
			{
				COA_Log("no physics ragdoll to enable - controller ragdoll");
				Ragdoll();
			}

			COA_ScheduleBodyCheck(500);
			return;
		}

		// The physics request didn't take: the controller's ragdoll, once
		if (m_iCOA_RagdollChecks <= COA_JIP_RAGDOLL_MAX_CHECKS + 1)
		{
			COA_Log("physics ragdoll didn't start - controller ragdoll");
			Ragdoll();
			m_iCOA_RagdollChecks = COA_JIP_RAGDOLL_MAX_CHECKS + 2;
			COA_ScheduleBodyCheck(500);
			return;
		}

		COA_Log("still not ragdolling after both attempts");
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
