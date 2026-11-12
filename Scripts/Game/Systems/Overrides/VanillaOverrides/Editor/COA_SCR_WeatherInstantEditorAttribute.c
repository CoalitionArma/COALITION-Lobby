//! Game Master "Weather" attribute. Vanilla forces the new state with a 0 h transition (its transition-time
//! attribute is unfinished and always reads 0), so clouds, fog and rain snap over in one frame. Blend instead,
//! over COA_WEATHER_TRANSITION_REAL_SECONDS of real time.
//! Attributes repeated from vanilla: this class is built from the editor attribute configs.
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
modded class SCR_WeatherInstantEditorAttribute : SCR_BasePresetsEditorAttribute
{
	protected static const float COA_WEATHER_TRANSITION_REAL_SECONDS = 10;

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		// No item = the Game Master's local preview while browsing the list; leave that instant
		if (!item || !var)
		{
			super.WriteVariable(item, var, manager, playerID);
			return;
		}

		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;

		ChimeraWorld world = ChimeraWorld.CastFrom(gameMode.GetWorld());
		if (!world)
			return;

		TimeAndWeatherManagerEntity weatherManager = world.GetTimeAndWeatherManager();
		if (!weatherManager)
			return;

		int stateIndex = var.GetInt();

		array<ref WeatherState> weatherStates = {};
		weatherManager.GetWeatherStatesList(weatherStates);
		if (stateIndex < 0 || stateIndex >= weatherStates.Count())
			return;

		WeatherState weatherToSet = weatherStates[stateIndex];
		if (!weatherToSet)
			return;

		// Same call as vanilla (looping, so the state holds; player ID for the GM notification) - only the transition differs
		weatherManager.ForceWeatherTo(true, weatherToSet.GetStateName(), COA_GetTransitionHours(weatherManager), 0.001, playerID);
	}

	//------------------------------------------------------------------------------------------------
	//! The transition length is in in-game hours, so convert from real seconds using the current day length
	//! (missions speed time up through COA_Gamemode's time scale, which shortens the day).
	//! \return in-game hours lasting COA_WEATHER_TRANSITION_REAL_SECONDS, or 0 (instant) when time is frozen
	protected float COA_GetTransitionHours(TimeAndWeatherManagerEntity weatherManager)
	{
		// Weather only progresses while time advances (vanilla's forecast UI assumes the same), so a
		// transition with time frozen could hang half-way - keep the instant change there
		if (!weatherManager.GetIsDayAutoAdvanced())
			return 0;

		float dayDurationSeconds = weatherManager.GetDayDuration();
		if (dayDurationSeconds <= 0)
			return 0;

		return COA_WEATHER_TRANSITION_REAL_SECONDS * 24 / dayDurationSeconds;
	}
}
