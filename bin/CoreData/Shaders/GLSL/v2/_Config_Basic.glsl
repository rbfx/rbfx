/// _Config_Basic.glsl
/// Don't include!
/// Shader configuration invariant to material properties.

/// =================================== Types and constants ===================================

/// Medium-precision float with relative precision of 1/1024 and range [-16384, 16384].
// #define half
// #define half2
// #define half3
// #define half4

/// Low-precision fixel point real number with absolute precision of 1/256 and range [-2, 2].
// #define fixed
// #define fixed2
// #define fixed3
// #define fixed4

/// =================================== Flow control defines ===================================

/// Defined for corresponding shader stage.
// #define URHO3D_VERTEX_SHADER
// #define URHO3D_PIXEL_SHADER

/// Whether Y axis of clip space is the opposite of Y axis of render target texture.
// #define URHO3D_FEATURE_FRAMEBUFFER_Y_INVERTED

/// URHO3D_DEPTH_ONLY_PASS: Whether there's no color output and caller needs only depth.
// #define URHO3D_DEPTH_ONLY_PASS

/// URHO3D_LIGHT_PASS: Whether there's active per-pixel light source in this pass (litbase, light and lightvolume passes).
// #define URHO3D_LIGHT_PASS

/// =================================== Utility defines ===================================

/// Declare vertex input attribute received from vertex buffer.
// #define VERTEX_INPUT([precision] type name)

/// Declare vertex output variable passed to next stage.
/// Don't use highp or unqualified variables because it may cause linker error if highp is not supported in pixel shader.
// #define VERTEX_OUTPUT(precision type name)

/// Declare vertex output that is highp (if supported) or mediump (otherwise).
// #define VERTEX_OUTPUT_HIGHP(type name)

/// Declare uniform buffer. All uniforms must belong to appropriate uniform buffer.
// #define UNIFORM_BUFFER_BEGIN(slot, name)
// #define UNIFORM_BUFFER_END(slot, name)

/// Declare uniform.
/// Don't use highp or unqualified variables because it may cause linker error if highp is not supported in pixel shader.
// #define UNIFORM(precision type name)

/// Declare uniform with high precision, excluded from stage if not supported for the stage.
// #define UNIFORM_HIGHP(type name)

/// Declare sampler.
// #define SAMPLER([precision] type name)

/// Declare sampler with high precision, with medium precision if not supported.
// #define SAMPLER_HIGHP([precision] type name)

/// [Pixel shader only] Condition whether the pixel corresponds to front face.
// #define IS_FRONT_FACE

/// [Pixel shader only] Select value for front and back faces separately.
// #define SELECT_FRONT_BACK_FACE(frontValue, backValue)


/// =================================== Extensions ===================================

#extension GL_ARB_shading_language_420pack: enable
#extension GL_EXT_clip_cull_distance: enable
#extension GL_OES_standard_derivatives : enable


/// =================================== Types and constants ===================================

#define M_PI 3.1415927
#define M_MEDIUMP_FLT_MAX 65504.0

/// Disable precision modifiers if not GL ES.
#ifndef GL_ES
    #define highp
    #define mediump
    #define lowp
#endif

/// Define shortcuts for precision-qualified types.
#define half   mediump float
#define half2  mediump vec2
#define half3  mediump vec3
#define half4  mediump vec4
#define fixed  lowp float
#define fixed2 lowp vec2
#define fixed3 lowp vec3
#define fixed4 lowp vec4


/// =================================== Compiler features ===================================

#ifdef COMPILEVS
    #define URHO3D_VERTEX_SHADER
#endif
#ifdef COMPILEPS
    #define URHO3D_PIXEL_SHADER
#endif

#if !defined(GL_ES) || defined(GL_FRAGMENT_PRECISION_HIGH) || defined(URHO3D_VERTEX_SHADER)
    #define URHO3D_FEATURE_HIGHP_IN_STAGE
#endif

#if !defined(GL_ES) || defined(GL_EXT_clip_cull_distance)
    #define URHO3D_FEATURE_CLIP_DISTANCE
#endif

#ifndef URHO3D_OPENGL
    #define URHO3D_FEATURE_FRAMEBUFFER_Y_INVERTED
#endif

/// =================================== Precision ===================================

#ifdef GL_ES
    precision highp float;
#endif

/// =================================== Global functions ===================================

#define SaturateMediump(x) min(x, M_MEDIUMP_FLT_MAX)
#define Saturate(x) clamp(x, 0.0, 1.0)

half SpecularPowerToRoughness(half specularPower) { return 1.0 - specularPower / 255.0; }
half RoughnessToSpecularPower(half roughness) { return (1.0 - roughness) * 255.0; }


/// =================================== Stage inputs and outputs ===================================

#if defined(URHO3D_VERTEX_SHADER)
    #define VERTEX_INPUT(decl) in decl;
    #define VERTEX_OUTPUT(decl) out decl;
    #define VERTEX_OUTPUT_QUAL(qual, decl) qual out decl;
#elif defined(URHO3D_PIXEL_SHADER)
    #define VERTEX_OUTPUT(decl) in decl;
    #define VERTEX_OUTPUT_QUAL(qual, decl) qual in decl;
#endif

#define VERTEX_OUTPUT_HIGHP(decl) VERTEX_OUTPUT(highp decl)

/// Default to 1 render target if not defined.
#ifndef URHO3D_NUM_RENDER_TARGETS
    #define URHO3D_NUM_RENDER_TARGETS 1
#endif

/// Define gl_FragColor and gl_FragData for convenience.
#if defined(URHO3D_PIXEL_SHADER) && URHO3D_NUM_RENDER_TARGETS > 0
    out vec4 _fragData[URHO3D_NUM_RENDER_TARGETS];

    #define gl_FragColor _fragData[0]
    #define gl_FragData _fragData
#endif

/// Don't take chances: vertex position must be the same.
#if defined(URHO3D_VERTEX_SHADER) && defined(GL_ES)
    invariant gl_Position;
#endif


/// =================================== Uniforms ===================================

#ifdef GL_ARB_shading_language_420pack
    #define _URHO3D_LAYOUT(index) layout(binding=index)
#else
    #define _URHO3D_LAYOUT(index)
#endif

#define UNIFORM_BUFFER_BEGIN(index, name) _URHO3D_LAYOUT(index) uniform name {
#define UNIFORM(decl) decl;
#define UNIFORM_BUFFER_END(index, name) };
#define SAMPLER(index, decl) _URHO3D_LAYOUT(index) uniform decl;

#define UNIFORM_HIGHP(decl) UNIFORM(highp decl)
#define SAMPLER_HIGHP(index, decl) SAMPLER(index, highp decl)


/// =================================== Consolidate ShaderProgramCompositor defines ===================================

#ifndef URHO3D_SPECULAR
    #define URHO3D_SPECULAR 0
#endif

#ifndef URHO3D_MAX_SHADOW_CASCADES
    #define URHO3D_MAX_SHADOW_CASCADES 1
#endif

#if defined(URHO3D_SHADOW_PASS) || URHO3D_NUM_RENDER_TARGETS == 0
    #define URHO3D_DEPTH_ONLY_PASS
#endif

#if defined(URHO3D_LIGHT_DIRECTIONAL) || defined(URHO3D_LIGHT_POINT) || defined(URHO3D_LIGHT_SPOT)
    #define URHO3D_LIGHT_PASS
#endif

#ifdef URHO3D_VERTEX_SHADER
    /// Some geometries require certain vertex attributes.
    #if defined(URHO3D_GEOMETRY_SKINNED)
        #ifndef URHO3D_VERTEX_HAS_BONE_WEIGHTS_AND_INDICES
            #define URHO3D_VERTEX_HAS_BONE_WEIGHTS_AND_INDICES
        #endif
    #endif

    #if defined(URHO3D_GEOMETRY_BILLBOARD) || defined(URHO3D_GEOMETRY_DIRBILLBOARD)
        #ifndef URHO3D_VERTEX_HAS_TEXCOORD1
            #define URHO3D_VERTEX_HAS_TEXCOORD1
        #endif
    #endif

    #if defined(URHO3D_GEOMETRY_DIRBILLBOARD)
        #ifndef URHO3D_VERTEX_HAS_NORMAL
            #define URHO3D_VERTEX_HAS_NORMAL
        #endif
    #endif

    #if defined(URHO3D_GEOMETRY_TRAIL_FACE_CAMERA) || defined(URHO3D_GEOMETRY_TRAIL_BONE)
        #ifndef URHO3D_VERTEX_HAS_TANGENT
            #define URHO3D_VERTEX_HAS_TANGENT
        #endif
    #endif

    /// Normal offset is disabled if vertex normal is not available.
    #if defined(URHO3D_SHADOW_NORMAL_OFFSET) && !defined(URHO3D_VERTEX_NORMAL_AVAILABLE)
        #undef URHO3D_SHADOW_NORMAL_OFFSET
    #endif
#endif // URHO3D_VERTEX_SHADER

#ifdef URHO3D_PIXEL_SHADER
    #if defined(URHO3D_CAMERA_REVERSED)
        #define IS_FRONT_FACE (!gl_FrontFacing)
    #else
        #define IS_FRONT_FACE (gl_FrontFacing)
    #endif
    #define SELECT_FRONT_BACK_FACE(frontValue, backValue) ((IS_FRONT_FACE) ? (frontValue) : (backValue))
#endif // URHO3D_PIXEL_SHADER


/// =================================== Syntax sugar ===================================

// Help standard shaders to work properly if standard texture macros are not defined
#ifndef URHO3D_TEXTURE_ALBEDO
    #define URHO3D_TEXTURE_ALBEDO 0
#endif
#ifndef URHO3D_TEXTURE_NORMAL
    #define URHO3D_TEXTURE_NORMAL 0
#endif
#ifndef URHO3D_TEXTURE_PROPERTIES
    #define URHO3D_TEXTURE_PROPERTIES 0
#endif
#ifndef URHO3D_TEXTURE_EMISSION
    #define URHO3D_TEXTURE_EMISSION 0
#endif


/// =================================== Deprecated ===================================

// Aliases for easier migration.
// TODO: Remove those later
#define texture2D texture
#define texture2DProj textureProj
#define texture3D texture
#define textureCube texture
#define textureCubeLod textureLod
#define texture2DLod textureLod
#define texture2DLodOffset textureLodOffset
