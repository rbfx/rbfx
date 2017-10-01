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

namespace Urho3D
{

	const Vector2 Graphics::pixelUVOffset(0.f, 0.f);

	/// Construct.
	Graphics::Graphics(Context* context)
		: Object(context),
		window_(0),
		externalWindow_(0),
		width_(0),
		height_(0),
		position_(0, 0),
		multiSample_(1),
		fullscreen_(false),
		borderless_(false),
		resizable_(false),
		highDPI_(false),
		vsync_(false),
		monitor_(0),
		refreshRate_(0),
		tripleBuffer_(false),
		flushGPU_(false),
		sRGB_(false),
		anisotropySupport_(false),
		dxtTextureSupport_(false),
		etcTextureSupport_(false),
		pvrtcTextureSupport_(false),
		hardwareShadowSupport_(false),
		lightPrepassSupport_(false),
		deferredSupport_(false),
		instancingSupport_(false),
		sRGBSupport_(false),
		sRGBWriteSupport_(false),
		numPrimitives_(0),
		numBatches_(0),
		maxScratchBufferRequest_(0),
		defaultTextureFilterMode_(FILTER_TRILINEAR),
		defaultTextureAnisotropy_(4),
		orientations_("LandscapeLeft LandscapeRight"),
		apiName_("Noop")
	{
		RegisterGraphicsLibrary(context_);
	}
	/// Destruct. Release the Direct3D11 device and close the window.
	Graphics::~Graphics() {}

	/// Set screen mode. Return true if successful.
	bool Graphics::SetMode(
		int width, 
		int height, 
		bool fullscreen, 
		bool borderless, 
		bool resizable, 
		bool highDPI, 
		bool vsync, 
		bool tripleBuffer,
		int multiSample, 
		int monitor, 
		int refreshRate) 
	{
		if (!width || !height)
		{
			width = 1024;
			height = 768;
		}

		width_ = width;
		height_ = height;
		fullscreen_ = fullscreen;
		borderless_ = borderless;
		resizable_ = resizable;
		highDPI_ = highDPI;
		vsync_ = vsync;
		tripleBuffer_ = tripleBuffer;
		multiSample_ = multiSample;
		monitor_ = monitor;
		refreshRate_ = refreshRate;

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
	/// Set screen resolution only. Return true if successful.
	bool Graphics::SetMode(int width, int height) { return true; }
	/// Set whether the main window uses sRGB conversion on write.
	void Graphics::SetSRGB(bool enable) {}
	/// Set whether rendering output is dithered. Default true on OpenGL. No effect on Direct3D.
	void Graphics::SetDither(bool enable) {}
	/// Set whether to flush the GPU command buffer to prevent multiple frames being queued and uneven frame timesteps. Default off, may decrease performance if enabled. Not currently implemented on OpenGL.
	void Graphics::SetFlushGPU(bool enable) {}
	/// Set forced use of OpenGL 2 even if OpenGL 3 is available. Must be called before setting the screen mode for the first time. Default false. No effect on Direct3D9 & 11.
	void Graphics::SetForceGL2(bool enable) {}
	/// Close the window.
	void Graphics::Close() {}
	/// Take a screenshot. Return true if successful.
	bool Graphics::TakeScreenShot(Image& destImage) { return false; }
	/// Begin frame rendering. Return true if device available and can render.
	bool Graphics::BeginFrame() { return true; }
	/// End frame rendering and swap buffers.
	void Graphics::EndFrame() {}
	/// Clear any or all of rendertarget, depth buffer and stencil buffer.
	void Graphics::Clear(unsigned flags, const Color& color, float depth, unsigned stencil) {}
	/// Resolve multisampled backbuffer to a texture rendertarget. The texture's size should match the viewport size.
	bool Graphics::ResolveToTexture(Texture2D* destination, const IntRect& viewport) { return true; }
	/// Resolve a multisampled texture on itself.
	bool Graphics::ResolveToTexture(Texture2D* texture) { return true; }
	/// Resolve a multisampled cube texture on itself.
	bool Graphics::ResolveToTexture(TextureCube* texture) { return true; }
	/// Draw non-indexed geometry.
	void Graphics::Draw(PrimitiveType type, unsigned vertexStart, unsigned vertexCount) {}
	/// Draw indexed geometry.
	void Graphics::Draw(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned minVertex, unsigned vertexCount) {}
	/// Draw indexed geometry with vertex index offset.
	void Graphics::Draw(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned baseVertexIndex, unsigned minVertex, unsigned vertexCount) {}
	/// Draw indexed, instanced geometry. An instancing vertex buffer must be set.
	void Graphics::DrawInstanced(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned minVertex, unsigned vertexCount,
		unsigned instanceCount) {}
	/// Draw indexed, instanced geometry with vertex index offset.
	void Graphics::DrawInstanced(PrimitiveType type, unsigned indexStart, unsigned indexCount, unsigned baseVertexIndex, unsigned minVertex,
		unsigned vertexCount, unsigned instanceCount) {}
	/// Set vertex buffer.
	void Graphics::SetVertexBuffer(VertexBuffer* buffer) {}
	/// Set multiple vertex buffers.
	bool Graphics::SetVertexBuffers(const PODVector<VertexBuffer*>& buffers, unsigned instanceOffset) { return true; }
	/// Set multiple vertex buffers.
	bool Graphics::SetVertexBuffers(const Vector<SharedPtr<VertexBuffer> >& buffers, unsigned instanceOffset) { return true; }
	/// Set index buffer.
	void Graphics::SetIndexBuffer(IndexBuffer* buffer) {}
	/// Set shaders.
	void Graphics::SetShaders(ShaderVariation* vs, ShaderVariation* ps) {}
	/// Set shader float constants.
	void Graphics::SetShaderParameter(StringHash param, const float* data, unsigned count) {}
	/// Set shader float constant.
	void Graphics::SetShaderParameter(StringHash param, float value) {}
	/// Set shader integer constant.
	void Graphics::SetShaderParameter(StringHash param, int value) {}
	/// Set shader boolean constant.
	void Graphics::SetShaderParameter(StringHash param, bool value) {}
	/// Set shader color constant.
	void Graphics::SetShaderParameter(StringHash param, const Color& color) {}
	/// Set shader 2D vector constant.
	void Graphics::SetShaderParameter(StringHash param, const Vector2& vector) {}
	/// Set shader 3x3 matrix constant.
	void Graphics::SetShaderParameter(StringHash param, const Matrix3& matrix) {}
	/// Set shader 3D vector constant.
	void Graphics::SetShaderParameter(StringHash param, const Vector3& vector) {}
	/// Set shader 4x4 matrix constant.
	void Graphics::SetShaderParameter(StringHash param, const Matrix4& matrix) {}
	/// Set shader 4D vector constant.
	void Graphics::SetShaderParameter(StringHash param, const Vector4& vector) {}
	/// Set shader 3x4 matrix constant.
	void Graphics::SetShaderParameter(StringHash param, const Matrix3x4& matrix) {}
	/// Check whether a shader parameter group needs update. Does not actually check whether parameters exist in the shaders.
	bool Graphics::NeedParameterUpdate(ShaderParameterGroup group, const void* source) { return false; }
	/// Check whether a shader parameter exists on the currently set shaders.
	bool Graphics::HasShaderParameter(StringHash param) { return true; }
	/// Check whether the current vertex or pixel shader uses a texture unit.
	bool Graphics::HasTextureUnit(TextureUnit unit) { return true; }
	/// Clear remembered shader parameter source group.
	void Graphics::ClearParameterSource(ShaderParameterGroup group) {}
	/// Clear remembered shader parameter sources.
	void Graphics::ClearParameterSources() {}
	/// Clear remembered transform shader parameter sources.
	void Graphics::ClearTransformSources() {}
	/// Set texture.
	void Graphics::SetTexture(unsigned index, Texture* texture) {}
	/// Bind texture unit 0 for update. Called by Texture. Used only on OpenGL.
	void Graphics::SetTextureForUpdate(Texture* texture) {}
	/// Dirty texture parameters of all textures (when global settings change.)
	void Graphics::SetTextureParametersDirty() {}
	/// Set default texture filtering mode. Called by Renderer before rendering.
	void Graphics::SetDefaultTextureFilterMode(TextureFilterMode mode) {}
	/// Set default texture anisotropy level. Called by Renderer before rendering.
	void Graphics::SetDefaultTextureAnisotropy(unsigned level) {}
	/// Reset all rendertargets, depth-stencil surface and viewport.
	void Graphics::ResetRenderTargets() {}
	/// Reset specific rendertarget.
	void Graphics::ResetRenderTarget(unsigned index) {}
	/// Reset depth-stencil surface.
	void Graphics::ResetDepthStencil() {}
	/// Set rendertarget.
	void Graphics::SetRenderTarget(unsigned index, RenderSurface* renderTarget) 
	{
	}
	/// Set rendertarget.
	void Graphics::SetRenderTarget(unsigned index, Texture2D* texture)
	{

	}
	/// Set depth-stencil surface.
	void Graphics::SetDepthStencil(RenderSurface* depthStencil)
	{

	}
	/// Set depth-stencil surface.
	void Graphics::SetDepthStencil(Texture2D* texture)
	{

	}
	/// Set viewport.
	void Graphics::SetViewport(const IntRect& rect)
	{

	}
	/// Set blending and alpha-to-coverage modes. Alpha-to-coverage is not supported on Direct3D9.
	void Graphics::SetBlendMode(BlendMode mode, bool alphaToCoverage)
	{

	}
	/// Set color write on/off.
	void Graphics::SetColorWrite(bool enable)
	{

	}
	/// Set hardware culling mode.
	void Graphics::SetCullMode(CullMode mode)
	{

	}
	/// Set depth bias.
	void Graphics::SetDepthBias(float constantBias, float slopeScaledBias)
	{

	}
	/// Set depth compare.
	void Graphics::SetDepthTest(CompareMode mode)
	{

	}
	/// Set depth write on/off.
	void Graphics::SetDepthWrite(bool enable)
	{

	}
	/// Set polygon fill mode.
	void Graphics::SetFillMode(FillMode mode)
	{

	}
	/// Set line antialiasing on/off.
	void Graphics::SetLineAntiAlias(bool enable)
	{

	}
	/// Set scissor test.
	void Graphics::SetScissorTest(bool enable, const Rect& rect, bool borderInclusive)
	{

	}
	/// Set scissor test.
	void Graphics::SetScissorTest(bool enable, const IntRect& rect)
	{

	}
	/// Set stencil test.
	void Graphics::SetStencilTest(
		bool enable,
		CompareMode mode,
		StencilOp pass,
		StencilOp fail,
		StencilOp zFail,
		unsigned stencilRef,
		unsigned compareMask,
		unsigned writeMask)
	{

	}
	/// Set a custom clipping plane. The plane is specified in world space, but is dependent on the view and projection matrices.
	void Graphics::SetClipPlane(bool enable, const Plane& clipPlane, const Matrix3x4& view, const Matrix4& projection)
	{

	}
	///// Begin dumping shader variation names to an XML file for precaching.
	//void Graphics::BeginDumpShaders(const String& fileName)
	//{

	//}
	///// End dumping shader variations names.
	//void Graphics::EndDumpShaders()
	//{

	//}
	/// Set shader cache directory, Direct3D only. This can either be an absolute path or a path within the resource system.
	//void Graphics::SetShaderCacheDir(const String& path)
	//{

	//}
	
	/// Return whether rendering initialized.
	bool Graphics::IsInitialized() const
	{
		return true;
	}
	
	/// Return whether rendering output is dithered.
	bool Graphics::GetDither() const
	{
		return false;
	}

	void Graphics::OnDeviceLost()
	{
	}

	//
	///// Return whether graphics context is lost and can not render or load GPU resources.
	bool Graphics::IsDeviceLost() const { return false; }

	/// Return supported multisampling levels.
	PODVector<int> Graphics::GetMultiSampleLevels() const
	{
		PODVector<int> ret;
		ret.Push(1);
		return ret;
	}

	/// Return hardware format for a compressed image format, or 0 if unsupported.
	unsigned Graphics::GetFormat(CompressedFormat format) const
	{
		//TOFIX
		return 0;
	}
	/// Return a shader variation by name and defines.
	ShaderVariation* Graphics::GetShader(ShaderType type, const String& name, const String& defines) const
	{
		//TOFIX
		return nullptr;
	}
	/// Return a shader variation by name and defines.
	ShaderVariation* Graphics::GetShader(ShaderType type, const char* name, const char* defines) const
	{
		//TOFIX
		return nullptr;
	}
	/// Return current vertex buffer by index.
	VertexBuffer* Graphics::GetVertexBuffer(unsigned index) const
	{
		//TOFIX
		return nullptr;
	}
	
	
	/// Return shader program. This is an API-specific class and should not be used by applications.
	ShaderProgram* Graphics::GetShaderProgram() const
	{
		//TOFIX
		return nullptr;
	}
	
	/// Return texture unit index by name.
	TextureUnit Graphics::GetTextureUnit(const String& name)
	{
		//TOFIX
		return TU_DIFFUSE;
	}
	/// Return texture unit name by index.
	const String& Graphics::GetTextureUnitName(TextureUnit unit)
	{
		//TOFIX
		static String ret = "TU_DIFFUSE";
		return ret;
	}
	/// Return current texture by texture unit index.
	Texture* Graphics::GetTexture(unsigned index) const
	{
		//TOFIX
		return nullptr;
	}
	
	/// Return current rendertarget by index.
	RenderSurface* Graphics::GetRenderTarget(unsigned index) const
	{
		//TOFIX
		return nullptr;
	}
		
	/// Return current rendertarget width and height.
	IntVector2 Graphics::GetRenderTargetDimensions() const
	{
		//TOFIX
		return IntVector2(0, 0);
	}

	/// Window was resized through user interaction. Called by Input subsystem.
	void Graphics::OnWindowResized() {}
	/// Window was moved through user interaction. Called by Input subsystem.
	void Graphics::OnWindowMoved() {}
	/// Restore GPU objects and reinitialize state. Requires an open window. Used only on OpenGL.
	void Graphics::Restore() {}



	/// Clean up shader parameters when a shader variation is released or destroyed.
	void Graphics::CleanupShaderPrograms(ShaderVariation* variation){}
	/// Clean up a render surface from all FBOs. Used only on OpenGL.
	void Graphics::CleanupRenderSurface(RenderSurface* surface) {}

	/// Get or create a constant buffer. Will be shared between shaders if possible.
	ConstantBuffer* Graphics::GetOrCreateConstantBuffer(ShaderType type, unsigned index, unsigned size)
	{
		//TOFIX
		return nullptr;
	}

	/// Mark the FBO needing an update. Used only on OpenGL.
	void Graphics::MarkFBODirty() {}
	/// Bind a VBO, avoiding redundant operation. Used only on OpenGL.
	void Graphics::SetVBO(unsigned object) {}
	/// Bind a UBO, avoiding redundant operation. Used only on OpenGL.
	void Graphics::SetUBO(unsigned object) {}

	/// Return the API-specific alpha texture format.
	unsigned Graphics::GetAlphaFormat() { return 0; }
	/// Return the API-specific luminance texture format.
	unsigned Graphics::GetLuminanceFormat() { return 0; }
	/// Return the API-specific luminance alpha texture format.
	unsigned Graphics::GetLuminanceAlphaFormat() { return 0; }
	/// Return the API-specific RGB texture format.
	unsigned Graphics::GetRGBFormat() { return 0; }
	/// Return the API-specific RGBA texture format.
	unsigned Graphics::GetRGBAFormat() { return 0; }
	/// Return the API-specific RGBA 16-bit texture format.
	unsigned Graphics::GetRGBA16Format() { return 0; }
	/// Return the API-specific RGBA 16-bit float texture format.
	unsigned Graphics::GetRGBAFloat16Format() { return 0; }
	/// Return the API-specific RGBA 32-bit float texture format.
	unsigned Graphics::GetRGBAFloat32Format() { return 0; }
	/// Return the API-specific RG 16-bit texture format.
	unsigned Graphics::GetRG16Format() { return 0; }
	/// Return the API-specific RG 16-bit float texture format.
	unsigned Graphics::GetRGFloat16Format() { return 0; }
	/// Return the API-specific RG 32-bit float texture format.
	unsigned Graphics::GetRGFloat32Format() { return 0; }
	/// Return the API-specific single channel 16-bit float texture format.
	unsigned Graphics::GetFloat16Format() { return 0; }
	/// Return the API-specific single channel 32-bit float texture format.
	unsigned Graphics::GetFloat32Format() { return 0; }
	/// Return the API-specific linear depth texture format.
	unsigned Graphics::GetLinearDepthFormat() { return 0; }
	/// Return the API-specific hardware depth-stencil texture format.
	unsigned Graphics::GetDepthStencilFormat() { return 0; }
	/// Return the API-specific readable hardware depth format, or 0 if not supported.
	unsigned Graphics::GetReadableDepthFormat() { return 0; }
	/// Return the API-specific texture format from a textual description, for example "rgb".
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

	/// Return maximum number of supported bones for skinning.
	unsigned Graphics::GetMaxBones() { return 0; }
	/// Return whether is using an OpenGL 3 context. Return always false on Direct3D9 & Direct3D11.
	bool Graphics::GetGL3Support() { return false; }

}