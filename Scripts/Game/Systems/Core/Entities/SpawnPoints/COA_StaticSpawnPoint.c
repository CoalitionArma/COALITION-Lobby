class COA_StaticSpawnPointClass : COA_SpawnPointClass
{
}

class COA_StaticSpawnPoint: COA_SpawnPoint
{
	// COA_SpawnPoint is a Vehicle with static physics and no vehicle movement replication, so nothing
	// syncs its transform: a pole moved on the server stayed put on every client (Zeus moves snapped
	// back, scripted moves left the flag and map marker behind). Moves go through MoveTo_S, which
	// replicates the new transform to live clients and late joiners. m_iMoveCount 0 = never moved:
	// keep the world-file position.
	[RplProp(onRplName: "OnMoveReplicated")]
	protected vector m_vMovedPosition;

	[RplProp(onRplName: "OnMoveReplicated")]
	protected vector m_vMovedAngles;

	[RplProp(onRplName: "OnMoveReplicated")]
	protected int m_iMoveCount;

	//------------------------------------------------------------------------------------------------
	//! Server: move the spawn point (and its flag and map marker with it) on every machine.
	//! Spawning reads the entity's position, so players spawn at the new place right away.
	void MoveTo_S(vector transform[4])
	{
		if (!Replication.IsServer())
			return;

		ApplyMovedTransform(transform);

		vector applied[4];
		GetWorldTransform(applied);
		m_vMovedPosition = applied[3];
		m_vMovedAngles = Math3D.MatrixToAngles(applied);
		m_iMoveCount++;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnMoveReplicated()
	{
		if (m_iMoveCount <= 0)
			return;

		vector transform[4];
		Math3D.AnglesToMatrix(m_vMovedAngles, transform);
		transform[3] = m_vMovedPosition;
		ApplyMovedTransform(transform);
	}

	//------------------------------------------------------------------------------------------------
	//! What vanilla's SCR_EditableEntityComponent.SetTransformBroadcast does for non-vehicles
	protected void ApplyMovedTransform(vector transform[4])
	{
		float scale = GetScale();
		SetWorldTransform(transform);
		SetScale(scale);
		Update();
		OnTransformReset();

		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(FindComponent(SCR_EditableEntityComponent));
		if (!editable)
			return;

		editable.UpdateStaticPos();

		SCR_EditableEntityCore core = SCR_EditableEntityCore.Cast(SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
		if (core)
			core.Event_OnEntityTransformChanged.Invoke(editable);
	}
};
