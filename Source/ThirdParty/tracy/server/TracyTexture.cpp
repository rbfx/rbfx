#include "../../../Urho3D/Graphics/GraphicsImpl.h"
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

void FreeTexture( void* _tex, void(*runOnMainThread)(std::function<void()>) )
{
    runOnMainThread( [_tex] { delete static_cast<Urho3D::Texture2D*>(_tex); });
}

void UpdateTexture( void* _tex, const char* data, int w, int h )
{
    auto* texture = static_cast<Urho3D::Texture2D*>(_tex);
#if defined(URHO3D_OPENGL)
    unsigned format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
#elif defined(URHO3D_D3D11)
    unsigned format = DXGI_FORMAT_BC1_UNORM;
#else
    unsigned format = D3DFMT_DXT1;
#endif
    texture->SetSize(w, h, format);
    texture->SetData(0, 0, 0, w, h, data);
}

}
