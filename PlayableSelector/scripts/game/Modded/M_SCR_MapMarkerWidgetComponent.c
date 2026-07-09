//! Map marker layout component
//! Attached to root of marker base layout
modded class SCR_MapMarkerWidgetComponent : SCR_ScriptedWidgetComponent
{

	//------------------------------------------------------------------------------------------------
	//! \param[in] state
	//! \param[in] isPublic
	override void SetModeIcon(bool state, bool isPublic)
	{
		m_bIsOwnerMode = true;

		// m_wMarkerModeIcon.SetVisible(m_MarkerObject.GetMarkerOwnerID() == SCR_PlayerController.GetLocalPlayerId()); // original line
		m_wMarkerModeIcon.SetVisible(false);

		if (isPublic)
			m_wMarkerModeIcon.LoadImageFromSet(0, UIConstants.ICONS_IMAGE_SET, PUBLIC_QUAD);
		else
			m_wMarkerModeIcon.LoadImageFromSet(0, UIConstants.ICONS_IMAGE_SET, PRIVATE_QUAD);
	}

	//------------------------------------------------------------------------------------------------
	//! \param[in] state
	override void SetTextVisible(bool state)
	{
		m_bShowText = state;

		m_wMarkerText.SetVisible(true);
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (!m_bIsEventListening || !m_MarkerObject)
			return false;

		m_MarkerObject.LayerChangeLogic(0);

		SetTypeIconsVisible(true);

		if (m_bShowAuthor)
			ShowAuthor(true);

		if (m_bShowText)
			m_wMarkerText.SetVisible(true);

		if (m_MarkerObject.GetMarkerOwnerID() == SCR_PlayerController.GetLocalPlayerId())
			m_wMarkerModeIcon.SetVisible(true);

		if (!SCR_MapMarkersUI.IsOwnedMarker(m_MarkerObject))
			return false;

		if (m_bIsSymbolMode)
			m_wSymbolOverlay.SetColor(GUIColors.ORANGE);
		else
			m_wMarkerGlowIcon.SetColor(m_GlowSelected);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (!m_bIsEventListening || !m_MarkerObject)
			return false;

		m_MarkerObject.LayerChangeLogic(m_iLayerID);

		SetTypeIconsVisible(false);

		if (m_bShowAuthor)
			ShowAuthor(false);

		if (m_bShowText)
			m_wMarkerText.SetVisible(true);

		if (m_MarkerObject.GetMarkerOwnerID() == SCR_PlayerController.GetLocalPlayerId())
			m_wMarkerModeIcon.SetVisible(false);

		if (!SCR_MapMarkersUI.IsOwnedMarker(m_MarkerObject))
			return false;

		if (m_bIsSymbolMode)
			m_wSymbolOverlay.SetColor(m_CurrentImageColor);
		else
			m_wMarkerGlowIcon.SetColor(m_GlowDefault);

		return true;
	}
}
