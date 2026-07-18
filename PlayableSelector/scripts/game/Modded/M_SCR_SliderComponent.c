modded class SCR_SliderComponent
{
    override bool OnMouseEnter(Widget w, int x, int y)
    {
        if (SCR_MapMarkersUI.PS_IsMarkerTextInputActive())
            return false;

        return super.OnMouseEnter(w, x, y);
    }

    override bool OnFocus(Widget w, int x, int y)
    {
        if (SCR_MapMarkersUI.PS_IsMarkerTextInputActive())
            return false;

        return super.OnFocus(w, x, y);
    }
}
