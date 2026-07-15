// Strange default button
modded class SCR_ButtonComponent
{
	ref ScriptInvoker<Widget> m_OnHoverWithWidget = new ScriptInvoker();

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		Widget focusedWidget = GetGame().GetWorkspace().GetFocusedWidget();
		if (focusedWidget && (focusedWidget.GetUserID() == 1000 || focusedWidget.GetUserID() == 1001))
		{
			m_OnHover.Invoke();
			m_OnHoverWithWidget.Invoke(w);
			return false;
		}

		super.OnMouseEnter(w, x, y);
		m_OnHover.Invoke(); // Why not pass by default???
		m_OnHoverWithWidget.Invoke(w);

		return false;
	}
}