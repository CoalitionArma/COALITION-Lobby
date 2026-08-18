[BaseContainerProps(), SCR_BaseManualCameraComponentTitle()]
modded class SCR_AdjustSpeedManualCameraComponent : SCR_BaseManualCameraComponent
{
	//------------------------------------------------------------------------------------------------
	override void EOnCameraFrame(SCR_ManualCameraParam param)
	{
		if (!param.isManualInputEnabled)
		{
			if (m_Widget)
				m_Widget.SetVisible(false);
			
			return;
		}
		
		//--- Adjust
		if (param.flag & EManualCameraFlag.ROTATE)
		{
			bool camOnRails;
			SCR_ManualCamera manCamera = GetCameraEntity();
			
			if (manCamera)
			{
				COA_SpectatorCamera specCamera = COA_SpectatorCamera.Cast(manCamera);
				
				if (!specCamera || !specCamera.GetIfCameraOnRails())
				{
					float inputValue = GetInputManager().GetActionValue("ManualCameraSpeedAdjust");
					if (inputValue != 0 && !camOnRails)
					{
						inputValue = Math.Clamp(1 + inputValue * param.timeSlice, 0.5, 2);
						m_fMultiplier = Math.Clamp(m_fMultiplier * inputValue, m_fMinMultiplier, m_fMaxMultiplier);
						m_OnSpeedChange.Invoke(m_fMultiplier, true);
						UpdateWidget();
					}
				};
			};
		}
		
		//--- Apply
		param.multiplier *= m_fMultiplier;
		
		//--- Visualize
		FadeOutWidget(param.timeSlice);
	}
}