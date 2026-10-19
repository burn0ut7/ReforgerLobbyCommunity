//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class PS_VoNRoomsManagerClass: ScriptComponentClass
{
	
};

// just string but funnier 
// struct: [FactionKey + "|"] + string
typedef string VoNRoomKey;

enum EChannelType
{
	ADMIN,
	PRIMARY,
	SECONDARY
}

// Manage VoN "Rooms"
// Flying wallles rooms for naked VoN bois.

class PS_VoNRoomsManager : ScriptComponent
{
	protected static const int PS_LOCAL_ROOM_FREQUENCY_RETRY_DELAY_MS = 250;
	protected static const int PS_LOCAL_ROOM_FREQUENCY_MAX_ATTEMPTS = 40;

	// server data
	ref map<VoNRoomKey, int> m_mVoiceRoomsFromName = new map<VoNRoomKey, int>(); // room key to roomId relationship
	
	// Replication data
	ref map<int, VoNRoomKey> m_mVoiceRooms = new map<int, VoNRoomKey>(); // room names for UI
	ref map<int, int> m_mPlayersRooms = new map<int, int>(); // player to room relationship
	
	// offset every room
	vector lastOffset;
	int m_iLastRoomId = 1;
	
	// Invokers
	ref ScriptInvoker m_eOnRoomChanged = new ScriptInvoker(); // int playerId, int roomId, int oldRoomId
		
	bool m_bRplLoaded = false;
	protected int m_iLocalRoomFrequencyReconcileGeneration;
	bool IsReplicated()
	{
		return m_bRplLoaded;
	}
	
	override protected void OnPostInit(IEntity owner)
	{
		SCR_BaseGameMode baseGameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		baseGameMode.GetOnPlayerConnected().Insert(OnPlayerConnected);
		
		// Set default room position
		m_mVoiceRooms[0] = "";
		m_mVoiceRoomsFromName[""] = 0;
		if (Replication.IsServer()) m_bRplLoaded = true;
	}
	
	void OnPlayerConnected(int playerId)
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.GetInstance();
		PrintFormat("[PS_VON_DIAG] event=ROOM_MANAGER_PLAYER_CONNECTED playerId=%1 gameState=%2 existingRoomId=%3", playerId, gameMode.GetState(), GetPlayerRoom(playerId));
		// Create local room for player
		GetOrCreateRoomWithFaction("", "#PS-VoNRoom_Local" + playerId.ToString());
		GetOrCreateRoomWithFaction("", "#PS-VoNRoom_Public" + playerId.ToString());
	}
	
	// more singletons for singletons god, make our spagetie kingdom great
	static PS_VoNRoomsManager GetInstance() 
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return PS_VoNRoomsManager.Cast(gameMode.FindComponent(PS_VoNRoomsManager));
		else
			return null;
	}
	
	// ------------------------- Room changing -------------------------
	// Move to room by key and create if not exist, RUN ONLY ON SERVER
	void MoveToRoom(int playerId, FactionKey factionKey, string roomName)
	{
		if (!Replication.IsServer())
			return;
		
		// Create new room and id if need
		int roomId = GetOrCreateRoomWithFaction(factionKey, roomName);
		PS_PlayableManager playableManagerSnapshot = PS_PlayableManager.GetInstance();
		PrintFormat("[PS_VON_DIAG] event=ROOM_MOVE_SERVER_BEGIN playerId=%1 requestedFaction=%2 requestedRoom='%3' newRoomId=%4 oldRoomId=%5 customFaction=%6 rememberedFaction=%7", playerId, factionKey, roomName, roomId, GetPlayerRoom(playerId), playableManagerSnapshot.GetPlayerFactionKey(playerId), playableManagerSnapshot.GetPlayerFactionKeyRemembered(playerId));
		
		// A same-room request is intentional for JIP and entity replacement. The
		// room map can already be correct while the newly-created radio is still
		// on its authored default frequency, so the frequency must be reasserted.
		bool sameRoom = roomId == GetPlayerRoom(playerId);
		if (sameRoom)
			PrintFormat("[PS_VON_DIAG] event=ROOM_MOVE_REASSERTED playerId=%1 roomId=%2", playerId, roomId);
		
		// Apply on the server when the radio is already available. The owning
		// client also applies it from RPC_MoveToRoom with lifecycle retries.
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PlayerController playerController = playerManager.GetPlayerController(playerId);
		PS_PlayableControllerComponent playableController;
		if (playerController)
			playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));

		BaseTransceiver transceiver;
		if (playableController)
			transceiver = playableController.GetTransceiver(EChannelType.PRIMARY);

		if (transceiver)
		{
			int oldFrequency = transceiver.GetFrequency();
			BaseRadioComponent radio = transceiver.GetRadio();
			transceiver.SetFrequency(roomId);
			if (radio)
				PrintFormat("[PS_VON_DIAG] event=SERVER_ROOM_FREQUENCY_APPLIED playerId=%1 roomId=%2 oldFrequency=%3 newFrequency=%4 sameRoom=%5 radio=%6 powered=%7 encryption=%8", playerId, roomId, oldFrequency, transceiver.GetFrequency(), sameRoom, radio, radio.IsPowered(), radio.GetEncryptionKey());
			else
				PrintFormat("[PS_VON_DIAG] event=SERVER_ROOM_FREQUENCY_APPLIED playerId=%1 roomId=%2 oldFrequency=%3 newFrequency=%4 sameRoom=%5 radio=NULL", playerId, roomId, oldFrequency, transceiver.GetFrequency(), sameRoom);
		}
		else
		{
			PrintFormat("[PS_VON_DIAG] event=SERVER_ROOM_FREQUENCY_DEFERRED playerId=%1 roomId=%2 sameRoom=%3 reason=transceiver_unavailable", playerId, roomId, sameRoom);
		}
		
		// Finally move client to room
		RPC_MoveToRoom(playerId, roomId);
		Rpc(RPC_MoveToRoom, playerId, roomId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_MoveToRoom(int playerId, int roomId)
	{
		int oldRoomId = GetPlayerRoom(playerId);
		
		m_mPlayersRooms[playerId] = roomId;
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		FactionKey customFaction = "<manager unavailable>";
		FactionKey rememberedFaction = "<manager unavailable>";
		if (playableManager)
		{
			customFaction = playableManager.GetPlayerFactionKey(playerId);
			rememberedFaction = playableManager.GetPlayerFactionKeyRemembered(playerId);
		}
		PrintFormat("[PS_VON_DIAG] event=ROOM_MOVE_REPLICATED playerId=%1 oldRoomId=%2 newRoomId=%3 roomName='%4' customFaction=%5 rememberedFaction=%6 isServer=%7 rplMode=%8", playerId, oldRoomId, roomId, GetRoomName(roomId), customFaction, rememberedFaction, Replication.IsServer(), RplSession.Mode());

		// The room map is replicated independently from the radio. Reconcile the
		// owning client's radio after the RPC, retrying while JIP creates its
		// player controller, initial entity, gadget, and transceiver.
		PS_ScheduleLocalRoomFrequencyReconcile("room_rpc");
		
		m_eOnRoomChanged.Invoke(playerId, roomId, oldRoomId);
	}

	//------------------------------------------------------------------------------------------------
	protected void PS_ScheduleLocalRoomFrequencyReconcile(string source)
	{
		if (RplSession.Mode() == RplMode.Dedicated)
			return;

		m_iLocalRoomFrequencyReconcileGeneration++;
		int generation = m_iLocalRoomFrequencyReconcileGeneration;
		GetGame().GetCallqueue().CallLater(PS_ReconcileLocalRoomFrequency, 1, false, source, generation, 1);
	}

	//------------------------------------------------------------------------------------------------
	protected void PS_ReconcileLocalRoomFrequency(string source, int generation, int attempt)
	{
		if (generation != m_iLocalRoomFrequencyReconcileGeneration)
			return;

		PlayerController playerController = GetGame().GetPlayerController();
		int playerId;
		int roomId = -1;
		string roomKey;
		FactionKey factionKey;
		int factionFrequency;
		PS_PlayableControllerComponent playableController;
		BaseTransceiver primaryTransceiver;
		BaseTransceiver secondaryTransceiver;
		string waitingFor = "player_controller";
		bool factionFrequencyRequired;
		bool factionFrequencyReady;

		if (playerController)
		{
			playerId = playerController.GetPlayerId();
			waitingFor = "player_id";
			if (playerId != 0)
			{
				roomId = GetPlayerRoom(playerId);
				waitingFor = "room_assignment";
				if (roomId >= 0)
				{
					roomKey = GetRoomName(roomId);
					factionFrequencyRequired = roomKey != "" && !roomKey.StartsWith("|");
					playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
					waitingFor = "playable_controller";
					if (playableController)
					{
						primaryTransceiver = playableController.GetTransceiver(EChannelType.PRIMARY);
						waitingFor = "primary_transceiver";

						if (primaryTransceiver && factionFrequencyRequired)
						{
							PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
							waitingFor = "playable_manager_snapshot";
							if (playableManager && playableManager.IsReplicated())
							{
								factionKey = playableManager.GetPlayerFactionKey(playerId);
								waitingFor = "faction_assignment";
								if (factionKey != "" && roomKey.StartsWith(factionKey + "|"))
								{
									FactionManager factionManager = GetGame().GetFactionManager();
									Faction faction;
									waitingFor = "faction_manager";
									if (factionManager)
									{
										faction = factionManager.GetFactionByKey(factionKey);
										waitingFor = "faction_definition";
										if (faction)
										{
											int factionIndex = factionManager.GetFactionIndex(faction);
											waitingFor = "faction_index";
											if (factionIndex >= 0)
											{
												factionFrequency = factionIndex + 1000;
												secondaryTransceiver = playableController.GetTransceiver(EChannelType.SECONDARY);
												waitingFor = "secondary_transceiver";
												factionFrequencyReady = secondaryTransceiver != null;
											}
										}
									}
								}
							}
						}
						else if (primaryTransceiver)
						{
							factionFrequencyReady = true;
						}
					}
				}
			}
		}

		if (primaryTransceiver)
		{
			int oldPrimaryFrequency = primaryTransceiver.GetFrequency();
			primaryTransceiver.SetFrequency(roomId);
			int newPrimaryFrequency = primaryTransceiver.GetFrequency();
			if (oldPrimaryFrequency != roomId || attempt == 1)
				PrintFormat("[PS_VON_DIAG] event=LOCAL_ROOM_FREQUENCY_APPLIED source=%1 attempt=%2 playerId=%3 roomId=%4 oldFrequency=%5 newFrequency=%6", source, attempt, playerId, roomId, oldPrimaryFrequency, newPrimaryFrequency);

			if (newPrimaryFrequency != roomId)
				waitingFor = "primary_frequency_apply";
		}

		if (primaryTransceiver && primaryTransceiver.GetFrequency() == roomId && factionFrequencyReady)
		{
			if (factionFrequencyRequired)
			{
				int oldFactionFrequency = secondaryTransceiver.GetFrequency();
				secondaryTransceiver.SetFrequency(factionFrequency);
				int newFactionFrequency = secondaryTransceiver.GetFrequency();
				if (newFactionFrequency != factionFrequency)
				{
					waitingFor = "faction_frequency_apply";
					factionFrequencyReady = false;
				}
				else
				{
					PrintFormat("[PS_VON_DIAG] event=LOCAL_FACTION_FREQUENCY_APPLIED source=%1 attempt=%2 playerId=%3 factionKey=%4 roomId=%5 oldFrequency=%6 newFrequency=%7", source, attempt, playerId, factionKey, roomId, oldFactionFrequency, newFactionFrequency);
					return;
				}
			}
			else
			{
				return;
			}
		}

		if (!primaryTransceiver || primaryTransceiver.GetFrequency() != roomId || !factionFrequencyReady)
		{
			if (attempt >= PS_LOCAL_ROOM_FREQUENCY_MAX_ATTEMPTS)
			{
				PrintFormat("[PS_VON_DIAG] event=LOCAL_RADIO_FREQUENCY_FAILED source=%1 attempt=%2 playerId=%3 roomId=%4 roomKey=%5 factionKey=%6 waitingFor=%7", source, attempt, playerId, roomId, roomKey, factionKey, waitingFor);
				return;
			}

			if (attempt == 1 || attempt % 8 == 0)
				PrintFormat("[PS_VON_DIAG] event=LOCAL_RADIO_FREQUENCY_WAIT source=%1 attempt=%2 playerId=%3 roomId=%4 roomKey=%5 factionKey=%6 waitingFor=%7", source, attempt, playerId, roomId, roomKey, factionKey, waitingFor);

			GetGame().GetCallqueue().CallLater(PS_ReconcileLocalRoomFrequency, PS_LOCAL_ROOM_FREQUENCY_RETRY_DELAY_MS, false, source, generation, attempt + 1);
			return;
		}
	}
	void RestoreRoom(int playerId)
	{
		int roomId = GetPlayerRoom(playerId);
		if (roomId == -1)
			return;
		
		string roomKey = GetRoomName(roomId);
		// TODO: separate to static method
		string factionKey = "";
		string roomName = "#PS-VoNRoom_Global";	
		if (roomKey.Contains("|")) {
			array<string> outTokens = new array<string>();
			roomKey.Split("|", outTokens, false);
			factionKey = outTokens[0];
			roomName = outTokens[1];
		}
		
		RPC_MoveToRoom(playerId, roomId);
		Rpc(RPC_MoveToRoom, playerId, roomId);
	}
	
	// ------------------------- Room creation -------------------------
	// Create room if new key provided, RUN ONLY ON SERVER
	int GetOrCreateRoomWithFaction(FactionKey factionKey, string roomName)
	{
		VoNRoomKey roomKey = factionKey + "|" + roomName;
		if (roomKey == "|") roomKey = "";
		return GetOrCreateRoom(roomKey);
	}
	int GetOrCreateRoom(VoNRoomKey roomKey)
	{
		if (!Replication.IsServer()) return -1;
		if (!m_mVoiceRoomsFromName.Contains(roomKey)) {
			// Create on every client
			RPC_CreateRoom(m_iLastRoomId, roomKey);
			Rpc(RPC_CreateRoom, m_iLastRoomId, roomKey);
			m_iLastRoomId++;
		}
		return m_mVoiceRoomsFromName[roomKey];
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_CreateRoom(int roomId, VoNRoomKey roomKey)
	{
		m_mVoiceRoomsFromName[roomKey] = roomId;
		m_mVoiceRooms[roomId] = roomKey;
	}
	
	// ------------------------- Get -------------------------
	int GetPlayerRoom(int playerId)
	{
		if (!m_mPlayersRooms.Contains(playerId)) return -1;
		return m_mPlayersRooms[playerId];
	}
	int GetRoomWithFaction(FactionKey factionKey, string roomName)
	{
		string roomKey = factionKey + "|" + roomName;
		if (roomKey == "|") roomKey = "";
		if (!m_mVoiceRoomsFromName.Contains(roomKey)) return -1;
		return m_mVoiceRoomsFromName[roomKey];
	}
	string GetRoomName(int roomId)
	{
		if (!m_mVoiceRooms.Contains(roomId)) return "";
		return m_mVoiceRooms[roomId];
	}
	
	void GetPlayersPublicRooms(out notnull array<int> rooms)
	{
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			int playerRoomId = GetPlayerRoom(playerId);
			if (rooms.Contains(playerRoomId)) continue;
			if (IsPublicRoom(playerRoomId))
			{
				rooms.Insert(playerRoomId);
			}
		}
	}
	
	void GetPlayersInRoom(out notnull array<int> players, int roomId)
	{
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			if (GetPlayerRoom(playerId) == roomId)
				players.Insert(playerId);
		}
	}
	
	bool IsPublicRoom(int roomId)
	{
		string playerRoomName = GetRoomName(roomId);
		if (playerRoomName.Length() <= 13) return false;
		if (playerRoomName.ContainsAt("Public", 13))
		{
			return true;
		}
		return false;
	}
	
	bool IsFactionRoom(int roomId, FactionKey factionKey)
	{
		string playerRoomName = GetRoomName(roomId);
		if (factionKey == "")
		{
			if (playerRoomName.StartsWith("|")) return true;
			return false;
		}
		if (playerRoomName.StartsWith(factionKey + "|"))
		{
			return true;
		}
		return false;
	}
	
	bool IsGlobalRoom(int roomId)
	{
		string playerRoomName = GetRoomName(roomId);
		return playerRoomName == "|#PS-VoNRoom_Global";
	}
	
	bool IsLocalRoom(int roomId)
	{
		string playerRoomName = GetRoomName(roomId);
		return playerRoomName.StartsWith("|#PS-VoNRoom_Local");
	}
	
	// ------------------------- JIP Replication -------------------------
	// Send our precision data, we need it on clients
	override bool RplSave(ScriptBitWriter writer)
	{
		int playersRoomsCount = m_mPlayersRooms.Count();
		writer.WriteInt(playersRoomsCount);
		for (int i = 0; i < playersRoomsCount; i++)
		{
			writer.WriteInt(m_mPlayersRooms.GetKey(i));
			writer.WriteInt(m_mPlayersRooms.GetElement(i));
		}
		
		int voiceRoomsCount = m_mVoiceRooms.Count();
		writer.WriteInt(voiceRoomsCount);
		for (int i = 0; i < voiceRoomsCount; i++)
		{
			writer.WriteInt(m_mVoiceRooms.GetKey(i));
			writer.WriteString(m_mVoiceRooms.GetElement(i));
		}
		
		return true;
	}
	
	override bool RplLoad(ScriptBitReader reader)
	{
		int playersRoomsCount;
		reader.ReadInt(playersRoomsCount);
		for (int i = 0; i < playersRoomsCount; i++)
		{
			int key;
			int value;
			reader.ReadInt(key);
			reader.ReadInt(value);
			
			m_mPlayersRooms.Insert(key, value);
		}
		int voiceRoomsCount;
		reader.ReadInt(voiceRoomsCount);
		for (int i = 0; i < voiceRoomsCount; i++)
		{
			int key;
			string value;
			reader.ReadInt(key);
			reader.ReadString(value);
			
			m_mVoiceRooms.Insert(key, value);
			m_mVoiceRoomsFromName.Insert(value, key);
		}
		
		m_bRplLoaded = true;
		PrintFormat("[PS_VON_DIAG] event=ROOM_JIP_SNAPSHOT_LOADED playerRoomCount=%1 voiceRoomCount=%2 localPlayerController=%3 isServer=%4 rplMode=%5", playersRoomsCount, voiceRoomsCount, GetGame().GetPlayerController(), Replication.IsServer(), RplSession.Mode());
		PS_ScheduleLocalRoomFrequencyReconcile("jip_snapshot");
		
		return true;
	}
};
