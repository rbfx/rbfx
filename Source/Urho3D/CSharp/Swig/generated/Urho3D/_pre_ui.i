%ignore Urho3D::UI_VERTEX_SIZE;
%csconst(1) Urho3D::UI_VERTEX_SIZE;
%constant unsigned int UiVertexSize = 6;
%ignore Urho3D::FONT_TEXTURE_MIN_SIZE;
%csconst(1) Urho3D::FONT_TEXTURE_MIN_SIZE;
%constant int FontTextureMinSize = 128;
%ignore Urho3D::FONT_DPI;
%csconst(1) Urho3D::FONT_DPI;
%constant int FontDpi = 96;
%ignore Urho3D::DEFAULT_FONT_SIZE;
%csconst(1) Urho3D::DEFAULT_FONT_SIZE;
%constant float DefaultFontSize = (float)12;
%csconstvalue("0") Urho3D::HA_LEFT;
%csconstvalue("0") Urho3D::VA_TOP;
%csconstvalue("0") Urho3D::C_TOPLEFT;
%csconstvalue("0") Urho3D::O_HORIZONTAL;
%typemap(csattributes) Urho3D::DragAndDropMode "[global::System.Flags]";
using DragAndDropModeFlags = Urho3D::DragAndDropMode;
%typemap(ctype) DragAndDropModeFlags "size_t";
%typemap(out) DragAndDropModeFlags "$result = (size_t)$1.AsInteger();"
%csconstvalue("0") Urho3D::CS_NORMAL;
%csconstvalue("0") Urho3D::FONT_NONE;
%csconstvalue("0") Urho3D::TE_NONE;
%csattribute(Urho3D::UIElement, %arg(ea::string), Name, GetName, SetName);
%csattribute(Urho3D::UIElement, %arg(Urho3D::IntVector2), Position, GetPosition, SetPosition);
%csattribute(Urho3D::UIElement, %arg(Urho3D::IntVector2), Size, GetSize, SetSize);
%csattribute(Urho3D::UIElement, %arg(int), Width, GetWidth, SetWidth);
%csattribute(Urho3D::UIElement, %arg(int), Height, GetHeight, SetHeight);
%csattribute(Urho3D::UIElement, %arg(Urho3D::IntVector2), MinSize, GetMinSize, SetMinSize);
%csattribute(Urho3D::UIElement, %arg(int), MinWidth, GetMinWidth, SetMinWidth);
%csattribute(Urho3D::UIElement, %arg(int), MinHeight, GetMinHeight, SetMinHeight);
%csattribute(Urho3D::UIElement, %arg(Urho3D::IntVector2), MaxSize, GetMaxSize, SetMaxSize);
%csattribute(Urho3D::UIElement, %arg(int), MaxWidth, GetMaxWidth, SetMaxWidth);
%csattribute(Urho3D::UIElement, %arg(int), MaxHeight, GetMaxHeight, SetMaxHeight);
%csattribute(Urho3D::UIElement, %arg(bool), IsFixedSize, IsFixedSize);
%csattribute(Urho3D::UIElement, %arg(bool), IsFixedWidth, IsFixedWidth);
%csattribute(Urho3D::UIElement, %arg(bool), IsFixedHeight, IsFixedHeight);
%csattribute(Urho3D::UIElement, %arg(Urho3D::IntVector2), ChildOffset, GetChildOffset, SetChildOffset);
%csattribute(Urho3D::UIElement, %arg(Urho3D::HorizontalAlignment), HorizontalAlignment, GetHorizontalAlignment, SetHorizontalAlignment);
%csattribute(Urho3D::UIElement, %arg(Urho3D::VerticalAlignment), VerticalAlignment, GetVerticalAlignment, SetVerticalAlignment);
%csattribute(Urho3D::UIElement, %arg(bool), EnableAnchor, GetEnableAnchor, SetEnableAnchor);
%csattribute(Urho3D::UIElement, %arg(Urho3D::Vector2), MinAnchor, GetMinAnchor, SetMinAnchor);
%csattribute(Urho3D::UIElement, %arg(Urho3D::Vector2), MaxAnchor, GetMaxAnchor, SetMaxAnchor);
%csattribute(Urho3D::UIElement, %arg(Urho3D::IntVector2), MinOffset, GetMinOffset, SetMinOffset);
%csattribute(Urho3D::UIElement, %arg(Urho3D::IntVector2), MaxOffset, GetMaxOffset, SetMaxOffset);
%csattribute(Urho3D::UIElement, %arg(Urho3D::Vector2), Pivot, GetPivot, SetPivot);
%csattribute(Urho3D::UIElement, %arg(Urho3D::IntRect), ClipBorder, GetClipBorder, SetClipBorder);
%csattribute(Urho3D::UIElement, %arg(int), Priority, GetPriority, SetPriority);
%csattribute(Urho3D::UIElement, %arg(float), Opacity, GetOpacity, SetOpacity);
%csattribute(Urho3D::UIElement, %arg(float), DerivedOpacity, GetDerivedOpacity);
%csattribute(Urho3D::UIElement, %arg(bool), BringToBack, GetBringToBack, SetBringToBack);
%csattribute(Urho3D::UIElement, %arg(bool), ClipChildren, GetClipChildren, SetClipChildren);
%csattribute(Urho3D::UIElement, %arg(bool), UseDerivedOpacity, GetUseDerivedOpacity, SetUseDerivedOpacity);
%csattribute(Urho3D::UIElement, %arg(bool), IsEnabled, IsEnabled, SetEnabled);
%csattribute(Urho3D::UIElement, %arg(bool), IsEnabledSelf, IsEnabledSelf);
%csattribute(Urho3D::UIElement, %arg(bool), IsEditable, IsEditable, SetEditable);
%csattribute(Urho3D::UIElement, %arg(bool), IsSelected, IsSelected, SetSelected);
%csattribute(Urho3D::UIElement, %arg(bool), IsVisible, IsVisible, SetVisible);
%csattribute(Urho3D::UIElement, %arg(bool), IsVisibleEffective, IsVisibleEffective);
%csattribute(Urho3D::UIElement, %arg(bool), IsHovering, IsHovering, SetHovering);
%csattribute(Urho3D::UIElement, %arg(bool), IsInternal, IsInternal, SetInternal);
%csattribute(Urho3D::UIElement, %arg(Urho3D::FocusMode), FocusMode, GetFocusMode, SetFocusMode);
%csattribute(Urho3D::UIElement, %arg(Urho3D::DragAndDropModeFlags), DragDropMode, GetDragDropMode, SetDragDropMode);
%csattribute(Urho3D::UIElement, %arg(ea::string), AppliedStyle, GetAppliedStyle);
%csattribute(Urho3D::UIElement, %arg(Urho3D::LayoutMode), LayoutMode, GetLayoutMode, SetLayoutMode);
%csattribute(Urho3D::UIElement, %arg(int), LayoutSpacing, GetLayoutSpacing, SetLayoutSpacing);
%csattribute(Urho3D::UIElement, %arg(Urho3D::IntRect), LayoutBorder, GetLayoutBorder, SetLayoutBorder);
%csattribute(Urho3D::UIElement, %arg(Urho3D::Vector2), LayoutFlexScale, GetLayoutFlexScale, SetLayoutFlexScale);
%csattribute(Urho3D::UIElement, %arg(Urho3D::UIElement *), Root, GetRoot);
%csattribute(Urho3D::UIElement, %arg(Urho3D::Color), DerivedColor, GetDerivedColor);
%csattribute(Urho3D::UIElement, %arg(Urho3D::StringVariantMap), Vars, GetVars);
%csattribute(Urho3D::UIElement, %arg(Urho3D::StringVector), Tags, GetTags, SetTags);
%csattribute(Urho3D::UIElement, %arg(Urho3D::MouseButtonFlags), DragButtonCombo, GetDragButtonCombo);
%csattribute(Urho3D::UIElement, %arg(unsigned int), DragButtonCount, GetDragButtonCount);
%csattribute(Urho3D::UIElement, %arg(Urho3D::IntRect), CombinedScreenRect, GetCombinedScreenRect);
%csattribute(Urho3D::UIElement, %arg(int), LayoutElementMaxSize, GetLayoutElementMaxSize);
%csattribute(Urho3D::UIElement, %arg(int), Indent, GetIndent, SetIndent);
%csattribute(Urho3D::UIElement, %arg(int), IndentSpacing, GetIndentSpacing, SetIndentSpacing);
%csattribute(Urho3D::UIElement, %arg(int), IndentWidth, GetIndentWidth);
%csattribute(Urho3D::UIElement, %arg(Urho3D::Color), ColorAttr, GetColorAttr);
%csattribute(Urho3D::UIElement, %arg(Urho3D::TraversalMode), TraversalMode, GetTraversalMode, SetTraversalMode);
%csattribute(Urho3D::UIElement, %arg(bool), IsElementEventSender, IsElementEventSender, SetElementEventSender);
%csattribute(Urho3D::UIElement, %arg(Urho3D::IntVector2), EffectiveMinSize, GetEffectiveMinSize);
%csattribute(Urho3D::BorderImage, %arg(Urho3D::Texture *), Texture, GetTexture, SetTexture);
%csattribute(Urho3D::BorderImage, %arg(Urho3D::IntRect), ImageRect, GetImageRect, SetImageRect);
%csattribute(Urho3D::BorderImage, %arg(Urho3D::IntRect), Border, GetBorder, SetBorder);
%csattribute(Urho3D::BorderImage, %arg(Urho3D::IntRect), ImageBorder, GetImageBorder, SetImageBorder);
%csattribute(Urho3D::BorderImage, %arg(Urho3D::IntVector2), HoverOffset, GetHoverOffset, SetHoverOffset);
%csattribute(Urho3D::BorderImage, %arg(Urho3D::IntVector2), DisabledOffset, GetDisabledOffset, SetDisabledOffset);
%csattribute(Urho3D::BorderImage, %arg(Urho3D::BlendMode), BlendMode, GetBlendMode, SetBlendMode);
%csattribute(Urho3D::BorderImage, %arg(bool), IsTiled, IsTiled, SetTiled);
%csattribute(Urho3D::BorderImage, %arg(Urho3D::Material *), Material, GetMaterial, SetMaterial);
%csattribute(Urho3D::BorderImage, %arg(Urho3D::ResourceRef), TextureAttr, GetTextureAttr, SetTextureAttr);
%csattribute(Urho3D::BorderImage, %arg(Urho3D::ResourceRef), MaterialAttr, GetMaterialAttr, SetMaterialAttr);
%csattribute(Urho3D::Cursor, %arg(ea::string), Shape, GetShape, SetShape);
%csattribute(Urho3D::Cursor, %arg(bool), UseSystemShapes, GetUseSystemShapes, SetUseSystemShapes);
%csattribute(Urho3D::Cursor, %arg(Urho3D::VariantVector), ShapesAttr, GetShapesAttr, SetShapesAttr);
%csattribute(Urho3D::Window, %arg(bool), IsMovable, IsMovable, SetMovable);
%csattribute(Urho3D::Window, %arg(bool), IsResizable, IsResizable, SetResizable);
%csattribute(Urho3D::Window, %arg(bool), FixedWidthResizing, GetFixedWidthResizing, SetFixedWidthResizing);
%csattribute(Urho3D::Window, %arg(bool), FixedHeightResizing, GetFixedHeightResizing, SetFixedHeightResizing);
%csattribute(Urho3D::Window, %arg(Urho3D::IntRect), ResizeBorder, GetResizeBorder, SetResizeBorder);
%csattribute(Urho3D::Window, %arg(bool), IsModal, IsModal, SetModal);
%csattribute(Urho3D::Window, %arg(Urho3D::Color), ModalShadeColor, GetModalShadeColor, SetModalShadeColor);
%csattribute(Urho3D::Window, %arg(Urho3D::Color), ModalFrameColor, GetModalFrameColor, SetModalFrameColor);
%csattribute(Urho3D::Window, %arg(Urho3D::IntVector2), ModalFrameSize, GetModalFrameSize, SetModalFrameSize);
%csattribute(Urho3D::Window, %arg(bool), ModalAutoDismiss, GetModalAutoDismiss, SetModalAutoDismiss);
%csattribute(Urho3D::Button, %arg(Urho3D::IntVector2), PressedOffset, GetPressedOffset, SetPressedOffset);
%csattribute(Urho3D::Button, %arg(Urho3D::IntVector2), PressedChildOffset, GetPressedChildOffset, SetPressedChildOffset);
%csattribute(Urho3D::Button, %arg(float), RepeatDelay, GetRepeatDelay, SetRepeatDelay);
%csattribute(Urho3D::Button, %arg(float), RepeatRate, GetRepeatRate, SetRepeatRate);
%csattribute(Urho3D::Button, %arg(bool), IsPressed, IsPressed);
%csattribute(Urho3D::CheckBox, %arg(bool), IsChecked, IsChecked, SetChecked);
%csattribute(Urho3D::CheckBox, %arg(Urho3D::IntVector2), CheckedOffset, GetCheckedOffset, SetCheckedOffset);
%csattribute(Urho3D::Menu, %arg(Urho3D::UIElement *), Popup, GetPopup, SetPopup);
%csattribute(Urho3D::Menu, %arg(Urho3D::IntVector2), PopupOffset, GetPopupOffset, SetPopupOffset);
%csattribute(Urho3D::Menu, %arg(int), AcceleratorKey, GetAcceleratorKey);
%csattribute(Urho3D::Menu, %arg(int), AcceleratorQualifiers, GetAcceleratorQualifiers);
%csattribute(Urho3D::DropDownList, %arg(unsigned int), NumItems, GetNumItems);
%csattribute(Urho3D::DropDownList, %arg(ea::vector<UIElement *>), Items, GetItems);
%csattribute(Urho3D::DropDownList, %arg(unsigned int), Selection, GetSelection, SetSelection);
%csattribute(Urho3D::DropDownList, %arg(Urho3D::UIElement *), SelectedItem, GetSelectedItem);
%csattribute(Urho3D::DropDownList, %arg(Urho3D::ListView *), ListView, GetListView);
%csattribute(Urho3D::DropDownList, %arg(Urho3D::UIElement *), Placeholder, GetPlaceholder);
%csattribute(Urho3D::DropDownList, %arg(ea::string), PlaceholderText, GetPlaceholderText, SetPlaceholderText);
%csattribute(Urho3D::DropDownList, %arg(bool), ResizePopup, GetResizePopup, SetResizePopup);
%csattribute(Urho3D::FileSelector, %arg(Urho3D::XMLFile *), DefaultStyle, GetDefaultStyle, SetDefaultStyle);
%csattribute(Urho3D::FileSelector, %arg(Urho3D::Window *), Window, GetWindow);
%csattribute(Urho3D::FileSelector, %arg(Urho3D::Text *), TitleText, GetTitleText);
%csattribute(Urho3D::FileSelector, %arg(Urho3D::ListView *), FileList, GetFileList);
%csattribute(Urho3D::FileSelector, %arg(Urho3D::LineEdit *), PathEdit, GetPathEdit);
%csattribute(Urho3D::FileSelector, %arg(Urho3D::LineEdit *), FileNameEdit, GetFileNameEdit);
%csattribute(Urho3D::FileSelector, %arg(Urho3D::DropDownList *), FilterList, GetFilterList);
%csattribute(Urho3D::FileSelector, %arg(Urho3D::Button *), OKButton, GetOKButton);
%csattribute(Urho3D::FileSelector, %arg(Urho3D::Button *), CancelButton, GetCancelButton);
%csattribute(Urho3D::FileSelector, %arg(Urho3D::Button *), CloseButton, GetCloseButton);
%csattribute(Urho3D::FileSelector, %arg(ea::string), Title, GetTitle, SetTitle);
%csattribute(Urho3D::FileSelector, %arg(ea::string), Path, GetPath, SetPath);
%csattribute(Urho3D::FileSelector, %arg(ea::string), FileName, GetFileName, SetFileName);
%csattribute(Urho3D::FileSelector, %arg(ea::string), Filter, GetFilter);
%csattribute(Urho3D::FileSelector, %arg(unsigned int), FilterIndex, GetFilterIndex);
%csattribute(Urho3D::FileSelector, %arg(bool), DirectoryMode, GetDirectoryMode, SetDirectoryMode);
%csattribute(Urho3D::Font, %arg(Urho3D::FontType), FontType, GetFontType);
%csattribute(Urho3D::Font, %arg(bool), IsSDFFont, IsSDFFont);
%csattribute(Urho3D::Font, %arg(Urho3D::IntVector2), AbsoluteGlyphOffset, GetAbsoluteGlyphOffset, SetAbsoluteGlyphOffset);
%csattribute(Urho3D::Font, %arg(Urho3D::Vector2), ScaledGlyphOffset, GetScaledGlyphOffset, SetScaledGlyphOffset);
%csattribute(Urho3D::FontFace, %arg(bool), IsDataLost, IsDataLost);
%csattribute(Urho3D::FontFace, %arg(float), PointSize, GetPointSize);
%csattribute(Urho3D::FontFace, %arg(float), RowHeight, GetRowHeight);
%csattribute(Urho3D::FontFace, %arg(ea::vector<SharedPtr<Texture2D>>), Textures, GetTextures);
%csattribute(Urho3D::LineEdit, %arg(ea::string), Text, GetText, SetText);
%csattribute(Urho3D::LineEdit, %arg(unsigned int), CursorPosition, GetCursorPosition, SetCursorPosition);
%csattribute(Urho3D::LineEdit, %arg(float), CursorBlinkRate, GetCursorBlinkRate, SetCursorBlinkRate);
%csattribute(Urho3D::LineEdit, %arg(unsigned int), MaxLength, GetMaxLength, SetMaxLength);
%csattribute(Urho3D::LineEdit, %arg(unsigned int), EchoCharacter, GetEchoCharacter, SetEchoCharacter);
%csattribute(Urho3D::LineEdit, %arg(bool), IsCursorMovable, IsCursorMovable, SetCursorMovable);
%csattribute(Urho3D::LineEdit, %arg(bool), IsTextSelectable, IsTextSelectable, SetTextSelectable);
%csattribute(Urho3D::LineEdit, %arg(bool), IsTextCopyable, IsTextCopyable, SetTextCopyable);
%csattribute(Urho3D::LineEdit, %arg(Urho3D::Text *), TextElement, GetTextElement);
%csattribute(Urho3D::LineEdit, %arg(Urho3D::BorderImage *), Cursor, GetCursor);
%csattribute(Urho3D::ScrollView, %arg(bool), IsWheelHandler, IsWheelHandler);
%csattribute(Urho3D::ScrollView, %arg(Urho3D::IntVector2), ViewPosition, GetViewPosition, SetViewPosition);
%csattribute(Urho3D::ScrollView, %arg(Urho3D::UIElement *), ContentElement, GetContentElement, SetContentElement);
%csattribute(Urho3D::ScrollView, %arg(Urho3D::ScrollBar *), HorizontalScrollBar, GetHorizontalScrollBar);
%csattribute(Urho3D::ScrollView, %arg(Urho3D::ScrollBar *), VerticalScrollBar, GetVerticalScrollBar);
%csattribute(Urho3D::ScrollView, %arg(Urho3D::BorderImage *), ScrollPanel, GetScrollPanel);
%csattribute(Urho3D::ScrollView, %arg(bool), ScrollBarsAutoVisible, GetScrollBarsAutoVisible, SetScrollBarsAutoVisible);
%csattribute(Urho3D::ScrollView, %arg(bool), HorizontalScrollBarVisible, GetHorizontalScrollBarVisible, SetHorizontalScrollBarVisible);
%csattribute(Urho3D::ScrollView, %arg(bool), VerticalScrollBarVisible, GetVerticalScrollBarVisible, SetVerticalScrollBarVisible);
%csattribute(Urho3D::ScrollView, %arg(float), ScrollStep, GetScrollStep, SetScrollStep);
%csattribute(Urho3D::ScrollView, %arg(float), PageStep, GetPageStep, SetPageStep);
%csattribute(Urho3D::ScrollView, %arg(float), ScrollDeceleration, GetScrollDeceleration, SetScrollDeceleration);
%csattribute(Urho3D::ScrollView, %arg(float), ScrollSnapEpsilon, GetScrollSnapEpsilon, SetScrollSnapEpsilon);
%csattribute(Urho3D::ScrollView, %arg(bool), AutoDisableChildren, GetAutoDisableChildren, SetAutoDisableChildren);
%csattribute(Urho3D::ScrollView, %arg(float), AutoDisableThreshold, GetAutoDisableThreshold, SetAutoDisableThreshold);
%csattribute(Urho3D::ListView, %arg(unsigned int), NumItems, GetNumItems);
%csattribute(Urho3D::ListView, %arg(ea::vector<UIElement *>), Items, GetItems);
%csattribute(Urho3D::ListView, %arg(unsigned int), Selection, GetSelection, SetSelection);
%csattribute(Urho3D::ListView, %arg(ea::vector<unsigned int>), Selections, GetSelections, SetSelections);
%csattribute(Urho3D::ListView, %arg(Urho3D::UIElement *), SelectedItem, GetSelectedItem);
%csattribute(Urho3D::ListView, %arg(ea::vector<UIElement *>), SelectedItems, GetSelectedItems);
%csattribute(Urho3D::ListView, %arg(Urho3D::HighlightMode), HighlightMode, GetHighlightMode, SetHighlightMode);
%csattribute(Urho3D::ListView, %arg(bool), Multiselect, GetMultiselect, SetMultiselect);
%csattribute(Urho3D::ListView, %arg(bool), ClearSelectionOnDefocus, GetClearSelectionOnDefocus, SetClearSelectionOnDefocus);
%csattribute(Urho3D::ListView, %arg(bool), SelectOnClickEnd, GetSelectOnClickEnd, SetSelectOnClickEnd);
%csattribute(Urho3D::ListView, %arg(bool), HierarchyMode, GetHierarchyMode, SetHierarchyMode);
%csattribute(Urho3D::ListView, %arg(int), BaseIndent, GetBaseIndent, SetBaseIndent);
%csattribute(Urho3D::MessageBox, %arg(ea::string), Title, GetTitle, SetTitle);
%csattribute(Urho3D::MessageBox, %arg(ea::string), Message, GetMessage, SetMessage);
%csattribute(Urho3D::MessageBox, %arg(Urho3D::UIElement *), Window, GetWindow);
%csattribute(Urho3D::UISelectable, %arg(Urho3D::Color), SelectionColor, GetSelectionColor, SetSelectionColor);
%csattribute(Urho3D::UISelectable, %arg(Urho3D::Color), HoverColor, GetHoverColor, SetHoverColor);
%csattribute(Urho3D::Text, %arg(float), FontSize, GetFontSize, SetFontSize);
%csattribute(Urho3D::Text, %arg(Urho3D::HorizontalAlignment), TextAlignment, GetTextAlignment, SetTextAlignment);
%csattribute(Urho3D::Text, %arg(float), RowSpacing, GetRowSpacing, SetRowSpacing);
%csattribute(Urho3D::Text, %arg(bool), Wordwrap, GetWordwrap, SetWordwrap);
%csattribute(Urho3D::Text, %arg(bool), AutoLocalizable, GetAutoLocalizable, SetAutoLocalizable);
%csattribute(Urho3D::Text, %arg(unsigned int), SelectionStart, GetSelectionStart);
%csattribute(Urho3D::Text, %arg(unsigned int), SelectionLength, GetSelectionLength);
%csattribute(Urho3D::Text, %arg(Urho3D::TextEffect), TextEffect, GetTextEffect, SetTextEffect);
%csattribute(Urho3D::Text, %arg(Urho3D::IntVector2), EffectShadowOffset, GetEffectShadowOffset, SetEffectShadowOffset);
%csattribute(Urho3D::Text, %arg(int), EffectStrokeThickness, GetEffectStrokeThickness, SetEffectStrokeThickness);
%csattribute(Urho3D::Text, %arg(bool), EffectRoundStroke, GetEffectRoundStroke, SetEffectRoundStroke);
%csattribute(Urho3D::Text, %arg(Urho3D::Color), EffectColor, GetEffectColor, SetEffectColor);
%csattribute(Urho3D::Text, %arg(float), RowHeight, GetRowHeight);
%csattribute(Urho3D::Text, %arg(unsigned int), NumRows, GetNumRows);
%csattribute(Urho3D::Text, %arg(unsigned int), NumChars, GetNumChars);
%csattribute(Urho3D::Text, %arg(float), EffectDepthBias, GetEffectDepthBias, SetEffectDepthBias);
%csattribute(Urho3D::Text, %arg(Urho3D::ResourceRef), FontAttr, GetFontAttr, SetFontAttr);
%csattribute(Urho3D::Text, %arg(ea::string), TextAttr, GetTextAttr, SetTextAttr);
%csattribute(Urho3D::ProgressBar, %arg(Urho3D::Orientation), Orientation, GetOrientation, SetOrientation);
%csattribute(Urho3D::ProgressBar, %arg(float), Range, GetRange, SetRange);
%csattribute(Urho3D::ProgressBar, %arg(float), Value, GetValue, SetValue);
%csattribute(Urho3D::ProgressBar, %arg(Urho3D::BorderImage *), Knob, GetKnob);
%csattribute(Urho3D::ProgressBar, %arg(ea::string), LoadingPercentStyle, GetLoadingPercentStyle, SetLoadingPercentStyle);
%csattribute(Urho3D::ProgressBar, %arg(bool), ShowPercentText, GetShowPercentText, SetShowPercentText);
%csattribute(Urho3D::ScrollBar, %arg(Urho3D::Orientation), Orientation, GetOrientation, SetOrientation);
%csattribute(Urho3D::ScrollBar, %arg(float), Range, GetRange, SetRange);
%csattribute(Urho3D::ScrollBar, %arg(float), Value, GetValue, SetValue);
%csattribute(Urho3D::ScrollBar, %arg(float), ScrollStep, GetScrollStep, SetScrollStep);
%csattribute(Urho3D::ScrollBar, %arg(float), StepFactor, GetStepFactor, SetStepFactor);
%csattribute(Urho3D::ScrollBar, %arg(float), EffectiveScrollStep, GetEffectiveScrollStep);
%csattribute(Urho3D::ScrollBar, %arg(Urho3D::Button *), BackButton, GetBackButton);
%csattribute(Urho3D::ScrollBar, %arg(Urho3D::Button *), ForwardButton, GetForwardButton);
%csattribute(Urho3D::ScrollBar, %arg(Urho3D::Slider *), Slider, GetSlider);
%csattribute(Urho3D::Slider, %arg(Urho3D::Orientation), Orientation, GetOrientation, SetOrientation);
%csattribute(Urho3D::Slider, %arg(float), Range, GetRange, SetRange);
%csattribute(Urho3D::Slider, %arg(float), Value, GetValue, SetValue);
%csattribute(Urho3D::Slider, %arg(Urho3D::BorderImage *), Knob, GetKnob);
%csattribute(Urho3D::Slider, %arg(float), RepeatRate, GetRepeatRate, SetRepeatRate);
%csattribute(Urho3D::Sprite, %arg(Urho3D::IntVector2), ScreenPosition, GetScreenPosition);
%csattribute(Urho3D::Sprite, %arg(Urho3D::Vector2), Position, GetPosition, SetPosition);
%csattribute(Urho3D::Sprite, %arg(Urho3D::IntVector2), HotSpot, GetHotSpot, SetHotSpot);
%csattribute(Urho3D::Sprite, %arg(Urho3D::Vector2), Scale, GetScale, SetScale);
%csattribute(Urho3D::Sprite, %arg(float), Rotation, GetRotation, SetRotation);
%csattribute(Urho3D::Sprite, %arg(Urho3D::Texture *), Texture, GetTexture, SetTexture);
%csattribute(Urho3D::Sprite, %arg(Urho3D::IntRect), ImageRect, GetImageRect, SetImageRect);
%csattribute(Urho3D::Sprite, %arg(Urho3D::BlendMode), BlendMode, GetBlendMode, SetBlendMode);
%csattribute(Urho3D::Sprite, %arg(Urho3D::ResourceRef), TextureAttr, GetTextureAttr, SetTextureAttr);
%csattribute(Urho3D::Sprite, %arg(Urho3D::Matrix3x4), TransformMatrix, GetTransformMatrix);
%csattribute(Urho3D::SplashScreen, %arg(Urho3D::Texture *), BackgroundImage, GetBackgroundImage, SetBackgroundImage);
%csattribute(Urho3D::SplashScreen, %arg(Urho3D::Texture *), ForegroundImage, GetForegroundImage, SetForegroundImage);
%csattribute(Urho3D::SplashScreen, %arg(Urho3D::Texture *), ProgressImage, GetProgressImage, SetProgressImage);
%csattribute(Urho3D::SplashScreen, %arg(Urho3D::Color), ProgressColor, GetProgressColor, SetProgressColor);
%csattribute(Urho3D::SplashScreen, %arg(float), Duration, GetDuration, SetDuration);
%csattribute(Urho3D::SplashScreen, %arg(bool), IsSkippable, IsSkippable, SetSkippable);
%csattribute(Urho3D::Text3D, %arg(Urho3D::UpdateGeometryType), UpdateGeometryType, GetUpdateGeometryType);
%csattribute(Urho3D::Text3D, %arg(float), FontSize, GetFontSize, SetFontSize);
%csattribute(Urho3D::Text3D, %arg(Urho3D::Material *), Material, GetMaterial, SetMaterial);
%csattribute(Urho3D::Text3D, %arg(ea::string), Text, GetText, SetText);
%csattribute(Urho3D::Text3D, %arg(Urho3D::HorizontalAlignment), TextAlignment, GetTextAlignment, SetTextAlignment);
%csattribute(Urho3D::Text3D, %arg(Urho3D::HorizontalAlignment), HorizontalAlignment, GetHorizontalAlignment, SetHorizontalAlignment);
%csattribute(Urho3D::Text3D, %arg(Urho3D::VerticalAlignment), VerticalAlignment, GetVerticalAlignment, SetVerticalAlignment);
%csattribute(Urho3D::Text3D, %arg(float), RowSpacing, GetRowSpacing, SetRowSpacing);
%csattribute(Urho3D::Text3D, %arg(bool), Wordwrap, GetWordwrap, SetWordwrap);
%csattribute(Urho3D::Text3D, %arg(Urho3D::TextEffect), TextEffect, GetTextEffect, SetTextEffect);
%csattribute(Urho3D::Text3D, %arg(Urho3D::IntVector2), EffectShadowOffset, GetEffectShadowOffset, SetEffectShadowOffset);
%csattribute(Urho3D::Text3D, %arg(int), EffectStrokeThickness, GetEffectStrokeThickness, SetEffectStrokeThickness);
%csattribute(Urho3D::Text3D, %arg(bool), EffectRoundStroke, GetEffectRoundStroke, SetEffectRoundStroke);
%csattribute(Urho3D::Text3D, %arg(Urho3D::Color), EffectColor, GetEffectColor, SetEffectColor);
%csattribute(Urho3D::Text3D, %arg(float), EffectDepthBias, GetEffectDepthBias, SetEffectDepthBias);
%csattribute(Urho3D::Text3D, %arg(int), Width, GetWidth, SetWidth);
%csattribute(Urho3D::Text3D, %arg(int), Height, GetHeight);
%csattribute(Urho3D::Text3D, %arg(int), RowHeight, GetRowHeight);
%csattribute(Urho3D::Text3D, %arg(unsigned int), NumRows, GetNumRows);
%csattribute(Urho3D::Text3D, %arg(unsigned int), NumChars, GetNumChars);
%csattribute(Urho3D::Text3D, %arg(float), Opacity, GetOpacity, SetOpacity);
%csattribute(Urho3D::Text3D, %arg(bool), IsFixedScreenSize, IsFixedScreenSize, SetFixedScreenSize);
%csattribute(Urho3D::Text3D, %arg(bool), SnapToPixels, GetSnapToPixels, SetSnapToPixels);
%csattribute(Urho3D::Text3D, %arg(Urho3D::FaceCameraMode), FaceCameraMode, GetFaceCameraMode, SetFaceCameraMode);
%csattribute(Urho3D::Text3D, %arg(Urho3D::ResourceRef), FontAttr, GetFontAttr, SetFontAttr);
%csattribute(Urho3D::Text3D, %arg(Urho3D::ResourceRef), MaterialAttr, GetMaterialAttr, SetMaterialAttr);
%csattribute(Urho3D::Text3D, %arg(ea::string), TextAttr, GetTextAttr, SetTextAttr);
%csattribute(Urho3D::Text3D, %arg(Urho3D::Color), ColorAttr, GetColorAttr);
%csattribute(Urho3D::TextRenderer3D, %arg(float), DefaultFontSize, GetDefaultFontSize, SetDefaultFontSize);
%csattribute(Urho3D::TextRenderer3D, %arg(Urho3D::ResourceRef), DefaultFontAttr, GetDefaultFontAttr, SetDefaultFontAttr);
%csattribute(Urho3D::ToolTip, %arg(float), Delay, GetDelay, SetDelay);
%csattribute(Urho3D::UI, %arg(Urho3D::UIElement *), Root, GetRoot, SetRoot);
%csattribute(Urho3D::UI, %arg(Urho3D::UIElement *), RootModalElement, GetRootModalElement, SetRootModalElement);
%csattribute(Urho3D::UI, %arg(Urho3D::Cursor *), Cursor, GetCursor, SetCursor);
%csattribute(Urho3D::UI, %arg(Urho3D::IntVector2), UICursorPosition, GetUICursorPosition);
%csattribute(Urho3D::UI, %arg(Urho3D::IntVector2), SystemCursorPosition, GetSystemCursorPosition);
%csattribute(Urho3D::UI, %arg(Urho3D::UIElement *), FrontElement, GetFrontElement);
%csattribute(Urho3D::UI, %arg(ea::vector<UIElement *>), DragElements, GetDragElements);
%csattribute(Urho3D::UI, %arg(unsigned int), NumDragElements, GetNumDragElements);
%csattribute(Urho3D::UI, %arg(ea::string), ClipboardText, GetClipboardText, SetClipboardText);
%csattribute(Urho3D::UI, %arg(float), DoubleClickInterval, GetDoubleClickInterval, SetDoubleClickInterval);
%csattribute(Urho3D::UI, %arg(float), MaxDoubleClickDistance, GetMaxDoubleClickDistance, SetMaxDoubleClickDistance);
%csattribute(Urho3D::UI, %arg(float), DragBeginInterval, GetDragBeginInterval, SetDragBeginInterval);
%csattribute(Urho3D::UI, %arg(int), DragBeginDistance, GetDragBeginDistance, SetDragBeginDistance);
%csattribute(Urho3D::UI, %arg(float), DefaultToolTipDelay, GetDefaultToolTipDelay, SetDefaultToolTipDelay);
%csattribute(Urho3D::UI, %arg(int), MaxFontTextureSize, GetMaxFontTextureSize, SetMaxFontTextureSize);
%csattribute(Urho3D::UI, %arg(bool), IsNonFocusedMouseWheel, IsNonFocusedMouseWheel, SetNonFocusedMouseWheel);
%csattribute(Urho3D::UI, %arg(bool), UseSystemClipboard, GetUseSystemClipboard, SetUseSystemClipboard);
%csattribute(Urho3D::UI, %arg(bool), UseScreenKeyboard, GetUseScreenKeyboard, SetUseScreenKeyboard);
%csattribute(Urho3D::UI, %arg(bool), UseMutableGlyphs, GetUseMutableGlyphs, SetUseMutableGlyphs);
%csattribute(Urho3D::UI, %arg(bool), ForceAutoHint, GetForceAutoHint, SetForceAutoHint);
%csattribute(Urho3D::UI, %arg(Urho3D::FontHintLevel), FontHintLevel, GetFontHintLevel, SetFontHintLevel);
%csattribute(Urho3D::UI, %arg(float), FontSubpixelThreshold, GetFontSubpixelThreshold, SetFontSubpixelThreshold);
%csattribute(Urho3D::UI, %arg(int), FontOversampling, GetFontOversampling, SetFontOversampling);
%csattribute(Urho3D::UI, %arg(bool), IsDragging, IsDragging);
%csattribute(Urho3D::UI, %arg(float), Scale, GetScale, SetScale);
%csattribute(Urho3D::UI, %arg(Urho3D::IntVector2), Size, GetSize);
%csattribute(Urho3D::UI, %arg(Urho3D::IntVector2), CustomSize, GetCustomSize, SetCustomSize);
%csattribute(Urho3D::UI, %arg(bool), IsRendered, IsRendered);
%csattribute(Urho3D::UIComponent, %arg(Urho3D::UIElement *), Root, GetRoot);
%csattribute(Urho3D::UIComponent, %arg(Urho3D::Material *), Material, GetMaterial);
%csattribute(Urho3D::UIComponent, %arg(Urho3D::Texture2D *), Texture, GetTexture);
%csattribute(Urho3D::View3D, %arg(bool), AutoUpdate, GetAutoUpdate, SetAutoUpdate);
%csattribute(Urho3D::View3D, %arg(Urho3D::Scene *), Scene, GetScene);
%csattribute(Urho3D::View3D, %arg(Urho3D::Node *), CameraNode, GetCameraNode);
%csattribute(Urho3D::View3D, %arg(Urho3D::Texture2D *), RenderTexture, GetRenderTexture);
%csattribute(Urho3D::View3D, %arg(Urho3D::Texture2D *), DepthTexture, GetDepthTexture);
%csattribute(Urho3D::View3D, %arg(Urho3D::Viewport *), Viewport, GetViewport);
%pragma(csharp) moduleimports=%{
public static partial class E
{
    public class UIMouseClickEvent {
        private StringHash _event = new StringHash("UIMouseClick");
        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public UIMouseClickEvent() { }
        public static implicit operator StringHash(UIMouseClickEvent e) { return e._event; }
    }
    public static UIMouseClickEvent UIMouseClick = new UIMouseClickEvent();
    public class UIMouseClickEndEvent {
        private StringHash _event = new StringHash("UIMouseClickEnd");
        public StringHash Element = new StringHash("Element");
        public StringHash BeginElement = new StringHash("BeginElement");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public UIMouseClickEndEvent() { }
        public static implicit operator StringHash(UIMouseClickEndEvent e) { return e._event; }
    }
    public static UIMouseClickEndEvent UIMouseClickEnd = new UIMouseClickEndEvent();
    public class UIMouseDoubleClickEvent {
        private StringHash _event = new StringHash("UIMouseDoubleClick");
        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash XBegin = new StringHash("XBegin");
        public StringHash YBegin = new StringHash("YBegin");
        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public UIMouseDoubleClickEvent() { }
        public static implicit operator StringHash(UIMouseDoubleClickEvent e) { return e._event; }
    }
    public static UIMouseDoubleClickEvent UIMouseDoubleClick = new UIMouseDoubleClickEvent();
    public class ClickEvent {
        private StringHash _event = new StringHash("Click");
        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public ClickEvent() { }
        public static implicit operator StringHash(ClickEvent e) { return e._event; }
    }
    public static ClickEvent Click = new ClickEvent();
    public class ClickEndEvent {
        private StringHash _event = new StringHash("ClickEnd");
        public StringHash Element = new StringHash("Element");
        public StringHash BeginElement = new StringHash("BeginElement");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public ClickEndEvent() { }
        public static implicit operator StringHash(ClickEndEvent e) { return e._event; }
    }
    public static ClickEndEvent ClickEnd = new ClickEndEvent();
    public class DoubleClickEvent {
        private StringHash _event = new StringHash("DoubleClick");
        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash XBegin = new StringHash("XBegin");
        public StringHash YBegin = new StringHash("YBegin");
        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public DoubleClickEvent() { }
        public static implicit operator StringHash(DoubleClickEvent e) { return e._event; }
    }
    public static DoubleClickEvent DoubleClick = new DoubleClickEvent();
    public class DragDropTestEvent {
        private StringHash _event = new StringHash("DragDropTest");
        public StringHash Source = new StringHash("Source");
        public StringHash Target = new StringHash("Target");
        public StringHash Accept = new StringHash("Accept");
        public DragDropTestEvent() { }
        public static implicit operator StringHash(DragDropTestEvent e) { return e._event; }
    }
    public static DragDropTestEvent DragDropTest = new DragDropTestEvent();
    public class DragDropFinishEvent {
        private StringHash _event = new StringHash("DragDropFinish");
        public StringHash Source = new StringHash("Source");
        public StringHash Target = new StringHash("Target");
        public StringHash Accept = new StringHash("Accept");
        public DragDropFinishEvent() { }
        public static implicit operator StringHash(DragDropFinishEvent e) { return e._event; }
    }
    public static DragDropFinishEvent DragDropFinish = new DragDropFinishEvent();
    public class FocusChangedEvent {
        private StringHash _event = new StringHash("FocusChanged");
        public StringHash Element = new StringHash("Element");
        public StringHash ClickedElement = new StringHash("ClickedElement");
        public FocusChangedEvent() { }
        public static implicit operator StringHash(FocusChangedEvent e) { return e._event; }
    }
    public static FocusChangedEvent FocusChanged = new FocusChangedEvent();
    public class NameChangedEvent {
        private StringHash _event = new StringHash("NameChanged");
        public StringHash Element = new StringHash("Element");
        public NameChangedEvent() { }
        public static implicit operator StringHash(NameChangedEvent e) { return e._event; }
    }
    public static NameChangedEvent NameChanged = new NameChangedEvent();
    public class ResizedEvent {
        private StringHash _event = new StringHash("Resized");
        public StringHash Element = new StringHash("Element");
        public StringHash Width = new StringHash("Width");
        public StringHash Height = new StringHash("Height");
        public StringHash DX = new StringHash("DX");
        public StringHash DY = new StringHash("DY");
        public ResizedEvent() { }
        public static implicit operator StringHash(ResizedEvent e) { return e._event; }
    }
    public static ResizedEvent Resized = new ResizedEvent();
    public class PositionedEvent {
        private StringHash _event = new StringHash("Positioned");
        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public PositionedEvent() { }
        public static implicit operator StringHash(PositionedEvent e) { return e._event; }
    }
    public static PositionedEvent Positioned = new PositionedEvent();
    public class VisibleChangedEvent {
        private StringHash _event = new StringHash("VisibleChanged");
        public StringHash Element = new StringHash("Element");
        public StringHash Visible = new StringHash("Visible");
        public VisibleChangedEvent() { }
        public static implicit operator StringHash(VisibleChangedEvent e) { return e._event; }
    }
    public static VisibleChangedEvent VisibleChanged = new VisibleChangedEvent();
    public class FocusedEvent {
        private StringHash _event = new StringHash("Focused");
        public StringHash Element = new StringHash("Element");
        public StringHash ByKey = new StringHash("ByKey");
        public FocusedEvent() { }
        public static implicit operator StringHash(FocusedEvent e) { return e._event; }
    }
    public static FocusedEvent Focused = new FocusedEvent();
    public class DefocusedEvent {
        private StringHash _event = new StringHash("Defocused");
        public StringHash Element = new StringHash("Element");
        public DefocusedEvent() { }
        public static implicit operator StringHash(DefocusedEvent e) { return e._event; }
    }
    public static DefocusedEvent Defocused = new DefocusedEvent();
    public class LayoutUpdatedEvent {
        private StringHash _event = new StringHash("LayoutUpdated");
        public StringHash Element = new StringHash("Element");
        public LayoutUpdatedEvent() { }
        public static implicit operator StringHash(LayoutUpdatedEvent e) { return e._event; }
    }
    public static LayoutUpdatedEvent LayoutUpdated = new LayoutUpdatedEvent();
    public class PressedEvent {
        private StringHash _event = new StringHash("Pressed");
        public StringHash Element = new StringHash("Element");
        public PressedEvent() { }
        public static implicit operator StringHash(PressedEvent e) { return e._event; }
    }
    public static PressedEvent Pressed = new PressedEvent();
    public class ReleasedEvent {
        private StringHash _event = new StringHash("Released");
        public StringHash Element = new StringHash("Element");
        public ReleasedEvent() { }
        public static implicit operator StringHash(ReleasedEvent e) { return e._event; }
    }
    public static ReleasedEvent Released = new ReleasedEvent();
    public class ToggledEvent {
        private StringHash _event = new StringHash("Toggled");
        public StringHash Element = new StringHash("Element");
        public StringHash State = new StringHash("State");
        public ToggledEvent() { }
        public static implicit operator StringHash(ToggledEvent e) { return e._event; }
    }
    public static ToggledEvent Toggled = new ToggledEvent();
    public class SliderChangedEvent {
        private StringHash _event = new StringHash("SliderChanged");
        public StringHash Element = new StringHash("Element");
        public StringHash Value = new StringHash("Value");
        public SliderChangedEvent() { }
        public static implicit operator StringHash(SliderChangedEvent e) { return e._event; }
    }
    public static SliderChangedEvent SliderChanged = new SliderChangedEvent();
    public class SliderPagedEvent {
        private StringHash _event = new StringHash("SliderPaged");
        public StringHash Element = new StringHash("Element");
        public StringHash Offset = new StringHash("Offset");
        public StringHash Pressed = new StringHash("Pressed");
        public SliderPagedEvent() { }
        public static implicit operator StringHash(SliderPagedEvent e) { return e._event; }
    }
    public static SliderPagedEvent SliderPaged = new SliderPagedEvent();
    public class ProgressBarChangedEvent {
        private StringHash _event = new StringHash("ProgressBarChanged");
        public StringHash Element = new StringHash("Element");
        public StringHash Value = new StringHash("Value");
        public ProgressBarChangedEvent() { }
        public static implicit operator StringHash(ProgressBarChangedEvent e) { return e._event; }
    }
    public static ProgressBarChangedEvent ProgressBarChanged = new ProgressBarChangedEvent();
    public class ScrollBarChangedEvent {
        private StringHash _event = new StringHash("ScrollBarChanged");
        public StringHash Element = new StringHash("Element");
        public StringHash Value = new StringHash("Value");
        public ScrollBarChangedEvent() { }
        public static implicit operator StringHash(ScrollBarChangedEvent e) { return e._event; }
    }
    public static ScrollBarChangedEvent ScrollBarChanged = new ScrollBarChangedEvent();
    public class ViewChangedEvent {
        private StringHash _event = new StringHash("ViewChanged");
        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public ViewChangedEvent() { }
        public static implicit operator StringHash(ViewChangedEvent e) { return e._event; }
    }
    public static ViewChangedEvent ViewChanged = new ViewChangedEvent();
    public class ModalChangedEvent {
        private StringHash _event = new StringHash("ModalChanged");
        public StringHash Element = new StringHash("Element");
        public StringHash Modal = new StringHash("Modal");
        public ModalChangedEvent() { }
        public static implicit operator StringHash(ModalChangedEvent e) { return e._event; }
    }
    public static ModalChangedEvent ModalChanged = new ModalChangedEvent();
    public class TextEntryEvent {
        private StringHash _event = new StringHash("TextEntry");
        public StringHash Element = new StringHash("Element");
        public StringHash Text = new StringHash("Text");
        public TextEntryEvent() { }
        public static implicit operator StringHash(TextEntryEvent e) { return e._event; }
    }
    public static TextEntryEvent TextEntry = new TextEntryEvent();
    public class TextChangedEvent {
        private StringHash _event = new StringHash("TextChanged");
        public StringHash Element = new StringHash("Element");
        public StringHash Text = new StringHash("Text");
        public TextChangedEvent() { }
        public static implicit operator StringHash(TextChangedEvent e) { return e._event; }
    }
    public static TextChangedEvent TextChanged = new TextChangedEvent();
    public class TextFinishedEvent {
        private StringHash _event = new StringHash("TextFinished");
        public StringHash Element = new StringHash("Element");
        public StringHash Text = new StringHash("Text");
        public StringHash Value = new StringHash("Value");
        public TextFinishedEvent() { }
        public static implicit operator StringHash(TextFinishedEvent e) { return e._event; }
    }
    public static TextFinishedEvent TextFinished = new TextFinishedEvent();
    public class MenuSelectedEvent {
        private StringHash _event = new StringHash("MenuSelected");
        public StringHash Element = new StringHash("Element");
        public MenuSelectedEvent() { }
        public static implicit operator StringHash(MenuSelectedEvent e) { return e._event; }
    }
    public static MenuSelectedEvent MenuSelected = new MenuSelectedEvent();
    public class ItemSelectedEvent {
        private StringHash _event = new StringHash("ItemSelected");
        public StringHash Element = new StringHash("Element");
        public StringHash Selection = new StringHash("Selection");
        public ItemSelectedEvent() { }
        public static implicit operator StringHash(ItemSelectedEvent e) { return e._event; }
    }
    public static ItemSelectedEvent ItemSelected = new ItemSelectedEvent();
    public class ItemDeselectedEvent {
        private StringHash _event = new StringHash("ItemDeselected");
        public StringHash Element = new StringHash("Element");
        public StringHash Selection = new StringHash("Selection");
        public ItemDeselectedEvent() { }
        public static implicit operator StringHash(ItemDeselectedEvent e) { return e._event; }
    }
    public static ItemDeselectedEvent ItemDeselected = new ItemDeselectedEvent();
    public class SelectionChangedEvent {
        private StringHash _event = new StringHash("SelectionChanged");
        public StringHash Element = new StringHash("Element");
        public SelectionChangedEvent() { }
        public static implicit operator StringHash(SelectionChangedEvent e) { return e._event; }
    }
    public static SelectionChangedEvent SelectionChanged = new SelectionChangedEvent();
    public class ItemClickedEvent {
        private StringHash _event = new StringHash("ItemClicked");
        public StringHash Element = new StringHash("Element");
        public StringHash Item = new StringHash("Item");
        public StringHash Selection = new StringHash("Selection");
        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public ItemClickedEvent() { }
        public static implicit operator StringHash(ItemClickedEvent e) { return e._event; }
    }
    public static ItemClickedEvent ItemClicked = new ItemClickedEvent();
    public class ItemDoubleClickedEvent {
        private StringHash _event = new StringHash("ItemDoubleClicked");
        public StringHash Element = new StringHash("Element");
        public StringHash Item = new StringHash("Item");
        public StringHash Selection = new StringHash("Selection");
        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public ItemDoubleClickedEvent() { }
        public static implicit operator StringHash(ItemDoubleClickedEvent e) { return e._event; }
    }
    public static ItemDoubleClickedEvent ItemDoubleClicked = new ItemDoubleClickedEvent();
    public class UnhandledKeyEvent {
        private StringHash _event = new StringHash("UnhandledKey");
        public StringHash Element = new StringHash("Element");
        public StringHash Key = new StringHash("Key");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public UnhandledKeyEvent() { }
        public static implicit operator StringHash(UnhandledKeyEvent e) { return e._event; }
    }
    public static UnhandledKeyEvent UnhandledKey = new UnhandledKeyEvent();
    public class FileSelectedEvent {
        private StringHash _event = new StringHash("FileSelected");
        public StringHash FileName = new StringHash("FileName");
        public StringHash Filter = new StringHash("Filter");
        public StringHash OK = new StringHash("OK");
        public FileSelectedEvent() { }
        public static implicit operator StringHash(FileSelectedEvent e) { return e._event; }
    }
    public static FileSelectedEvent FileSelected = new FileSelectedEvent();
    public class MessageACKEvent {
        private StringHash _event = new StringHash("MessageACK");
        public StringHash OK = new StringHash("OK");
        public MessageACKEvent() { }
        public static implicit operator StringHash(MessageACKEvent e) { return e._event; }
    }
    public static MessageACKEvent MessageACK = new MessageACKEvent();
    public class ElementAddedEvent {
        private StringHash _event = new StringHash("ElementAdded");
        public StringHash Root = new StringHash("Root");
        public StringHash Parent = new StringHash("Parent");
        public StringHash Element = new StringHash("Element");
        public ElementAddedEvent() { }
        public static implicit operator StringHash(ElementAddedEvent e) { return e._event; }
    }
    public static ElementAddedEvent ElementAdded = new ElementAddedEvent();
    public class ElementRemovedEvent {
        private StringHash _event = new StringHash("ElementRemoved");
        public StringHash Root = new StringHash("Root");
        public StringHash Parent = new StringHash("Parent");
        public StringHash Element = new StringHash("Element");
        public ElementRemovedEvent() { }
        public static implicit operator StringHash(ElementRemovedEvent e) { return e._event; }
    }
    public static ElementRemovedEvent ElementRemoved = new ElementRemovedEvent();
    public class HoverBeginEvent {
        private StringHash _event = new StringHash("HoverBegin");
        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash ElementX = new StringHash("ElementX");
        public StringHash ElementY = new StringHash("ElementY");
        public HoverBeginEvent() { }
        public static implicit operator StringHash(HoverBeginEvent e) { return e._event; }
    }
    public static HoverBeginEvent HoverBegin = new HoverBeginEvent();
    public class HoverEndEvent {
        private StringHash _event = new StringHash("HoverEnd");
        public StringHash Element = new StringHash("Element");
        public HoverEndEvent() { }
        public static implicit operator StringHash(HoverEndEvent e) { return e._event; }
    }
    public static HoverEndEvent HoverEnd = new HoverEndEvent();
    public class DragBeginEvent {
        private StringHash _event = new StringHash("DragBegin");
        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash ElementX = new StringHash("ElementX");
        public StringHash ElementY = new StringHash("ElementY");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash NumButtons = new StringHash("NumButtons");
        public DragBeginEvent() { }
        public static implicit operator StringHash(DragBeginEvent e) { return e._event; }
    }
    public static DragBeginEvent DragBegin = new DragBeginEvent();
    public class DragMoveEvent {
        private StringHash _event = new StringHash("DragMove");
        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash DX = new StringHash("DX");
        public StringHash DY = new StringHash("DY");
        public StringHash ElementX = new StringHash("ElementX");
        public StringHash ElementY = new StringHash("ElementY");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash NumButtons = new StringHash("NumButtons");
        public DragMoveEvent() { }
        public static implicit operator StringHash(DragMoveEvent e) { return e._event; }
    }
    public static DragMoveEvent DragMove = new DragMoveEvent();
    public class DragEndEvent {
        private StringHash _event = new StringHash("DragEnd");
        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash ElementX = new StringHash("ElementX");
        public StringHash ElementY = new StringHash("ElementY");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash NumButtons = new StringHash("NumButtons");
        public DragEndEvent() { }
        public static implicit operator StringHash(DragEndEvent e) { return e._event; }
    }
    public static DragEndEvent DragEnd = new DragEndEvent();
    public class DragCancelEvent {
        private StringHash _event = new StringHash("DragCancel");
        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash ElementX = new StringHash("ElementX");
        public StringHash ElementY = new StringHash("ElementY");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash NumButtons = new StringHash("NumButtons");
        public DragCancelEvent() { }
        public static implicit operator StringHash(DragCancelEvent e) { return e._event; }
    }
    public static DragCancelEvent DragCancel = new DragCancelEvent();
    public class UIDropFileEvent {
        private StringHash _event = new StringHash("UIDropFile");
        public StringHash FileName = new StringHash("FileName");
        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash ElementX = new StringHash("ElementX");
        public StringHash ElementY = new StringHash("ElementY");
        public UIDropFileEvent() { }
        public static implicit operator StringHash(UIDropFileEvent e) { return e._event; }
    }
    public static UIDropFileEvent UIDropFile = new UIDropFileEvent();
} %}
