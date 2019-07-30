%typemap(cscode) Urho3D::BorderImage %{
  public $typemap(cstype, Urho3D::Texture *) Texture {
    get { return GetTexture(); }
    set { SetTexture(value); }
  }
  public $typemap(cstype, const Urho3D::IntRect &) ImageRect {
    get { return GetImageRect(); }
    set { SetImageRect(value); }
  }
  public $typemap(cstype, const Urho3D::IntRect &) Border {
    get { return GetBorder(); }
    set { SetBorder(value); }
  }
  public $typemap(cstype, const Urho3D::IntRect &) ImageBorder {
    get { return GetImageBorder(); }
    set { SetImageBorder(value); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) HoverOffset {
    get { return GetHoverOffset(); }
    set { SetHoverOffset(value); }
  }
  public $typemap(cstype, Urho3D::BlendMode) BlendMode {
    get { return GetBlendMode(); }
    set { SetBlendMode(value); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) TextureAttr {
    get { return GetTextureAttr(); }
  }
%}
%csmethodmodifiers Urho3D::BorderImage::GetTexture "private";
%csmethodmodifiers Urho3D::BorderImage::SetTexture "private";
%csmethodmodifiers Urho3D::BorderImage::GetImageRect "private";
%csmethodmodifiers Urho3D::BorderImage::SetImageRect "private";
%csmethodmodifiers Urho3D::BorderImage::GetBorder "private";
%csmethodmodifiers Urho3D::BorderImage::SetBorder "private";
%csmethodmodifiers Urho3D::BorderImage::GetImageBorder "private";
%csmethodmodifiers Urho3D::BorderImage::SetImageBorder "private";
%csmethodmodifiers Urho3D::BorderImage::GetHoverOffset "private";
%csmethodmodifiers Urho3D::BorderImage::SetHoverOffset "private";
%csmethodmodifiers Urho3D::BorderImage::GetBlendMode "private";
%csmethodmodifiers Urho3D::BorderImage::SetBlendMode "private";
%csmethodmodifiers Urho3D::BorderImage::GetTextureAttr "private";
%typemap(cscode) Urho3D::Button %{
  public $typemap(cstype, const Urho3D::IntVector2 &) PressedOffset {
    get { return GetPressedOffset(); }
    set { SetPressedOffset(value); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) DisabledOffset {
    get { return GetDisabledOffset(); }
    set { SetDisabledOffset(value); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) PressedChildOffset {
    get { return GetPressedChildOffset(); }
    set { SetPressedChildOffset(value); }
  }
  public $typemap(cstype, float) RepeatDelay {
    get { return GetRepeatDelay(); }
    set { SetRepeatDelay(value); }
  }
  public $typemap(cstype, float) RepeatRate {
    get { return GetRepeatRate(); }
    set { SetRepeatRate(value); }
  }
%}
%csmethodmodifiers Urho3D::Button::GetPressedOffset "private";
%csmethodmodifiers Urho3D::Button::SetPressedOffset "private";
%csmethodmodifiers Urho3D::Button::GetDisabledOffset "private";
%csmethodmodifiers Urho3D::Button::SetDisabledOffset "private";
%csmethodmodifiers Urho3D::Button::GetPressedChildOffset "private";
%csmethodmodifiers Urho3D::Button::SetPressedChildOffset "private";
%csmethodmodifiers Urho3D::Button::GetRepeatDelay "private";
%csmethodmodifiers Urho3D::Button::SetRepeatDelay "private";
%csmethodmodifiers Urho3D::Button::GetRepeatRate "private";
%csmethodmodifiers Urho3D::Button::SetRepeatRate "private";
%typemap(cscode) Urho3D::CheckBox %{
  public $typemap(cstype, const Urho3D::IntVector2 &) CheckedOffset {
    get { return GetCheckedOffset(); }
    set { SetCheckedOffset(value); }
  }
%}
%csmethodmodifiers Urho3D::CheckBox::GetCheckedOffset "private";
%csmethodmodifiers Urho3D::CheckBox::SetCheckedOffset "private";
%typemap(cscode) Urho3D::Cursor %{
  public $typemap(cstype, const eastl::string &) Shape {
    get { return GetShape(); }
    set { SetShape(value); }
  }
  public $typemap(cstype, bool) UseSystemShapes {
    get { return GetUseSystemShapes(); }
    set { SetUseSystemShapes(value); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::Variant>) ShapesAttr {
    get { return GetShapesAttr(); }
  }
%}
%csmethodmodifiers Urho3D::Cursor::GetShape "private";
%csmethodmodifiers Urho3D::Cursor::SetShape "private";
%csmethodmodifiers Urho3D::Cursor::GetUseSystemShapes "private";
%csmethodmodifiers Urho3D::Cursor::SetUseSystemShapes "private";
%csmethodmodifiers Urho3D::Cursor::GetShapesAttr "private";
%typemap(cscode) Urho3D::DropDownList %{
  public $typemap(cstype, unsigned int) NumItems {
    get { return GetNumItems(); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::UIElement *>) Items {
    get { return GetItems(); }
  }
  public $typemap(cstype, unsigned int) Selection {
    get { return GetSelection(); }
    set { SetSelection(value); }
  }
  public $typemap(cstype, Urho3D::UIElement *) SelectedItem {
    get { return GetSelectedItem(); }
  }
  public $typemap(cstype, Urho3D::ListView *) ListView {
    get { return GetListView(); }
  }
  public $typemap(cstype, Urho3D::UIElement *) Placeholder {
    get { return GetPlaceholder(); }
  }
  public $typemap(cstype, const eastl::string &) PlaceholderText {
    get { return GetPlaceholderText(); }
    set { SetPlaceholderText(value); }
  }
  public $typemap(cstype, bool) ResizePopup {
    get { return GetResizePopup(); }
    set { SetResizePopup(value); }
  }
%}
%csmethodmodifiers Urho3D::DropDownList::GetNumItems "private";
%csmethodmodifiers Urho3D::DropDownList::GetItems "private";
%csmethodmodifiers Urho3D::DropDownList::GetSelection "private";
%csmethodmodifiers Urho3D::DropDownList::SetSelection "private";
%csmethodmodifiers Urho3D::DropDownList::GetSelectedItem "private";
%csmethodmodifiers Urho3D::DropDownList::GetListView "private";
%csmethodmodifiers Urho3D::DropDownList::GetPlaceholder "private";
%csmethodmodifiers Urho3D::DropDownList::GetPlaceholderText "private";
%csmethodmodifiers Urho3D::DropDownList::SetPlaceholderText "private";
%csmethodmodifiers Urho3D::DropDownList::GetResizePopup "private";
%csmethodmodifiers Urho3D::DropDownList::SetResizePopup "private";
%typemap(cscode) Urho3D::FileSelector %{
  public $typemap(cstype, Urho3D::XMLFile *) DefaultStyle {
    get { return GetDefaultStyle(); }
    set { SetDefaultStyle(value); }
  }
  public $typemap(cstype, Urho3D::Window *) Window {
    get { return GetWindow(); }
  }
  public $typemap(cstype, Urho3D::Text *) TitleText {
    get { return GetTitleText(); }
  }
  public $typemap(cstype, Urho3D::ListView *) FileList {
    get { return GetFileList(); }
  }
  public $typemap(cstype, Urho3D::LineEdit *) PathEdit {
    get { return GetPathEdit(); }
  }
  public $typemap(cstype, Urho3D::LineEdit *) FileNameEdit {
    get { return GetFileNameEdit(); }
  }
  public $typemap(cstype, Urho3D::DropDownList *) FilterList {
    get { return GetFilterList(); }
  }
  public $typemap(cstype, Urho3D::Button *) OKButton {
    get { return GetOKButton(); }
  }
  public $typemap(cstype, Urho3D::Button *) CancelButton {
    get { return GetCancelButton(); }
  }
  public $typemap(cstype, Urho3D::Button *) CloseButton {
    get { return GetCloseButton(); }
  }
  public $typemap(cstype, const eastl::string &) Title {
    get { return GetTitle(); }
    set { SetTitle(value); }
  }
  public $typemap(cstype, const eastl::string &) Path {
    get { return GetPath(); }
    set { SetPath(value); }
  }
  public $typemap(cstype, const eastl::string &) FileName {
    get { return GetFileName(); }
    set { SetFileName(value); }
  }
  public $typemap(cstype, const eastl::string &) Filter {
    get { return GetFilter(); }
  }
  public $typemap(cstype, unsigned int) FilterIndex {
    get { return GetFilterIndex(); }
  }
  public $typemap(cstype, bool) DirectoryMode {
    get { return GetDirectoryMode(); }
    set { SetDirectoryMode(value); }
  }
%}
%csmethodmodifiers Urho3D::FileSelector::GetDefaultStyle "private";
%csmethodmodifiers Urho3D::FileSelector::SetDefaultStyle "private";
%csmethodmodifiers Urho3D::FileSelector::GetWindow "private";
%csmethodmodifiers Urho3D::FileSelector::GetTitleText "private";
%csmethodmodifiers Urho3D::FileSelector::GetFileList "private";
%csmethodmodifiers Urho3D::FileSelector::GetPathEdit "private";
%csmethodmodifiers Urho3D::FileSelector::GetFileNameEdit "private";
%csmethodmodifiers Urho3D::FileSelector::GetFilterList "private";
%csmethodmodifiers Urho3D::FileSelector::GetOKButton "private";
%csmethodmodifiers Urho3D::FileSelector::GetCancelButton "private";
%csmethodmodifiers Urho3D::FileSelector::GetCloseButton "private";
%csmethodmodifiers Urho3D::FileSelector::GetTitle "private";
%csmethodmodifiers Urho3D::FileSelector::SetTitle "private";
%csmethodmodifiers Urho3D::FileSelector::GetPath "private";
%csmethodmodifiers Urho3D::FileSelector::SetPath "private";
%csmethodmodifiers Urho3D::FileSelector::GetFileName "private";
%csmethodmodifiers Urho3D::FileSelector::SetFileName "private";
%csmethodmodifiers Urho3D::FileSelector::GetFilter "private";
%csmethodmodifiers Urho3D::FileSelector::GetFilterIndex "private";
%csmethodmodifiers Urho3D::FileSelector::GetDirectoryMode "private";
%csmethodmodifiers Urho3D::FileSelector::SetDirectoryMode "private";
%typemap(cscode) Urho3D::Font %{
  public $typemap(cstype, Urho3D::FontType) FontType {
    get { return GetFontType(); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) AbsoluteGlyphOffset {
    get { return GetAbsoluteGlyphOffset(); }
    set { SetAbsoluteGlyphOffset(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) ScaledGlyphOffset {
    get { return GetScaledGlyphOffset(); }
    set { SetScaledGlyphOffset(value); }
  }
%}
%csmethodmodifiers Urho3D::Font::GetFontType "private";
%csmethodmodifiers Urho3D::Font::GetAbsoluteGlyphOffset "private";
%csmethodmodifiers Urho3D::Font::SetAbsoluteGlyphOffset "private";
%csmethodmodifiers Urho3D::Font::GetScaledGlyphOffset "private";
%csmethodmodifiers Urho3D::Font::SetScaledGlyphOffset "private";
%typemap(cscode) Urho3D::FontFace %{
  public $typemap(cstype, float) PointSize {
    get { return GetPointSize(); }
  }
  public $typemap(cstype, float) RowHeight {
    get { return GetRowHeight(); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::SharedPtr<Urho3D::Texture2D>> &) Textures {
    get { return GetTextures(); }
  }
%}
%csmethodmodifiers Urho3D::FontFace::GetPointSize "private";
%csmethodmodifiers Urho3D::FontFace::GetRowHeight "private";
%csmethodmodifiers Urho3D::FontFace::GetTextures "private";
%typemap(cscode) Urho3D::LineEdit %{
  public $typemap(cstype, const eastl::string &) Value {
    get { return GetText(); }
    set { SetText(value); }
  }
  public $typemap(cstype, unsigned int) CursorPosition {
    get { return GetCursorPosition(); }
    set { SetCursorPosition(value); }
  }
  public $typemap(cstype, float) CursorBlinkRate {
    get { return GetCursorBlinkRate(); }
    set { SetCursorBlinkRate(value); }
  }
  public $typemap(cstype, unsigned int) MaxLength {
    get { return GetMaxLength(); }
    set { SetMaxLength(value); }
  }
  public $typemap(cstype, unsigned int) EchoCharacter {
    get { return GetEchoCharacter(); }
    set { SetEchoCharacter(value); }
  }
  public $typemap(cstype, Urho3D::Text *) TextElement {
    get { return GetTextElement(); }
  }
  public $typemap(cstype, Urho3D::BorderImage *) CursorImage {
    get { return GetCursor(); }
  }
%}
%csmethodmodifiers Urho3D::LineEdit::GetText "private";
%csmethodmodifiers Urho3D::LineEdit::SetText "private";
%csmethodmodifiers Urho3D::LineEdit::GetCursorPosition "private";
%csmethodmodifiers Urho3D::LineEdit::SetCursorPosition "private";
%csmethodmodifiers Urho3D::LineEdit::GetCursorBlinkRate "private";
%csmethodmodifiers Urho3D::LineEdit::SetCursorBlinkRate "private";
%csmethodmodifiers Urho3D::LineEdit::GetMaxLength "private";
%csmethodmodifiers Urho3D::LineEdit::SetMaxLength "private";
%csmethodmodifiers Urho3D::LineEdit::GetEchoCharacter "private";
%csmethodmodifiers Urho3D::LineEdit::SetEchoCharacter "private";
%csmethodmodifiers Urho3D::LineEdit::GetTextElement "private";
%csmethodmodifiers Urho3D::LineEdit::GetCursor "private";
%typemap(cscode) Urho3D::ListView %{
  public $typemap(cstype, unsigned int) NumItems {
    get { return GetNumItems(); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::UIElement *>) Items {
    get { return GetItems(); }
  }
  public $typemap(cstype, unsigned int) Selection {
    get { return GetSelection(); }
    set { SetSelection(value); }
  }
  public $typemap(cstype, const eastl::vector<unsigned int> &) Selections {
    get { return GetSelections(); }
    set { SetSelections(value); }
  }
  public $typemap(cstype, Urho3D::UIElement *) SelectedItem {
    get { return GetSelectedItem(); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::UIElement *>) SelectedItems {
    get { return GetSelectedItems(); }
  }
  public $typemap(cstype, Urho3D::HighlightMode) HighlightMode {
    get { return GetHighlightMode(); }
    set { SetHighlightMode(value); }
  }
  public $typemap(cstype, bool) Multiselect {
    get { return GetMultiselect(); }
    set { SetMultiselect(value); }
  }
  public $typemap(cstype, bool) ClearSelectionOnDefocus {
    get { return GetClearSelectionOnDefocus(); }
    set { SetClearSelectionOnDefocus(value); }
  }
  public $typemap(cstype, bool) SelectOnClickEnd {
    get { return GetSelectOnClickEnd(); }
    set { SetSelectOnClickEnd(value); }
  }
  public $typemap(cstype, bool) HierarchyMode {
    get { return GetHierarchyMode(); }
    set { SetHierarchyMode(value); }
  }
  public $typemap(cstype, int) BaseIndent {
    get { return GetBaseIndent(); }
    set { SetBaseIndent(value); }
  }
%}
%csmethodmodifiers Urho3D::ListView::GetNumItems "private";
%csmethodmodifiers Urho3D::ListView::GetItems "private";
%csmethodmodifiers Urho3D::ListView::GetSelection "private";
%csmethodmodifiers Urho3D::ListView::SetSelection "private";
%csmethodmodifiers Urho3D::ListView::GetSelections "private";
%csmethodmodifiers Urho3D::ListView::SetSelections "private";
%csmethodmodifiers Urho3D::ListView::GetSelectedItem "private";
%csmethodmodifiers Urho3D::ListView::GetSelectedItems "private";
%csmethodmodifiers Urho3D::ListView::GetHighlightMode "private";
%csmethodmodifiers Urho3D::ListView::SetHighlightMode "private";
%csmethodmodifiers Urho3D::ListView::GetMultiselect "private";
%csmethodmodifiers Urho3D::ListView::SetMultiselect "private";
%csmethodmodifiers Urho3D::ListView::GetClearSelectionOnDefocus "private";
%csmethodmodifiers Urho3D::ListView::SetClearSelectionOnDefocus "private";
%csmethodmodifiers Urho3D::ListView::GetSelectOnClickEnd "private";
%csmethodmodifiers Urho3D::ListView::SetSelectOnClickEnd "private";
%csmethodmodifiers Urho3D::ListView::GetHierarchyMode "private";
%csmethodmodifiers Urho3D::ListView::SetHierarchyMode "private";
%csmethodmodifiers Urho3D::ListView::GetBaseIndent "private";
%csmethodmodifiers Urho3D::ListView::SetBaseIndent "private";
%typemap(cscode) Urho3D::Menu %{
  public $typemap(cstype, Urho3D::UIElement *) Popup {
    get { return GetPopup(); }
    set { SetPopup(value); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) PopupOffset {
    get { return GetPopupOffset(); }
    set { SetPopupOffset(value); }
  }
  public $typemap(cstype, int) AcceleratorKey {
    get { return GetAcceleratorKey(); }
  }
  public $typemap(cstype, int) AcceleratorQualifiers {
    get { return GetAcceleratorQualifiers(); }
  }
%}
%csmethodmodifiers Urho3D::Menu::GetPopup "private";
%csmethodmodifiers Urho3D::Menu::SetPopup "private";
%csmethodmodifiers Urho3D::Menu::GetPopupOffset "private";
%csmethodmodifiers Urho3D::Menu::SetPopupOffset "private";
%csmethodmodifiers Urho3D::Menu::GetAcceleratorKey "private";
%csmethodmodifiers Urho3D::Menu::GetAcceleratorQualifiers "private";
%typemap(cscode) Urho3D::MessageBox %{
  public $typemap(cstype, const eastl::string &) Title {
    get { return GetTitle(); }
    set { SetTitle(value); }
  }
  public $typemap(cstype, const eastl::string &) Message {
    get { return GetMessage(); }
    set { SetMessage(value); }
  }
  public $typemap(cstype, Urho3D::UIElement *) Window {
    get { return GetWindow(); }
  }
%}
%csmethodmodifiers Urho3D::MessageBox::GetTitle "private";
%csmethodmodifiers Urho3D::MessageBox::SetTitle "private";
%csmethodmodifiers Urho3D::MessageBox::GetMessage "private";
%csmethodmodifiers Urho3D::MessageBox::SetMessage "private";
%csmethodmodifiers Urho3D::MessageBox::GetWindow "private";
%typemap(cscode) Urho3D::ProgressBar %{
  public $typemap(cstype, Urho3D::Orientation) Orientation {
    get { return GetOrientation(); }
    set { SetOrientation(value); }
  }
  public $typemap(cstype, float) Range {
    get { return GetRange(); }
    set { SetRange(value); }
  }
  public $typemap(cstype, float) Value {
    get { return GetValue(); }
    set { SetValue(value); }
  }
  public $typemap(cstype, Urho3D::BorderImage *) Knob {
    get { return GetKnob(); }
  }
  public $typemap(cstype, const eastl::string &) LoadingPercentStyle {
    get { return GetLoadingPercentStyle(); }
    set { SetLoadingPercentStyle(value); }
  }
  public $typemap(cstype, bool) ShowPercentText {
    get { return GetShowPercentText(); }
    set { SetShowPercentText(value); }
  }
%}
%csmethodmodifiers Urho3D::ProgressBar::GetOrientation "private";
%csmethodmodifiers Urho3D::ProgressBar::SetOrientation "private";
%csmethodmodifiers Urho3D::ProgressBar::GetRange "private";
%csmethodmodifiers Urho3D::ProgressBar::SetRange "private";
%csmethodmodifiers Urho3D::ProgressBar::GetValue "private";
%csmethodmodifiers Urho3D::ProgressBar::SetValue "private";
%csmethodmodifiers Urho3D::ProgressBar::GetKnob "private";
%csmethodmodifiers Urho3D::ProgressBar::GetLoadingPercentStyle "private";
%csmethodmodifiers Urho3D::ProgressBar::SetLoadingPercentStyle "private";
%csmethodmodifiers Urho3D::ProgressBar::GetShowPercentText "private";
%csmethodmodifiers Urho3D::ProgressBar::SetShowPercentText "private";
%typemap(cscode) Urho3D::ScrollBar %{
  public $typemap(cstype, Urho3D::Orientation) Orientation {
    get { return GetOrientation(); }
    set { SetOrientation(value); }
  }
  public $typemap(cstype, float) Range {
    get { return GetRange(); }
    set { SetRange(value); }
  }
  public $typemap(cstype, float) Value {
    get { return GetValue(); }
    set { SetValue(value); }
  }
  public $typemap(cstype, float) ScrollStep {
    get { return GetScrollStep(); }
    set { SetScrollStep(value); }
  }
  public $typemap(cstype, float) StepFactor {
    get { return GetStepFactor(); }
    set { SetStepFactor(value); }
  }
  public $typemap(cstype, float) EffectiveScrollStep {
    get { return GetEffectiveScrollStep(); }
  }
  public $typemap(cstype, Urho3D::Button *) BackButton {
    get { return GetBackButton(); }
  }
  public $typemap(cstype, Urho3D::Button *) ForwardButton {
    get { return GetForwardButton(); }
  }
  public $typemap(cstype, Urho3D::Slider *) Slider {
    get { return GetSlider(); }
  }
%}
%csmethodmodifiers Urho3D::ScrollBar::GetOrientation "private";
%csmethodmodifiers Urho3D::ScrollBar::SetOrientation "private";
%csmethodmodifiers Urho3D::ScrollBar::GetRange "private";
%csmethodmodifiers Urho3D::ScrollBar::SetRange "private";
%csmethodmodifiers Urho3D::ScrollBar::GetValue "private";
%csmethodmodifiers Urho3D::ScrollBar::SetValue "private";
%csmethodmodifiers Urho3D::ScrollBar::GetScrollStep "private";
%csmethodmodifiers Urho3D::ScrollBar::SetScrollStep "private";
%csmethodmodifiers Urho3D::ScrollBar::GetStepFactor "private";
%csmethodmodifiers Urho3D::ScrollBar::SetStepFactor "private";
%csmethodmodifiers Urho3D::ScrollBar::GetEffectiveScrollStep "private";
%csmethodmodifiers Urho3D::ScrollBar::GetBackButton "private";
%csmethodmodifiers Urho3D::ScrollBar::GetForwardButton "private";
%csmethodmodifiers Urho3D::ScrollBar::GetSlider "private";
%typemap(cscode) Urho3D::ScrollView %{
  public $typemap(cstype, const Urho3D::IntVector2 &) ViewPosition {
    get { return GetViewPosition(); }
    set { SetViewPosition(value); }
  }
  public $typemap(cstype, Urho3D::UIElement *) ContentElement {
    get { return GetContentElement(); }
    set { SetContentElement(value); }
  }
  public $typemap(cstype, Urho3D::ScrollBar *) HorizontalScrollBar {
    get { return GetHorizontalScrollBar(); }
  }
  public $typemap(cstype, Urho3D::ScrollBar *) VerticalScrollBar {
    get { return GetVerticalScrollBar(); }
  }
  public $typemap(cstype, Urho3D::BorderImage *) ScrollPanel {
    get { return GetScrollPanel(); }
  }
  public $typemap(cstype, bool) ScrollBarsAutoVisible {
    get { return GetScrollBarsAutoVisible(); }
    set { SetScrollBarsAutoVisible(value); }
  }
  public $typemap(cstype, bool) HorizontalScrollBarVisible {
    get { return GetHorizontalScrollBarVisible(); }
    set { SetHorizontalScrollBarVisible(value); }
  }
  public $typemap(cstype, bool) VerticalScrollBarVisible {
    get { return GetVerticalScrollBarVisible(); }
    set { SetVerticalScrollBarVisible(value); }
  }
  public $typemap(cstype, float) ScrollStep {
    get { return GetScrollStep(); }
    set { SetScrollStep(value); }
  }
  public $typemap(cstype, float) PageStep {
    get { return GetPageStep(); }
    set { SetPageStep(value); }
  }
  public $typemap(cstype, float) ScrollDeceleration {
    get { return GetScrollDeceleration(); }
    set { SetScrollDeceleration(value); }
  }
  public $typemap(cstype, float) ScrollSnapEpsilon {
    get { return GetScrollSnapEpsilon(); }
    set { SetScrollSnapEpsilon(value); }
  }
  public $typemap(cstype, bool) AutoDisableChildren {
    get { return GetAutoDisableChildren(); }
    set { SetAutoDisableChildren(value); }
  }
  public $typemap(cstype, float) AutoDisableThreshold {
    get { return GetAutoDisableThreshold(); }
    set { SetAutoDisableThreshold(value); }
  }
%}
%csmethodmodifiers Urho3D::ScrollView::GetViewPosition "private";
%csmethodmodifiers Urho3D::ScrollView::SetViewPosition "private";
%csmethodmodifiers Urho3D::ScrollView::GetContentElement "private";
%csmethodmodifiers Urho3D::ScrollView::SetContentElement "private";
%csmethodmodifiers Urho3D::ScrollView::GetHorizontalScrollBar "private";
%csmethodmodifiers Urho3D::ScrollView::GetVerticalScrollBar "private";
%csmethodmodifiers Urho3D::ScrollView::GetScrollPanel "private";
%csmethodmodifiers Urho3D::ScrollView::GetScrollBarsAutoVisible "private";
%csmethodmodifiers Urho3D::ScrollView::SetScrollBarsAutoVisible "private";
%csmethodmodifiers Urho3D::ScrollView::GetHorizontalScrollBarVisible "private";
%csmethodmodifiers Urho3D::ScrollView::SetHorizontalScrollBarVisible "private";
%csmethodmodifiers Urho3D::ScrollView::GetVerticalScrollBarVisible "private";
%csmethodmodifiers Urho3D::ScrollView::SetVerticalScrollBarVisible "private";
%csmethodmodifiers Urho3D::ScrollView::GetScrollStep "private";
%csmethodmodifiers Urho3D::ScrollView::SetScrollStep "private";
%csmethodmodifiers Urho3D::ScrollView::GetPageStep "private";
%csmethodmodifiers Urho3D::ScrollView::SetPageStep "private";
%csmethodmodifiers Urho3D::ScrollView::GetScrollDeceleration "private";
%csmethodmodifiers Urho3D::ScrollView::SetScrollDeceleration "private";
%csmethodmodifiers Urho3D::ScrollView::GetScrollSnapEpsilon "private";
%csmethodmodifiers Urho3D::ScrollView::SetScrollSnapEpsilon "private";
%csmethodmodifiers Urho3D::ScrollView::GetAutoDisableChildren "private";
%csmethodmodifiers Urho3D::ScrollView::SetAutoDisableChildren "private";
%csmethodmodifiers Urho3D::ScrollView::GetAutoDisableThreshold "private";
%csmethodmodifiers Urho3D::ScrollView::SetAutoDisableThreshold "private";
%typemap(cscode) Urho3D::Slider %{
  public $typemap(cstype, Urho3D::Orientation) Orientation {
    get { return GetOrientation(); }
    set { SetOrientation(value); }
  }
  public $typemap(cstype, float) Range {
    get { return GetRange(); }
    set { SetRange(value); }
  }
  public $typemap(cstype, float) Value {
    get { return GetValue(); }
    set { SetValue(value); }
  }
  public $typemap(cstype, Urho3D::BorderImage *) Knob {
    get { return GetKnob(); }
  }
  public $typemap(cstype, float) RepeatRate {
    get { return GetRepeatRate(); }
    set { SetRepeatRate(value); }
  }
%}
%csmethodmodifiers Urho3D::Slider::GetOrientation "private";
%csmethodmodifiers Urho3D::Slider::SetOrientation "private";
%csmethodmodifiers Urho3D::Slider::GetRange "private";
%csmethodmodifiers Urho3D::Slider::SetRange "private";
%csmethodmodifiers Urho3D::Slider::GetValue "private";
%csmethodmodifiers Urho3D::Slider::SetValue "private";
%csmethodmodifiers Urho3D::Slider::GetKnob "private";
%csmethodmodifiers Urho3D::Slider::GetRepeatRate "private";
%csmethodmodifiers Urho3D::Slider::SetRepeatRate "private";
%typemap(cscode) Urho3D::Sprite %{
  public $typemap(cstype, const Urho3D::Vector2 &) Position {
    get { return GetPosition(); }
    set { SetPosition(value); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) HotSpot {
    get { return GetHotSpot(); }
    set { SetHotSpot(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) Scale {
    get { return GetScale(); }
    set { SetScale(value); }
  }
  public $typemap(cstype, float) Rotation {
    get { return GetRotation(); }
    set { SetRotation(value); }
  }
  public $typemap(cstype, Urho3D::Texture *) Texture {
    get { return GetTexture(); }
    set { SetTexture(value); }
  }
  public $typemap(cstype, const Urho3D::IntRect &) ImageRect {
    get { return GetImageRect(); }
    set { SetImageRect(value); }
  }
  public $typemap(cstype, Urho3D::BlendMode) BlendMode {
    get { return GetBlendMode(); }
    set { SetBlendMode(value); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) TextureAttr {
    get { return GetTextureAttr(); }
  }
  public $typemap(cstype, const Urho3D::Matrix3x4 &) Transform {
    get { return GetTransform(); }
  }
%}
%csmethodmodifiers Urho3D::Sprite::GetPosition "private";
%csmethodmodifiers Urho3D::Sprite::SetPosition "private";
%csmethodmodifiers Urho3D::Sprite::GetHotSpot "private";
%csmethodmodifiers Urho3D::Sprite::SetHotSpot "private";
%csmethodmodifiers Urho3D::Sprite::GetScale "private";
%csmethodmodifiers Urho3D::Sprite::SetScale "private";
%csmethodmodifiers Urho3D::Sprite::GetRotation "private";
%csmethodmodifiers Urho3D::Sprite::SetRotation "private";
%csmethodmodifiers Urho3D::Sprite::GetTexture "private";
%csmethodmodifiers Urho3D::Sprite::SetTexture "private";
%csmethodmodifiers Urho3D::Sprite::GetImageRect "private";
%csmethodmodifiers Urho3D::Sprite::SetImageRect "private";
%csmethodmodifiers Urho3D::Sprite::GetBlendMode "private";
%csmethodmodifiers Urho3D::Sprite::SetBlendMode "private";
%csmethodmodifiers Urho3D::Sprite::GetTextureAttr "private";
%csmethodmodifiers Urho3D::Sprite::GetTransform "private";
%typemap(cscode) Urho3D::Text %{
  public $typemap(cstype, Urho3D::Font *) Font {
    get { return GetFont(); }
  }
  public $typemap(cstype, float) FontSize {
    get { return GetFontSize(); }
    set { SetFontSize(value); }
  }
  public $typemap(cstype, Urho3D::HorizontalAlignment) TextAlignment {
    get { return GetTextAlignment(); }
    set { SetTextAlignment(value); }
  }
  public $typemap(cstype, float) RowSpacing {
    get { return GetRowSpacing(); }
    set { SetRowSpacing(value); }
  }
  public $typemap(cstype, bool) Wordwrap {
    get { return GetWordwrap(); }
    set { SetWordwrap(value); }
  }
  public $typemap(cstype, bool) AutoLocalizable {
    get { return GetAutoLocalizable(); }
    set { SetAutoLocalizable(value); }
  }
  public $typemap(cstype, unsigned int) SelectionStart {
    get { return GetSelectionStart(); }
  }
  public $typemap(cstype, unsigned int) SelectionLength {
    get { return GetSelectionLength(); }
  }
  public $typemap(cstype, Urho3D::TextEffect) TextEffect {
    get { return GetTextEffect(); }
    set { SetTextEffect(value); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) EffectShadowOffset {
    get { return GetEffectShadowOffset(); }
    set { SetEffectShadowOffset(value); }
  }
  public $typemap(cstype, int) EffectStrokeThickness {
    get { return GetEffectStrokeThickness(); }
    set { SetEffectStrokeThickness(value); }
  }
  public $typemap(cstype, bool) EffectRoundStroke {
    get { return GetEffectRoundStroke(); }
    set { SetEffectRoundStroke(value); }
  }
  public $typemap(cstype, const Urho3D::Color &) EffectColor {
    get { return GetEffectColor(); }
    set { SetEffectColor(value); }
  }
  public $typemap(cstype, float) RowHeight {
    get { return GetRowHeight(); }
  }
  public $typemap(cstype, unsigned int) NumRows {
    get { return GetNumRows(); }
  }
  public $typemap(cstype, unsigned int) NumChars {
    get { return GetNumChars(); }
  }
  public $typemap(cstype, float) EffectDepthBias {
    get { return GetEffectDepthBias(); }
    set { SetEffectDepthBias(value); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) FontAttr {
    get { return GetFontAttr(); }
  }
  public $typemap(cstype, eastl::string) TextAttr {
    get { return GetTextAttr(); }
  }
%}
%csmethodmodifiers Urho3D::Text::GetFont "private";
%csmethodmodifiers Urho3D::Text::GetFontSize "private";
%csmethodmodifiers Urho3D::Text::SetFontSize "private";
%csmethodmodifiers Urho3D::Text::GetTextAlignment "private";
%csmethodmodifiers Urho3D::Text::SetTextAlignment "private";
%csmethodmodifiers Urho3D::Text::GetRowSpacing "private";
%csmethodmodifiers Urho3D::Text::SetRowSpacing "private";
%csmethodmodifiers Urho3D::Text::GetWordwrap "private";
%csmethodmodifiers Urho3D::Text::SetWordwrap "private";
%csmethodmodifiers Urho3D::Text::GetAutoLocalizable "private";
%csmethodmodifiers Urho3D::Text::SetAutoLocalizable "private";
%csmethodmodifiers Urho3D::Text::GetSelectionStart "private";
%csmethodmodifiers Urho3D::Text::GetSelectionLength "private";
%csmethodmodifiers Urho3D::Text::GetTextEffect "private";
%csmethodmodifiers Urho3D::Text::SetTextEffect "private";
%csmethodmodifiers Urho3D::Text::GetEffectShadowOffset "private";
%csmethodmodifiers Urho3D::Text::SetEffectShadowOffset "private";
%csmethodmodifiers Urho3D::Text::GetEffectStrokeThickness "private";
%csmethodmodifiers Urho3D::Text::SetEffectStrokeThickness "private";
%csmethodmodifiers Urho3D::Text::GetEffectRoundStroke "private";
%csmethodmodifiers Urho3D::Text::SetEffectRoundStroke "private";
%csmethodmodifiers Urho3D::Text::GetEffectColor "private";
%csmethodmodifiers Urho3D::Text::SetEffectColor "private";
%csmethodmodifiers Urho3D::Text::GetRowHeight "private";
%csmethodmodifiers Urho3D::Text::GetNumRows "private";
%csmethodmodifiers Urho3D::Text::GetNumChars "private";
%csmethodmodifiers Urho3D::Text::GetEffectDepthBias "private";
%csmethodmodifiers Urho3D::Text::SetEffectDepthBias "private";
%csmethodmodifiers Urho3D::Text::GetFontAttr "private";
%csmethodmodifiers Urho3D::Text::GetTextAttr "private";
%typemap(cscode) Urho3D::Text3D %{
  public $typemap(cstype, Urho3D::Font *) Font {
    get { return GetFont(); }
  }
  public $typemap(cstype, float) FontSize {
    get { return GetFontSize(); }
    set { SetFontSize(value); }
  }
  public $typemap(cstype, Urho3D::Material *) Material {
    get { return GetMaterial(); }
    set { SetMaterial(value); }
  }
  public $typemap(cstype, const eastl::string &) Text {
    get { return GetText(); }
    set { SetText(value); }
  }
  public $typemap(cstype, Urho3D::HorizontalAlignment) TextAlignment {
    get { return GetTextAlignment(); }
    set { SetTextAlignment(value); }
  }
  public $typemap(cstype, Urho3D::HorizontalAlignment) HorizontalAlignment {
    get { return GetHorizontalAlignment(); }
    set { SetHorizontalAlignment(value); }
  }
  public $typemap(cstype, Urho3D::VerticalAlignment) VerticalAlignment {
    get { return GetVerticalAlignment(); }
    set { SetVerticalAlignment(value); }
  }
  public $typemap(cstype, float) RowSpacing {
    get { return GetRowSpacing(); }
    set { SetRowSpacing(value); }
  }
  public $typemap(cstype, bool) Wordwrap {
    get { return GetWordwrap(); }
    set { SetWordwrap(value); }
  }
  public $typemap(cstype, Urho3D::TextEffect) TextEffect {
    get { return GetTextEffect(); }
    set { SetTextEffect(value); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) EffectShadowOffset {
    get { return GetEffectShadowOffset(); }
    set { SetEffectShadowOffset(value); }
  }
  public $typemap(cstype, int) EffectStrokeThickness {
    get { return GetEffectStrokeThickness(); }
    set { SetEffectStrokeThickness(value); }
  }
  public $typemap(cstype, bool) EffectRoundStroke {
    get { return GetEffectRoundStroke(); }
    set { SetEffectRoundStroke(value); }
  }
  public $typemap(cstype, const Urho3D::Color &) EffectColor {
    get { return GetEffectColor(); }
    set { SetEffectColor(value); }
  }
  public $typemap(cstype, float) EffectDepthBias {
    get { return GetEffectDepthBias(); }
    set { SetEffectDepthBias(value); }
  }
  public $typemap(cstype, int) Width {
    get { return GetWidth(); }
    set { SetWidth(value); }
  }
  public $typemap(cstype, int) Height {
    get { return GetHeight(); }
  }
  public $typemap(cstype, int) RowHeight {
    get { return GetRowHeight(); }
  }
  public $typemap(cstype, unsigned int) NumRows {
    get { return GetNumRows(); }
  }
  public $typemap(cstype, unsigned int) NumChars {
    get { return GetNumChars(); }
  }
  public $typemap(cstype, float) Opacity {
    get { return GetOpacity(); }
    set { SetOpacity(value); }
  }
  public $typemap(cstype, Urho3D::FaceCameraMode) FaceCameraMode {
    get { return GetFaceCameraMode(); }
    set { SetFaceCameraMode(value); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) FontAttr {
    get { return GetFontAttr(); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) MaterialAttr {
    get { return GetMaterialAttr(); }
  }
  public $typemap(cstype, eastl::string) TextAttr {
    get { return GetTextAttr(); }
  }
  public $typemap(cstype, const Urho3D::Color &) ColorAttr {
    get { return GetColorAttr(); }
  }
%}
%csmethodmodifiers Urho3D::Text3D::GetFont "private";
%csmethodmodifiers Urho3D::Text3D::GetFontSize "private";
%csmethodmodifiers Urho3D::Text3D::SetFontSize "private";
%csmethodmodifiers Urho3D::Text3D::GetMaterial "private";
%csmethodmodifiers Urho3D::Text3D::SetMaterial "private";
%csmethodmodifiers Urho3D::Text3D::GetText "private";
%csmethodmodifiers Urho3D::Text3D::SetText "private";
%csmethodmodifiers Urho3D::Text3D::GetTextAlignment "private";
%csmethodmodifiers Urho3D::Text3D::SetTextAlignment "private";
%csmethodmodifiers Urho3D::Text3D::GetHorizontalAlignment "private";
%csmethodmodifiers Urho3D::Text3D::SetHorizontalAlignment "private";
%csmethodmodifiers Urho3D::Text3D::GetVerticalAlignment "private";
%csmethodmodifiers Urho3D::Text3D::SetVerticalAlignment "private";
%csmethodmodifiers Urho3D::Text3D::GetRowSpacing "private";
%csmethodmodifiers Urho3D::Text3D::SetRowSpacing "private";
%csmethodmodifiers Urho3D::Text3D::GetWordwrap "private";
%csmethodmodifiers Urho3D::Text3D::SetWordwrap "private";
%csmethodmodifiers Urho3D::Text3D::GetTextEffect "private";
%csmethodmodifiers Urho3D::Text3D::SetTextEffect "private";
%csmethodmodifiers Urho3D::Text3D::GetEffectShadowOffset "private";
%csmethodmodifiers Urho3D::Text3D::SetEffectShadowOffset "private";
%csmethodmodifiers Urho3D::Text3D::GetEffectStrokeThickness "private";
%csmethodmodifiers Urho3D::Text3D::SetEffectStrokeThickness "private";
%csmethodmodifiers Urho3D::Text3D::GetEffectRoundStroke "private";
%csmethodmodifiers Urho3D::Text3D::SetEffectRoundStroke "private";
%csmethodmodifiers Urho3D::Text3D::GetEffectColor "private";
%csmethodmodifiers Urho3D::Text3D::SetEffectColor "private";
%csmethodmodifiers Urho3D::Text3D::GetEffectDepthBias "private";
%csmethodmodifiers Urho3D::Text3D::SetEffectDepthBias "private";
%csmethodmodifiers Urho3D::Text3D::GetWidth "private";
%csmethodmodifiers Urho3D::Text3D::SetWidth "private";
%csmethodmodifiers Urho3D::Text3D::GetHeight "private";
%csmethodmodifiers Urho3D::Text3D::GetRowHeight "private";
%csmethodmodifiers Urho3D::Text3D::GetNumRows "private";
%csmethodmodifiers Urho3D::Text3D::GetNumChars "private";
%csmethodmodifiers Urho3D::Text3D::GetOpacity "private";
%csmethodmodifiers Urho3D::Text3D::SetOpacity "private";
%csmethodmodifiers Urho3D::Text3D::GetFaceCameraMode "private";
%csmethodmodifiers Urho3D::Text3D::SetFaceCameraMode "private";
%csmethodmodifiers Urho3D::Text3D::GetFontAttr "private";
%csmethodmodifiers Urho3D::Text3D::GetMaterialAttr "private";
%csmethodmodifiers Urho3D::Text3D::GetTextAttr "private";
%csmethodmodifiers Urho3D::Text3D::GetColorAttr "private";
%typemap(cscode) Urho3D::ToolTip %{
  public $typemap(cstype, float) Delay {
    get { return GetDelay(); }
    set { SetDelay(value); }
  }
%}
%csmethodmodifiers Urho3D::ToolTip::GetDelay "private";
%csmethodmodifiers Urho3D::ToolTip::SetDelay "private";
%typemap(cscode) Urho3D::UI %{
  public $typemap(cstype, Urho3D::UIElement *) Root {
    get { return GetRoot(); }
    set { SetRoot(value); }
  }
  public $typemap(cstype, Urho3D::UIElement *) RootModalElement {
    get { return GetRootModalElement(); }
    set { SetRootModalElement(value); }
  }
  public $typemap(cstype, Urho3D::Cursor *) Cursor {
    get { return GetCursor(); }
    set { SetCursor(value); }
  }
  public $typemap(cstype, Urho3D::IntVector2) CursorPosition {
    get { return GetCursorPosition(); }
  }
  public $typemap(cstype, Urho3D::UIElement *) FocusElement {
    get { return GetFocusElement(); }
  }
  public $typemap(cstype, Urho3D::UIElement *) FrontElement {
    get { return GetFrontElement(); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::UIElement *> &) DragElements {
    get { return GetDragElements(); }
  }
  public $typemap(cstype, unsigned int) NumDragElements {
    get { return GetNumDragElements(); }
  }
  public $typemap(cstype, const eastl::string &) ClipboardText {
    get { return GetClipboardText(); }
    set { SetClipboardText(value); }
  }
  public $typemap(cstype, float) DoubleClickInterval {
    get { return GetDoubleClickInterval(); }
    set { SetDoubleClickInterval(value); }
  }
  public $typemap(cstype, float) MaxDoubleClickDistance {
    get { return GetMaxDoubleClickDistance(); }
    set { SetMaxDoubleClickDistance(value); }
  }
  public $typemap(cstype, float) DragBeginInterval {
    get { return GetDragBeginInterval(); }
    set { SetDragBeginInterval(value); }
  }
  public $typemap(cstype, int) DragBeginDistance {
    get { return GetDragBeginDistance(); }
    set { SetDragBeginDistance(value); }
  }
  public $typemap(cstype, float) DefaultToolTipDelay {
    get { return GetDefaultToolTipDelay(); }
    set { SetDefaultToolTipDelay(value); }
  }
  public $typemap(cstype, int) MaxFontTextureSize {
    get { return GetMaxFontTextureSize(); }
    set { SetMaxFontTextureSize(value); }
  }
  public $typemap(cstype, bool) UseSystemClipboard {
    get { return GetUseSystemClipboard(); }
    set { SetUseSystemClipboard(value); }
  }
  public $typemap(cstype, bool) UseScreenKeyboard {
    get { return GetUseScreenKeyboard(); }
    set { SetUseScreenKeyboard(value); }
  }
  public $typemap(cstype, bool) UseMutableGlyphs {
    get { return GetUseMutableGlyphs(); }
    set { SetUseMutableGlyphs(value); }
  }
  public $typemap(cstype, bool) ForceAutoHint {
    get { return GetForceAutoHint(); }
    set { SetForceAutoHint(value); }
  }
  public $typemap(cstype, Urho3D::FontHintLevel) FontHintLevel {
    get { return GetFontHintLevel(); }
    set { SetFontHintLevel(value); }
  }
  public $typemap(cstype, float) FontSubpixelThreshold {
    get { return GetFontSubpixelThreshold(); }
    set { SetFontSubpixelThreshold(value); }
  }
  public $typemap(cstype, int) FontOversampling {
    get { return GetFontOversampling(); }
    set { SetFontOversampling(value); }
  }
  public $typemap(cstype, float) Scale {
    get { return GetScale(); }
    set { SetScale(value); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) CustomSize {
    get { return GetCustomSize(); }
    set { SetCustomSize(value); }
  }
  public $typemap(cstype, Urho3D::Texture2D *) RenderTarget {
    get { return GetRenderTarget(); }
  }
  public $typemap(cstype, bool) RenderInSystemUI {
    get { return GetRenderInSystemUI(); }
    set { SetRenderInSystemUI(value); }
  }
%}
%csmethodmodifiers Urho3D::UI::GetRoot "private";
%csmethodmodifiers Urho3D::UI::SetRoot "private";
%csmethodmodifiers Urho3D::UI::GetRootModalElement "private";
%csmethodmodifiers Urho3D::UI::SetRootModalElement "private";
%csmethodmodifiers Urho3D::UI::GetCursor "private";
%csmethodmodifiers Urho3D::UI::SetCursor "private";
%csmethodmodifiers Urho3D::UI::GetCursorPosition "private";
%csmethodmodifiers Urho3D::UI::GetFocusElement "private";
%csmethodmodifiers Urho3D::UI::GetFrontElement "private";
%csmethodmodifiers Urho3D::UI::GetDragElements "private";
%csmethodmodifiers Urho3D::UI::GetNumDragElements "private";
%csmethodmodifiers Urho3D::UI::GetClipboardText "private";
%csmethodmodifiers Urho3D::UI::SetClipboardText "private";
%csmethodmodifiers Urho3D::UI::GetDoubleClickInterval "private";
%csmethodmodifiers Urho3D::UI::SetDoubleClickInterval "private";
%csmethodmodifiers Urho3D::UI::GetMaxDoubleClickDistance "private";
%csmethodmodifiers Urho3D::UI::SetMaxDoubleClickDistance "private";
%csmethodmodifiers Urho3D::UI::GetDragBeginInterval "private";
%csmethodmodifiers Urho3D::UI::SetDragBeginInterval "private";
%csmethodmodifiers Urho3D::UI::GetDragBeginDistance "private";
%csmethodmodifiers Urho3D::UI::SetDragBeginDistance "private";
%csmethodmodifiers Urho3D::UI::GetDefaultToolTipDelay "private";
%csmethodmodifiers Urho3D::UI::SetDefaultToolTipDelay "private";
%csmethodmodifiers Urho3D::UI::GetMaxFontTextureSize "private";
%csmethodmodifiers Urho3D::UI::SetMaxFontTextureSize "private";
%csmethodmodifiers Urho3D::UI::GetUseSystemClipboard "private";
%csmethodmodifiers Urho3D::UI::SetUseSystemClipboard "private";
%csmethodmodifiers Urho3D::UI::GetUseScreenKeyboard "private";
%csmethodmodifiers Urho3D::UI::SetUseScreenKeyboard "private";
%csmethodmodifiers Urho3D::UI::GetUseMutableGlyphs "private";
%csmethodmodifiers Urho3D::UI::SetUseMutableGlyphs "private";
%csmethodmodifiers Urho3D::UI::GetForceAutoHint "private";
%csmethodmodifiers Urho3D::UI::SetForceAutoHint "private";
%csmethodmodifiers Urho3D::UI::GetFontHintLevel "private";
%csmethodmodifiers Urho3D::UI::SetFontHintLevel "private";
%csmethodmodifiers Urho3D::UI::GetFontSubpixelThreshold "private";
%csmethodmodifiers Urho3D::UI::SetFontSubpixelThreshold "private";
%csmethodmodifiers Urho3D::UI::GetFontOversampling "private";
%csmethodmodifiers Urho3D::UI::SetFontOversampling "private";
%csmethodmodifiers Urho3D::UI::GetScale "private";
%csmethodmodifiers Urho3D::UI::SetScale "private";
%csmethodmodifiers Urho3D::UI::GetCustomSize "private";
%csmethodmodifiers Urho3D::UI::SetCustomSize "private";
%csmethodmodifiers Urho3D::UI::GetRenderTarget "private";
%csmethodmodifiers Urho3D::UI::GetRenderInSystemUI "private";
%csmethodmodifiers Urho3D::UI::SetRenderInSystemUI "private";
%typemap(cscode) Urho3D::UIComponent %{
  public $typemap(cstype, Urho3D::UIElement *) Root {
    get { return GetRoot(); }
  }
  public $typemap(cstype, Urho3D::Material *) Material {
    get { return GetMaterial(); }
  }
  public $typemap(cstype, Urho3D::Texture2D *) Texture {
    get { return GetTexture(); }
  }
%}
%csmethodmodifiers Urho3D::UIComponent::GetRoot "private";
%csmethodmodifiers Urho3D::UIComponent::GetMaterial "private";
%csmethodmodifiers Urho3D::UIComponent::GetTexture "private";
%typemap(cscode) Urho3D::UIElement %{
  public $typemap(cstype, const eastl::string &) Name {
    get { return GetName(); }
    set { SetName(value); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) Position {
    get { return GetPosition(); }
    set { SetPosition(value); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) Size {
    get { return GetSize(); }
    set { SetSize(value); }
  }
  public $typemap(cstype, int) Width {
    get { return GetWidth(); }
    set { SetWidth(value); }
  }
  public $typemap(cstype, int) Height {
    get { return GetHeight(); }
    set { SetHeight(value); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) MinSize {
    get { return GetMinSize(); }
    set { SetMinSize(value); }
  }
  public $typemap(cstype, int) MinWidth {
    get { return GetMinWidth(); }
    set { SetMinWidth(value); }
  }
  public $typemap(cstype, int) MinHeight {
    get { return GetMinHeight(); }
    set { SetMinHeight(value); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) MaxSize {
    get { return GetMaxSize(); }
    set { SetMaxSize(value); }
  }
  public $typemap(cstype, int) MaxWidth {
    get { return GetMaxWidth(); }
    set { SetMaxWidth(value); }
  }
  public $typemap(cstype, int) MaxHeight {
    get { return GetMaxHeight(); }
    set { SetMaxHeight(value); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) ChildOffset {
    get { return GetChildOffset(); }
    set { SetChildOffset(value); }
  }
  public $typemap(cstype, Urho3D::HorizontalAlignment) HorizontalAlignment {
    get { return GetHorizontalAlignment(); }
    set { SetHorizontalAlignment(value); }
  }
  public $typemap(cstype, Urho3D::VerticalAlignment) VerticalAlignment {
    get { return GetVerticalAlignment(); }
    set { SetVerticalAlignment(value); }
  }
  public $typemap(cstype, bool) EnableAnchor {
    get { return GetEnableAnchor(); }
    set { SetEnableAnchor(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) MinAnchor {
    get { return GetMinAnchor(); }
    set { SetMinAnchor(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) MaxAnchor {
    get { return GetMaxAnchor(); }
    set { SetMaxAnchor(value); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) MinOffset {
    get { return GetMinOffset(); }
    set { SetMinOffset(value); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) MaxOffset {
    get { return GetMaxOffset(); }
    set { SetMaxOffset(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) Pivot {
    get { return GetPivot(); }
    set { SetPivot(value); }
  }
  public $typemap(cstype, const Urho3D::IntRect &) ClipBorder {
    get { return GetClipBorder(); }
    set { SetClipBorder(value); }
  }
  public $typemap(cstype, int) Priority {
    get { return GetPriority(); }
    set { SetPriority(value); }
  }
  public $typemap(cstype, float) Opacity {
    get { return GetOpacity(); }
    set { SetOpacity(value); }
  }
  public $typemap(cstype, float) DerivedOpacity {
    get { return GetDerivedOpacity(); }
  }
  public $typemap(cstype, bool) BringToBack {
    get { return GetBringToBack(); }
    set { SetBringToBack(value); }
  }
  public $typemap(cstype, bool) ClipChildren {
    get { return GetClipChildren(); }
    set { SetClipChildren(value); }
  }
  public $typemap(cstype, bool) UseDerivedOpacity {
    get { return GetUseDerivedOpacity(); }
    set { SetUseDerivedOpacity(value); }
  }
  public $typemap(cstype, Urho3D::FocusMode) FocusMode {
    get { return GetFocusMode(); }
    set { SetFocusMode(value); }
  }
  public $typemap(cstype, Urho3D::DragAndDropMode) DragDropMode {
    get { return GetDragDropMode(); }
    set { SetDragDropMode(value); }
  }
  public $typemap(cstype, const eastl::string &) AppliedStyle {
    get { return GetAppliedStyle(); }
  }
  public $typemap(cstype, Urho3D::LayoutMode) LayoutMode {
    get { return GetLayoutMode(); }
    set { SetLayoutMode(value); }
  }
  public $typemap(cstype, int) LayoutSpacing {
    get { return GetLayoutSpacing(); }
    set { SetLayoutSpacing(value); }
  }
  public $typemap(cstype, const Urho3D::IntRect &) LayoutBorder {
    get { return GetLayoutBorder(); }
    set { SetLayoutBorder(value); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) LayoutFlexScale {
    get { return GetLayoutFlexScale(); }
    set { SetLayoutFlexScale(value); }
  }
  public $typemap(cstype, Urho3D::UIElement *) Parent {
    get { return GetParent(); }
  }
  public $typemap(cstype, Urho3D::UIElement *) Root {
    get { return GetRoot(); }
  }
  public $typemap(cstype, const Urho3D::Color &) DerivedColor {
    get { return GetDerivedColor(); }
  }
  public $typemap(cstype, const eastl::unordered_map<Urho3D::StringHash, Urho3D::Variant> &) Vars {
    get { return GetVars(); }
  }
  public $typemap(cstype, const eastl::vector<eastl::string> &) Tags {
    get { return GetTags(); }
    set { SetTags(value); }
  }
  public $typemap(cstype, int) DragButtonCombo {
    get { return GetDragButtonCombo(); }
  }
  public $typemap(cstype, unsigned int) DragButtonCount {
    get { return GetDragButtonCount(); }
  }
  public $typemap(cstype, Urho3D::IntRect) CombinedScreenRect {
    get { return GetCombinedScreenRect(); }
  }
  public $typemap(cstype, int) LayoutElementMaxSize {
    get { return GetLayoutElementMaxSize(); }
  }
  public $typemap(cstype, int) Indent {
    get { return GetIndent(); }
    set { SetIndent(value); }
  }
  public $typemap(cstype, int) IndentSpacing {
    get { return GetIndentSpacing(); }
    set { SetIndentSpacing(value); }
  }
  public $typemap(cstype, int) IndentWidth {
    get { return GetIndentWidth(); }
  }
  public $typemap(cstype, const Urho3D::Color &) ColorAttr {
    get { return GetColorAttr(); }
  }
  public $typemap(cstype, Urho3D::TraversalMode) TraversalMode {
    get { return GetTraversalMode(); }
    set { SetTraversalMode(value); }
  }
  public $typemap(cstype, Urho3D::UIElement *) ElementEventSender {
    get { return GetElementEventSender(); }
  }
  public $typemap(cstype, Urho3D::IntVector2) EffectiveMinSize {
    get { return GetEffectiveMinSize(); }
  }
%}
%csmethodmodifiers Urho3D::UIElement::GetName "private";
%csmethodmodifiers Urho3D::UIElement::SetName "private";
%csmethodmodifiers Urho3D::UIElement::GetPosition "private";
%csmethodmodifiers Urho3D::UIElement::SetPosition "private";
%csmethodmodifiers Urho3D::UIElement::GetSize "private";
%csmethodmodifiers Urho3D::UIElement::SetSize "private";
%csmethodmodifiers Urho3D::UIElement::GetWidth "private";
%csmethodmodifiers Urho3D::UIElement::SetWidth "private";
%csmethodmodifiers Urho3D::UIElement::GetHeight "private";
%csmethodmodifiers Urho3D::UIElement::SetHeight "private";
%csmethodmodifiers Urho3D::UIElement::GetMinSize "private";
%csmethodmodifiers Urho3D::UIElement::SetMinSize "private";
%csmethodmodifiers Urho3D::UIElement::GetMinWidth "private";
%csmethodmodifiers Urho3D::UIElement::SetMinWidth "private";
%csmethodmodifiers Urho3D::UIElement::GetMinHeight "private";
%csmethodmodifiers Urho3D::UIElement::SetMinHeight "private";
%csmethodmodifiers Urho3D::UIElement::GetMaxSize "private";
%csmethodmodifiers Urho3D::UIElement::SetMaxSize "private";
%csmethodmodifiers Urho3D::UIElement::GetMaxWidth "private";
%csmethodmodifiers Urho3D::UIElement::SetMaxWidth "private";
%csmethodmodifiers Urho3D::UIElement::GetMaxHeight "private";
%csmethodmodifiers Urho3D::UIElement::SetMaxHeight "private";
%csmethodmodifiers Urho3D::UIElement::GetChildOffset "private";
%csmethodmodifiers Urho3D::UIElement::SetChildOffset "private";
%csmethodmodifiers Urho3D::UIElement::GetHorizontalAlignment "private";
%csmethodmodifiers Urho3D::UIElement::SetHorizontalAlignment "private";
%csmethodmodifiers Urho3D::UIElement::GetVerticalAlignment "private";
%csmethodmodifiers Urho3D::UIElement::SetVerticalAlignment "private";
%csmethodmodifiers Urho3D::UIElement::GetEnableAnchor "private";
%csmethodmodifiers Urho3D::UIElement::SetEnableAnchor "private";
%csmethodmodifiers Urho3D::UIElement::GetMinAnchor "private";
%csmethodmodifiers Urho3D::UIElement::SetMinAnchor "private";
%csmethodmodifiers Urho3D::UIElement::GetMaxAnchor "private";
%csmethodmodifiers Urho3D::UIElement::SetMaxAnchor "private";
%csmethodmodifiers Urho3D::UIElement::GetMinOffset "private";
%csmethodmodifiers Urho3D::UIElement::SetMinOffset "private";
%csmethodmodifiers Urho3D::UIElement::GetMaxOffset "private";
%csmethodmodifiers Urho3D::UIElement::SetMaxOffset "private";
%csmethodmodifiers Urho3D::UIElement::GetPivot "private";
%csmethodmodifiers Urho3D::UIElement::SetPivot "private";
%csmethodmodifiers Urho3D::UIElement::GetClipBorder "private";
%csmethodmodifiers Urho3D::UIElement::SetClipBorder "private";
%csmethodmodifiers Urho3D::UIElement::GetPriority "private";
%csmethodmodifiers Urho3D::UIElement::SetPriority "private";
%csmethodmodifiers Urho3D::UIElement::GetOpacity "private";
%csmethodmodifiers Urho3D::UIElement::SetOpacity "private";
%csmethodmodifiers Urho3D::UIElement::GetDerivedOpacity "private";
%csmethodmodifiers Urho3D::UIElement::GetBringToBack "private";
%csmethodmodifiers Urho3D::UIElement::SetBringToBack "private";
%csmethodmodifiers Urho3D::UIElement::GetClipChildren "private";
%csmethodmodifiers Urho3D::UIElement::SetClipChildren "private";
%csmethodmodifiers Urho3D::UIElement::GetUseDerivedOpacity "private";
%csmethodmodifiers Urho3D::UIElement::SetUseDerivedOpacity "private";
%csmethodmodifiers Urho3D::UIElement::GetFocusMode "private";
%csmethodmodifiers Urho3D::UIElement::SetFocusMode "private";
%csmethodmodifiers Urho3D::UIElement::GetDragDropMode "private";
%csmethodmodifiers Urho3D::UIElement::SetDragDropMode "private";
%csmethodmodifiers Urho3D::UIElement::GetAppliedStyle "private";
%csmethodmodifiers Urho3D::UIElement::GetLayoutMode "private";
%csmethodmodifiers Urho3D::UIElement::SetLayoutMode "private";
%csmethodmodifiers Urho3D::UIElement::GetLayoutSpacing "private";
%csmethodmodifiers Urho3D::UIElement::SetLayoutSpacing "private";
%csmethodmodifiers Urho3D::UIElement::GetLayoutBorder "private";
%csmethodmodifiers Urho3D::UIElement::SetLayoutBorder "private";
%csmethodmodifiers Urho3D::UIElement::GetLayoutFlexScale "private";
%csmethodmodifiers Urho3D::UIElement::SetLayoutFlexScale "private";
%csmethodmodifiers Urho3D::UIElement::GetParent "private";
%csmethodmodifiers Urho3D::UIElement::GetRoot "private";
%csmethodmodifiers Urho3D::UIElement::GetDerivedColor "private";
%csmethodmodifiers Urho3D::UIElement::GetVars "private";
%csmethodmodifiers Urho3D::UIElement::GetTags "private";
%csmethodmodifiers Urho3D::UIElement::SetTags "private";
%csmethodmodifiers Urho3D::UIElement::GetDragButtonCombo "private";
%csmethodmodifiers Urho3D::UIElement::GetDragButtonCount "private";
%csmethodmodifiers Urho3D::UIElement::GetCombinedScreenRect "private";
%csmethodmodifiers Urho3D::UIElement::GetLayoutElementMaxSize "private";
%csmethodmodifiers Urho3D::UIElement::GetIndent "private";
%csmethodmodifiers Urho3D::UIElement::SetIndent "private";
%csmethodmodifiers Urho3D::UIElement::GetIndentSpacing "private";
%csmethodmodifiers Urho3D::UIElement::SetIndentSpacing "private";
%csmethodmodifiers Urho3D::UIElement::GetIndentWidth "private";
%csmethodmodifiers Urho3D::UIElement::GetColorAttr "private";
%csmethodmodifiers Urho3D::UIElement::GetTraversalMode "private";
%csmethodmodifiers Urho3D::UIElement::SetTraversalMode "private";
%csmethodmodifiers Urho3D::UIElement::GetElementEventSender "private";
%csmethodmodifiers Urho3D::UIElement::GetEffectiveMinSize "private";
%typemap(cscode) Urho3D::UISelectable %{
  public $typemap(cstype, const Urho3D::Color &) SelectionColor {
    get { return GetSelectionColor(); }
    set { SetSelectionColor(value); }
  }
  public $typemap(cstype, const Urho3D::Color &) HoverColor {
    get { return GetHoverColor(); }
    set { SetHoverColor(value); }
  }
%}
%csmethodmodifiers Urho3D::UISelectable::GetSelectionColor "private";
%csmethodmodifiers Urho3D::UISelectable::SetSelectionColor "private";
%csmethodmodifiers Urho3D::UISelectable::GetHoverColor "private";
%csmethodmodifiers Urho3D::UISelectable::SetHoverColor "private";
%typemap(cscode) Urho3D::View3D %{
  public $typemap(cstype, unsigned int) Format {
    get { return GetFormat(); }
    set { SetFormat(value); }
  }
  public $typemap(cstype, bool) AutoUpdate {
    get { return GetAutoUpdate(); }
    set { SetAutoUpdate(value); }
  }
  public $typemap(cstype, Urho3D::Scene *) Scene {
    get { return GetScene(); }
  }
  public $typemap(cstype, Urho3D::Node *) CameraNode {
    get { return GetCameraNode(); }
  }
  public $typemap(cstype, Urho3D::Texture2D *) RenderTexture {
    get { return GetRenderTexture(); }
  }
  public $typemap(cstype, Urho3D::Texture2D *) DepthTexture {
    get { return GetDepthTexture(); }
  }
  public $typemap(cstype, Urho3D::Viewport *) Viewport {
    get { return GetViewport(); }
  }
%}
%csmethodmodifiers Urho3D::View3D::GetFormat "private";
%csmethodmodifiers Urho3D::View3D::SetFormat "private";
%csmethodmodifiers Urho3D::View3D::GetAutoUpdate "private";
%csmethodmodifiers Urho3D::View3D::SetAutoUpdate "private";
%csmethodmodifiers Urho3D::View3D::GetScene "private";
%csmethodmodifiers Urho3D::View3D::GetCameraNode "private";
%csmethodmodifiers Urho3D::View3D::GetRenderTexture "private";
%csmethodmodifiers Urho3D::View3D::GetDepthTexture "private";
%csmethodmodifiers Urho3D::View3D::GetViewport "private";
%typemap(cscode) Urho3D::Window %{
  public $typemap(cstype, bool) FixedWidthResizing {
    get { return GetFixedWidthResizing(); }
    set { SetFixedWidthResizing(value); }
  }
  public $typemap(cstype, bool) FixedHeightResizing {
    get { return GetFixedHeightResizing(); }
    set { SetFixedHeightResizing(value); }
  }
  public $typemap(cstype, const Urho3D::IntRect &) ResizeBorder {
    get { return GetResizeBorder(); }
    set { SetResizeBorder(value); }
  }
  public $typemap(cstype, const Urho3D::Color &) ModalShadeColor {
    get { return GetModalShadeColor(); }
    set { SetModalShadeColor(value); }
  }
  public $typemap(cstype, const Urho3D::Color &) ModalFrameColor {
    get { return GetModalFrameColor(); }
    set { SetModalFrameColor(value); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) ModalFrameSize {
    get { return GetModalFrameSize(); }
    set { SetModalFrameSize(value); }
  }
  public $typemap(cstype, bool) ModalAutoDismiss {
    get { return GetModalAutoDismiss(); }
    set { SetModalAutoDismiss(value); }
  }
%}
%csmethodmodifiers Urho3D::Window::GetFixedWidthResizing "private";
%csmethodmodifiers Urho3D::Window::SetFixedWidthResizing "private";
%csmethodmodifiers Urho3D::Window::GetFixedHeightResizing "private";
%csmethodmodifiers Urho3D::Window::SetFixedHeightResizing "private";
%csmethodmodifiers Urho3D::Window::GetResizeBorder "private";
%csmethodmodifiers Urho3D::Window::SetResizeBorder "private";
%csmethodmodifiers Urho3D::Window::GetModalShadeColor "private";
%csmethodmodifiers Urho3D::Window::SetModalShadeColor "private";
%csmethodmodifiers Urho3D::Window::GetModalFrameColor "private";
%csmethodmodifiers Urho3D::Window::SetModalFrameColor "private";
%csmethodmodifiers Urho3D::Window::GetModalFrameSize "private";
%csmethodmodifiers Urho3D::Window::SetModalFrameSize "private";
%csmethodmodifiers Urho3D::Window::GetModalAutoDismiss "private";
%csmethodmodifiers Urho3D::Window::SetModalAutoDismiss "private";
