// Enable markers only for briefing if gamemode coop and flag is set
modded class SCR_MapMarkersUI
{
	protected EditBoxWidget m_wMarkerEditBox;
	protected SCR_EventHandlerComponent m_EditBoxEventHandler;
	protected SCR_EventHandlerComponent m_SliderEventHandler;

	static bool PS_IsMarkerTextInputActive()
	{
		SCR_MapEntity mapEnt = SCR_MapEntity.GetMapInstance();
		if (!mapEnt)
			return false;

		SCR_MapMarkersUI mapMarkersUI = SCR_MapMarkersUI.Cast(mapEnt.GetMapUIComponent(SCR_MapMarkersUI));
		return mapMarkersUI && mapMarkersUI.m_MarkerEditRoot && mapMarkersUI.m_wMarkerEditBox;
	}

	override protected void OnInputQuickMarkerMenu(float value, EActionTrigger reason)
	{
		PS_GameModeCoop psGameMode = PS_GameModeCoop.GetInstance();
		if (psGameMode)
		{
			if (psGameMode.GetMarkersOnlyOnBriefing())
			{
				if (psGameMode.GetState() != SCR_EGameModeState.BRIEFING)
					return;
				PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
				PlayerController playerController = GetGame().GetPlayerController();
				if (!playableManager.IsPlayerGroupLeader(playerController.GetPlayerId()))
					return;

				SCR_MapEntity mapEnt = SCR_MapEntity.GetMapInstance();
				SCR_MapMarkersUI mapMarkersUI = SCR_MapMarkersUI.Cast(mapEnt.GetMapUIComponent(SCR_MapMarkersUI));

				SCR_MapMarkerMenuEntry markerEntry = new SCR_MapMarkerMenuEntry();
				markerEntry.SetMarkerType(SCR_EMapMarkerType.PLACED_CUSTOM);
				mapMarkersUI.PS_OnEntryPerformed(markerEntry);

				float wX, wY, sX, sY;
				mapEnt.GetMapCursorWorldPosition(wX, wY);
				mapEnt.WorldToScreen(wX, wY, sX, sY);
				mapEnt.PanSmooth(sX, sY, 0.05);

				return;
			}
		}

		SCR_MapRadialUI mapRadial = SCR_MapRadialUI.GetInstance();
		if (!mapRadial)
			return;

		mapRadial.GetRadialController().OnInputOpen();
		mapRadial.GetRadialController().GetRadialMenu().PerformEntry(m_RootCategoryEntry);
	}

	void PS_OnEntryPerformed(SCR_MapMarkerMenuEntry entry)
	{
		OnEntryPerformed(entry);
	}

	override void OnDragWidget(Widget widget)
	{
		PS_GameModeCoop psGameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (psGameMode)
		{
			if (psGameMode.GetMarkersOnlyOnBriefing())
			{
				if (psGameMode.GetState() != SCR_EGameModeState.GAME)
					super.OnDragWidget(widget);
				return;
			}
		}
		super.OnDragWidget(widget);
	}

	//------------------------------------------------------------------------------------------------
	override protected void CreateMarkerEditDialog(bool isEditing = false, int tabID = 0, int selectedIconEntry = -1, int selectedColorEntry = -1)
	{
		super.CreateMarkerEditDialog(isEditing, tabID, selectedIconEntry, selectedColorEntry);

		PS_HookMarkerEditBox(USERID_EDITBOX);
	}

	//------------------------------------------------------------------------------------------------
	override protected void CreateMilitaryMarkerEditDialog(bool isEditing = false, int selectedFactionEntry = -1, int selectedDimensionEntry = -1)
	{
		super.CreateMilitaryMarkerEditDialog(isEditing, selectedFactionEntry, selectedDimensionEntry);

		PS_HookMarkerEditBox(USERID_EDITBOX_MIL);
	}

	//! Wires the edit box (found via userID) for mouse-hover focus persistence on PC.
	protected void PS_HookMarkerEditBox(int userID)
	{
		Widget editBoxRoot = m_MarkerEditRoot.FindAnyWidget("EditBoxRoot");
		if (!editBoxRoot)
			return;

		m_wMarkerEditBox = EditBoxWidget.Cast(editBoxRoot.FindAnyWidget("EditBox"));
		if (!m_wMarkerEditBox)
			return;

		// super already set the userID; ensure it matches so PS_IsMarkerEditBox() fires
		m_wMarkerEditBox.SetUserID(userID);

		// Hook the actual EditBox widget
		SCR_EventHandlerComponent editBoxEVH = SCR_EventHandlerComponent.Cast(m_wMarkerEditBox.FindHandler(SCR_EventHandlerComponent));
		if (editBoxEVH)
		{
			editBoxEVH.GetOnMouseEnter().Insert(OnMarkerEditBoxActivate);
			editBoxEVH.GetOnMouseButtonDown().Insert(OnMarkerEditBoxActivate);
		}

		// Hook the EditBoxRoot container
		m_EditBoxEventHandler = SCR_EventHandlerComponent.Cast(editBoxRoot.FindHandler(SCR_EventHandlerComponent));
		if (m_EditBoxEventHandler)
		{
			m_EditBoxEventHandler.GetOnMouseEnter().Insert(OnMarkerEditBoxActivate);
			m_EditBoxEventHandler.GetOnMouseButtonDown().Insert(OnMarkerEditBoxActivate);
		}

		// Hook the SliderRoot (custom marker only; safe to skip if absent)
		Widget sliderRoot = m_MarkerEditRoot.FindAnyWidget("SliderRoot");
		if (sliderRoot)
		{
			m_SliderEventHandler = SCR_EventHandlerComponent.Cast(sliderRoot.FindHandler(SCR_EventHandlerComponent));
			if (m_SliderEventHandler)
				m_SliderEventHandler.GetOnMouseEnter().Insert(OnMarkerEditBoxActivate);
		}

		// Redirect focus to the edit box and arm write mode
		FocusWidget(m_wMarkerEditBox);
		m_wMarkerEditBox.ActivateWriteMode();
	}

	protected void OnMarkerEditBoxActivate(Widget w)
	{
		if (!m_MarkerEditRoot || !m_wMarkerEditBox)
			return;

		if (m_EditBoxComp)
			m_EditBoxComp.ActivateWriteMode(true);
		else
			m_wMarkerEditBox.ActivateWriteMode();
	}
}