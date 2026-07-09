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
		m_bIsMilitaryMarker = false;

		if (m_MarkerEditRoot)
			CleanupMarkerEditWidget();

		m_iWantedIconEntry = selectedIconEntry;

		m_MarkerEditRoot = GetGame().GetWorkspace().CreateWidgets(m_sEditBoxLayout, m_RootWidget);

		float screenX, screenY;
		m_MapEntity.GetMapWidget().GetScreenSize(screenX, screenY);
		FrameSlot.SetPos(m_MarkerEditRoot, GetGame().GetWorkspace().DPIUnscale(screenX * 0.5), GetGame().GetWorkspace().DPIUnscale(screenY * 0.5));

		m_wMarkerPreview = ImageWidget.Cast(m_MarkerEditRoot.FindAnyWidget("MarkerIcon"));
		m_wMarkerPreviewGlow = ImageWidget.Cast(m_MarkerEditRoot.FindAnyWidget("MarkerIconGlow"));
		m_wMarkerPreviewText = TextWidget.Cast(m_MarkerEditRoot.FindAnyWidget("MarkerText"));

		InitColorIcons(selectedColorEntry);

		Widget sliderRotation = m_MarkerEditRoot.FindAnyWidget("SliderRoot");
		m_SliderComp = SCR_SliderComponent.Cast(sliderRotation.FindHandler(SCR_SliderComponent));
		m_SliderComp.m_OnChanged.Insert(OnSliderChanged);
		m_SliderEventHandler = SCR_EventHandlerComponent.Cast(sliderRotation.FindHandler(SCR_EventHandlerComponent));
		if (m_SliderEventHandler)
		{
			m_SliderEventHandler.GetOnMouseEnter().Insert(OnMarkerEditBoxMouseEnter);
			m_SliderEventHandler.GetOnMouseButtonDown().Insert(OnMarkerEditBoxMouseButtonDown);
		}

		Widget categoryTab = m_MarkerEditRoot.FindAnyWidget("MarkerEditTab");
		m_TabComponent = SCR_TabViewComponent.Cast(categoryTab.FindHandler(SCR_TabViewComponent));

		array<ref SCR_MarkerIconCategory> categoriesArr = m_PlacedMarkerConfig.GetIconCategories();
		foreach (SCR_MarkerIconCategory category : categoriesArr)
		{
			m_TabComponent.AddTab(string.Empty, category.m_sName, identifier : category.m_sIdentifier);
		}

		m_TabComponent.GetOnChanged().Insert(OnTabChanged);
		m_TabComponent.ShowTab(tabID, true, false);

		Widget editBoxRoot = m_MarkerEditRoot.FindAnyWidget("EditBoxRoot");
		m_wMarkerEditBox = EditBoxWidget.Cast(editBoxRoot.FindAnyWidget("EditBox"));
		m_wMarkerEditBox.SetUserID(USERID_EDITBOX);
		SCR_EventHandlerComponent editBoxWidgetEventHandler = SCR_EventHandlerComponent.Cast(m_wMarkerEditBox.FindHandler(SCR_EventHandlerComponent));
		if (editBoxWidgetEventHandler)
		{
			editBoxWidgetEventHandler.GetOnMouseEnter().Insert(OnMarkerEditBoxMouseEnter);
			editBoxWidgetEventHandler.GetOnMouseButtonDown().Insert(OnMarkerEditBoxMouseButtonDown);
		}
		m_EditBoxEventHandler = SCR_EventHandlerComponent.Cast(editBoxRoot.FindHandler(SCR_EventHandlerComponent));
		if (m_EditBoxEventHandler)
		{
			m_EditBoxEventHandler.GetOnMouseEnter().Insert(OnMarkerEditBoxMouseEnter);
			m_EditBoxEventHandler.GetOnMouseButtonDown().Insert(OnMarkerEditBoxMouseButtonDown);
		}
		m_EditBoxComp = SCR_EditBoxComponent.Cast(editBoxRoot.FindHandler(SCR_EditBoxComponent));
		if (m_EditBoxComp)
		{
			m_EditBoxComp.m_OnTextChange.Insert(OnEditBoxTextChanged);
			m_EditBoxComp.SetValue(string.Empty);
		}

		SCR_InputButtonComponent confirmComp = SCR_InputButtonComponent.Cast(m_MarkerEditRoot.FindAnyWidget("ButtonPublic").FindHandler(SCR_InputButtonComponent));

		if (isEditing)
		{
			if (m_EditedMarker.GetMarkerID() >= 0)
				confirmComp.m_OnClicked.Insert(OnPlaceMarkerConfirmed);
			else
				confirmComp.m_OnClicked.Insert(OnPlaceMarkerConfirmedPrivate);

			confirmComp.SetLabel("#AR-ServerHosting_Edit");

			m_MarkerEditRoot.FindAnyWidget("ButtonPrivate").SetVisible(false);
			m_MarkerEditRoot.FindAnyWidget("ButtonPrivate").SetEnabled(false);
		}
		else
		{
			confirmComp.m_OnClicked.Insert(OnPlaceMarkerConfirmed);
			confirmComp.SetLabel("#AR-MapMarker_PlacePublic");

			SocialComponent sc = GetSocialComponent();
			if (sc && !sc.IsPrivilegedTo(EUserInteraction.UserGeneratedContent))
				confirmComp.SetEnabled(false);
			else
				confirmComp.SetEnabled(true);

			confirmComp = SCR_InputButtonComponent.Cast(m_MarkerEditRoot.FindAnyWidget("ButtonPrivate").FindHandler(SCR_InputButtonComponent));
			confirmComp.m_OnClicked.Insert(OnPlaceMarkerConfirmedPrivate);
		}

		confirmComp = SCR_InputButtonComponent.Cast(m_MarkerEditRoot.FindAnyWidget("ButtonCancel").FindHandler(SCR_InputButtonComponent));
		confirmComp.m_OnClicked.Insert(OnEditCancelled);

		FocusWidget(m_wMarkerEditBox);
		m_wMarkerEditBox.ActivateWriteMode();

		m_CursorModule.HandleDialog(true);
	}

	//------------------------------------------------------------------------------------------------
	override protected void CreateMilitaryMarkerEditDialog(bool isEditing = false, int selectedFactionEntry = -1, int selectedDimensionEntry = -1)
	{
		m_bIsMilitaryMarker = true;
		m_iWantedDimensionEntry = selectedDimensionEntry;

		m_MarkerEditRoot = GetGame().GetWorkspace().CreateWidgets(m_sMilitaryEditBoxLayout, m_RootWidget);

		float screenX, screenY;
		m_MapEntity.GetMapWidget().GetScreenSize(screenX, screenY);
		FrameSlot.SetPos(m_MarkerEditRoot, GetGame().GetWorkspace().DPIUnscale(screenX * 0.5), GetGame().GetWorkspace().DPIUnscale(screenY * 0.5));

		m_wMarkerPreviewMilitary = OverlayWidget.Cast(m_MarkerEditRoot.FindAnyWidget("SymbolOverlay"));
		m_MarkerPreviewMilitaryComp = SCR_MilitarySymbolUIComponent.Cast(m_wMarkerPreviewMilitary.FindHandler(SCR_MilitarySymbolUIComponent));
		m_wMarkerPreviewText = TextWidget.Cast(m_MarkerEditRoot.FindAnyWidget("MarkerText"));

		m_MilSymbolPreview = new SCR_MilitarySymbol();
		m_MilSymbolPreview.SetIdentity(EMilitarySymbolIdentity.BLUFOR);
		m_MilSymbolPreview.SetDimension(EMilitarySymbolDimension.LAND);

		InitFactionIcons(selectedFactionEntry);

		m_ComboBoxComp1 = SCR_ComboBoxComponent.Cast(m_MarkerEditRoot.FindAnyWidget("ComboBox1").FindHandler(SCR_ComboBoxComponent));
		m_ComboBoxComp1.m_OnChanged.Insert(OnComboBoxChangedA);

		m_ComboBoxComp2 = SCR_ComboBoxComponent.Cast(m_MarkerEditRoot.FindAnyWidget("ComboBox2").FindHandler(SCR_ComboBoxComponent));
		m_ComboBoxComp2.m_OnChanged.Insert(OnComboBoxChangedB);

		m_ComboBoxComp1.AddItem("");
		m_ComboBoxComp2.AddItem("");

		array<ref SCR_MarkerMilitaryType> types = m_MilitaryMarkerConfig.GetMilitaryTypes();
		foreach (int i, SCR_MarkerMilitaryType markerType : types)
		{
			m_ComboBoxComp1.AddItem(markerType.GetTranslation(), false, markerType);
			m_ComboBoxComp2.AddItem(markerType.GetTranslation(), false, markerType);
		}

		m_ComboBoxComp1.SetCurrentItem(0);
		OnComboBoxChangedA(m_ComboBoxComp1, -1);

		m_ComboBoxComp2.SetCurrentItem(0);
		OnComboBoxChangedB(m_ComboBoxComp2, -1);

		Widget editBoxRoot = m_MarkerEditRoot.FindAnyWidget("EditBoxRoot");
		m_wMarkerEditBox = EditBoxWidget.Cast(editBoxRoot.FindAnyWidget("EditBox"));
		m_wMarkerEditBox.SetUserID(USERID_EDITBOX_MIL);
		SCR_EventHandlerComponent editBoxWidgetEventHandler = SCR_EventHandlerComponent.Cast(m_wMarkerEditBox.FindHandler(SCR_EventHandlerComponent));
		if (editBoxWidgetEventHandler)
		{
			editBoxWidgetEventHandler.GetOnMouseEnter().Insert(OnMarkerEditBoxMouseEnter);
			editBoxWidgetEventHandler.GetOnMouseButtonDown().Insert(OnMarkerEditBoxMouseButtonDown);
		}
		m_EditBoxEventHandler = SCR_EventHandlerComponent.Cast(editBoxRoot.FindHandler(SCR_EventHandlerComponent));
		if (m_EditBoxEventHandler)
		{
			m_EditBoxEventHandler.GetOnMouseEnter().Insert(OnMarkerEditBoxMouseEnter);
			m_EditBoxEventHandler.GetOnMouseButtonDown().Insert(OnMarkerEditBoxMouseButtonDown);
		}
		m_EditBoxComp = SCR_EditBoxComponent.Cast(editBoxRoot.FindHandler(SCR_EditBoxComponent));
		if (m_EditBoxComp)
		{
			m_EditBoxComp.m_OnTextChange.Insert(OnEditBoxTextChanged);
			m_EditBoxComp.SetValue(string.Empty);
		}

		SCR_InputButtonComponent confirmComp = SCR_InputButtonComponent.Cast(m_MarkerEditRoot.FindAnyWidget("ButtonPublic").FindHandler(SCR_InputButtonComponent));
		confirmComp.m_OnClicked.Insert(OnPlaceMarkerConfirmed);

		if (isEditing)
			confirmComp.SetLabel("#AR-ServerHosting_Edit");
		else
			confirmComp.SetLabel("#AR-MapMarker_PlacePublic");

		confirmComp = SCR_InputButtonComponent.Cast(m_MarkerEditRoot.FindAnyWidget("ButtonCancel").FindHandler(SCR_InputButtonComponent));
		confirmComp.m_OnClicked.Insert(OnEditCancelled);

		FocusWidget(m_wMarkerEditBox);
		m_wMarkerEditBox.ActivateWriteMode();

		m_CursorModule.HandleDialog(true);
		Widget timestampWidget = m_MarkerEditRoot.FindAnyWidget("TimestampSpinBox");
		if (timestampWidget)
		{
			m_TimestampSpinBox = SCR_SpinBoxComponent.Cast(timestampWidget.FindHandler(SCR_SpinBoxComponent));
			m_TimestampSpinBox.m_OnChanged.Insert(OnTimestampSpinBoxChanged);
			m_TimestampSpinBox.SetCurrentItem(SPIN_BOX_YES, false, false);
			OnTimestampSpinBoxChanged(m_TimestampSpinBox, SPIN_BOX_YES);
		}
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

	protected void OnMarkerEditBoxSliderMouseEnter(Widget w)
	{
		if (!m_MarkerEditRoot || !m_wMarkerEditBox)
			return;

		if (m_EditBoxComp)
			m_EditBoxComp.ActivateWriteMode(true);
		else
			m_wMarkerEditBox.ActivateWriteMode();
	}
}