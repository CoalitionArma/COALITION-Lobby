//----------------------------------------------------------------
// Moving static spawn points (flag poles) in Zeus.
//
// COA_StaticSpawnPoint inherits Vehicle and has a RigidBody, so vanilla's
// SCR_EditableEntityComponent.SetTransform takes its "Vehicle" branch: Teleport() on the server
// and an RPC to the owner (also the server), expecting vehicle movement replication to carry the
// move to clients. The spawn point has none, so the pole never moved for anyone else and snapped
// back in Zeus; the only way to relocate a spawn was to delete the pole and place a new one.
// Route the move through COA_StaticSpawnPoint.MoveTo_S, which replicates it.
//----------------------------------------------------------------
modded class SCR_EditableSystemComponent : SCR_EditableEntityComponent
{
	//------------------------------------------------------------------------------------------------
	override bool SetTransform(vector transform[4], bool changedByUser = false)
	{
		COA_StaticSpawnPoint spawnPoint = COA_StaticSpawnPoint.Cast(GetOwner());
		if (!spawnPoint || !Replication.IsServer())
			return super.SetTransform(transform, changedByUser);

		vector prevTransform[4];
		spawnPoint.GetWorldTransform(prevTransform);

		if (changedByUser)
			SetHierarchyAsDirtyInParents();

		spawnPoint.MoveTo_S(transform);

		SCR_EditableEntityCore core = SCR_EditableEntityCore.Cast(SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
		if (core)
			core.Event_OnEntityTransformChangedServer.Invoke(this, prevTransform);

		RplComponent rpl = RplComponent.Cast(spawnPoint.FindComponent(RplComponent));
		if (rpl)
			rpl.ForceNodeMovement(prevTransform[3]);

		return true;
	}
}
