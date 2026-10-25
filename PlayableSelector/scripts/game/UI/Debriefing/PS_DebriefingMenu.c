modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	DebriefingMenu
}

//------------------------------------------------------------------------------------------------
class PS_DebriefingMenu : ChimeraMenuBase
{
	static const ResourceName m_sFactionReportLayout = "{2987421913A58DF6}UI/Debriefing/FactionReportFrame.layout";
	static const ResourceName m_sVoiceChatLayout = "{35DB604900C55B98}UI/VoiceChat/VoiceChatFrame.layout";

	protected Widget m_wVoiceChatList;

	protected HorizontalLayoutWidget m_wBodyHorizontalLayout;
	
	protected Widget m_wGameModeHeader;
	protected PS_GameModeHeader m_hGameModeHeader;
		
	ref array<PS_DebriefingFactionReportWidgetComponent> m_aFactionReports = {};

	protected ImageWidget m_wFade;
	
	float m_fOpacity = 0;
	float m_fFadeOpacity = 1;
	
	// -------------------- Menu events --------------------
	override void OnMenuOpen()
	{
		m_wBodyHorizontalLayout = HorizontalLayoutWidget.Cast(GetRootWidget().FindAnyWidget("BodyHorizontalLayout"));
		
		m_wFade = ImageWidget.Cast(GetRootWidget().FindAnyWidget("Fade"));
		
		m_wGameModeHeader = GetRootWidget().FindAnyWidget("GameModeHeader");
		m_hGameModeHeader = PS_GameModeHeader.Cast(m_wGameModeHeader.FindHandler(PS_GameModeHeader));
		
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.DOWN, Action_LobbyVoNOn);
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.UP, Action_LobbyVoNOff);
		GetGame().GetInputManager().AddActionListener("VONChannel", EActionTrigger.DOWN, Action_LobbyVoNFactionOn);
		GetGame().GetInputManager().AddActionListener("VONChannel", EActionTrigger.UP, Action_LobbyVoNOff);
		GetGame().GetInputManager().AddActionListener("LobbyAdminVON", EActionTrigger.DOWN, Action_LobbyVoNAdminOn);
		GetGame().GetInputManager().AddActionListener("LobbyAdminVON", EActionTrigger.UP, Action_LobbyVoNOff);
		GetGame().GetInputManager().AddActionListener("SwitchVoiceChat", EActionTrigger.DOWN, Action_SwitchVoiceChat);

		GetRootWidget().SetOpacity(0);

		CreateVoiceChat();
		FillFactions();
		Update();
		GetGame().GetCallqueue().CallLater(UpdateCycle, 100);
		
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (playableController)
			playableController.LobbyVoNDisable();
	}
	
	void UpdateCycle() 
	{
		Update();
		GetGame().GetCallqueue().CallLater(UpdateCycle, 100);
	}
	
	void Update()
	{
		m_hGameModeHeader.Update();
	}

	override void OnMenuInit()
	{
		
	}

	override void OnMenuUpdate(float tDelta)
	{
		if (m_fOpacity < 1.0)
		{
			m_fOpacity += tDelta;
			if (m_fOpacity > 1.0) 
			{
				m_fOpacity = 1.0;
				m_fFadeOpacity -= tDelta;
			}
			GetRootWidget().SetOpacity(m_fOpacity);
		}
	}

	override void OnMenuClose()
	{
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_LobbyVoNOn);
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.UP, Action_LobbyVoNOff);
		GetGame().GetInputManager().RemoveActionListener("VONChannel", EActionTrigger.DOWN, Action_LobbyVoNFactionOn);
		GetGame().GetInputManager().RemoveActionListener("VONChannel", EActionTrigger.UP, Action_LobbyVoNOff);
		GetGame().GetInputManager().RemoveActionListener("LobbyAdminVON", EActionTrigger.DOWN, Action_LobbyVoNAdminOn);
		GetGame().GetInputManager().RemoveActionListener("LobbyAdminVON", EActionTrigger.UP, Action_LobbyVoNOff);
		GetGame().GetInputManager().RemoveActionListener("SwitchVoiceChat", EActionTrigger.DOWN, Action_SwitchVoiceChat);
	}

	// Same voice panel as in briefing, anchored to the middle of the right edge
	void CreateVoiceChat()
	{
		m_wVoiceChatList = GetGame().GetWorkspace().CreateWidgets(m_sVoiceChatLayout, GetRootWidget());
		if (!m_wVoiceChatList)
			return;

		FrameSlot.SetAnchorMin(m_wVoiceChatList, 1, 0.5);
		FrameSlot.SetAnchorMax(m_wVoiceChatList, 1, 0.5);
		FrameSlot.SetSize(m_wVoiceChatList, 480, 640);
		FrameSlot.SetPos(m_wVoiceChatList, -490, -320);
	}

	void FillFactions()
	{
		array<Faction> outFactions = {};
		GetGame().GetFactionManager().GetFactionsList(outFactions);
		foreach (Faction faction : outFactions)
		{
			Widget factionReportWidget = GetGame().GetWorkspace().CreateWidgets(m_sFactionReportLayout, m_wBodyHorizontalLayout);
			PS_DebriefingFactionReportWidgetComponent factionReport = PS_DebriefingFactionReportWidgetComponent.Cast(factionReportWidget.FindHandler(PS_DebriefingFactionReportWidgetComponent));
			m_aFactionReports.Insert(factionReport);
			factionReport.SetFaction(faction);
		}
	}
	
	void Action_Exit()
	{
		GetGame().GetCallqueue().CallLater(OpenPauseMenuWrap, 0); //  Else menu auto close itself
	}
	void OpenPauseMenuWrap()
	{
		ArmaReforgerScripted.OpenPauseMenu();
	}

	// -------------------------------------- VoN --------------------------------------
	// After the mission player may control nothing, so check everything before transmitting
	PS_PlayableControllerComponent GetVoNPlayableController()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return null;

		IEntity controlledEntity = playerController.GetControlledEntity();
		if (!controlledEntity || !controlledEntity.FindComponent(PS_LobbyVoNComponent))
			return null;

		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController || !playableController.GetTransceiver(EChannelType.PRIMARY))
			return null;

		return playableController;
	}
	void Action_LobbyVoNOn()
	{
		PS_PlayableControllerComponent playableController = GetVoNPlayableController();
		if (playableController)
			playableController.LobbyVoNEnable();
	}
	void Action_LobbyVoNOff()
	{
		PS_PlayableControllerComponent playableController = GetVoNPlayableController();
		if (playableController)
			playableController.LobbyVoNDisable();
	}
	// Channel
	void Action_LobbyVoNFactionOn()
	{
		PS_PlayableControllerComponent playableController = GetVoNPlayableController();
		if (playableController)
			playableController.LobbyVoNFactionEnable();
	}
	void Action_LobbyVoNAdminOn()
	{
		PS_PlayableControllerComponent playableController = GetVoNPlayableController();
		if (playableController)
			playableController.LobbyVoNAdminEnable();
	}

	void Action_SwitchVoiceChat()
	{
		if (m_wVoiceChatList)
			m_wVoiceChatList.SetVisible(!m_wVoiceChatList.IsVisible());
	}
}
