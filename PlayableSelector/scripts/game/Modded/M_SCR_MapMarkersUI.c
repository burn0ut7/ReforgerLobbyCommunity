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
	//! Create custom marker dialog
	//! \param tabID is ID of selected tabWidget tab, if not set first is default
	//! \param selectedIconEntry is ID of selected icon, if not set first is default
	override protected void CreateMarkerEditDialog(bool isEditing = false, int tabID = 0, int selectedIconEntry = -1, int selectedColorEntry = -1)
	{
		super.CreateMarkerEditDialog(isEditing, tabID, selectedIconEntry, selectedColorEntry);

		Widget editBoxRoot = m_MarkerEditRoot.FindAnyWidget("EditBoxRoot");
		m_wMarkerEditBox = EditBoxWidget.Cast(editBoxRoot.FindAnyWidget("EditBox"));

		SCR_EventHandlerComponent editBoxWidgetHandler = SCR_EventHandlerComponent.Cast(m_wMarkerEditBox.FindHandler(SCR_EventHandlerComponent));
		if (editBoxWidgetHandler)
		{
			editBoxWidgetHandler.GetOnMouseEnter().Insert(OnMarkerEditBoxMouseEnter);
			editBoxWidgetHandler.GetOnMouseButtonDown().Insert(OnMarkerEditBoxMouseButtonDown);
		}

		m_EditBoxEventHandler = SCR_EventHandlerComponent.Cast(editBoxRoot.FindHandler(SCR_EventHandlerComponent));
		if (m_EditBoxEventHandler)
		{
			m_EditBoxEventHandler.GetOnMouseEnter().Insert(OnMarkerEditBoxMouseEnter);
			m_EditBoxEventHandler.GetOnMouseButtonDown().Insert(OnMarkerEditBoxMouseButtonDown);
		}

		Widget sliderRotation = m_MarkerEditRoot.FindAnyWidget("SliderRoot");
		m_SliderEventHandler = SCR_EventHandlerComponent.Cast(sliderRotation.FindHandler(SCR_EventHandlerComponent));
		if (m_SliderEventHandler)
		{
			m_SliderEventHandler.GetOnMouseEnter().Insert(OnMarkerEditBoxMouseEnter);
			m_SliderEventHandler.GetOnMouseButtonDown().Insert(OnMarkerEditBoxMouseButtonDown);
		}

		FocusWidget(m_wMarkerEditBox);
		m_wMarkerEditBox.ActivateWriteMode();
	}

	//------------------------------------------------------------------------------------------------
	override protected void CreateMilitaryMarkerEditDialog(bool isEditing = false, int selectedFactionEntry = -1, int selectedDimensionEntry = -1)
	{
		super.CreateMilitaryMarkerEditDialog(isEditing, selectedFactionEntry, selectedDimensionEntry);

		Widget editBoxRoot = m_MarkerEditRoot.FindAnyWidget("EditBoxRoot");
		m_wMarkerEditBox = EditBoxWidget.Cast(editBoxRoot.FindAnyWidget("EditBox"));

		SCR_EventHandlerComponent editBoxWidgetHandler = SCR_EventHandlerComponent.Cast(m_wMarkerEditBox.FindHandler(SCR_EventHandlerComponent));
		if (editBoxWidgetHandler)
		{
			editBoxWidgetHandler.GetOnMouseEnter().Insert(OnMarkerEditBoxMouseEnter);
			editBoxWidgetHandler.GetOnMouseButtonDown().Insert(OnMarkerEditBoxMouseButtonDown);
		}

		m_EditBoxEventHandler = SCR_EventHandlerComponent.Cast(editBoxRoot.FindHandler(SCR_EventHandlerComponent));
		if (m_EditBoxEventHandler)
		{
			m_EditBoxEventHandler.GetOnMouseEnter().Insert(OnMarkerEditBoxMouseEnter);
			m_EditBoxEventHandler.GetOnMouseButtonDown().Insert(OnMarkerEditBoxMouseButtonDown);
		}

		FocusWidget(m_wMarkerEditBox);
		m_wMarkerEditBox.ActivateWriteMode();
	}

	protected void OnMarkerEditBoxMouseEnter(Widget w)
	{
		if (!m_MarkerEditRoot || !m_wMarkerEditBox)
			return;

		if (m_EditBoxComp)
			m_EditBoxComp.ActivateWriteMode(true);
		else
			m_wMarkerEditBox.ActivateWriteMode();
	}

	protected void OnMarkerEditBoxMouseButtonDown(Widget w)
	{
		if (!m_MarkerEditRoot || !m_wMarkerEditBox)
			return;

		if (m_EditBoxComp)
			m_EditBoxComp.ActivateWriteMode(true);
		else
			m_wMarkerEditBox.ActivateWriteMode();
	}
}