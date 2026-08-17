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
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		OnReceiveHandle(playerId, transmitter, 0, 0);
	}
	
	override protected event void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)
	{		
		OnReceiveHandle(playerId, receiver, frequency, quality);
	}
	
	void OnReceiveHandle(int playerId, BaseTransceiver receiver, int frequency, float quality)
	{
		if (!IsPlayerSpeech(playerId))
		{
			GetGame().GetCallqueue().Call(AwaitReceiveEnd, playerId);
		}
		bool alreadyReceive = IsPlayerSpeech(playerId);
		m_fPlayerSpeachReciveTime[playerId] = GetGame().GetWorld().GetWorldTime() + 100;
		if (frequency == 32000) {
			bool isChannel = m_fPlayerSpeachReciveIsChannel[playerId];
			m_fPlayerSpeachReciveIsChannel[playerId] = true;
			if (!alreadyReceive || !isChannel)
				m_ScriptInvokerOnReceiveStart.Invoke(playerId, receiver, frequency, quality);
		}
		else {
			bool isChannel = m_fPlayerSpeachReciveIsChannel[playerId];
			m_fPlayerSpeachReciveIsChannel[playerId] = false;
			if (!alreadyReceive || isChannel)
				m_ScriptInvokerOnReceiveStart.Invoke(playerId, receiver, frequency, quality);
		}
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
// performing one delayed power cycle after the VON entry has stabilized.
// Frequency and encryption are intentionally left unchanged.
modded class SCR_VONController
{
	protected static const int PS_RADIO_RECEIVER_STABILIZATION_DELAY_MS = 3000;
	protected static const int PS_RADIO_RECEIVER_POWER_OFF_MS = 150;

	protected ref array<BaseRadioComponent> m_aPSReinitializedRadios = {};

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
		if (!radio || radio.IsEditorRadio() || !radio.IsPowered())
			return;

		if (m_aPSReinitializedRadios.Contains(radio))
			return;

		m_aPSReinitializedRadios.Insert(radio);
		GetGame().GetCallqueue().CallLater(
			PS_ReinitializeRadioReceiver,
			PS_RADIO_RECEIVER_STABILIZATION_DELAY_MS,
			false,
			radio);
	}

	//------------------------------------------------------------------------------------------------
	protected void PS_ReinitializeRadioReceiver(BaseRadioComponent radio)
	{
		// Do not override a deliberate power-off made while waiting.
		if (!radio || !radio.IsPowered())
			return;

		radio.SetPower(false);
		PS_SetRadioEntriesUsable(radio, false);

		// Keep the off/on operations on separate call-queue turns so the native
		// radio system has time to unregister the receiver.
		GetGame().GetCallqueue().CallLater(
			PS_RestoreRadioReceiver,
			PS_RADIO_RECEIVER_POWER_OFF_MS,
			false,
			radio);
	}

	//------------------------------------------------------------------------------------------------
	protected void PS_RestoreRadioReceiver(BaseRadioComponent radio)
	{
		if (!radio)
			return;

		radio.SetPower(true);
		PS_SetRadioEntriesUsable(radio, true);
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
