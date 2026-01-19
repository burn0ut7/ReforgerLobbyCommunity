class PS_Helper
{
	static IEntity SpawnPrefab(ResourceName prefab)
	{
		EntitySpawnParams params = new EntitySpawnParams();
		
		GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), params);
		
		return entity;
	}
	
	static IEntity SpawnPrefab(ResourceName prefab, vector position)
	{
		EntitySpawnParams params = new EntitySpawnParams();
		params.Transform[3] = position;
		
		GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), params);
		
		return entity;
	}
	
	static IEntity SpawnPrefab(ResourceName prefab, vector[4] transform)
	{
		EntitySpawnParams params = new EntitySpawnParams();
		params.Transform = transform;
		
		GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), params);
		
		return entity;
	}
}