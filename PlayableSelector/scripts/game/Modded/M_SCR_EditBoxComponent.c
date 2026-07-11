modded class SCR_EditBoxComponent
{
    protected bool PS_IsMarkerEditBox()
    {
        if (!m_wEditBox)
            return false;
        if (!GetGame().GetInputManager().IsUsingMouseAndKeyboard())
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
        // Re-arm native write mode before super polls it, so super sees no state change
        if (PS_IsMarkerEditBox() && !forceDisabled && m_wEditBoxWidget && !m_wEditBoxWidget.IsInWriteMode())
            ActivateWriteMode(true);
        super.UpdateInteractionState(forceDisabled);
    }
}