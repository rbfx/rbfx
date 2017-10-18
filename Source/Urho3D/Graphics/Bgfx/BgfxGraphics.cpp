#include "../../Precompiled.h"

//#include "../../Graphics/ConstantBuffer.h"
//#include "../../Graphics/GraphicsDefs.h"
//
//#include "../../Core/Context.h"
//#include "../../Core/ProcessUtils.h"
//#include "../../Core/Profiler.h"
#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsEvents.h"
//#include "../../Graphics/GraphicsImpl.h"
//#include "../../Graphics/IndexBuffer.h"
//#include "../../Graphics/Shader.h"
#include "../../Graphics/ShaderPrecache.h"
//#include "../../Graphics/ShaderProgram.h"
//#include "../../Graphics/Texture2D.h"
//#include "../../Graphics/TextureCube.h"
//#include "../../Graphics/VertexBuffer.h"
//#include "../../Graphics/VertexDeclaration.h"

#include "BgfxGraphicsImpl.h"

#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>


namespace Urho3D
{

const Vector2 Graphics::pixelUVOffset(0.0f, 0.0f);

Graphics::Graphics(Context* context_) :
    Object(context_),
    impl_(new GraphicsImpl()),
    window_(nullptr),
    externalWindow_(nullptr),
    width_(0),
    height_(0),
    position_(SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED),
    multiSample_(1),
    fullscreen_(false),
    borderless_(false),
    resizable_(false),
    highDPI_(false),
    vsync_(false),
    monitor_(0),
    refreshRate_(0),
    tripleBuffer_(false),
    sRGB_(false),
    instancingSupport_(false),
    lightPrepassSupport_(false),
    deferredSupport_(false),
    anisotropySupport_(false),
    dxtTextureSupport_(false),
    etcTextureSupport_(false),
    pvrtcTextureSupport_(false),
    hardwareShadowSupport_(false),
    sRGBSupport_(false),
    sRGBWriteSupport_(false),
    numPrimitives_(0),
    numBatches_(0),
    maxScratchBufferRequest_(0),
    defaultTextureFilterMode_(FILTER_TRILINEAR),
    defaultTextureAnisotropy_(4),
    shaderPath_("Shaders/GLSL/"),
    shaderExtension_(".glsl"),
    orientations_("LandscapeLeft LandscapeRight"),
    apiName_("Bgfx")
{

    // Register Graphics library object factories
    RegisterGraphicsLibrary(context_);
}

Graphics::~Graphics()
{
    Close();

    delete impl_;
    impl_ = nullptr;
}

bool Graphics::SetMode(int width, int height, bool fullscreen, bool borderless, bool resizable, bool highDPI, bool vsync,
    bool tripleBuffer, int multiSample, int monitor, int refreshRate)

{
    fullscreen_ = fullscreen;
    borderless_ = borderless;
    resizable_ = resizable;
    highDPI_ = highDPI;
    vsync_ = vsync;
    tripleBuffer_ = tripleBuffer;
    multiSample_ = multiSample;
    monitor_ = monitor;
    refreshRate_ = refreshRate;

    if (!IsInitialized())
    {
        int x = 0;
        int y = 0;

        unsigned flags =  SDL_WINDOW_SHOWN;

        window_ = SDL_CreateWindow(windowTitle_.CString(), x, y, width, height, flags);
        SDL_ShowWindow(window_);

        SDL_SysWMinfo wmi;
        SDL_VERSION(&wmi.version);
        SDL_GetWindowWMInfo(window_, &wmi);

        bgfx::PlatformData pd;
        pd.ndt          = NULL;
#if defined(WIN32)
        pd.ndt          = NULL;
        pd.nwh          = wmi.info.win.window;
#elif defined(__APPLE__)
        pd.nwh          = wmi.info.cocoa.window;
#else
        // Other unixes
        pd.nwh          = reinterpret_cast<void*>(wmi.info.x11.window);
        pd.ndt          = reinterpret_cast<void*>(wmi.info.x11.display);
#endif
        pd.context      = NULL;
        pd.backBuffer   = NULL;
        pd.backBufferDS = NULL;
        bgfx::setPlatformData(pd);

    }

    bgfx::init(bgfx::RendererType::OpenGL);
    SetMode(width, height);
    return true;
}

bool Graphics::SetMode(int width, int height)
{
    if (!width || !height)
    {
        width = 1024;
        height = 768;
    }

    width_ = width;
    height_ = height;

    bgfx::reset(width, height);

    ResetRenderTargets();

    using namespace ScreenMode;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_WIDTH] = width_;
    eventData[P_HEIGHT] = height_;
    eventData[P_FULLSCREEN] = fullscreen_;
    eventData[P_BORDERLESS] = borderless_;
    eventData[P_RESIZABLE] = resizable_;
    eventData[P_HIGHDPI] = highDPI_;
    eventData[P_MONITOR] = monitor_;
    eventData[P_REFRESHRATE] = refreshRate_;
    SendEvent(E_SCREENMODE, eventData);

    return true;
}

void Graphics::SetSRGB(bool enable) {}
void Graphics::SetDither(bool enable) {}
void Graphics::SetFlushGPU(bool enable) {}
void Graphics::SetForceGL2(bool enable) {}

void Graphics::Close()
{
    if (!IsInitialized())
        return;

    // Actually close the window
    Release(true, true);
}
void Graphics::Release(bool clearGPUObjects, bool closeWindow)
{
    if (!window_)
        return;

    bgfx::shutdown();

    // End fullscreen mode first to counteract transition and getting stuck problems on OS X
#if defined(__APPLE__) && !defined(IOS) && !defined(TVOS)
    if (closeWindow && fullscreen_ && !externalWindow_)
        SDL_SetWindowFullscreen(window_, 0);
#endif

    if (closeWindow)
    {
        SDL_ShowCursor(SDL_TRUE);

        // Do not destroy external window except when shutting down
        if (!externalWindow_ || clearGPUObjects)
        {
            SDL_DestroyWindow(window_);
            window_ = nullptr;
        }
    }
}

bool Graphics::TakeScreenShot(Image& destImage) { return false; }

bool Graphics::BeginFrame()
{
    static uint8_t col = 0;

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH, col++ << 8, 1.0f, 0);
    bgfx::touch(0);

    return true;
}

void Graphics::EndFrame()
{
    bgfx::frame();
}

void Graphics::Clear(unsigned flags, const Color& color, float depth, unsigned stencil) {}
bool Graphics::ResolveToTexture(Texture2D* destination, const IntRect& viewport) { return true; }
bool Graphics::ResolveToTexture(Texture2D* texture) { return true; }
bool Graphics::ResolveToTexture(TextureCube* texture) { return true; }
void Graphics::Draw(PrimitiveType type, unsigned vertexStart, unsigned vertexCount) {}
void Graphics::Draw(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned minVertex, unsigned vertexCount) {}
void Graphics::Draw(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned baseVertexIndex, unsigned minVertex, unsigned vertexCount) {}
void Graphics::DrawInstanced(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned minVertex, unsigned vertexCount, unsigned instanceCount) {}
void Graphics::DrawInstanced(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned baseVertexIndex, unsigned minVertex, unsigned vertexCount, unsigned instanceCount) {}
void Graphics::SetVertexBuffer(VertexBuffer* buffer) {}
bool Graphics::SetVertexBuffers(const PODVector<VertexBuffer*>& buffers, unsigned instanceOffset) { return true; }
bool Graphics::SetVertexBuffers(const Vector<SharedPtr<VertexBuffer> >& buffers, unsigned instanceOffset) { return true; }
void Graphics::SetIndexBuffer(IndexBuffer* buffer) {}
void Graphics::SetShaders(ShaderVariation* vs, ShaderVariation* ps) {}
void Graphics::SetShaderParameter(StringHash param, const float* data, unsigned count) {}
void Graphics::SetShaderParameter(StringHash param, float value) {}
void Graphics::SetShaderParameter(StringHash param, int value) {}
void Graphics::SetShaderParameter(StringHash param, bool value) {}
void Graphics::SetShaderParameter(StringHash param, const Color& color) {}
void Graphics::SetShaderParameter(StringHash param, const Vector2& vector) {}
void Graphics::SetShaderParameter(StringHash param, const Matrix3& matrix) {}
void Graphics::SetShaderParameter(StringHash param, const Vector3& vector) {}
void Graphics::SetShaderParameter(StringHash param, const Matrix4& matrix) {}
void Graphics::SetShaderParameter(StringHash param, const Vector4& vector) {}
void Graphics::SetShaderParameter(StringHash param, const Matrix3x4& matrix) {}
bool Graphics::NeedParameterUpdate(ShaderParameterGroup group, const void* source) { return false; }
bool Graphics::HasShaderParameter(StringHash param) { return true; }
bool Graphics::HasTextureUnit(TextureUnit unit) { return true; }
void Graphics::ClearParameterSource(ShaderParameterGroup group) {}
void Graphics::ClearParameterSources() {}
void Graphics::ClearTransformSources() {}
void Graphics::SetTexture(unsigned index, Texture* texture) {}
void Graphics::SetTextureForUpdate(Texture* texture) {}
void Graphics::SetTextureParametersDirty() {}
void Graphics::SetDefaultTextureFilterMode(TextureFilterMode mode) {}
void Graphics::SetDefaultTextureAnisotropy(unsigned level) {}

void Graphics::ResetRenderTargets()
{
    for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
        SetRenderTarget(i, (RenderSurface*)nullptr);
    SetDepthStencil((RenderSurface*)nullptr);
    SetViewport(IntRect(0, 0, width_, height_));
}

void Graphics::ResetRenderTarget(unsigned index)
{
    bgfx::resetView(index);
}

void Graphics::ResetDepthStencil() { }
void Graphics::SetRenderTarget(unsigned index, RenderSurface* renderTarget) { }
void Graphics::SetRenderTarget(unsigned index, Texture2D* texture) { }
void Graphics::SetDepthStencil(RenderSurface* depthStencil) { }
void Graphics::SetDepthStencil(Texture2D* texture) { }

void Graphics::SetViewport(const IntRect& rect)
{
    bgfx::setViewRect(0, rect.left_, rect.top_, rect.right_, rect.bottom_);
}

void Graphics::SetBlendMode(BlendMode mode, bool alphaToCoverage)
{

}

void Graphics::SetColorWrite(bool enable)
{

}

void Graphics::SetCullMode(CullMode mode)
{

}

void Graphics::SetDepthBias(float constantBias, float slopeScaledBias)
{

}

void Graphics::SetDepthTest(CompareMode mode)
{

}

void Graphics::SetDepthWrite(bool enable)
{

}

void Graphics::SetFillMode(FillMode mode)
{

}

void Graphics::SetLineAntiAlias(bool enable)
{

}

void Graphics::SetScissorTest(bool enable, const Rect& rect, bool borderInclusive)
{

}

void Graphics::SetScissorTest(bool enable, const IntRect& rect)
{

}

void Graphics::SetStencilTest(bool enable, CompareMode mode, StencilOp pass, StencilOp fail, StencilOp zFail,
    unsigned stencilRef, unsigned compareMask, unsigned writeMask)
{

}

void Graphics::SetClipPlane(bool enable, const Plane& clipPlane, const Matrix3x4& view, const Matrix4& projection)
{

}

//void Graphics::BeginDumpShaders(const String& fileName)
//{

//}

//void Graphics::EndDumpShaders()
//{

//}

//void Graphics::SetShaderCacheDir(const String& path)
//{

//}

bool Graphics::IsInitialized() const
{
    return window_ != nullptr;
}

bool Graphics::GetDither() const
{
    return false;
}

void Graphics::OnDeviceLost()
{
}

bool Graphics::IsDeviceLost() const { return false; }

PODVector<int> Graphics::GetMultiSampleLevels() const
{
    PODVector<int> ret;
    ret.Push(1);
    return ret;
}

unsigned Graphics::GetFormat(CompressedFormat format) const
{
    return 0;
}

ShaderVariation* Graphics::GetShader(ShaderType type, const String& name, const String& defines) const
{
    return nullptr;
}

ShaderVariation* Graphics::GetShader(ShaderType type, const char* name, const char* defines) const
{
    return nullptr;
}

VertexBuffer* Graphics::GetVertexBuffer(unsigned index) const
{
    return nullptr;
}

ShaderProgram* Graphics::GetShaderProgram() const
{
    return nullptr;
}

TextureUnit Graphics::GetTextureUnit(const String& name)
{
    return TU_DIFFUSE;
}

const String& Graphics::GetTextureUnitName(TextureUnit unit)
{
    static String ret = "TU_DIFFUSE";
    return ret;
}

Texture* Graphics::GetTexture(unsigned index) const
{
    return nullptr;
}

RenderSurface* Graphics::GetRenderTarget(unsigned index) const
{
    return nullptr;
}

IntVector2 Graphics::GetRenderTargetDimensions() const
{
    return IntVector2(0, 0);
}

void Graphics::OnWindowResized() {}
void Graphics::OnWindowMoved() {}
void Graphics::Restore() {}
void Graphics::CleanupShaderPrograms(ShaderVariation* variation){}
void Graphics::CleanupRenderSurface(RenderSurface* surface) {}

ConstantBuffer* Graphics::GetOrCreateConstantBuffer(ShaderType type, unsigned index, unsigned size)
{
    return nullptr;
}

void Graphics::MarkFBODirty() {}
void Graphics::SetVBO(unsigned object) {}
void Graphics::SetUBO(unsigned object) {}
unsigned Graphics::GetAlphaFormat() { return 0; }
unsigned Graphics::GetLuminanceFormat() { return 0; }
unsigned Graphics::GetLuminanceAlphaFormat() { return 0; }
unsigned Graphics::GetRGBFormat() { return 0; }
unsigned Graphics::GetRGBAFormat() { return 0; }
unsigned Graphics::GetRGBA16Format() { return 0; }
unsigned Graphics::GetRGBAFloat16Format() { return 0; }
unsigned Graphics::GetRGBAFloat32Format() { return 0; }
unsigned Graphics::GetRG16Format() { return 0; }
unsigned Graphics::GetRGFloat16Format() { return 0; }
unsigned Graphics::GetRGFloat32Format() { return 0; }
unsigned Graphics::GetFloat16Format() { return 0; }
unsigned Graphics::GetFloat32Format() { return 0; }
unsigned Graphics::GetLinearDepthFormat() { return 0; }
unsigned Graphics::GetDepthStencilFormat() { return 0; }
unsigned Graphics::GetReadableDepthFormat() { return 0; }

unsigned Graphics::GetFormat(const String& formatName)
{
    String nameLower = formatName.ToLower().Trimmed();

    if (nameLower == "a")
        return GetAlphaFormat();
    if (nameLower == "l")
        return GetLuminanceFormat();
    if (nameLower == "la")
        return GetLuminanceAlphaFormat();
    if (nameLower == "rgb")
        return GetRGBFormat();
    if (nameLower == "rgba")
        return GetRGBAFormat();
    if (nameLower == "rgba16")
        return GetRGBA16Format();
    if (nameLower == "rgba16f")
        return GetRGBAFloat16Format();
    if (nameLower == "rgba32f")
        return GetRGBAFloat32Format();
    if (nameLower == "rg16")
        return GetRG16Format();
    if (nameLower == "rg16f")
        return GetRGFloat16Format();
    if (nameLower == "rg32f")
        return GetRGFloat32Format();
    if (nameLower == "r16f")
        return GetFloat16Format();
    if (nameLower == "r32f" || nameLower == "float")
        return GetFloat32Format();
    if (nameLower == "lineardepth" || nameLower == "depth")
        return GetLinearDepthFormat();
    if (nameLower == "d24s8")
        return GetDepthStencilFormat();
    if (nameLower == "readabledepth" || nameLower == "hwdepth")
        return GetReadableDepthFormat();

    return GetRGBFormat();
}

unsigned Graphics::GetMaxBones() { return 0; }
bool Graphics::GetGL3Support() { return false; }

}