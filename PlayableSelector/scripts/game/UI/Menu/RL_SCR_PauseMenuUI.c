modded class PauseMenuUI
{
	// Fix for Respawn button not showing in RL gamemode due to not using RespawnComponent
	// Could additionally add a checkbox to the RL gamemode
	
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		
		SCR_ButtonTextComponent buttonComp = SCR_ButtonTextComponent.GetButtonText("Respawn", m_wRoot);
		SCR_RespawnSystemComponent respawnComp = SCR_RespawnSystemComponent.GetInstance();
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		
		if (!buttonComp)
			return;
		
		buttonComp.m_OnClicked.Clear();
		buttonComp.m_OnClicked.Insert(OnRespawn);
		
		bool respawnAllowed = (respawnComp && respawnComp.IsPauseMenuRespawnEnabled()) || (gameMode.IsInherited(PS_GameModeCoop) && gameMode.GetState() == SCR_EGameModeState.GAME);
		buttonComp.SetVisible(respawnAllowed && CanRespawn());
	}
	
	override private void OnRespawn()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		IEntity ce = pc.GetControlledEntity();
		if (!ce)
			return;

		Close();
		
		CharacterControllerComponent cc = CharacterControllerComponent.Cast(ce.FindComponent(CharacterControllerComponent));
		if (cc)
			cc.ForceDeath();
	}
}