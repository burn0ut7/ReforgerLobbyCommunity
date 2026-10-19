void PS_ScriptInvokerOnCaptureMethod(BaseTransceiver transmitter);
typedef func PS_ScriptInvokerOnCaptureMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerOnCaptureMethod> PS_ScriptInvokerOnCapture;

void PS_ScriptInvokerOnReceiveMethod(int playerId, BaseTransceiver receiver, int frequency, float quality);
typedef func PS_ScriptInvokerOnReceiveMethod;
typedef ScriptInvokerBase<PS_ScriptInvokerOnReceiveMethod> PS_ScriptInvokerOnReceive;

class APS_LobbyVoNComponentClass : SCR_VoNComponentClass
{}

//------------------------------------------------------------------------------------------------
class APS_LobbyVoNComponent : SCR_VoNComponent
{
	override protected event void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		
		IEntity player = pc.GetControlledEntity();
		if (!player)
			return;
		
		PS_LobbyVoNComponent vonComp = PS_LobbyVoNComponent.Cast(player.FindComponent(PS_LobbyVoNComponent));
		if (!vonComp)
			return;
		
		vonComp.OnReceiveHandle(playerId, receiver, frequency, quality);
	}
}

//------------------------------------------------------------------------------------------------
class PS_LobbyVoNComponentClass : SCR_VoNComponentClass
{}

//------------------------------------------------------------------------------------------------
class PS_LobbyVoNComponent : SCR_VoNComponent
{
	const float PS_TRANSMISSION_TIMEOUT_MS = 400;
	protected float PS_m_fTransmitingTimeout;
	protected float m_fPSCaptureDiagnosticTimeout;
	ref map<int, float> m_fPlayerSpeachReciveTime = new map<int, float>();
	ref map<int, bool> m_fPlayerSpeachReciveIsChannel = new map<int, bool>();
	
	ref PS_ScriptInvokerOnReceive m_ScriptInvokerOnReceiveStart = new PS_ScriptInvokerOnReceive();
	PS_ScriptInvokerOnReceive GetOnReceiveStart()
	{
		return m_ScriptInvokerOnReceiveStart;
	}
	ref ScriptInvokerInt m_ScriptInvokerOnReceiveEnd = new ScriptInvokerInt();
	ScriptInvokerInt GetOnReceiveEnd()
	{
		return m_ScriptInvokerOnReceiveEnd;
	}
	ref PS_ScriptInvokerOnCapture m_ScriptInvokerOnCaptured = new PS_ScriptInvokerOnCapture();
	PS_ScriptInvokerOnCapture GetOnCaptured()
	{
		return m_ScriptInvokerOnCaptured;
	}
	
	void PS_LobbyVoNComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		GetGame().GetCallqueue().CallLater(DisablePhysic, 1, false, ent);
		GetGame().GetCallqueue().CallLater(ToggleInvisible, 1, false, ent);
		SetActive(ent);
	}
	
	void DisablePhysic(IEntity owner)
	{
		Physics physics = owner.GetPhysics();
		if (physics)
		{
			physics.SetVelocity("0 0 0");
			physics.SetAngularVelocity("0 0 0");
			physics.SetMass(0);
			physics.SetDamping(1, 1);
			physics.EnableGravity(false);
			physics.SetActive(ActiveState.INACTIVE);
			physics.ChangeSimulationState(SimulationState.NONE);
			physics.SetInteractionLayer(EPhysicsLayerDefs.Unused);
		}
	}
	
	void ToggleInvisible(IEntity owner)
    {
        owner.ClearFlags(EntityFlags.VISIBLE, false);
    }
	
	void SetActive(IEntity owner)
	{
		PlayerController pc = GetGame().GetPlayerController();
		
		SCR_VONController vonContr = SCR_VONController.Cast(owner.FindComponent(SCR_VONController));
		if (!vonContr)
			return;
		
		SCR_VoNComponent vonComp = SCR_VoNComponent.Cast(pc.FindComponent(PS_LobbyVoNComponent));
		
		vonContr.SetVONComponent(vonComp);
	}
	
	
	float GetPlayerSpeechTime(int playerId)
	{
		if (!m_fPlayerSpeachReciveTime.Contains(playerId)) return 0.0;
		return m_fPlayerSpeachReciveTime[playerId];
	}
	
	float IsPlayerSpeechInChanel(int playerId)
	{		
		PlayerController playerController = GetGame().GetPlayerController();
		if (playerController.GetPlayerId() == playerId) {
			return GetCommMethod() == ECommMethod.SQUAD_RADIO;
		}
		
		if (!m_fPlayerSpeachReciveIsChannel.Contains(playerId)) return false;
		return m_fPlayerSpeachReciveIsChannel[playerId];
	}
	
	bool IsPlayerSpeech(int playerId)
	{
		float worldTime = GetGame().GetWorld().GetWorldTime();
		return GetPlayerSpeechTime(playerId) > worldTime;
	}
	
	override bool IsTransmiting()
	{
		float worldTime = GetGame().GetWorld().GetWorldTime();
		return PS_m_fTransmitingTimeout >= worldTime;
	}
	
	override protected event void OnCapture(BaseTransceiver transmitter)
	{
		float worldTime = GetGame().GetWorld().GetWorldTime();
		if (m_fPSCaptureDiagnosticTimeout < worldTime)
			PS_LogCaptureDiagnostic(transmitter);
		m_fPSCaptureDiagnosticTimeout = worldTime + PS_TRANSMISSION_TIMEOUT_MS;

		int playerId = GetGame().GetPlayerController().GetPlayerId();
		OnReceiveHandle(playerId, transmitter, 0, 0);
	}
	
	override protected event void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)
	{		
		OnReceiveHandle(playerId, receiver, frequency, quality);
	}
	
	void OnReceiveHandle(int playerId, BaseTransceiver receiver, int frequency, float quality)
	{
		bool shouldLogReceive = !IsPlayerSpeech(playerId);
		if (!IsPlayerSpeech(playerId))
		{
			GetGame().GetCallqueue().Call(AwaitReceiveEnd, playerId);
		}
		bool alreadyReceive = IsPlayerSpeech(playerId);
		m_fPlayerSpeachReciveTime[playerId] = GetGame().GetWorld().GetWorldTime() + 100;
		if (frequency == 32000) {
			bool isChannel = m_fPlayerSpeachReciveIsChannel[playerId];
			m_fPlayerSpeachReciveIsChannel[playerId] = true;
			if (!isChannel)
				shouldLogReceive = true;
			if (!alreadyReceive || !isChannel)
				m_ScriptInvokerOnReceiveStart.Invoke(playerId, receiver, frequency, quality);
		}
		else {
			bool isChannel = m_fPlayerSpeachReciveIsChannel[playerId];
			m_fPlayerSpeachReciveIsChannel[playerId] = false;
			if (isChannel)
				shouldLogReceive = true;
			if (!alreadyReceive || isChannel)
				m_ScriptInvokerOnReceiveStart.Invoke(playerId, receiver, frequency, quality);
		}

		if (shouldLogReceive)
			PS_LogReceiveDiagnostic(playerId, receiver, frequency, quality);
	}

	//------------------------------------------------------------------------------------------------
	protected void PS_LogCaptureDiagnostic(BaseTransceiver transmitter)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
		{
			Print("[PS_VON_DIAG] event=NATIVE_CAPTURE playerController=NULL");
			return;
		}

		int playerId = playerController.GetPlayerId();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		string playerName = "<unknown>";
		if (playerManager)
			playerName = playerManager.GetPlayerName(playerId);

		IEntity entity = playerController.GetControlledEntity();
		vector position = vector.Zero;
		if (entity)
			position = entity.GetOrigin();

		BaseRadioComponent radio;
		if (transmitter)
			radio = transmitter.GetRadio();

		PrintFormat("[PS_VON_DIAG] event=NATIVE_CAPTURE player=%1 id=%2 position=%3 commMethod=%4", playerName, playerId, position, GetCommMethod());
		if (transmitter)
			PrintFormat("[PS_VON_DIAG] event=NATIVE_CAPTURE transmitter=%1 frequency=%2 radio=%3", transmitter, transmitter.GetFrequency(), radio);
		else
			Print("[PS_VON_DIAG] event=NATIVE_CAPTURE transmitter=NULL frequency=UNAVAILABLE radio=NULL");
		if (radio)
			PrintFormat("[PS_VON_DIAG] event=NATIVE_CAPTURE radioPowered=%1 editorRadio=%2 encryption=%3", radio.IsPowered(), radio.IsEditorRadio(), radio.GetEncryptionKey());
	}

	//------------------------------------------------------------------------------------------------
	protected void PS_LogReceiveDiagnostic(int senderId, BaseTransceiver receiver, int callbackFrequency, float quality)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		string senderName = "<unknown>";
		string listenerName = "<unknown>";
		IEntity senderEntity;
		if (playerManager)
		{
			senderName = playerManager.GetPlayerName(senderId);
			senderEntity = playerManager.GetPlayerControlledEntity(senderId);
		}

		vector senderPosition = vector.Zero;
		if (senderEntity)
			senderPosition = senderEntity.GetOrigin();

		PlayerController listenerController = GetGame().GetPlayerController();
		int listenerId = -1;
		IEntity listenerEntity;
		if (listenerController)
		{
			listenerId = listenerController.GetPlayerId();
			listenerEntity = listenerController.GetControlledEntity();
			if (playerManager)
				listenerName = playerManager.GetPlayerName(listenerId);
		}

		vector listenerPosition = vector.Zero;
		if (listenerEntity)
			listenerPosition = listenerEntity.GetOrigin();

		BaseRadioComponent radio;
		if (receiver)
			radio = receiver.GetRadio();

		FactionKey senderFaction = "<unavailable>";
		FactionKey listenerFaction = "<unavailable>";
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager && playableManager.IsReplicated())
		{
			senderFaction = playableManager.GetPlayerFactionKey(senderId);
			listenerFaction = playableManager.GetPlayerFactionKey(listenerId);
			if (senderFaction == "")
				senderFaction = "<unassigned>";
			if (listenerFaction == "")
				listenerFaction = "<unassigned>";
		}

		PrintFormat("[PS_VON_DIAG] event=RECEIVE_START sender=%1 senderId=%2 senderFaction=%3 senderPos=%4 listener=%5 listenerId=%6 listenerFaction=%7 listenerPos=%8 callbackFrequency=%9", senderName, senderId, senderFaction, senderPosition, listenerName, listenerId, listenerFaction, listenerPosition, callbackFrequency);
		if (receiver)
			PrintFormat("[PS_VON_DIAG] event=RECEIVE_START receiver=%1 receiverFrequency=%2", receiver, receiver.GetFrequency());
		else
			Print("[PS_VON_DIAG] event=RECEIVE_START receiver=NULL receiverFrequency=UNAVAILABLE");
		PrintFormat("[PS_VON_DIAG] event=RECEIVE_START senderId=%1 listenerId=%2 quality=%3", senderId, listenerId, quality);
		if (radio)
			PrintFormat("[PS_VON_DIAG] event=RECEIVE_START radio=%1 powered=%2 editorRadio=%3 encryption=%4", radio, radio.IsPowered(), radio.IsEditorRadio(), radio.GetEncryptionKey());
		else
			Print("[PS_VON_DIAG] event=RECEIVE_START radio=NULL (direct/non-radio VON or missing receiver)");
	}
	
	void AwaitReceiveEnd(int playerId)
	{
		if (IsPlayerSpeech(playerId))
		{
			GetGame().GetCallqueue().Call(AwaitReceiveEnd, playerId);
			return;
		}
		
		m_ScriptInvokerOnReceiveEnd.Invoke(playerId);
	}
}

//------------------------------------------------------------------------------------------------
// Arma Reforger 1.8 can leave an already-powered local radio unable to receive
// VON after its radio entry is registered. Re-register the native receiver by
// performing a verified delayed power cycle after each VON entry registration.
// This recovery is restricted to the always-on lobby radio and never changes
// its encryption. The primary channel is reconciled with the replicated room.
modded class SCR_VONController
{
	protected static const string PS_LOBBY_RADIO_ENCRYPTION_KEY = "lobby";
	protected static const int PS_LOBBY_PRIMARY_TRANSCEIVER_NUMBER = 2;
	protected static const int PS_RADIO_RECEIVER_STABILIZATION_DELAY_MS = 3000;
	protected static const int PS_RADIO_RECEIVER_POWER_OFF_MS = 150;
	protected static const int PS_RADIO_RECEIVER_VERIFY_DELAY_MS = 250;
	protected static const int PS_RADIO_RECEIVER_RETRY_DELAY_MS = 500;
	protected static const int PS_RADIO_RECEIVER_MONITOR_DELAY_MS = 2000;
	protected static const int PS_RADIO_RECEIVER_MAX_RESTORE_ATTEMPTS = 6;

	protected ref array<BaseRadioComponent> m_aPSRecoveryRadios = {};
	protected ref array<int> m_aPSRecoveryGenerations = {};

	//------------------------------------------------------------------------------------------------
	override void AddEntry(SCR_VONEntry entry)
	{
		super.AddEntry(entry);

		SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(entry);
		if (!radioEntry)
			return;

		BaseTransceiver transceiver = radioEntry.GetTransceiver();
		if (!transceiver)
			return;

		BaseRadioComponent radio = transceiver.GetRadio();
		if (radio)
			PrintFormat("[PS_VON_DIAG] event=RADIO_ENTRY_ADDED entry=%1 usable=%2 transceiver=%3 frequency=%4 radio=%5 powered=%6 editorRadio=%7 encryption=%8", entry, entry.IsUsable(), transceiver, transceiver.GetFrequency(), radio, radio.IsPowered(), radio.IsEditorRadio(), radio.GetEncryptionKey());
		else
			PrintFormat("[PS_VON_DIAG] event=RADIO_ENTRY_ADDED entry=%1 usable=%2 transceiver=%3 frequency=%4 radio=NULL", entry, entry.IsUsable(), transceiver, transceiver.GetFrequency());

		if (!radio || radio.IsEditorRadio())
			return;

		// Do not alter ordinary gameplay radios which players may deliberately
		// power off. The lobby radio is authored as always-on with this key.
		if (radio.GetEncryptionKey() != PS_LOBBY_RADIO_ENCRYPTION_KEY)
			return;

		// RegisterVONEntries adds channels in order. Schedule once for channel 1,
		// while every later re-registration receives a fresh generation.
		if (radioEntry.GetTransceiverNumber() != 1)
			return;

		int generation = PS_AdvanceRadioRecoveryGeneration(radio);
		PrintFormat("[PS_VON_DIAG] event=RADIO_RECOVERY_SCHEDULED radio=%1 generation=%2 powered=%3 delayMs=%4", radio, generation, radio.IsPowered(), PS_RADIO_RECEIVER_STABILIZATION_DELAY_MS);
		GetGame().GetCallqueue().CallLater(
			PS_BeginRadioReceiverRecovery,
			PS_RADIO_RECEIVER_STABILIZATION_DELAY_MS,
			false,
			radio,
			generation);
	}

	//------------------------------------------------------------------------------------------------
	override void RemoveEntry(SCR_VONEntry entry)
	{
		SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(entry);
		BaseRadioComponent radio;
		int transceiverNumber;
		if (radioEntry)
		{
			BaseTransceiver transceiver = radioEntry.GetTransceiver();
			if (transceiver)
				radio = transceiver.GetRadio();
			transceiverNumber = radioEntry.GetTransceiverNumber();
		}

		super.RemoveEntry(entry);

		if (!radio || transceiverNumber != 1 || radio.GetEncryptionKey() != PS_LOBBY_RADIO_ENCRYPTION_KEY)
			return;

		int generation = PS_AdvanceRadioRecoveryGeneration(radio);
		PrintFormat("[PS_VON_DIAG] event=RADIO_RECOVERY_INVALIDATED radio=%1 generation=%2 reason=entry_removed powered=%3", radio, generation, radio.IsPowered());
	}

	//------------------------------------------------------------------------------------------------
	protected int PS_AdvanceRadioRecoveryGeneration(BaseRadioComponent radio)
	{
		int index = m_aPSRecoveryRadios.Find(radio);
		if (index == -1)
		{
			m_aPSRecoveryRadios.Insert(radio);
			m_aPSRecoveryGenerations.Insert(1);
			return 1;
		}

		int generation = m_aPSRecoveryGenerations[index] + 1;
		m_aPSRecoveryGenerations[index] = generation;
		return generation;
	}

	//------------------------------------------------------------------------------------------------
	protected bool PS_IsCurrentRadioRecovery(BaseRadioComponent radio, int generation)
	{
		int index = m_aPSRecoveryRadios.Find(radio);
		return index != -1 && m_aPSRecoveryGenerations[index] == generation;
	}

	//------------------------------------------------------------------------------------------------
	protected void PS_BeginRadioReceiverRecovery(BaseRadioComponent radio, int generation)
	{
		if (!radio)
			return;

		if (!PS_IsCurrentRadioRecovery(radio, generation))
		{
			PrintFormat("[PS_VON_DIAG] event=RADIO_RECOVERY_STALE radio=%1 generation=%2 phase=begin powered=%3", radio, generation, radio.IsPowered());
			return;
		}

		if (radio.GetEncryptionKey() != PS_LOBBY_RADIO_ENCRYPTION_KEY)
			return;

		bool poweredBefore = radio.IsPowered();
		PS_SetRadioEntriesUsable(radio, false);

		if (poweredBefore)
		{
			radio.SetPower(false);
			PrintFormat("[PS_VON_DIAG] event=RADIO_RECOVERY_POWER_OFF radio=%1 generation=%2 poweredBefore=%3 poweredAfter=%4", radio, generation, poweredBefore, radio.IsPowered());
		}
		else
		{
			PrintFormat("[PS_VON_DIAG] event=RADIO_RECOVERY_FOUND_OFF radio=%1 generation=%2 action=restore", radio, generation);
		}

		GetGame().GetCallqueue().CallLater(
			PS_RestoreRadioReceiver,
			PS_RADIO_RECEIVER_POWER_OFF_MS,
			false,
			radio,
			generation,
			1);
	}

	//------------------------------------------------------------------------------------------------
	protected void PS_RestoreRadioReceiver(BaseRadioComponent radio, int generation, int attempt)
	{
		if (!radio)
			return;

		if (!PS_IsCurrentRadioRecovery(radio, generation))
		{
			// A stale recovery may have powered the shared lobby radio off just
			// before entries were removed. Restore its authored always-on state,
			// then let the current generation perform its own verified cycle.
			if (!radio.IsPowered())
				radio.SetPower(true);
			PrintFormat("[PS_VON_DIAG] event=RADIO_RECOVERY_STALE radio=%1 generation=%2 phase=restore powered=%3", radio, generation, radio.IsPowered());
			return;
		}

		bool poweredBefore = radio.IsPowered();
		radio.SetPower(true);
		PrintFormat("[PS_VON_DIAG] event=RADIO_RECOVERY_POWER_ON radio=%1 generation=%2 attempt=%3 poweredBefore=%4 poweredImmediate=%5", radio, generation, attempt, poweredBefore, radio.IsPowered());

		GetGame().GetCallqueue().CallLater(
			PS_VerifyRadioReceiverRecovery,
			PS_RADIO_RECEIVER_VERIFY_DELAY_MS,
			false,
			radio,
			generation,
			attempt);
	}

	//------------------------------------------------------------------------------------------------
	protected void PS_VerifyRadioReceiverRecovery(BaseRadioComponent radio, int generation, int attempt)
	{
		if (!radio || !PS_IsCurrentRadioRecovery(radio, generation))
			return;

		if (radio.IsPowered())
		{
			PS_ApplyLobbyRoomFrequency(radio);
			PS_SetRadioEntriesUsable(radio, true);
			PrintFormat("[PS_VON_DIAG] event=RADIO_RECOVERY_VERIFIED radio=%1 generation=%2 attempt=%3 powered=%4 entriesUsable=1", radio, generation, attempt, radio.IsPowered());
			GetGame().GetCallqueue().CallLater(
				PS_MonitorLobbyRadioReceiver,
				PS_RADIO_RECEIVER_MONITOR_DELAY_MS,
				false,
				radio,
				generation);
			return;
		}

		PS_SetRadioEntriesUsable(radio, false);
		if (attempt >= PS_RADIO_RECEIVER_MAX_RESTORE_ATTEMPTS)
		{
			PrintFormat("[PS_VON_DIAG] event=RADIO_RECOVERY_FAILED radio=%1 generation=%2 attempts=%3 powered=0 entriesUsable=0", radio, generation, attempt);
			return;
		}

		PrintFormat("[PS_VON_DIAG] event=RADIO_RECOVERY_RETRY radio=%1 generation=%2 nextAttempt=%3 powered=0 delayMs=%4", radio, generation, attempt + 1, PS_RADIO_RECEIVER_RETRY_DELAY_MS);
		GetGame().GetCallqueue().CallLater(
			PS_RestoreRadioReceiver,
			PS_RADIO_RECEIVER_RETRY_DELAY_MS,
			false,
			radio,
			generation,
			attempt + 1);
	}

	//------------------------------------------------------------------------------------------------
	protected void PS_MonitorLobbyRadioReceiver(BaseRadioComponent radio, int generation)
	{
		if (!radio || !PS_IsCurrentRadioRecovery(radio, generation))
			return;

		if (!radio.IsPowered())
		{
			PS_SetRadioEntriesUsable(radio, false);
			PrintFormat("[PS_VON_DIAG] event=RADIO_RECOVERY_POWER_LOST radio=%1 generation=%2 action=restore entriesUsable=0", radio, generation);
			GetGame().GetCallqueue().CallLater(
				PS_RestoreRadioReceiver,
				PS_RADIO_RECEIVER_RETRY_DELAY_MS,
				false,
				radio,
				generation,
				1);
			return;
		}

		GetGame().GetCallqueue().CallLater(
			PS_MonitorLobbyRadioReceiver,
			PS_RADIO_RECEIVER_MONITOR_DELAY_MS,
			false,
			radio,
			generation);
	}

	//------------------------------------------------------------------------------------------------
	protected void PS_ApplyLobbyRoomFrequency(BaseRadioComponent radio)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return;

		PS_VoNRoomsManager roomsManager = PS_VoNRoomsManager.GetInstance();
		if (!roomsManager)
			return;

		int playerId = playerController.GetPlayerId();
		int roomId = roomsManager.GetPlayerRoom(playerId);
		if (roomId < 0)
			return;

		array<ref SCR_VONEntry> entries = {};
		GetVONEntries(entries);
		foreach (SCR_VONEntry candidate : entries)
		{
			SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(candidate);
			if (!radioEntry || radioEntry.GetTransceiverNumber() != PS_LOBBY_PRIMARY_TRANSCEIVER_NUMBER)
				continue;

			BaseTransceiver transceiver = radioEntry.GetTransceiver();
			if (!transceiver || transceiver.GetRadio() != radio)
				continue;

			int oldFrequency = transceiver.GetFrequency();
			transceiver.SetFrequency(roomId);
			PrintFormat("[PS_VON_DIAG] event=RADIO_RECOVERY_ROOM_FREQUENCY_APPLIED playerId=%1 roomId=%2 oldFrequency=%3 newFrequency=%4", playerId, roomId, oldFrequency, transceiver.GetFrequency());
			return;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void PS_SetRadioEntriesUsable(BaseRadioComponent radio, bool usable)
	{
		array<ref SCR_VONEntry> entries = {};
		GetVONEntries(entries);

		foreach (SCR_VONEntry candidate : entries)
		{
			SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(candidate);
			if (!radioEntry)
				continue;

			BaseTransceiver transceiver = radioEntry.GetTransceiver();
			if (transceiver && transceiver.GetRadio() == radio)
				candidate.SetUsable(usable);
		}
	}
}
