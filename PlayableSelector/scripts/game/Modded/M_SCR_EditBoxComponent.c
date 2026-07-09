modded class SCR_EditBoxComponent
{
    protected bool PS_IsMarkerEditBox()
    {
        if (!m_wEditBox)
            return false;
        int userID = m_wEditBox.GetUserID();
        return userID == 1000 || userID == 1001;
    }

    override bool OnFocusLost(Widget w, int x, int y)
    {
        if (PS_IsMarkerEditBox())
            return true;
        return super.OnFocusLost(w, x, y);
    }

    override void OnHandlerFocusLost()
    {
        if (PS_IsMarkerEditBox())
            return;
        super.OnHandlerFocusLost();
    }

    override void UpdateInteractionState(bool forceDisabled)
    {
        if (PS_IsMarkerEditBox() && !forceDisabled)
        {
            // Native widget may have exited write mode when focus shifted — re-arm it
            if (m_wEditBoxWidget && !m_wEditBoxWidget.IsInWriteMode())
                ActivateWriteMode(true);

            // Still track text changes
            string currentText = GetEditBoxText();
            if (currentText != m_sTextPrevious)
            {
                m_sTextPrevious = currentText;
                m_OnTextChange.Invoke(m_sTextPrevious);
            }

            return;
        }

        super.UpdateInteractionState(forceDisabled);
    }
}