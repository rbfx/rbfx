Editor notes
============

This document maintains a list of details developer should be aware of when working on the editor.

### Tab content padding hack

In some cases it is desirable that tab content would cover entire tab content region. Basically tab with no window 
padding. This is achieved by doing following:

```cpp
void UITab::OnBeforeBegin()
{
    // Allow viewport texture to cover entire window
    ui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
}

void UITab::OnAfterBegin()
{
    ui::PopStyleVar();  // ImGuiStyleVar_WindowPadding
}

void UITab::OnBeforeEnd()
{
    ui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
    BaseClassName::OnBeforeEnd();   // Required only if parent overrides it.
}

void UITab::OnAfterEnd()
{
    ui::PopStyleVar();  // ImGuiStyleVar_WindowPadding
}
```

This code renders tab with zero window padding while tab content is rendered with proper window padding. It is important
to maintain default window padding when rendering tab content because tab may spawn it's own windows which would then 
look broken. In case tab needs a toolbar - this toolbar would be sticking to top-left corner of the window. It is 
responsibility of that tab to apply correct padding before button rendering is started:

```cpp

void UITab::RenderToolbarButtons()
{
    ui::SetCursorPos(ui::GetCursorPos() + ImVec2{4_dpx, 4_dpy});
    // <...>
}
``` 

### Serialization of editor objects

We have extra objects when editing scene in the editor. Such objects can be helper icons indicating type of 3d object 
(lights for example), developer camera and others. Such objects are tagged with `__EDITOR_OBJECT__` tag. This tag is 
used to save/restore state of editor objects and then restore them when scene simulation is stopped. This hack should be
reworked into a more proper solution eventually.
