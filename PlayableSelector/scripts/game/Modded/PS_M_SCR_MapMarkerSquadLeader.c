modded class SCR_MapMarkerSquadLeader
{
	override void OnCreateMarker()
	{
		PS_GameModeCoop gameModeCoop = PS_GameModeCoop.GetInstance();
		if (!gameModeCoop)
		{
			super.OnCreateMarker();
			return;
		}
		
		if (!gameModeCoop.GetDisableLeaderSquadMarkers())
		{
			SCR_EntityHelper.DeleteEntityAndChildren(this);
			super.OnCreateMarker();
			return;
		}
	}
}