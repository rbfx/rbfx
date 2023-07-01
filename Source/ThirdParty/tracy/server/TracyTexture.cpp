#include "../../../Urho3D/Graphics/Texture2D.h"
#include "../../../Urho3D/SystemUI/SystemUI.h"

#include "TracyTexture.hpp"

namespace tracy
{

void* MakeTexture()
{
    auto* systemUI = static_cast<Urho3D::SystemUI*>(ui::GetIO().UserData);
    return new Urho3D::Texture2D(systemUI->GetContext());
}

void FreeTexture( void* _tex, void(*runOnMainThread)(std::function<void()>, bool) )
{
    runOnMainThread( [_tex] { delete static_cast<Urho3D::Texture2D*>(_tex); }, false );
}

void UpdateTexture( void* _tex, const char* data, int w, int h )
{
    auto* texture = static_cast<Urho3D::Texture2D*>(_tex);
    texture->SetSize(w, h, Diligent::TEX_FORMAT_BC1_UNORM);
    texture->SetData(0, 0, 0, w, h, data);
}

}
