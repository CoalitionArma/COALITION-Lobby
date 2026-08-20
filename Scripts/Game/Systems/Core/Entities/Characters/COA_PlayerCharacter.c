class COA_PlayerCharacterClass : SCR_ChimeraCharacterClass
{
}

class COA_PlayerCharacter : SCR_ChimeraCharacter
{
//=============================================================================================================================================================================================================================================================================================================================================================
//	 CONSTRUCTOR/DESTRUCTOR
//=============================================================================================================================================================================================================================================================================================================================================================
	
	void COA_PlayerCharacter(IEntitySource src, IEntity parent)
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if(gamemode)
			gamemode.AddActiveCharacter(this);
	}
	
	void ~COA_PlayerCharacter()
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if(gamemode)
			gamemode.RemoveActiveCharacter(this);
	}
}