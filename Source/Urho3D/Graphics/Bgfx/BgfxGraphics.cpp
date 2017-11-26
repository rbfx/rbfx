#include "../../Precompiled.h"

#include "../../Core/Context.h"
#include "../../Core/ProcessUtils.h"
#include "../../Core/Profiler.h"
#include "../../Graphics/ConstantBuffer.h"
#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsEvents.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/IndexBuffer.h"
#include "../../Graphics/Shader.h"
#include "../../Graphics/ShaderPrecache.h"
#include "../../Graphics/ShaderProgram.h"
#include "../../Graphics/Texture2D.h"
#include "../../Graphics/TextureCube.h"
#include "../../Graphics/VertexBuffer.h"
#include "../../IO/File.h"
#include "../../IO/Log.h"
#include "../../Resource/ResourceCache.h"

#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>
#include <bgfx/platform.h>


namespace Urho3D
{

static const uint64_t bgfxBlendState[] =
{
    BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ZERO) | BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_ADD), // BLEND_REPLACE
    BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE) | BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_ADD), // BLEND_ADD
    BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_COLOR, BGFX_STATE_BLEND_ZERO) | BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_ADD), // BLEND_MULTIPLY
    BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA) | BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_ADD), // BLEND_ALPHA
    BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE) | BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_ADD), // BLEND_ADDALPHA
    BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA) | BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_ADD), // BLEND_PREMULALPHA
    BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_INV_DST_ALPHA, BGFX_STATE_BLEND_DST_ALPHA) | BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_ADD), // BLEND_INVDESTALPHA
    BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE) | BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_REVSUB), // BLEND_SUBTRACT
    BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE) | BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_REVSUB) // BLEND_SUBTRACTALPHA
};

static const uint64_t bgfxCullMode[] =
{
    0,
    BGFX_STATE_CULL_CCW,
    BGFX_STATE_CULL_CW
};

static const uint64_t bgfxPrimitiveType[] =
{
    0, //TRIANGLE_LIST
    BGFX_STATE_PT_LINES, //LINE_LIST
    BGFX_STATE_PT_POINTS, //POINT_LIST
    BGFX_STATE_PT_TRISTRIP, //TRIANGLE_STRIP
    BGFX_STATE_PT_LINESTRIP, //LINE_STRIP
    0 //TRIANGLE_FAN (unsupported)
};

static const uint64_t bgfxDepthCompare[] =
{
    BGFX_STATE_DEPTH_TEST_ALWAYS, //CMP_ALWAYS = 0,
    BGFX_STATE_DEPTH_TEST_EQUAL, //CMP_EQUAL,
    BGFX_STATE_DEPTH_TEST_NOTEQUAL, //CMP_NOTEQUAL,
    BGFX_STATE_DEPTH_TEST_LESS, //CMP_LESS,
    BGFX_STATE_DEPTH_TEST_LEQUAL, //CMP_LESSEQUAL,
    BGFX_STATE_DEPTH_TEST_GREATER, //CMP_GREATER,
    BGFX_STATE_DEPTH_TEST_GEQUAL //CMP_GREATEREQUAL,
};

static const uint64_t bgfxStencilCompare[] =
{
    BGFX_STENCIL_TEST_ALWAYS, //CMP_ALWAYS = 0,
    BGFX_STENCIL_TEST_EQUAL, //CMP_EQUAL,
    BGFX_STENCIL_TEST_NOTEQUAL, //CMP_NOTEQUAL,
    BGFX_STENCIL_TEST_LESS, //CMP_LESS,
    BGFX_STENCIL_TEST_LEQUAL, //CMP_LESSEQUAL,
    BGFX_STENCIL_TEST_GREATER, //CMP_GREATER,
    BGFX_STENCIL_TEST_GEQUAL //CMP_GREATEREQUAL,
};

static const uint64_t bgfxStencilPass[] =
{
    BGFX_STENCIL_OP_PASS_Z_KEEP, //OP_KEEP = 0,
    BGFX_STENCIL_OP_PASS_Z_ZERO, //OP_ZERO,
    BGFX_STENCIL_OP_PASS_Z_REPLACE, //OP_REF,
    BGFX_STENCIL_OP_PASS_Z_INCR, //OP_INCR,
    BGFX_STENCIL_OP_PASS_Z_DECR //OP_DECR
};

static const uint64_t bgfxStencilFail[] =
{
    BGFX_STENCIL_OP_FAIL_S_KEEP, //OP_KEEP = 0,
    BGFX_STENCIL_OP_FAIL_S_ZERO, //OP_ZERO,
    BGFX_STENCIL_OP_FAIL_S_REPLACE, //OP_REF,
    BGFX_STENCIL_OP_FAIL_S_INCR, //OP_INCR,
    BGFX_STENCIL_OP_FAIL_S_DECR //OP_DECR
};

static const uint64_t bgfxStencilZFail[] =
{
    BGFX_STENCIL_OP_FAIL_Z_KEEP, //OP_KEEP = 0,
    BGFX_STENCIL_OP_FAIL_Z_ZERO, //OP_ZERO,
    BGFX_STENCIL_OP_FAIL_Z_REPLACE, //OP_REF,
    BGFX_STENCIL_OP_FAIL_Z_INCR, //OP_INCR,
    BGFX_STENCIL_OP_FAIL_Z_DECR //OP_DECR
};

static const GraphicsApiType bgfxToUrhoRenderer[] =
{
    BGFX_NOOP,
    BGFX_DIRECT3D9,
    BGFX_DIRECT3D11,
    BGFX_DIRECT3D12,
    BGFX_GNM,
    BGFX_METAL,
    BGFX_OPENGLES,
    BGFX_OPENGL,
    BGFX_VULKAN
};

static const bgfx::RendererType::Enum urhoToBgfxRenderer[] =
{
    bgfx::RendererType::Noop,
    bgfx::RendererType::Direct3D9,
    bgfx::RendererType::Direct3D11,
    bgfx::RendererType::OpenGLES,
    bgfx::RendererType::OpenGL,
    bgfx::RendererType::Noop,
    bgfx::RendererType::Direct3D9,
    bgfx::RendererType::Direct3D11,
    bgfx::RendererType::Direct3D12,
    bgfx::RendererType::Gnm,
    bgfx::RendererType::Metal,
    bgfx::RendererType::OpenGLES,
    bgfx::RendererType::OpenGL,
    bgfx::RendererType::Vulkan
};

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
    apiName_("Bgfx"),
    apiType_(GraphicsApiType::NOOP)
{
    SetTextureUnitMappings(); // TODO: Need to delay this on mobile check, 8 texture units limit
    ResetCachedState();

    context_->RequireSDL(SDL_INIT_VIDEO);

    // Register Graphics library object factories
    RegisterGraphicsLibrary(context_);
}

Graphics::~Graphics()
{
    Close();

    delete impl_;
    impl_ = nullptr;

    context_->ReleaseSDL();
}

bool Graphics::SetMode(int width, int height, bool fullscreen, bool borderless, bool resizable, bool highDPI, bool vsync,
    bool tripleBuffer, int multiSample, int monitor, int refreshRate)
{
    URHO3D_PROFILE(SetScreenMode);

    bool maximize = false;

#if defined(IOS) || defined(TVOS)
    // iOS and tvOS app always take the fullscreen (and with status bar hidden)
    fullscreen = true;
#endif

    // Make sure monitor index is not bigger than the currently detected monitors
    int monitors = SDL_GetNumVideoDisplays();
    if (monitor >= monitors || monitor < 0)
        monitor = 0; // this monitor is not present, use first monitor

    // Fullscreen or Borderless can not be resizable
    if (fullscreen || borderless)
        resizable = false;

    // Borderless cannot be fullscreen, they are mutually exclusive
    if (borderless)
        fullscreen = false;

    // If nothing changes, do not reset the device
    if (width == width_ && height == height_ && fullscreen == fullscreen_ && borderless == borderless_ && resizable == resizable_ &&
        vsync == vsync_ && tripleBuffer == tripleBuffer_ && multiSample == multiSample_)
        return true;

    SDL_SetHint(SDL_HINT_ORIENTATIONS, orientations_.CString());

    // If zero dimensions in windowed mode, set windowed mode to maximize and set a predefined default restored window size.
    // If zero in fullscreen, use desktop mode
    if (!width || !height)
    {
        if (fullscreen || borderless)
        {
            SDL_DisplayMode mode;
            SDL_GetDesktopDisplayMode(monitor, &mode);
            width = mode.w;
            height = mode.h;
        }
        else
        {
            maximize = resizable;
            width = 1024;
            height = 768;
        }
    }

    // Check fullscreen mode validity (desktop only). Use a closest match if not found
#ifdef DESKTOP_GRAPHICS
    if (fullscreen)
    {
        PODVector<IntVector3> resolutions = GetResolutions(monitor);
        if (resolutions.Size())
        {
            unsigned best = 0;
            unsigned bestError = M_MAX_UNSIGNED;

            for (unsigned i = 0; i < resolutions.Size(); ++i)
            {
                unsigned error = Abs(resolutions[i].x_ - width) + Abs(resolutions[i].y_ - height);
                if (error < bestError)
                {
                    best = i;
                    bestError = error;
                }
            }

            width = resolutions[best].x_;
            height = resolutions[best].y_;
            refreshRate = resolutions[best].z_;
        }
    }
#endif

    AdjustWindow(width, height, fullscreen, borderless, monitor);
    monitor_ = monitor;
    refreshRate_ = refreshRate;

    if (maximize)
    {
        Maximize();
        SDL_GetWindowSize(window_, &width, &height);
    }

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

    bgfx::init(urhoToBgfxRenderer[GraphicsApiType::BGFX_OPENGL]);
    apiType_ = bgfxToUrhoRenderer[bgfx::getRendererType()];

#ifdef URHO3D_LOGGING
    String msg;
    msg.AppendWithFormat("Set screen mode %dx%d %s monitor %d", width_, height_, (fullscreen_ ? "fullscreen" : "windowed"), monitor_);
    if (borderless_)
        msg.Append(" borderless");
    if (resizable_)
        msg.Append(" resizable");
    if (multiSample > 1)
        msg.AppendWithFormat(" multisample %d", multiSample);
    //URHO3D_LOGINFO(msg);
#endif
    return SetMode(width, height);
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
    bgfx::setDebug(BGFX_DEBUG_TEXT);
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);

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
    if (window_)
    {
        SDL_ShowCursor(SDL_TRUE);
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
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
    if (!IsInitialized())
        return false;

    // If using an external window, check it for size changes, and reset screen mode if necessary
    if (externalWindow_)
    {
        int width, height;

        SDL_GetWindowSize(window_, &width, &height);
        if (width != width_ || height != height_)
            SetMode(width, height);
    }
    else
    {
        // To prevent a loop of endless device loss and flicker, do not attempt to render when in fullscreen
        // and the window is minimized
        if (fullscreen_ && (SDL_GetWindowFlags(window_) & SDL_WINDOW_MINIMIZED))
            return false;
    }

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH, col++ << 8, 1.0f, 0);
    bgfx::touch(0);

    SendEvent(E_BEGINRENDERING);
    return true;
}

void Graphics::EndFrame()
{
    if (!IsInitialized())
        return;

    {
        URHO3D_PROFILE(Present);

        SendEvent(E_ENDRENDERING);
        bgfx::frame();
    }

    // Clean up too large scratch buffers
    CleanupScratchBuffers();
}

void Graphics::Clear(unsigned flags, const Color& color, float depth, unsigned stencil)
{
	// Urho3D clear flags conveniently match BGFX ones
	// TODO: need to implement scissor stuff here
	bgfx::setViewClear(impl_->view_, (uint16_t)flags, color.ToUInt(), depth, stencil);
}

bool Graphics::ResolveToTexture(Texture2D* destination, const IntRect& viewport)
{
    if (!destination || !destination->GetRenderSurface())
        return false;

    URHO3D_PROFILE(ResolveToTexture);

    IntRect vpCopy = viewport;
    if (vpCopy.right_ <= vpCopy.left_)
        vpCopy.right_ = vpCopy.left_ + 1;
    if (vpCopy.bottom_ <= vpCopy.top_)
        vpCopy.bottom_ = vpCopy.top_ + 1;
    vpCopy.left_ = Clamp(vpCopy.left_, 0, width_);
    vpCopy.top_ = Clamp(vpCopy.top_, 0, height_);
    vpCopy.right_ = Clamp(vpCopy.right_, 0, width_);
    vpCopy.bottom_ = Clamp(vpCopy.bottom_, 0, height_);

    bgfx::TextureHandle srcHandle = bgfx::getTexture(impl_->currentFramebuffer_, 0);

    bgfx::TextureHandle dstHandle;
    dstHandle.idx = destination->GetGPUObjectIdx();
    bool flip = false;
    if ((apiType_ == BGFX_OPENGL) || (apiType_ == BGFX_OPENGLES))
        flip = true;
    bgfx::blit(impl_->view_, dstHandle, vpCopy.left_, flip ? height_ - vpCopy.bottom_ : vpCopy.bottom_,
        srcHandle, vpCopy.left_, flip ? height_ - vpCopy.bottom_ : vpCopy.bottom_, vpCopy.Width(), vpCopy.Height());

    return true;
}

bool Graphics::ResolveToTexture(Texture2D* texture)
{
    return true;
}

bool Graphics::ResolveToTexture(TextureCube* texture)
{
    return true;
}

void Graphics::Draw(PrimitiveType type, unsigned vertexStart, unsigned vertexCount)
{
    if (!vertexCount || !bgfx::isValid(impl_->shaderProgram_->handle_))
        return;

    if (type != primitiveType_)
    {
        impl_->primitiveType_ = bgfxPrimitiveType[type];
        primitiveType_ = type;
        impl_->stateDirty_ = true;
    }

    PrepareDraw();

    if (bgfx::isValid(impl_->indexBuffer_))
        bgfx::setIndexBuffer(impl_->indexBuffer_);
    else
        bgfx::setIndexBuffer(impl_->dynamicIndexBuffer_);

    for (unsigned i = 0; i < MAX_VERTEX_STREAMS; ++i)
    {
        if (bgfx::isValid(impl_->vertexBuffer_[i]))
            bgfx::setVertexBuffer(i, impl_->vertexBuffer_[i], vertexStart, vertexCount);
        else
            bgfx::setVertexBuffer(i, impl_->dynamicVertexBuffer_[i], vertexStart, vertexCount);
    }

    uint32_t primitiveCount;
    primitiveCount = bgfx::submit(impl_->view_, impl_->shaderProgram_->handle_, impl_->drawDistance_, false);
    impl_->drawDistance_ = 0;
    numPrimitives_ += primitiveCount;
    ++numBatches_;
}

void Graphics::Draw(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned minVertex, unsigned vertexCount)
{
    if (!vertexCount || !bgfx::isValid(impl_->shaderProgram_->handle_))
        return;

    if (type != primitiveType_)
    {
        impl_->primitiveType_ = bgfxPrimitiveType[type];
        primitiveType_ = type;
        impl_->stateDirty_ = true;
    }

    PrepareDraw();

    if (bgfx::isValid(impl_->indexBuffer_))
        bgfx::setIndexBuffer(impl_->indexBuffer_, indexStart, indexCount);
    else
        bgfx::setIndexBuffer(impl_->dynamicIndexBuffer_, indexStart, indexCount);

    for (unsigned i = 0; i < MAX_VERTEX_STREAMS; ++i)
    {
        if (bgfx::isValid(impl_->vertexBuffer_[i]))
            bgfx::setVertexBuffer(i, impl_->vertexBuffer_[i], minVertex, vertexCount);
        else
            bgfx::setVertexBuffer(i, impl_->dynamicVertexBuffer_[i], minVertex, vertexCount);
    }

    uint32_t primitiveCount;
    primitiveCount = bgfx::submit(impl_->view_, impl_->shaderProgram_->handle_, impl_->drawDistance_, false);
    impl_->drawDistance_ = 0;
    numPrimitives_ += primitiveCount;
    ++numBatches_;
}

void Graphics::Draw(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned baseVertexIndex, unsigned minVertex, unsigned vertexCount)
{
    if (!vertexCount || !bgfx::isValid(impl_->shaderProgram_->handle_))
        return;

    if (type != primitiveType_)
    {
        impl_->primitiveType_ = bgfxPrimitiveType[type];
        primitiveType_ = type;
        impl_->stateDirty_ = true;
    }

    PrepareDraw();

    if (bgfx::isValid(impl_->indexBuffer_))
        bgfx::setIndexBuffer(impl_->indexBuffer_, indexStart, indexCount);
    else
        bgfx::setIndexBuffer(impl_->dynamicIndexBuffer_, indexStart, indexCount);

    for (unsigned i = 0; i < MAX_VERTEX_STREAMS; ++i)
    {
        if (bgfx::isValid(impl_->vertexBuffer_[i]))
            bgfx::setVertexBuffer(i, impl_->vertexBuffer_[i], minVertex, vertexCount);
        else
            bgfx::setVertexBuffer(i, impl_->dynamicVertexBuffer_[i], minVertex, vertexCount);
    }

    uint32_t primitiveCount;
    primitiveCount = bgfx::submit(impl_->view_, impl_->shaderProgram_->handle_, impl_->drawDistance_, false);
    impl_->drawDistance_ = 0;
    numPrimitives_ += primitiveCount;
    ++numBatches_;
}

void Graphics::DrawInstanced(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned minVertex, unsigned vertexCount, unsigned instanceCount)
{
    if (!indexCount || !instanceCount || !bgfx::isValid(impl_->shaderProgram_->handle_))
        return;

    if (type != primitiveType_)
    {
        impl_->primitiveType_ = bgfxPrimitiveType[type];
        primitiveType_ = type;
        impl_->stateDirty_ = true;
    }

    PrepareDraw();

    if (bgfx::isValid(impl_->indexBuffer_))
        bgfx::setIndexBuffer(impl_->indexBuffer_, indexStart, indexCount);
    else
        bgfx::setIndexBuffer(impl_->dynamicIndexBuffer_, indexStart, indexCount);

    for (unsigned i = 0; i < MAX_VERTEX_STREAMS; ++i)
    {
        if (bgfx::isValid(impl_->vertexBuffer_[i]))
            bgfx::setVertexBuffer(i, impl_->vertexBuffer_[i], minVertex, vertexCount);
        else
            bgfx::setVertexBuffer(i, impl_->dynamicVertexBuffer_[i], minVertex, vertexCount);
    }

    //bgfx::setInstanceDataBuffer(_, 0, instanceCount);

    uint32_t primitiveCount;
    primitiveCount = bgfx::submit(impl_->view_, impl_->shaderProgram_->handle_, impl_->drawDistance_, false);
    impl_->drawDistance_ = 0;
    numPrimitives_ += primitiveCount;
    ++numBatches_;
}

void Graphics::DrawInstanced(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned baseVertexIndex, unsigned minVertex, unsigned vertexCount, unsigned instanceCount)
{
    if (!indexCount || !instanceCount || !bgfx::isValid(impl_->shaderProgram_->handle_))
        return;

    if (type != primitiveType_)
    {
        impl_->primitiveType_ = bgfxPrimitiveType[type];
        primitiveType_ = type;
        impl_->stateDirty_ = true;
    }

    PrepareDraw();

    if (bgfx::isValid(impl_->indexBuffer_))
        bgfx::setIndexBuffer(impl_->indexBuffer_, indexStart, indexCount);
    else
        bgfx::setIndexBuffer(impl_->dynamicIndexBuffer_, indexStart, indexCount);

    for (unsigned i = 0; i < MAX_VERTEX_STREAMS; ++i)
    {
        if (bgfx::isValid(impl_->vertexBuffer_[i]))
            bgfx::setVertexBuffer(i, impl_->vertexBuffer_[i], minVertex, vertexCount);
        else
            bgfx::setVertexBuffer(i, impl_->dynamicVertexBuffer_[i], minVertex, vertexCount);
    }

    //bgfx::setInstanceDataBuffer(_, 0, instanceCount);

    uint32_t primitiveCount;
    primitiveCount = bgfx::submit(impl_->view_, impl_->shaderProgram_->handle_, impl_->drawDistance_, false);
    impl_->drawDistance_ = 0;
    numPrimitives_ += primitiveCount;
    ++numBatches_;
}

void Graphics::SetVertexBuffer(VertexBuffer* buffer)
{
    // Note: this is not multi-instance safe
    static PODVector<VertexBuffer*> vertexBuffers(1);
    vertexBuffers[0] = buffer;
    SetVertexBuffers(vertexBuffers);
}

bool Graphics::SetVertexBuffers(const PODVector<VertexBuffer*>& buffers, unsigned instanceOffset)
{
    if (buffers.Size() > MAX_VERTEX_STREAMS)
    {
        URHO3D_LOGERROR("Too many vertex buffers");
        return false;
    }

    /*
    if (instanceOffset != impl_->lastInstanceOffset_)
    {
        impl_->lastInstanceOffset_ = instanceOffset;
        impl_->vertexBuffersDirty_ = true;
    }
    */

    for (unsigned i = 0; i < MAX_VERTEX_STREAMS; ++i)
    {
        VertexBuffer* buffer = nullptr;
        if (i < buffers.Size())
            buffer = buffers[i];
        if (buffer != vertexBuffers_[i])
        {
            vertexBuffers_[i] = buffer;
            if (buffer->IsDynamic())
            {
                bgfx::DynamicVertexBufferHandle handle;
                handle.idx = buffer->GetGPUObjectIdx();
                impl_->dynamicVertexBuffer_[i] = handle;
                bgfx::VertexBufferHandle nullHandle;
                nullHandle.idx = bgfx::kInvalidHandle;
                impl_->vertexBuffer_[i] = nullHandle;
            }
            else
            {
                bgfx::VertexBufferHandle handle;
                handle.idx = buffer->GetGPUObjectIdx();
                impl_->vertexBuffer_[i] = handle;
                bgfx::DynamicVertexBufferHandle nullHandle;
                nullHandle.idx = bgfx::kInvalidHandle;
                impl_->dynamicVertexBuffer_[i] = nullHandle;
            }
        }
        else
        {
            bgfx::DynamicVertexBufferHandle dhandle;
            dhandle.idx = bgfx::kInvalidHandle;
            impl_->dynamicVertexBuffer_[i] = dhandle;
            bgfx::VertexBufferHandle handle;
            handle.idx = bgfx::kInvalidHandle;
            impl_->vertexBuffer_[i] = handle;
        }
    }

    return true;
}

bool Graphics::SetVertexBuffers(const Vector<SharedPtr<VertexBuffer> >& buffers, unsigned instanceOffset)
{
    return SetVertexBuffers(reinterpret_cast<const PODVector<VertexBuffer*>&>(buffers), instanceOffset);
}

void Graphics::SetIndexBuffer(IndexBuffer* buffer)
{
    // Have to defer this as the draw call sets index start/end.
    if (buffer != indexBuffer_)
    {
        if (buffer)
        {
            if (buffer->IsDynamic())
            {
                bgfx::DynamicIndexBufferHandle handle;
                handle.idx = buffer->GetGPUObjectIdx();
                //bgfx::setIndexBuffer(handle);
                impl_->dynamicIndexBuffer_ = handle;
                bgfx::IndexBufferHandle nullHandle;
                nullHandle.idx = bgfx::kInvalidHandle;
                impl_->indexBuffer_ = nullHandle;
            }
            else
            {
                bgfx::IndexBufferHandle handle;
                handle.idx = buffer->GetGPUObjectIdx();
                //bgfx::setIndexBuffer(handle);
                impl_->indexBuffer_ = handle;
                bgfx::DynamicIndexBufferHandle nullHandle;
                nullHandle.idx = bgfx::kInvalidHandle;
                impl_->dynamicIndexBuffer_ = nullHandle;
            }
        }
        indexBuffer_ = buffer;
    }
}

void Graphics::SetShaders(ShaderVariation* vs, ShaderVariation* ps)
{
	if (vs == vertexShader_ && ps == pixelShader_)
		return;

	if (vs != vertexShader_)
	{
		// Create the shader now if not yet created. If already attempted, do not retry
		if (vs && vs->GetGPUObjectIdx() == bgfx::kInvalidHandle)
		{
			if (vs->GetCompilerOutput().Empty())
			{
				URHO3D_PROFILE(CompileVertexShader);

				bool success = vs->Create();
				if (!success)
				{
					URHO3D_LOGERROR("Failed to compile vertex shader " + vs->GetFullName() + ":\n" + vs->GetCompilerOutput());
					vs = nullptr;
				}
			}
			else
				vs = nullptr;
		}

		//impl_->deviceContext_->VSSetShader((ID3D11VertexShader*)(vs ? vs->GetGPUObject() : nullptr), nullptr, 0);
		vertexShader_ = vs;
		impl_->vertexDeclarationDirty_ = true;
	}

	if (ps != pixelShader_)
	{
		if (ps && ps->GetGPUObjectIdx() == bgfx::kInvalidHandle)
		{
			if (ps->GetCompilerOutput().Empty())
			{
				URHO3D_PROFILE(CompilePixelShader);

				bool success = ps->Create();
				if (!success)
				{
					URHO3D_LOGERROR("Failed to compile pixel shader " + ps->GetFullName() + ":\n" + ps->GetCompilerOutput());
					ps = nullptr;
				}
			}
			else
				ps = nullptr;
		}

		//impl_->deviceContext_->PSSetShader((ID3D11PixelShader*)(ps ? ps->GetGPUObject() : nullptr), nullptr, 0);
		pixelShader_ = ps;
	}

	// Update current shader parameters & constant buffers
	if (vertexShader_ && pixelShader_)
	{
		Pair<ShaderVariation*, ShaderVariation*> key = MakePair(vertexShader_, pixelShader_);
		ShaderProgramMap::Iterator i = impl_->shaderPrograms_.Find(key);
		if (i != impl_->shaderPrograms_.End())
			impl_->shaderProgram_ = i->second_.Get();
		else
		{
			ShaderProgram* newProgram = impl_->shaderPrograms_[key] = new ShaderProgram(this, vertexShader_, pixelShader_);
			impl_->shaderProgram_ = newProgram;
		}
	}
	else
		impl_->shaderProgram_ = nullptr;

	// Store shader combination if shader dumping in progress
	if (shaderPrecache_)
		shaderPrecache_->StoreShaders(vertexShader_, pixelShader_);

	// Update clip plane parameter if necessary
	if (useClipPlane_)
		SetShaderParameter(VSP_CLIPPLANE, clipPlane_);
}

void Graphics::SetShaderParameter(StringHash param, const float* data, unsigned count) 
{
	HashMap<StringHash, ShaderParameter>::Iterator i;
	if (!impl_->shaderProgram_ || (i = impl_->shaderProgram_->parameters_.Find(param)) == impl_->shaderProgram_->parameters_.End())
		return;

	bgfx::UniformHandle handle;
	handle.idx = i->second_.idx_;
    switch (i->second_.bgfxType_)
    {
	case bgfx::UniformType::Vec4:
		bgfx::setUniform(handle, data, (uint16_t)count / 4);
        break;

	case bgfx::UniformType::Mat3:
		bgfx::setUniform(handle, data, (uint16_t)count / 9);
        break;

	case bgfx::UniformType::Mat4:
		bgfx::setUniform(handle, data, (uint16_t)count / 16);
        break;

    default: break;
	}
}

void Graphics::SetShaderParameter(StringHash param, float value)
{
	Vector4 vector(value, 0.0, 0.0, 0.0);
	SetShaderParameter(param, vector);
}

void Graphics::SetShaderParameter(StringHash param, int value)
{
	Vector4 vector(value, 0.0, 0.0, 0.0);
	SetShaderParameter(param, vector);
}

void Graphics::SetShaderParameter(StringHash param, bool value)
{
	Vector4 vector(value, 0.0, 0.0, 0.0);
	SetShaderParameter(param, vector);
}

void Graphics::SetShaderParameter(StringHash param, const Color& color)
{
	HashMap<StringHash, ShaderParameter>::Iterator i;
	if (!impl_->shaderProgram_ || (i = impl_->shaderProgram_->parameters_.Find(param)) == impl_->shaderProgram_->parameters_.End())
		return;

	bgfx::UniformHandle handle;
	handle.idx = i->second_.idx_;
	bgfx::setUniform(handle, color.Data());
}

void Graphics::SetShaderParameter(StringHash param, const Vector2& vector)
{
	Vector4 vec4(vector.x_, vector.y_, 0.0, 0.0);
	SetShaderParameter(param, vec4);
}

void Graphics::SetShaderParameter(StringHash param, const Matrix3& matrix)
{
	HashMap<StringHash, ShaderParameter>::Iterator i;
	if (!impl_->shaderProgram_ || (i = impl_->shaderProgram_->parameters_.Find(param)) == impl_->shaderProgram_->parameters_.End())
		return;

	bgfx::UniformHandle handle;
	handle.idx = i->second_.idx_;
	bgfx::setUniform(handle, matrix.Data());
}

void Graphics::SetShaderParameter(StringHash param, const Vector3& vector)
{
	Vector4 vec4(vector.x_, vector.y_, vector.z_, 0.0);
	SetShaderParameter(param, vec4);
}

void Graphics::SetShaderParameter(StringHash param, const Matrix4& matrix)
{
	HashMap<StringHash, ShaderParameter>::Iterator i;
	if (!impl_->shaderProgram_ || (i = impl_->shaderProgram_->parameters_.Find(param)) == impl_->shaderProgram_->parameters_.End())
		return;

	bgfx::UniformHandle handle;
	handle.idx = i->second_.idx_;
	bgfx::setUniform(handle, matrix.Data());
}

void Graphics::SetShaderParameter(StringHash param, const Vector4& vector)
{
	HashMap<StringHash, ShaderParameter>::Iterator i;
	if (!impl_->shaderProgram_ || (i = impl_->shaderProgram_->parameters_.Find(param)) == impl_->shaderProgram_->parameters_.End())
		return;

	bgfx::UniformHandle handle;
	handle.idx = i->second_.idx_;
	bgfx::setUniform(handle, vector.Data());
}

void Graphics::SetShaderParameter(StringHash param, const Matrix3x4& matrix)
{
	HashMap<StringHash, ShaderParameter>::Iterator i;
	if (!impl_->shaderProgram_ || (i = impl_->shaderProgram_->parameters_.Find(param)) == impl_->shaderProgram_->parameters_.End())
		return;

	// Expand to a full Matrix4
	static Matrix4 fullMatrix;
	fullMatrix.m00_ = matrix.m00_;
	fullMatrix.m01_ = matrix.m01_;
	fullMatrix.m02_ = matrix.m02_;
	fullMatrix.m03_ = matrix.m03_;
	fullMatrix.m10_ = matrix.m10_;
	fullMatrix.m11_ = matrix.m11_;
	fullMatrix.m12_ = matrix.m12_;
	fullMatrix.m13_ = matrix.m13_;
	fullMatrix.m20_ = matrix.m20_;
	fullMatrix.m21_ = matrix.m21_;
	fullMatrix.m22_ = matrix.m22_;
	fullMatrix.m23_ = matrix.m23_;

	bgfx::UniformHandle handle;
	handle.idx = i->second_.idx_;
	bgfx::setUniform(handle, fullMatrix.Data());
}

bool Graphics::NeedParameterUpdate(ShaderParameterGroup group, const void* source) { return false; }

bool Graphics::HasShaderParameter(StringHash param)
{
    return impl_->shaderProgram_ && impl_->shaderProgram_->parameters_.Find(param) != impl_->shaderProgram_->parameters_.End();
}

bool Graphics::HasTextureUnit(TextureUnit unit)
{
    return pixelShader_ && pixelShader_->HasTextureUnit(unit);
}

void Graphics::ClearParameterSource(ShaderParameterGroup group)
{
    shaderParameterSources_[group] = (const void*)M_MAX_UNSIGNED;
}

void Graphics::ClearParameterSources()
{
    for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
        shaderParameterSources_[i] = (const void*)M_MAX_UNSIGNED;
}

void Graphics::ClearTransformSources()
{
    shaderParameterSources_[SP_CAMERA] = (const void*)M_MAX_UNSIGNED;
    shaderParameterSources_[SP_OBJECT] = (const void*)M_MAX_UNSIGNED;
}

void Graphics::SetTexture(unsigned index, Texture* texture)
{
    if (index >= MAX_TEXTURE_UNITS)
        return;

    // Check if texture is currently bound as a rendertarget. In that case, use its backup texture, or blank if not defined
    if (texture)
    {
        if (renderTargets_[0] && renderTargets_[0]->GetParentTexture() == texture)
            texture = texture->GetBackupTexture();
        else
        {
            // Resolve multisampled texture now as necessary
            if (texture->GetMultiSample() > 1 && texture->GetAutoResolve() && texture->IsResolveDirty())
            {
                if (texture->GetType() == Texture2D::GetTypeStatic())
                    ResolveToTexture(static_cast<Texture2D*>(texture));
                if (texture->GetType() == TextureCube::GetTypeStatic())
                    ResolveToTexture(static_cast<TextureCube*>(texture));
            }
        }

        if (texture->GetLevelsDirty())
            texture->RegenerateLevels();
    }

	unsigned flags = UINT32_MAX;
    if (texture && texture->GetParametersDirty())
    {
		flags = texture->GetBGFXFlags();
        textures_[index] = nullptr; // Force reassign
    }

	if (texture != textures_[index])
	{
		if (bgfx::isValid(impl_->shaderProgram_->texSamplers_[index]))
		{
			bgfx::TextureHandle texHandle;
			texHandle.idx = texture->GetGPUObjectIdx();
			bgfx::setTexture(index, impl_->shaderProgram_->texSamplers_[index], texHandle, flags);
		}
	}
}

void SetTextureForUpdate(Texture* texture)
{
    // No-op on BGFX
}

void Graphics::SetDefaultTextureFilterMode(TextureFilterMode mode)
{
    if (mode != defaultTextureFilterMode_)
    {
        defaultTextureFilterMode_ = mode;
        SetTextureParametersDirty();
    }
}

void Graphics::SetDefaultTextureAnisotropy(unsigned level)
{
    level = Max(level, 1U);

    if (level != defaultTextureAnisotropy_)
    {
        defaultTextureAnisotropy_ = level;
        SetTextureParametersDirty();
    }
}

void Graphics::SetTextureParametersDirty()
{
    MutexLock lock(gpuObjectMutex_);

    for (PODVector<GPUObject*>::Iterator i = gpuObjects_.Begin(); i != gpuObjects_.End(); ++i)
    {
        Texture* texture = dynamic_cast<Texture*>(*i);
        if (texture)
            texture->SetParametersDirty();
    }
}

void Graphics::ResetRenderTargets()
{
    for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
        SetRenderTarget(i, (RenderSurface*)nullptr);
    SetDepthStencil((RenderSurface*)nullptr);
    SetViewport(IntRect(0, 0, width_, height_));
}

void Graphics::ResetRenderTarget(unsigned index)
{
    SetRenderTarget(index, (RenderSurface*)nullptr);
}

void Graphics::ResetDepthStencil()
{
    SetDepthStencil((RenderSurface*)nullptr);
}

void Graphics::SetRenderTarget(unsigned index, RenderSurface* renderTarget)
{
    if (index >= MAX_RENDERTARGETS)
        return;

    if (renderTarget != renderTargets_[index])
    {
        renderTargets_[index] = renderTarget;
        impl_->renderTargetsDirty_ = true;

        // If the rendertarget is also bound as a texture, replace with backup texture or null
        if (renderTarget)
        {
            Texture* parentTexture = renderTarget->GetParentTexture();

            for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
            {
                if (textures_[i] == parentTexture)
                    SetTexture(i, textures_[i]->GetBackupTexture());
            }

            // If multisampled, mark the texture & surface needing resolve
            if (parentTexture->GetMultiSample() > 1 && parentTexture->GetAutoResolve())
            {
                parentTexture->SetResolveDirty(true);
                renderTarget->SetResolveDirty(true);
            }

            // If mipmapped, mark the levels needing regeneration
            if (parentTexture->GetLevels() > 1)
                parentTexture->SetLevelsDirty();
        }
    }
}

void Graphics::SetRenderTarget(unsigned index, Texture2D* texture)
{
    RenderSurface* renderTarget = nullptr;
    if (texture)
        renderTarget = texture->GetRenderSurface();

    SetRenderTarget(index, renderTarget);
}

void Graphics::SetDepthStencil(RenderSurface* depthStencil)
{
    if (depthStencil != depthStencil_)
    {
        depthStencil_ = depthStencil;
        impl_->renderTargetsDirty_ = true;
    }
}

void Graphics::SetDepthStencil(Texture2D* texture)
{
    RenderSurface* depthStencil = nullptr;
    if (texture)
        depthStencil = texture->GetRenderSurface();

    SetDepthStencil(depthStencil);
    // Constant depth bias depends on the bitdepth
    impl_->stateDirty_ = true;
}

void Graphics::SetViewport(const IntRect& rect)
{
    IntVector2 size = GetRenderTargetDimensions();

    IntRect rectCopy = rect;

    if (rectCopy.right_ <= rectCopy.left_)
        rectCopy.right_ = rectCopy.left_ + 1;
    if (rectCopy.bottom_ <= rectCopy.top_)
        rectCopy.bottom_ = rectCopy.top_ + 1;
    rectCopy.left_ = Clamp(rectCopy.left_, 0, size.x_);
    rectCopy.top_ = Clamp(rectCopy.top_, 0, size.y_);
    rectCopy.right_ = Clamp(rectCopy.right_, 0, size.x_);
    rectCopy.bottom_ = Clamp(rectCopy.bottom_, 0, size.y_);

    bgfx::setViewRect(impl_->view_, rectCopy.left_, rectCopy.top_, rectCopy.right_, rectCopy.bottom_);
    viewport_ = rectCopy;

    // Disable scissor test, needs to be re-enabled by the user
    SetScissorTest(false);
}

void Graphics::SetBlendMode(BlendMode mode, bool alphaToCoverage)
{
    if (mode != blendMode_ || alphaToCoverage != alphaToCoverage_)
    {
        blendMode_ = mode;
        alphaToCoverage_ = alphaToCoverage;
        impl_->stateDirty_ = true;
    }
}

void Graphics::SetColorWrite(bool enable)
{
    if (enable != colorWrite_)
    {
        colorWrite_ = enable;
        impl_->stateDirty_ = true;
    }
}

void Graphics::SetCullMode(CullMode mode)
{
    if (mode != cullMode_)
    {
        cullMode_ = mode;
        impl_->stateDirty_ = true;
    }
}

void Graphics::SetDepthBias(float constantBias, float slopeScaledBias)
{
    if (constantBias != constantDepthBias_ || slopeScaledBias != slopeScaledDepthBias_)
    {
        constantDepthBias_ = constantBias;
        slopeScaledDepthBias_ = slopeScaledBias;
        impl_->stateDirty_ = true;
    }
}

void Graphics::SetDepthTest(CompareMode mode)
{
    if (mode != depthTestMode_)
    {
        depthTestMode_ = mode;
        impl_->stateDirty_ = true;
    }
}

void Graphics::SetDepthWrite(bool enable)
{
    if (enable != depthWrite_)
    {
        depthWrite_ = enable;
        impl_->stateDirty_ = true;
        // Also affects whether a read-only version of depth-stencil should be bound, to allow sampling
        impl_->renderTargetsDirty_ = true;
    }
}

void Graphics::SetFillMode(FillMode mode)
{
    if (mode != fillMode_)
    {
        fillMode_ = mode;
        impl_->stateDirty_ = true;
    }
}

void Graphics::SetLineAntiAlias(bool enable)
{
    if (enable != lineAntiAlias_)
    {
        lineAntiAlias_ = enable;
        impl_->stateDirty_ = true;
    }
}

void Graphics::SetScissorTest(bool enable, const Rect& rect, bool borderInclusive)
{
    // During some light rendering loops, a full rect is toggled on/off repeatedly.
    // Disable scissor in that case to reduce state changes
    if (rect.min_.x_ <= 0.0f && rect.min_.y_ <= 0.0f && rect.max_.x_ >= 1.0f && rect.max_.y_ >= 1.0f)
        enable = false;

    if (enable)
    {
        IntVector2 rtSize(GetRenderTargetDimensions());
        IntVector2 viewSize(viewport_.Size());
        IntVector2 viewPos(viewport_.left_, viewport_.top_);
        IntRect intRect;
        int expand = borderInclusive ? 1 : 0;

        intRect.left_ = Clamp((int)((rect.min_.x_ + 1.0f) * 0.5f * viewSize.x_) + viewPos.x_, 0, rtSize.x_ - 1);
        intRect.top_ = Clamp((int)((-rect.max_.y_ + 1.0f) * 0.5f * viewSize.y_) + viewPos.y_, 0, rtSize.y_ - 1);
        intRect.right_ = Clamp((int)((rect.max_.x_ + 1.0f) * 0.5f * viewSize.x_) + viewPos.x_ + expand, 0, rtSize.x_);
        intRect.bottom_ = Clamp((int)((-rect.min_.y_ + 1.0f) * 0.5f * viewSize.y_) + viewPos.y_ + expand, 0, rtSize.y_);

        if (intRect.right_ == intRect.left_)
            intRect.right_++;
        if (intRect.bottom_ == intRect.top_)
            intRect.bottom_++;

        if (intRect.right_ < intRect.left_ || intRect.bottom_ < intRect.top_)
            enable = false;

        if (enable && intRect != scissorRect_)
        {
            scissorRect_ = intRect;
            impl_->scissorRectDirty_ = true;
        }
    }

    if (enable != scissorTest_)
    {
        scissorTest_ = enable;
        impl_->stateDirty_ = true;
    }
}

void Graphics::SetScissorTest(bool enable, const IntRect& rect)
{
    IntVector2 rtSize(GetRenderTargetDimensions());
    IntVector2 viewPos(viewport_.left_, viewport_.top_);

    if (enable)
    {
        IntRect intRect;
        intRect.left_ = Clamp(rect.left_ + viewPos.x_, 0, rtSize.x_ - 1);
        intRect.top_ = Clamp(rect.top_ + viewPos.y_, 0, rtSize.y_ - 1);
        intRect.right_ = Clamp(rect.right_ + viewPos.x_, 0, rtSize.x_);
        intRect.bottom_ = Clamp(rect.bottom_ + viewPos.y_, 0, rtSize.y_);

        if (intRect.right_ == intRect.left_)
            intRect.right_++;
        if (intRect.bottom_ == intRect.top_)
            intRect.bottom_++;

        if (intRect.right_ < intRect.left_ || intRect.bottom_ < intRect.top_)
            enable = false;

        if (enable && intRect != scissorRect_)
        {
            scissorRect_ = intRect;
            impl_->scissorRectDirty_ = true;
        }
    }

    if (enable != scissorTest_)
    {
        scissorTest_ = enable;
        impl_->stateDirty_ = true;
    }
}

void Graphics::SetStencilTest(bool enable, CompareMode mode, StencilOp pass, StencilOp fail, StencilOp zFail,
    unsigned stencilRef, unsigned compareMask, unsigned writeMask)
{
    if (enable != stencilTest_)
    {
        stencilTest_ = enable;
        impl_->stateDirty_ = true;
    }

    if (enable)
    {
        if (mode != stencilTestMode_)
        {
            stencilTestMode_ = mode;
            impl_->stateDirty_ = true;
        }
        if (pass != stencilPass_)
        {
            stencilPass_ = pass;
            impl_->stateDirty_ = true;
        }
        if (fail != stencilFail_)
        {
            stencilFail_ = fail;
            impl_->stateDirty_ = true;
        }
        if (zFail != stencilZFail_)
        {
            stencilZFail_ = zFail;
            impl_->stateDirty_ = true;
        }
        if (compareMask != stencilCompareMask_)
        {
            stencilCompareMask_ = compareMask;
            impl_->stateDirty_ = true;
        }
        if (writeMask != stencilWriteMask_)
        {
            stencilWriteMask_ = writeMask;
            impl_->stateDirty_ = true;
        }
        if (stencilRef != stencilRef_)
        {
            stencilRef_ = stencilRef;
            impl_->stencilRefDirty_ = true;
            impl_->stateDirty_ = true;
        }
    }
}

void Graphics::SetClipPlane(bool enable, const Plane& clipPlane, const Matrix3x4& view, const Matrix4& projection)
{
    useClipPlane_ = enable;

    if (enable)
    {
        Matrix4 viewProj = projection * view;
        clipPlane_ = clipPlane.Transformed(viewProj).ToVector4();
        SetShaderParameter(VSP_CLIPPLANE, clipPlane_);
    }
}

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
    switch (format)
    {
    case CF_RGBA:
        return bgfx::TextureFormat::RGBA8;
    case CF_DXT1:
        return dxtTextureSupport_ ? bgfx::TextureFormat::BC1 : 0;
    case CF_DXT3:
        return dxtTextureSupport_ ? bgfx::TextureFormat::BC2 : 0;
    case CF_DXT5:
        return dxtTextureSupport_ ? bgfx::TextureFormat::BC3 : 0;
    case CF_ETC1:
        return etcTextureSupport_ ? bgfx::TextureFormat::ETC1 : 0;
    case CF_PVRTC_RGB_2BPP:
        return pvrtcTextureSupport_ ? bgfx::TextureFormat::PTC12 : 0;
    case CF_PVRTC_RGB_4BPP:
        return pvrtcTextureSupport_ ? bgfx::TextureFormat::PTC14 : 0;
    case CF_PVRTC_RGBA_2BPP:
        return pvrtcTextureSupport_ ? bgfx::TextureFormat::PTC12A : 0;
    case CF_PVRTC_RGBA_4BPP:
        return pvrtcTextureSupport_ ? bgfx::TextureFormat::PTC14A : 0;
    default:
        return 0;
    }
}

ShaderVariation* Graphics::GetShader(ShaderType type, const String& name, const String& defines) const
{
    return GetShader(type, name.CString(), defines.CString());
}

ShaderVariation* Graphics::GetShader(ShaderType type, const char* name, const char* defines) const
{
    if (lastShaderName_ != name || !lastShader_)
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();

        String fullShaderName = shaderPath_ + name + shaderExtension_;
        // Try to reduce repeated error log prints because of missing shaders
        if (lastShaderName_ == name && !cache->Exists(fullShaderName))
            return nullptr;

        lastShader_ = cache->GetResource<Shader>(fullShaderName);
        lastShaderName_ = name;
    }

    return lastShader_ ? lastShader_->GetVariation(type, defines) : nullptr;
}

VertexBuffer* Graphics::GetVertexBuffer(unsigned index) const
{
    return index < MAX_VERTEX_STREAMS ? vertexBuffers_[index] : nullptr;
}

ShaderProgram* Graphics::GetShaderProgram() const
{
    return nullptr;
}

TextureUnit Graphics::GetTextureUnit(const String& name)
{
    HashMap<String, TextureUnit>::Iterator i = textureUnits_.Find(name);
    if (i != textureUnits_.End())
        return i->second_;
    else
        return MAX_TEXTURE_UNITS;
}

const String& Graphics::GetTextureUnitName(TextureUnit unit)
{
    for (HashMap<String, TextureUnit>::Iterator i = textureUnits_.Begin(); i != textureUnits_.End(); ++i)
    {
        if (i->second_ == unit)
            return i->first_;
    }
    return String::EMPTY;
}

Texture* Graphics::GetTexture(unsigned index) const
{
    return index < MAX_TEXTURE_UNITS ? textures_[index] : nullptr;
}

RenderSurface* Graphics::GetRenderTarget(unsigned index) const
{
    return index < MAX_RENDERTARGETS ? renderTargets_[index] : nullptr;
}

IntVector2 Graphics::GetRenderTargetDimensions() const
{
    int width, height;

    if (renderTargets_[0])
    {
        width = renderTargets_[0]->GetWidth();
        height = renderTargets_[0]->GetHeight();
    }
    else if (depthStencil_) // Depth-only rendering
    {
        width = depthStencil_->GetWidth();
        height = depthStencil_->GetHeight();
    }
    else
    {
        width = width_;
        height = height_;
    }

    return IntVector2(width, height);
}

void Graphics::OnWindowResized()
{
    //if (!impl_->device_ || !window_)
    if (!window_)
        return;

    int newWidth, newHeight;

    SDL_GetWindowSize(window_, &newWidth, &newHeight);
    if (newWidth == width_ && newHeight == height_)
        return;

    //UpdateSwapChain(newWidth, newHeight);

    // Reset rendertargets and viewport for the new screen size
    ResetRenderTargets();

    //URHO3D_LOGDEBUGF("Window was resized to %dx%d", width_, height_);

    using namespace ScreenMode;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_WIDTH] = width_;
    eventData[P_HEIGHT] = height_;
    eventData[P_FULLSCREEN] = fullscreen_;
    eventData[P_RESIZABLE] = resizable_;
    eventData[P_BORDERLESS] = borderless_;
    eventData[P_HIGHDPI] = highDPI_;
    SendEvent(E_SCREENMODE, eventData);
}

void Graphics::OnWindowMoved()
{
    //if (!impl_->device_ || !window_ || fullscreen_)
    if (!window_ || fullscreen_)
        return;

    int newX, newY;

    SDL_GetWindowPosition(window_, &newX, &newY);
    if (newX == position_.x_ && newY == position_.y_)
        return;

    position_.x_ = newX;
    position_.y_ = newY;

    //URHO3D_LOGDEBUGF("Window was moved to %d,%d", position_.x_, position_.y_);

    using namespace WindowPos;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_X] = position_.x_;
    eventData[P_Y] = position_.y_;
    SendEvent(E_WINDOWPOS, eventData);
}

void Graphics::Restore() {}

void Graphics::CleanupShaderPrograms(ShaderVariation* variation)
{
    for (ShaderProgramMap::Iterator i = impl_->shaderPrograms_.Begin(); i != impl_->shaderPrograms_.End();)
    {
        if (i->first_.first_ == variation || i->first_.second_ == variation)
        {
            i = impl_->shaderPrograms_.Erase(i);
        }
        else
            ++i;
    }

    if (vertexShader_ == variation || pixelShader_ == variation)
        impl_->shaderProgram_ = nullptr;
}

void Graphics::CleanupRenderSurface(RenderSurface* surface)
{
    if (!surface)
        return;

    uint16_t texIdx = surface->GetParentTexture()->GetGPUObjectIdx();
    for (unsigned i = 0; i < impl_->frameBuffers_.Size(); ++i)
    {
        bool markForDeletion = false;
        for (unsigned j = 0; j < MAX_RENDERTARGETS + 1; ++j)
        {
            if (bgfx::getTexture(impl_->frameBuffers_[i], j).idx == texIdx)
                markForDeletion = true;
        }
        if (markForDeletion)
        {
            if (impl_->frameBuffers_[i].idx == impl_->currentFramebuffer_.idx)
            {
                bgfx::FrameBufferHandle handle;
                handle.idx = bgfx::kInvalidHandle;
                impl_->currentFramebuffer_ = handle;
            }
            bgfx::destroy(impl_->frameBuffers_[i]);
            impl_->frameBuffers_.Erase(i);
        }
    }
}

ConstantBuffer* Graphics::GetOrCreateConstantBuffer(ShaderType type, unsigned index, unsigned size)
{
    return nullptr;
}

void Graphics::MarkFBODirty() {}
void Graphics::SetVBO(unsigned object) {}
void Graphics::SetUBO(unsigned object) {}

unsigned Graphics::GetAlphaFormat() { return bgfx::TextureFormat::A8; }
unsigned Graphics::GetLuminanceFormat() { return bgfx::TextureFormat::R8; }
unsigned Graphics::GetLuminanceAlphaFormat() { return bgfx::TextureFormat::RG8; }
unsigned Graphics::GetRGBFormat() { return bgfx::TextureFormat::RGB8; }
unsigned Graphics::GetRGBAFormat() { return bgfx::TextureFormat::RGBA8; }
unsigned Graphics::GetRGBA16Format() { return bgfx::TextureFormat::RGBA16; }
unsigned Graphics::GetRGBAFloat16Format() { return bgfx::TextureFormat::RGBA16F; }
unsigned Graphics::GetRGBAFloat32Format() { return bgfx::TextureFormat::RGBA32F; }
unsigned Graphics::GetRG16Format() { return bgfx::TextureFormat::RG16; }
unsigned Graphics::GetRGFloat16Format() { return bgfx::TextureFormat::RG16F; }
unsigned Graphics::GetRGFloat32Format() { return bgfx::TextureFormat::RG32F; }
unsigned Graphics::GetFloat16Format() { return bgfx::TextureFormat::R16F; }
unsigned Graphics::GetFloat32Format() { return bgfx::TextureFormat::R32F; }
unsigned Graphics::GetLinearDepthFormat() { return bgfx::TextureFormat::D32; }
unsigned Graphics::GetDepthStencilFormat() { return bgfx::TextureFormat::D24S8; }
unsigned Graphics::GetReadableDepthFormat() { return bgfx::TextureFormat::D24S8; }

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

unsigned Graphics::GetMaxBones()
{ 
#if defined(RPI)
    // At the moment all RPI GPUs are low powered and only have limited number of uniforms
    return 32;
#elseif defined(URHO3D_MOBILE)
    return 64;
#else
    return 128;
#endif
}

bool Graphics::GetGL3Support()
{
    //gl3Support;
    return true;
}

bool Graphics::OpenWindow(int width, int height, bool resizable, bool borderless)
{
    if (!externalWindow_)
    {
        unsigned flags = 0;
        if (resizable)
            flags |= SDL_WINDOW_RESIZABLE;
        if (borderless)
            flags |= SDL_WINDOW_BORDERLESS;

        window_ = SDL_CreateWindow(windowTitle_.CString(), position_.x_, position_.y_, width, height, flags);
    }
    else
        window_ = SDL_CreateWindowFrom(externalWindow_, 0);

    if (!window_)
    {
        //URHO3D_LOGERRORF("Could not create window, root cause: '%s'", SDL_GetError());
        return false;
    }

    SDL_GetWindowPosition(window_, &position_.x_, &position_.y_);

    CreateWindowIcon();

    return true;
}

void Graphics::AdjustWindow(int& newWidth, int& newHeight, bool& newFullscreen, bool& newBorderless, int& monitor)
{
    if (!externalWindow_)
    {
        if (!newWidth || !newHeight)
        {
            SDL_MaximizeWindow(window_);
            SDL_GetWindowSize(window_, &newWidth, &newHeight);
        }
        else
        {
            SDL_Rect display_rect;
            SDL_GetDisplayBounds(monitor, &display_rect);

            if (newFullscreen || (newBorderless && newWidth >= display_rect.w && newHeight >= display_rect.h))
            {
                // Reposition the window on the specified monitor if it's supposed to cover the entire monitor
                SDL_SetWindowPosition(window_, display_rect.x, display_rect.y);
            }

            SDL_SetWindowSize(window_, newWidth, newHeight);
        }

        // Hack fix: on SDL 2.0.4 a fullscreen->windowed transition results in a maximized window when the D3D device is reset, so hide before
        SDL_HideWindow(window_);
        SDL_SetWindowFullscreen(window_, newFullscreen ? SDL_WINDOW_FULLSCREEN : 0);
        SDL_SetWindowBordered(window_, newBorderless ? SDL_FALSE : SDL_TRUE);
        SDL_ShowWindow(window_);
    }
    else
    {
        // If external window, must ask its dimensions instead of trying to set them
        SDL_GetWindowSize(window_, &newWidth, &newHeight);
        newFullscreen = false;
    }
}

void Graphics::CheckFeatureSupport()
{
    const bgfx::Caps* caps = bgfx::getCaps();
    anisotropySupport_ = true;
    dxtTextureSupport_ = 0 != (BGFX_CAPS_FORMAT_TEXTURE_2D & caps->formats[bgfx::TextureFormat::BC1]);
    etcTextureSupport_ = 0 != (BGFX_CAPS_FORMAT_TEXTURE_2D & caps->formats[bgfx::TextureFormat::ETC1]);
    pvrtcTextureSupport_ = 0 != (BGFX_CAPS_FORMAT_TEXTURE_2D & caps->formats[bgfx::TextureFormat::PTC12]);
    lightPrepassSupport_ = true;
    deferredSupport_ = true;
    hardwareShadowSupport_ = true;
    instancingSupport_ = (caps->supported & BGFX_CAPS_INSTANCING);
    shadowMapFormat_ = bgfx::TextureFormat::D16;
    hiresShadowMapFormat_ = bgfx::TextureFormat::D32;
    dummyColorFormat_ = bgfx::TextureFormat::Unknown;
    sRGBSupport_ = 0 != (BGFX_CAPS_FORMAT_TEXTURE_2D & caps->formats[bgfx::TextureFormat::RGBA8]);
    sRGBWriteSupport_ = 0 != (BGFX_CAPS_FORMAT_TEXTURE_2D & caps->formats[bgfx::TextureFormat::RGBA8]); // Not correct
}

void Graphics::CleanupFramebuffers()
{
    for (unsigned i = 0; i < impl_->frameBuffers_.Size(); ++i)
        bgfx::destroy(impl_->frameBuffers_[i]);

    impl_->frameBuffers_.Clear();
}

void Graphics::ResetCachedState()
{
    bgfx::IndexBufferHandle ibh;
    ibh.idx = bgfx::kInvalidHandle;
    impl_->indexBuffer_ = ibh;
    bgfx::DynamicIndexBufferHandle dibh;
    dibh.idx = bgfx::kInvalidHandle;
    impl_->dynamicIndexBuffer_ = dibh;

    bgfx::VertexBufferHandle vbh;
    vbh.idx = bgfx::kInvalidHandle;
    bgfx::DynamicVertexBufferHandle dvbh;
    dvbh.idx = bgfx::kInvalidHandle;
    for (unsigned i = 0; i < MAX_VERTEX_STREAMS; ++i)
    {
        vertexBuffers_[i] = nullptr;
        impl_->vertexBuffer_[i] = vbh;
        impl_->dynamicVertexBuffer_[i] = dvbh;
    }

    for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
        textures_[i] = nullptr;

    for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
    {
        renderTargets_[i] = nullptr;
    //    impl_->renderTargetViews_[i] = nullptr;
    }

    //for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
    //{
    //    impl_->constantBuffers_[VS][i] = nullptr;
    //    impl_->constantBuffers_[PS][i] = nullptr;
    //}

    depthStencil_ = nullptr;
    //impl_->depthStencilView_ = nullptr;
    viewport_ = IntRect(0, 0, width_, height_);

    indexBuffer_ = nullptr;
    vertexDeclarationHash_ = 0;
    primitiveType_ = 0;
    vertexShader_ = nullptr;
    pixelShader_ = nullptr;
    blendMode_ = BLEND_REPLACE;
    alphaToCoverage_ = false;
    colorWrite_ = true;
    cullMode_ = CULL_CCW;
    constantDepthBias_ = 0.0f;
    slopeScaledDepthBias_ = 0.0f;
    depthTestMode_ = CMP_LESSEQUAL;
    depthWrite_ = true;
    fillMode_ = FILL_SOLID;
    lineAntiAlias_ = false;
    scissorTest_ = false;
    scissorRect_ = IntRect::ZERO;
    stencilTest_ = false;
    stencilTestMode_ = CMP_ALWAYS;
    stencilPass_ = OP_KEEP;
    stencilFail_ = OP_KEEP;
    stencilZFail_ = OP_KEEP;
    stencilRef_ = 0;
    stencilCompareMask_ = M_MAX_UNSIGNED;
    stencilWriteMask_ = M_MAX_UNSIGNED;
    useClipPlane_ = false;
    impl_->shaderProgram_ = nullptr;
    impl_->view_ = 0;
    impl_->renderTargetsDirty_ = true;
    //impl_->texturesDirty_ = true;
    impl_->vertexDeclarationDirty_ = true;
    impl_->stateDirty_ = true;
    impl_->scissorRectDirty_ = true;
    impl_->stencilRefDirty_ = true;
    impl_->primitiveType_ = 0;
    //impl_->blendStateHash_ = M_MAX_UNSIGNED;
    //impl_->depthStateHash_ = M_MAX_UNSIGNED;
    //impl_->rasterizerStateHash_ = M_MAX_UNSIGNED;
    //impl_->firstDirtyTexture_ = impl_->lastDirtyTexture_ = M_MAX_UNSIGNED;
    //impl_->firstDirtyVB_ = impl_->lastDirtyVB_ = M_MAX_UNSIGNED;
    //impl_->dirtyConstantBuffers_.Clear();
}

void Graphics::PrepareDraw()
{
    uint64_t stateFlags = 0;
    uint32_t stencilFlags = 0;

    // Early-out if it's the backbuffer & not to be confused with a depth texture
    if ((impl_->renderTargetsDirty_) && 
        (!renderTargets_[0]) && 
        depthStencil_ && depthStencil_->GetUsage() != TEXTURE_DEPTHSTENCIL)
    {
        bgfx::setViewFrameBuffer(impl_->view_, BGFX_INVALID_HANDLE);
        impl_->renderTargetsDirty_ = false;
    }

    if (impl_->renderTargetsDirty_)
    {
        uint8_t texAttachments = 0;
        bgfx::Attachment attachments[MAX_RENDERTARGETS+1]; // all attachments + depth stencil
        for (unsigned i = 0; i < MAX_RENDERTARGETS; ++i)
        {
            attachments[i].handle.idx = bgfx::kInvalidHandle;
            if (renderTargets_[i])
            {
                attachments[i].handle.idx = renderTargets_[i]->GetParentTexture()->GetGPUObjectIdx();
                attachments[i].mip = 0;
                attachments[i].layer = renderTargets_[i]->GetBgfxLayer();
                ++texAttachments;
            }
        }
        if (depthStencil_)
        {
            // Get the last attachment, this will be depth+stencil
            attachments[texAttachments].handle.idx = depthStencil_->GetParentTexture()->GetGPUObjectIdx();
            attachments[texAttachments].mip = 0;
            attachments[texAttachments].layer = 0;
            ++texAttachments;
        }

        // Now lets find an existing framebuffer handle, and if there isn't, create one
        bgfx::FrameBufferHandle fbHandle;
        fbHandle.idx = bgfx::kInvalidHandle;
        for (unsigned i = 0; i < impl_->frameBuffers_.Size(); ++i)
        {
            bgfx::TextureHandle matchHandles[MAX_RENDERTARGETS+1];
            for (unsigned j = 0; j < MAX_RENDERTARGETS+1; ++j)
                matchHandles[j] = bgfx::getTexture(impl_->frameBuffers_[i], j);

            if ((attachments[0].handle.idx == matchHandles[0].idx) &&
                (attachments[1].handle.idx == matchHandles[1].idx) &&
                (attachments[2].handle.idx == matchHandles[2].idx) &&
                (attachments[3].handle.idx == matchHandles[3].idx) &&
                (attachments[4].handle.idx == matchHandles[4].idx))
            {
                fbHandle = impl_->frameBuffers_[i];
                break;
            }
        }
        if (!bgfx::isValid(fbHandle))
        {
            fbHandle = bgfx::createFrameBuffer(texAttachments, attachments);
            impl_->frameBuffers_.Push(fbHandle);
        }
        bgfx::setViewFrameBuffer(impl_->view_, fbHandle);

        impl_->renderTargetsDirty_ = false;
    }

    if (impl_->vertexDeclarationDirty_ && vertexShader_ && vertexShader_->GetByteCode().Size())
    {
        impl_->vertexDeclarationDirty_ = false;
    }

    if (impl_->stateDirty_)
    {
        // Writes
        if (colorWrite_)
            stateFlags |= BGFX_STATE_RGB_WRITE | BGFX_STATE_ALPHA_WRITE;
        if (depthWrite_)
            stateFlags |= BGFX_STATE_DEPTH_WRITE;
        // Blend state
        stateFlags |= bgfxBlendState[blendMode_];
        stateFlags |= alphaToCoverage_ ? BGFX_STATE_BLEND_ALPHA_TO_COVERAGE : 0;
        // Cull mode
        stateFlags |= bgfxCullMode[cullMode_];
        // Depth/stencil state
        stateFlags |= bgfxDepthCompare[depthTestMode_];
        if (stencilTest_)
        {
            stencilFlags |= bgfxStencilCompare[stencilTestMode_];
            stencilFlags |= bgfxStencilFail[stencilFail_];
            stencilFlags |= bgfxStencilZFail[stencilZFail_];
            stencilFlags |= bgfxStencilPass[stencilPass_];
            bgfx::setStencil(stencilFlags, BGFX_STENCIL_NONE);
        }
        // Rasterizer state
        stateFlags |= primitiveType_;
        bgfx::setState(stateFlags);
        impl_->stateDirty_ = false;
    }

    if (impl_->scissorRectDirty_)
    {
        impl_->scissorRectDirty_;
    }

    //for (unsigned i = 0; i < impl_->dirtyConstantBuffers_.Size(); ++i)
    //    impl_->dirtyConstantBuffers_[i]->Apply();
    //impl_->dirtyConstantBuffers_.Clear();
}

void Graphics::SetTextureUnitMappings()
{
    textureUnits_["DiffMap"] = TU_DIFFUSE;
    textureUnits_["DiffCubeMap"] = TU_DIFFUSE;
    textureUnits_["NormalMap"] = TU_NORMAL;
    textureUnits_["SpecMap"] = TU_SPECULAR;
    textureUnits_["EmissiveMap"] = TU_EMISSIVE;
    textureUnits_["EnvMap"] = TU_ENVIRONMENT;
    textureUnits_["EnvCubeMap"] = TU_ENVIRONMENT;
    textureUnits_["LightRampMap"] = TU_LIGHTRAMP;
    textureUnits_["LightSpotMap"] = TU_LIGHTSHAPE;
    textureUnits_["LightCubeMap"] = TU_LIGHTSHAPE;
    textureUnits_["ShadowMap"] = TU_SHADOWMAP;
    textureUnits_["FaceSelectCubeMap"] = TU_FACESELECT;
    textureUnits_["IndirectionCubeMap"] = TU_INDIRECTION;
    textureUnits_["VolumeMap"] = TU_VOLUMEMAP;
    textureUnits_["ZoneCubeMap"] = TU_ZONE;
    textureUnits_["ZoneVolumeMap"] = TU_ZONE;
}

}
