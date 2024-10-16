/*
 *  Copyright 2019-2022 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#include <array>
#include <vector>

#include "WinHPreface.h"
#include <atlcomcli.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include "WinHPostface.h"

#include "DXBCUtils.hpp"
#include "../../../ThirdParty/GPUOpenShaderUtils/DXBCChecksum.h"

namespace Diligent
{

namespace
{

struct DXBCHeader
{
    Uint32 Magic;       // 0..3 "DXBC"
    Uint32 Checksum[4]; // 4..19
    Uint32 Reserved;    // 20..23
    Uint32 TotalSize;   // 24..27
    Uint32 ChunkCount;  // 28..31
};
static_assert(sizeof(DXBCHeader) == 32, "The size of DXBC header must be 32 bytes");

struct ChunkHeader
{
    Uint32 Magic;  // 0..3 fourCC
    Uint32 Length; // 4..7
};

struct ResourceDefChunkHeader : ChunkHeader
{
    Uint32 CBuffCount;          // 8..11
    Uint32 CBuffOffset;         // 12..15, from start of chunk data
    Uint32 ResBindingCount;     // 16..19
    Uint32 ResBindingOffset;    // 20..23, from start of chunk data
    Uint8  MinorVersion;        // 24
    Uint8  MajorVersion;        // 25
    Uint16 ShaderType;          // 26..27
    Uint32 Flags;               // 28..31
    Uint32 CreatorStringOffset; // 32..35, from start of chunk data
};
static_assert(sizeof(ResourceDefChunkHeader) == 36, "The size of resource definition chunk header must be 36 bytes");

struct ResourceBindingInfo50
{
    Uint32                   NameOffset;       // 0..3, from start of chunk data
    D3D_SHADER_INPUT_TYPE    ShaderInputType;  // 4..7
    D3D_RESOURCE_RETURN_TYPE ReturnType;       // 8..11
    D3D_SRV_DIMENSION        ViewDim;          // 12..15
    Uint32                   NumSamples;       // 16..19
    Uint32                   BindPoint;        // 20..23
    Uint32                   BindCount;        // 24..27
    D3D_SHADER_INPUT_FLAGS   ShaderInputFlags; // 28..31
};
static_assert(sizeof(ResourceBindingInfo50) == 32, "The size of SM50 resource binding info struct must be 32 bytes");

struct ResourceBindingInfo51
{
    Uint32                   NameOffset;       // 0..3, from start of chunk data
    D3D_SHADER_INPUT_TYPE    ShaderInputType;  // 4..7
    D3D_RESOURCE_RETURN_TYPE ReturnType;       // 8..11
    D3D_SRV_DIMENSION        ViewDim;          // 12..15
    Uint32                   NumSamples;       // 16..19
    Uint32                   BindPoint;        // 20..23
    Uint32                   BindCount;        // 24..27
    D3D_SHADER_INPUT_FLAGS   ShaderInputFlags; // 28..31
    Uint32                   Space;            // 32..35
    Uint32                   Reserved;         // 36..39
};
static_assert(sizeof(ResourceBindingInfo51) == 40, "The size of SM51 resource binding info struct must be 40 bytes");

enum PROGRAM_TYPE : Uint32
{
    PROGRAM_TYPE_PIXEL  = 0,
    PROGRAM_TYPE_VERTEX = 1,
    PROGRAM_TYPE_COUNT_SM4,

    PROGRAM_TYPE_GEOMETRY = 2,
    PROGRAM_TYPE_HULL     = 3,
    PROGRAM_TYPE_DOMAIN   = 4,
    PROGRAM_TYPE_COMPUTE  = 5,
    PROGRAM_TYPE_COUNT_SM5,
};

struct ShaderChunkHeader : ChunkHeader
{
    Uint32       VersionMinor : 4;
    Uint32       VersionMajor : 4;
    Uint32       _Padding : 8;
    PROGRAM_TYPE ProgramType : 16; // 2..3
    Uint32       NumDWords;        // 4..7, number of DWORDs in the chunk
};
static_assert(sizeof(ShaderChunkHeader) == 16, "The size of shader code header must be 16 bytes");

// from d3d11TokenizedProgramFormat.hpp
enum D3D10_SB_OPCODE_TYPE : Uint32
{
    D3D10_SB_OPCODE_ADD,
    D3D10_SB_OPCODE_AND,
    D3D10_SB_OPCODE_BREAK,
    D3D10_SB_OPCODE_BREAKC,
    D3D10_SB_OPCODE_CALL,
    D3D10_SB_OPCODE_CALLC,
    D3D10_SB_OPCODE_CASE,
    D3D10_SB_OPCODE_CONTINUE,
    D3D10_SB_OPCODE_CONTINUEC,
    D3D10_SB_OPCODE_CUT,
    D3D10_SB_OPCODE_DEFAULT,
    D3D10_SB_OPCODE_DERIV_RTX,
    D3D10_SB_OPCODE_DERIV_RTY,
    D3D10_SB_OPCODE_DISCARD,
    D3D10_SB_OPCODE_DIV,
    D3D10_SB_OPCODE_DP2,
    D3D10_SB_OPCODE_DP3,
    D3D10_SB_OPCODE_DP4,
    D3D10_SB_OPCODE_ELSE,
    D3D10_SB_OPCODE_EMIT,
    D3D10_SB_OPCODE_EMITTHENCUT,
    D3D10_SB_OPCODE_ENDIF,
    D3D10_SB_OPCODE_ENDLOOP,
    D3D10_SB_OPCODE_ENDSWITCH,
    D3D10_SB_OPCODE_EQ,
    D3D10_SB_OPCODE_EXP,
    D3D10_SB_OPCODE_FRC,
    D3D10_SB_OPCODE_FTOI,
    D3D10_SB_OPCODE_FTOU,
    D3D10_SB_OPCODE_GE,
    D3D10_SB_OPCODE_IADD,
    D3D10_SB_OPCODE_IF,
    D3D10_SB_OPCODE_IEQ,
    D3D10_SB_OPCODE_IGE,
    D3D10_SB_OPCODE_ILT,
    D3D10_SB_OPCODE_IMAD,
    D3D10_SB_OPCODE_IMAX,
    D3D10_SB_OPCODE_IMIN,
    D3D10_SB_OPCODE_IMUL,
    D3D10_SB_OPCODE_INE,
    D3D10_SB_OPCODE_INEG,
    D3D10_SB_OPCODE_ISHL,
    D3D10_SB_OPCODE_ISHR,
    D3D10_SB_OPCODE_ITOF,
    D3D10_SB_OPCODE_LABEL,
    D3D10_SB_OPCODE_LD,
    D3D10_SB_OPCODE_LD_MS,
    D3D10_SB_OPCODE_LOG,
    D3D10_SB_OPCODE_LOOP,
    D3D10_SB_OPCODE_LT,
    D3D10_SB_OPCODE_MAD,
    D3D10_SB_OPCODE_MIN,
    D3D10_SB_OPCODE_MAX,
    D3D10_SB_OPCODE_CUSTOMDATA,
    D3D10_SB_OPCODE_MOV,
    D3D10_SB_OPCODE_MOVC,
    D3D10_SB_OPCODE_MUL,
    D3D10_SB_OPCODE_NE,
    D3D10_SB_OPCODE_NOP,
    D3D10_SB_OPCODE_NOT,
    D3D10_SB_OPCODE_OR,
    D3D10_SB_OPCODE_RESINFO,
    D3D10_SB_OPCODE_RET,
    D3D10_SB_OPCODE_RETC,
    D3D10_SB_OPCODE_ROUND_NE,
    D3D10_SB_OPCODE_ROUND_NI,
    D3D10_SB_OPCODE_ROUND_PI,
    D3D10_SB_OPCODE_ROUND_Z,
    D3D10_SB_OPCODE_RSQ,
    D3D10_SB_OPCODE_SAMPLE,
    D3D10_SB_OPCODE_SAMPLE_C,
    D3D10_SB_OPCODE_SAMPLE_C_LZ,
    D3D10_SB_OPCODE_SAMPLE_L,
    D3D10_SB_OPCODE_SAMPLE_D,
    D3D10_SB_OPCODE_SAMPLE_B,
    D3D10_SB_OPCODE_SQRT,
    D3D10_SB_OPCODE_SWITCH,
    D3D10_SB_OPCODE_SINCOS,
    D3D10_SB_OPCODE_UDIV,
    D3D10_SB_OPCODE_ULT,
    D3D10_SB_OPCODE_UGE,
    D3D10_SB_OPCODE_UMUL,
    D3D10_SB_OPCODE_UMAD,
    D3D10_SB_OPCODE_UMAX,
    D3D10_SB_OPCODE_UMIN,
    D3D10_SB_OPCODE_USHR,
    D3D10_SB_OPCODE_UTOF,
    D3D10_SB_OPCODE_XOR,
    D3D10_SB_OPCODE_DCL_RESOURCE,        // DCL* opcodes have
    D3D10_SB_OPCODE_DCL_CONSTANT_BUFFER, // custom operand formats.
    D3D10_SB_OPCODE_DCL_SAMPLER,
    D3D10_SB_OPCODE_DCL_INDEX_RANGE,
    D3D10_SB_OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY,
    D3D10_SB_OPCODE_DCL_GS_INPUT_PRIMITIVE,
    D3D10_SB_OPCODE_DCL_MAX_OUTPUT_VERTEX_COUNT,
    D3D10_SB_OPCODE_DCL_INPUT,
    D3D10_SB_OPCODE_DCL_INPUT_SGV,
    D3D10_SB_OPCODE_DCL_INPUT_SIV,
    D3D10_SB_OPCODE_DCL_INPUT_PS,
    D3D10_SB_OPCODE_DCL_INPUT_PS_SGV,
    D3D10_SB_OPCODE_DCL_INPUT_PS_SIV,
    D3D10_SB_OPCODE_DCL_OUTPUT,
    D3D10_SB_OPCODE_DCL_OUTPUT_SGV,
    D3D10_SB_OPCODE_DCL_OUTPUT_SIV,
    D3D10_SB_OPCODE_DCL_TEMPS,
    D3D10_SB_OPCODE_DCL_INDEXABLE_TEMP,
    D3D10_SB_OPCODE_DCL_GLOBAL_FLAGS,

    // -----------------------------------------------

    // This marks the end of D3D10.0 opcodes
    D3D10_SB_OPCODE_RESERVED0,

    // ---------- DX 10.1 op codes---------------------

    D3D10_1_SB_OPCODE_LOD,
    D3D10_1_SB_OPCODE_GATHER4,
    D3D10_1_SB_OPCODE_SAMPLE_POS,
    D3D10_1_SB_OPCODE_SAMPLE_INFO,

    // -----------------------------------------------

    // This marks the end of D3D10.1 opcodes
    D3D10_1_SB_OPCODE_RESERVED1,

    // ---------- DX 11 op codes---------------------
    D3D11_SB_OPCODE_HS_DECLS,               // token marks beginning of HS sub-shader
    D3D11_SB_OPCODE_HS_CONTROL_POINT_PHASE, // token marks beginning of HS sub-shader
    D3D11_SB_OPCODE_HS_FORK_PHASE,          // token marks beginning of HS sub-shader
    D3D11_SB_OPCODE_HS_JOIN_PHASE,          // token marks beginning of HS sub-shader

    D3D11_SB_OPCODE_EMIT_STREAM,
    D3D11_SB_OPCODE_CUT_STREAM,
    D3D11_SB_OPCODE_EMITTHENCUT_STREAM,
    D3D11_SB_OPCODE_INTERFACE_CALL,

    D3D11_SB_OPCODE_BUFINFO,
    D3D11_SB_OPCODE_DERIV_RTX_COARSE,
    D3D11_SB_OPCODE_DERIV_RTX_FINE,
    D3D11_SB_OPCODE_DERIV_RTY_COARSE,
    D3D11_SB_OPCODE_DERIV_RTY_FINE,
    D3D11_SB_OPCODE_GATHER4_C,
    D3D11_SB_OPCODE_GATHER4_PO,
    D3D11_SB_OPCODE_GATHER4_PO_C,
    D3D11_SB_OPCODE_RCP,
    D3D11_SB_OPCODE_F32TOF16,
    D3D11_SB_OPCODE_F16TOF32,
    D3D11_SB_OPCODE_UADDC,
    D3D11_SB_OPCODE_USUBB,
    D3D11_SB_OPCODE_COUNTBITS,
    D3D11_SB_OPCODE_FIRSTBIT_HI,
    D3D11_SB_OPCODE_FIRSTBIT_LO,
    D3D11_SB_OPCODE_FIRSTBIT_SHI,
    D3D11_SB_OPCODE_UBFE,
    D3D11_SB_OPCODE_IBFE,
    D3D11_SB_OPCODE_BFI,
    D3D11_SB_OPCODE_BFREV,
    D3D11_SB_OPCODE_SWAPC,

    D3D11_SB_OPCODE_DCL_STREAM,
    D3D11_SB_OPCODE_DCL_FUNCTION_BODY,
    D3D11_SB_OPCODE_DCL_FUNCTION_TABLE,
    D3D11_SB_OPCODE_DCL_INTERFACE,

    D3D11_SB_OPCODE_DCL_INPUT_CONTROL_POINT_COUNT,
    D3D11_SB_OPCODE_DCL_OUTPUT_CONTROL_POINT_COUNT,
    D3D11_SB_OPCODE_DCL_TESS_DOMAIN,
    D3D11_SB_OPCODE_DCL_TESS_PARTITIONING,
    D3D11_SB_OPCODE_DCL_TESS_OUTPUT_PRIMITIVE,
    D3D11_SB_OPCODE_DCL_HS_MAX_TESSFACTOR,
    D3D11_SB_OPCODE_DCL_HS_FORK_PHASE_INSTANCE_COUNT,
    D3D11_SB_OPCODE_DCL_HS_JOIN_PHASE_INSTANCE_COUNT,

    D3D11_SB_OPCODE_DCL_THREAD_GROUP,
    D3D11_SB_OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED,
    D3D11_SB_OPCODE_DCL_UNORDERED_ACCESS_VIEW_RAW,
    D3D11_SB_OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED,
    D3D11_SB_OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_RAW,
    D3D11_SB_OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_STRUCTURED,
    D3D11_SB_OPCODE_DCL_RESOURCE_RAW,
    D3D11_SB_OPCODE_DCL_RESOURCE_STRUCTURED,
    D3D11_SB_OPCODE_LD_UAV_TYPED,
    D3D11_SB_OPCODE_STORE_UAV_TYPED,
    D3D11_SB_OPCODE_LD_RAW,
    D3D11_SB_OPCODE_STORE_RAW,
    D3D11_SB_OPCODE_LD_STRUCTURED,
    D3D11_SB_OPCODE_STORE_STRUCTURED,
    D3D11_SB_OPCODE_ATOMIC_AND,
    D3D11_SB_OPCODE_ATOMIC_OR,
    D3D11_SB_OPCODE_ATOMIC_XOR,
    D3D11_SB_OPCODE_ATOMIC_CMP_STORE,
    D3D11_SB_OPCODE_ATOMIC_IADD,
    D3D11_SB_OPCODE_ATOMIC_IMAX,
    D3D11_SB_OPCODE_ATOMIC_IMIN,
    D3D11_SB_OPCODE_ATOMIC_UMAX,
    D3D11_SB_OPCODE_ATOMIC_UMIN,
    D3D11_SB_OPCODE_IMM_ATOMIC_ALLOC,
    D3D11_SB_OPCODE_IMM_ATOMIC_CONSUME,
    D3D11_SB_OPCODE_IMM_ATOMIC_IADD,
    D3D11_SB_OPCODE_IMM_ATOMIC_AND,
    D3D11_SB_OPCODE_IMM_ATOMIC_OR,
    D3D11_SB_OPCODE_IMM_ATOMIC_XOR,
    D3D11_SB_OPCODE_IMM_ATOMIC_EXCH,
    D3D11_SB_OPCODE_IMM_ATOMIC_CMP_EXCH,
    D3D11_SB_OPCODE_IMM_ATOMIC_IMAX,
    D3D11_SB_OPCODE_IMM_ATOMIC_IMIN,
    D3D11_SB_OPCODE_IMM_ATOMIC_UMAX,
    D3D11_SB_OPCODE_IMM_ATOMIC_UMIN,
    D3D11_SB_OPCODE_SYNC,

    D3D11_SB_OPCODE_DADD,
    D3D11_SB_OPCODE_DMAX,
    D3D11_SB_OPCODE_DMIN,
    D3D11_SB_OPCODE_DMUL,
    D3D11_SB_OPCODE_DEQ,
    D3D11_SB_OPCODE_DGE,
    D3D11_SB_OPCODE_DLT,
    D3D11_SB_OPCODE_DNE,
    D3D11_SB_OPCODE_DMOV,
    D3D11_SB_OPCODE_DMOVC,
    D3D11_SB_OPCODE_DTOF,
    D3D11_SB_OPCODE_FTOD,

    D3D11_SB_OPCODE_EVAL_SNAPPED,
    D3D11_SB_OPCODE_EVAL_SAMPLE_INDEX,
    D3D11_SB_OPCODE_EVAL_CENTROID,

    D3D11_SB_OPCODE_DCL_GS_INSTANCE_COUNT,

    D3D11_SB_OPCODE_ABORT,
    D3D11_SB_OPCODE_DEBUG_BREAK,

    // -----------------------------------------------

    // This marks the end of D3D11.0 opcodes
    D3D11_SB_OPCODE_RESERVED0,

    D3D11_1_SB_OPCODE_DDIV,
    D3D11_1_SB_OPCODE_DFMA,
    D3D11_1_SB_OPCODE_DRCP,

    D3D11_1_SB_OPCODE_MSAD,

    D3D11_1_SB_OPCODE_DTOI,
    D3D11_1_SB_OPCODE_DTOU,
    D3D11_1_SB_OPCODE_ITOD,
    D3D11_1_SB_OPCODE_UTOD,

    // -----------------------------------------------

    // This marks the end of D3D11.1 opcodes
    D3D11_1_SB_OPCODE_RESERVED0,

    D3DWDDM1_3_SB_OPCODE_GATHER4_FEEDBACK,
    D3DWDDM1_3_SB_OPCODE_GATHER4_C_FEEDBACK,
    D3DWDDM1_3_SB_OPCODE_GATHER4_PO_FEEDBACK,
    D3DWDDM1_3_SB_OPCODE_GATHER4_PO_C_FEEDBACK,
    D3DWDDM1_3_SB_OPCODE_LD_FEEDBACK,
    D3DWDDM1_3_SB_OPCODE_LD_MS_FEEDBACK,
    D3DWDDM1_3_SB_OPCODE_LD_UAV_TYPED_FEEDBACK,
    D3DWDDM1_3_SB_OPCODE_LD_RAW_FEEDBACK,
    D3DWDDM1_3_SB_OPCODE_LD_STRUCTURED_FEEDBACK,
    D3DWDDM1_3_SB_OPCODE_SAMPLE_L_FEEDBACK,
    D3DWDDM1_3_SB_OPCODE_SAMPLE_C_LZ_FEEDBACK,

    D3DWDDM1_3_SB_OPCODE_SAMPLE_CLAMP_FEEDBACK,
    D3DWDDM1_3_SB_OPCODE_SAMPLE_B_CLAMP_FEEDBACK,
    D3DWDDM1_3_SB_OPCODE_SAMPLE_D_CLAMP_FEEDBACK,
    D3DWDDM1_3_SB_OPCODE_SAMPLE_C_CLAMP_FEEDBACK,

    D3DWDDM1_3_SB_OPCODE_CHECK_ACCESS_FULLY_MAPPED,

    // -----------------------------------------------

    // This marks the end of WDDM 1.3 opcodes
    D3DWDDM1_3_SB_OPCODE_RESERVED0,

    D3D10_SB_NUM_OPCODES // Should be the last entry

};

// from d3d11TokenizedProgramFormat.hpp
enum D3D10_SB_OPERAND_NUM_COMPONENTS : Uint32
{
    D3D10_SB_OPERAND_0_COMPONENT = 0,
    D3D10_SB_OPERAND_1_COMPONENT = 1,
    D3D10_SB_OPERAND_4_COMPONENT = 2,
    D3D10_SB_OPERAND_N_COMPONENT = 3 // unused for now
};

// from d3d11TokenizedProgramFormat.hpp
enum D3D10_SB_OPERAND_4_COMPONENT_SELECTION_MODE : Uint32
{
    D3D10_SB_OPERAND_4_COMPONENT_MASK_MODE     = 0, // mask 4 components
    D3D10_SB_OPERAND_4_COMPONENT_SWIZZLE_MODE  = 1, // swizzle 4 components
    D3D10_SB_OPERAND_4_COMPONENT_SELECT_1_MODE = 2, // select 1 of 4 components
};

// from d3d11TokenizedProgramFormat.hpp
enum D3D10_SB_OPERAND_TYPE : Uint32
{
    D3D10_SB_OPERAND_TYPE_TEMP           = 0,                      // Temporary Register File
    D3D10_SB_OPERAND_TYPE_INPUT          = 1,                      // General Input Register File
    D3D10_SB_OPERAND_TYPE_OUTPUT         = 2,                      // General Output Register File
    D3D10_SB_OPERAND_TYPE_INDEXABLE_TEMP = 3,                      // Temporary Register File (indexable)
    D3D10_SB_OPERAND_TYPE_IMMEDIATE32    = 4,                      // 32bit/component immediate value(s)
                                                                   // If for example, operand token bits
                                                                   // [01:00]==D3D10_SB_OPERAND_4_COMPONENT,
                                                                   // this means that the operand type:
                                                                   // D3D10_SB_OPERAND_TYPE_IMMEDIATE32
                                                                   // results in 4 additional 32bit
                                                                   // DWORDS present for the operand.
    D3D10_SB_OPERAND_TYPE_IMMEDIATE64               = 5,           // 64bit/comp.imm.val(s)HI:LO
    D3D10_SB_OPERAND_TYPE_SAMPLER                   = 6,           // Reference to sampler state
    D3D10_SB_OPERAND_TYPE_RESOURCE                  = 7,           // Reference to memory resource (e.g. texture)
    D3D10_SB_OPERAND_TYPE_CONSTANT_BUFFER           = 8,           // Reference to constant buffer
    D3D10_SB_OPERAND_TYPE_IMMEDIATE_CONSTANT_BUFFER = 9,           // Reference to immediate constant buffer
    D3D10_SB_OPERAND_TYPE_LABEL                     = 10,          // Label
    D3D10_SB_OPERAND_TYPE_INPUT_PRIMITIVEID         = 11,          // Input primitive ID
    D3D10_SB_OPERAND_TYPE_OUTPUT_DEPTH              = 12,          // Output Depth
    D3D10_SB_OPERAND_TYPE_NULL                      = 13,          // Null register, used to discard results of operations
                                                                   // Below Are operands new in DX 10.1
    D3D10_SB_OPERAND_TYPE_RASTERIZER           = 14,               // DX10.1 Rasterizer register, used to denote the depth/stencil and render target resources
    D3D10_SB_OPERAND_TYPE_OUTPUT_COVERAGE_MASK = 15,               // DX10.1 PS output MSAA coverage mask (scalar)
                                                                   // Below Are operands new in DX 11
    D3D11_SB_OPERAND_TYPE_STREAM                             = 16, // Reference to GS stream output resource
    D3D11_SB_OPERAND_TYPE_FUNCTION_BODY                      = 17, // Reference to a function definition
    D3D11_SB_OPERAND_TYPE_FUNCTION_TABLE                     = 18, // Reference to a set of functions used by a class
    D3D11_SB_OPERAND_TYPE_INTERFACE                          = 19, // Reference to an interface
    D3D11_SB_OPERAND_TYPE_FUNCTION_INPUT                     = 20, // Reference to an input parameter to a function
    D3D11_SB_OPERAND_TYPE_FUNCTION_OUTPUT                    = 21, // Reference to an output parameter to a function
    D3D11_SB_OPERAND_TYPE_OUTPUT_CONTROL_POINT_ID            = 22, // HS Control Point phase input saying which output control point ID this is
    D3D11_SB_OPERAND_TYPE_INPUT_FORK_INSTANCE_ID             = 23, // HS Fork Phase input instance ID
    D3D11_SB_OPERAND_TYPE_INPUT_JOIN_INSTANCE_ID             = 24, // HS Join Phase input instance ID
    D3D11_SB_OPERAND_TYPE_INPUT_CONTROL_POINT                = 25, // HS Fork+Join, DS phase input control points (array of them)
    D3D11_SB_OPERAND_TYPE_OUTPUT_CONTROL_POINT               = 26, // HS Fork+Join phase output control points (array of them)
    D3D11_SB_OPERAND_TYPE_INPUT_PATCH_CONSTANT               = 27, // DS+HSJoin Input Patch Constants (array of them)
    D3D11_SB_OPERAND_TYPE_INPUT_DOMAIN_POINT                 = 28, // DS Input Domain point
    D3D11_SB_OPERAND_TYPE_THIS_POINTER                       = 29, // Reference to an interface this pointer
    D3D11_SB_OPERAND_TYPE_UNORDERED_ACCESS_VIEW              = 30, // Reference to UAV u#
    D3D11_SB_OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY         = 31, // Reference to Thread Group Shared Memory g#
    D3D11_SB_OPERAND_TYPE_INPUT_THREAD_ID                    = 32, // Compute Shader Thread ID
    D3D11_SB_OPERAND_TYPE_INPUT_THREAD_GROUP_ID              = 33, // Compute Shader Thread Group ID
    D3D11_SB_OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP           = 34, // Compute Shader Thread ID In Thread Group
    D3D11_SB_OPERAND_TYPE_INPUT_COVERAGE_MASK                = 35, // Pixel shader coverage mask input
    D3D11_SB_OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP_FLATTENED = 36, // Compute Shader Thread ID In Group Flattened to a 1D value.
    D3D11_SB_OPERAND_TYPE_INPUT_GS_INSTANCE_ID               = 37, // Input GS instance ID
    D3D11_SB_OPERAND_TYPE_OUTPUT_DEPTH_GREATER_EQUAL         = 38, // Output Depth, forced to be greater than or equal than current depth
    D3D11_SB_OPERAND_TYPE_OUTPUT_DEPTH_LESS_EQUAL            = 39, // Output Depth, forced to be less than or equal to current depth
    D3D11_SB_OPERAND_TYPE_CYCLE_COUNTER                      = 40, // Cycle counter

    D3D11_SB_NUM_OPERANDS,
};

// from d3d11TokenizedProgramFormat.hpp
enum D3D10_SB_OPERAND_INDEX_DIMENSION : Uint32
{
    D3D10_SB_OPERAND_INDEX_0D = 0, // e.g. Position
    D3D10_SB_OPERAND_INDEX_1D = 1, // Most common.  e.g. Temp registers.
    D3D10_SB_OPERAND_INDEX_2D = 2, // e.g. Geometry Program Input registers.
    D3D10_SB_OPERAND_INDEX_3D = 3, // 3D rarely if ever used.
};

// from d3d11TokenizedProgramFormat.hpp
enum D3D10_SB_OPERAND_INDEX_REPRESENTATION : Uint32
{
    D3D10_SB_OPERAND_INDEX_IMMEDIATE32 = 0,               // Extra DWORD
    D3D10_SB_OPERAND_INDEX_IMMEDIATE64 = 1,               // 2 Extra DWORDs
                                                          //   (HI32:LO32)
    D3D10_SB_OPERAND_INDEX_RELATIVE                  = 2, // Extra operand
    D3D10_SB_OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE = 3, // Extra DWORD followed by
                                                          //   extra operand
    D3D10_SB_OPERAND_INDEX_IMMEDIATE64_PLUS_RELATIVE = 4, // 2 Extra DWORDS
                                                          //   (HI32:LO32) followed
                                                          //   by extra operand
};

// standard opcode token
struct OpcodeToken
{
    D3D10_SB_OPCODE_TYPE OpcodeType : 11;  // bits 0..10
    Uint32               Controls : 13;    // bits 11..23, Opcode-Specific Controls
    Uint32               OpcodeLength : 7; // bita 24..30
    Uint32               Extended : 1;     // bit 31, 1 if opcode is "extended", otherwise 0
};
static_assert(sizeof(OpcodeToken) == 4, "");

struct OperandToken
{
    D3D10_SB_OPERAND_NUM_COMPONENTS             NumComponents : 2;  // 0..1
    D3D10_SB_OPERAND_4_COMPONENT_SELECTION_MODE CompSelection : 2;  // 2..3
    Uint32                                      CompMask : 8;       // 4..11
    D3D10_SB_OPERAND_TYPE                       OperandType : 8;    // 12..19
    D3D10_SB_OPERAND_INDEX_DIMENSION            IndexDim : 2;       // 20..21
    D3D10_SB_OPERAND_INDEX_REPRESENTATION       OperandIndex1D : 3; // 22..24
    D3D10_SB_OPERAND_INDEX_REPRESENTATION       OperandIndex2D : 3; // 25..27
    D3D10_SB_OPERAND_INDEX_REPRESENTATION       OperandIndex3D : 3; // 28..30
    Uint32                                      Extended : 1;       // 31
};
static_assert(sizeof(OperandToken) == 4, "");

// from d3d11TokenizedProgramFormat.hpp
enum D3D10_SB_CUSTOMDATA_CLASS
{
    D3D10_SB_CUSTOMDATA_COMMENT = 0,
    D3D10_SB_CUSTOMDATA_DEBUGINFO,
    D3D10_SB_CUSTOMDATA_OPAQUE,
    D3D10_SB_CUSTOMDATA_DCL_IMMEDIATE_CONSTANT_BUFFER,
    D3D11_SB_CUSTOMDATA_SHADER_MESSAGE,
    D3D11_SB_CUSTOMDATA_SHADER_CLIP_PLANE_CONSTANT_MAPPINGS_FOR_DX9,
};

enum RES_TYPE : Uint32
{
    RES_TYPE_CBV     = 0,
    RES_TYPE_SRV     = 1,
    RES_TYPE_SAMPLER = 2,
    RES_TYPE_UAV     = 3,
    RES_TYPE_COUNT,
};

struct ResourceExtendedInfo
{
    Uint32   SrcBindPoint = ~0u;
    Uint32   SrcSpace     = ~0u;
    RES_TYPE Type         = RES_TYPE_COUNT;
};

using ResourceBindingPerType = std::array<std::vector<DXBCUtils::BindInfo const*>, RES_TYPE_COUNT + 1>;
using TExtendedResourceMap   = std::unordered_map<DXBCUtils::BindInfo const*, ResourceExtendedInfo>;


#define FOURCC(a, b, c, d) (Uint32{(d) << 24} | Uint32{(c) << 16} | Uint32{(b) << 8} | Uint32{a})

constexpr Uint32 DXBCFourCC = FOURCC('D', 'X', 'B', 'C');
constexpr Uint32 RDEFFourCC = FOURCC('R', 'D', 'E', 'F'); // Resource definition. Describes constant buffers and resource bindings.
constexpr Uint32 SHDRFourCC = FOURCC('S', 'H', 'D', 'R'); // Shader (SM4)
constexpr Uint32 SHEXFourCC = FOURCC('S', 'H', 'E', 'X'); // Shader (SM5)
//constexpr Uint32 DXILFourCC = FOURCC('D', 'X', 'I', 'L'); // Shader (SM6)


RES_TYPE ToResType(D3D_SHADER_INPUT_TYPE InputType)
{
    switch (InputType)
    {
        case D3D_SIT_CBUFFER:
            return RES_TYPE_CBV;

        case D3D_SIT_SAMPLER:
            return RES_TYPE_SAMPLER;

        case D3D_SIT_TBUFFER:
        case D3D_SIT_TEXTURE:
        case D3D_SIT_STRUCTURED:
        case D3D_SIT_BYTEADDRESS:
            return RES_TYPE_SRV;

        case D3D_SIT_UAV_RWSTRUCTURED:
        case D3D_SIT_UAV_RWTYPED:
        case D3D_SIT_UAV_RWBYTEADDRESS:
        case D3D_SIT_UAV_APPEND_STRUCTURED:
        case D3D_SIT_UAV_CONSUME_STRUCTURED:
        case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
            return RES_TYPE_UAV;

        default:
            UNEXPECTED("Unsupported shader input type");
            return RES_TYPE_COUNT;
    }
}

const char* ResTypeToString(RES_TYPE Type)
{
    switch (Type)
    {
        // clang-format off
        case RES_TYPE_CBV:     return "CBV";
        case RES_TYPE_SRV:     return "SRV";
        case RES_TYPE_SAMPLER: return "Sampler";
        case RES_TYPE_UAV:     return "UAV";
            // clang-format on
        default:
            UNEXPECTED("Unknown resource type");
            return "";
    }
}

Uint32 GetNumOperands(D3D10_SB_OPCODE_TYPE Opcode)
{
    switch (Opcode)
    {
        case D3D10_SB_OPCODE_CUSTOMDATA: return 0;
        case D3D10_SB_OPCODE_ADD: return 3;
        case D3D10_SB_OPCODE_AND: return 3;
        case D3D10_SB_OPCODE_BREAK: return 0;
        case D3D10_SB_OPCODE_BREAKC: return 1;
        case D3D10_SB_OPCODE_CALL: return 1;
        case D3D10_SB_OPCODE_CALLC: return 2;
        case D3D10_SB_OPCODE_CONTINUE: return 0;
        case D3D10_SB_OPCODE_CONTINUEC: return 1;
        case D3D10_SB_OPCODE_CASE: return 1;
        case D3D10_SB_OPCODE_CUT: return 0;
        case D3D10_SB_OPCODE_DEFAULT: return 0;
        case D3D10_SB_OPCODE_DISCARD: return 1;
        case D3D10_SB_OPCODE_DIV: return 3;
        case D3D10_SB_OPCODE_DP2: return 3;
        case D3D10_SB_OPCODE_DP3: return 3;
        case D3D10_SB_OPCODE_DP4: return 3;
        case D3D10_SB_OPCODE_ELSE: return 0;
        case D3D10_SB_OPCODE_EMIT: return 0;
        case D3D10_SB_OPCODE_EMITTHENCUT: return 0;
        case D3D10_SB_OPCODE_ENDIF: return 0;
        case D3D10_SB_OPCODE_ENDLOOP: return 0;
        case D3D10_SB_OPCODE_ENDSWITCH: return 0;
        case D3D10_SB_OPCODE_EQ: return 3;
        case D3D10_SB_OPCODE_EXP: return 2;
        case D3D10_SB_OPCODE_FRC: return 2;
        case D3D10_SB_OPCODE_FTOI: return 2;
        case D3D10_SB_OPCODE_FTOU: return 2;
        case D3D10_SB_OPCODE_GE: return 3;
        case D3D10_SB_OPCODE_DERIV_RTX: return 2;
        case D3D10_SB_OPCODE_DERIV_RTY: return 2;
        case D3D10_SB_OPCODE_IADD: return 3;
        case D3D10_SB_OPCODE_IF: return 1;
        case D3D10_SB_OPCODE_IEQ: return 3;
        case D3D10_SB_OPCODE_IGE: return 3;
        case D3D10_SB_OPCODE_ILT: return 3;
        case D3D10_SB_OPCODE_IMAD: return 4;
        case D3D10_SB_OPCODE_IMAX: return 3;
        case D3D10_SB_OPCODE_IMIN: return 3;
        case D3D10_SB_OPCODE_IMUL: return 4;
        case D3D10_SB_OPCODE_INE: return 3;
        case D3D10_SB_OPCODE_INEG: return 2;
        case D3D10_SB_OPCODE_ISHL: return 3;
        case D3D10_SB_OPCODE_ISHR: return 3;
        case D3D10_SB_OPCODE_ITOF: return 2;
        case D3D10_SB_OPCODE_LABEL: return 1;
        case D3D10_SB_OPCODE_LD: return 3;
        case D3D10_SB_OPCODE_LD_MS: return 4;
        case D3D10_SB_OPCODE_LOG: return 2;
        case D3D10_SB_OPCODE_LOOP: return 0;
        case D3D10_SB_OPCODE_LT: return 3;
        case D3D10_SB_OPCODE_MAD: return 4;
        case D3D10_SB_OPCODE_MAX: return 3;
        case D3D10_SB_OPCODE_MIN: return 3;
        case D3D10_SB_OPCODE_MOV: return 2;
        case D3D10_SB_OPCODE_MOVC: return 4;
        case D3D10_SB_OPCODE_MUL: return 3;
        case D3D10_SB_OPCODE_NE: return 3;
        case D3D10_SB_OPCODE_NOP: return 0;
        case D3D10_SB_OPCODE_NOT: return 2;
        case D3D10_SB_OPCODE_OR: return 3;
        case D3D10_SB_OPCODE_RESINFO: return 3;
        case D3D10_SB_OPCODE_RET: return 0;
        case D3D10_SB_OPCODE_RETC: return 1;
        case D3D10_SB_OPCODE_ROUND_NE: return 2;
        case D3D10_SB_OPCODE_ROUND_NI: return 2;
        case D3D10_SB_OPCODE_ROUND_PI: return 2;
        case D3D10_SB_OPCODE_ROUND_Z: return 2;
        case D3D10_SB_OPCODE_RSQ: return 2;
        case D3D10_SB_OPCODE_SAMPLE: return 4;
        case D3D10_SB_OPCODE_SAMPLE_B: return 5;
        case D3D10_SB_OPCODE_SAMPLE_L: return 5;
        case D3D10_SB_OPCODE_SAMPLE_D: return 6;
        case D3D10_SB_OPCODE_SAMPLE_C: return 5;
        case D3D10_SB_OPCODE_SAMPLE_C_LZ: return 5;
        case D3D10_SB_OPCODE_SQRT: return 2;
        case D3D10_SB_OPCODE_SWITCH: return 1;
        case D3D10_SB_OPCODE_SINCOS: return 3;
        case D3D10_SB_OPCODE_UDIV: return 4;
        case D3D10_SB_OPCODE_ULT: return 3;
        case D3D10_SB_OPCODE_UGE: return 3;
        case D3D10_SB_OPCODE_UMAX: return 3;
        case D3D10_SB_OPCODE_UMIN: return 3;
        case D3D10_SB_OPCODE_UMUL: return 4;
        case D3D10_SB_OPCODE_UMAD: return 4;
        case D3D10_SB_OPCODE_USHR: return 3;
        case D3D10_SB_OPCODE_UTOF: return 2;
        case D3D10_SB_OPCODE_XOR: return 3;
        case D3D10_SB_OPCODE_RESERVED0: return 0;
        case D3D10_SB_OPCODE_DCL_INPUT: return 1;
        case D3D10_SB_OPCODE_DCL_OUTPUT: return 1;
        case D3D10_SB_OPCODE_DCL_INPUT_SGV: return 1;
        case D3D10_SB_OPCODE_DCL_INPUT_PS_SGV: return 1;
        case D3D10_SB_OPCODE_DCL_GS_INPUT_PRIMITIVE: return 0;
        case D3D10_SB_OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY: return 0;
        case D3D10_SB_OPCODE_DCL_MAX_OUTPUT_VERTEX_COUNT: return 0;
        case D3D10_SB_OPCODE_DCL_INPUT_PS: return 1;
        case D3D10_SB_OPCODE_DCL_CONSTANT_BUFFER: return 1;
        case D3D10_SB_OPCODE_DCL_SAMPLER: return 1;
        case D3D10_SB_OPCODE_DCL_RESOURCE: return 1;
        case D3D10_SB_OPCODE_DCL_INPUT_SIV: return 1;
        case D3D10_SB_OPCODE_DCL_INPUT_PS_SIV: return 1;
        case D3D10_SB_OPCODE_DCL_OUTPUT_SIV: return 1;
        case D3D10_SB_OPCODE_DCL_OUTPUT_SGV: return 1;
        case D3D10_SB_OPCODE_DCL_TEMPS: return 0;
        case D3D10_SB_OPCODE_DCL_INDEXABLE_TEMP: return 0;
        case D3D10_SB_OPCODE_DCL_INDEX_RANGE: return 1;
        case D3D10_SB_OPCODE_DCL_GLOBAL_FLAGS: return 0;
        case D3D10_1_SB_OPCODE_SAMPLE_INFO: return 2;
        case D3D10_1_SB_OPCODE_SAMPLE_POS: return 3;
        case D3D10_1_SB_OPCODE_GATHER4: return 4;
        case D3D10_1_SB_OPCODE_LOD: return 4;
        case D3D11_SB_OPCODE_EMIT_STREAM: return 1;
        case D3D11_SB_OPCODE_CUT_STREAM: return 1;
        case D3D11_SB_OPCODE_EMITTHENCUT_STREAM: return 1;
        case D3D11_SB_OPCODE_INTERFACE_CALL: return 1;
        case D3D11_SB_OPCODE_DCL_STREAM: return 1;
        case D3D11_SB_OPCODE_DCL_FUNCTION_BODY: return 0;
        case D3D11_SB_OPCODE_DCL_FUNCTION_TABLE: return 0;
        case D3D11_SB_OPCODE_DCL_INTERFACE: return 0;
        case D3D11_SB_OPCODE_BUFINFO: return 2;
        case D3D11_SB_OPCODE_DERIV_RTX_COARSE: return 2;
        case D3D11_SB_OPCODE_DERIV_RTX_FINE: return 2;
        case D3D11_SB_OPCODE_DERIV_RTY_COARSE: return 2;
        case D3D11_SB_OPCODE_DERIV_RTY_FINE: return 2;
        case D3D11_SB_OPCODE_GATHER4_C: return 5;
        case D3D11_SB_OPCODE_GATHER4_PO: return 5;
        case D3D11_SB_OPCODE_GATHER4_PO_C: return 6;
        case D3D11_SB_OPCODE_RCP: return 2;
        case D3D11_SB_OPCODE_F32TOF16: return 2;
        case D3D11_SB_OPCODE_F16TOF32: return 2;
        case D3D11_SB_OPCODE_UADDC: return 4;
        case D3D11_SB_OPCODE_USUBB: return 4;
        case D3D11_SB_OPCODE_COUNTBITS: return 2;
        case D3D11_SB_OPCODE_FIRSTBIT_HI: return 2;
        case D3D11_SB_OPCODE_FIRSTBIT_LO: return 2;
        case D3D11_SB_OPCODE_FIRSTBIT_SHI: return 2;
        case D3D11_SB_OPCODE_UBFE: return 4;
        case D3D11_SB_OPCODE_IBFE: return 4;
        case D3D11_SB_OPCODE_BFI: return 5;
        case D3D11_SB_OPCODE_BFREV: return 2;
        case D3D11_SB_OPCODE_SWAPC: return 5;
        case D3D11_SB_OPCODE_HS_DECLS: return 0;
        case D3D11_SB_OPCODE_HS_CONTROL_POINT_PHASE: return 0;
        case D3D11_SB_OPCODE_HS_FORK_PHASE: return 0;
        case D3D11_SB_OPCODE_HS_JOIN_PHASE: return 0;
        case D3D11_SB_OPCODE_DCL_INPUT_CONTROL_POINT_COUNT: return 0;
        case D3D11_SB_OPCODE_DCL_OUTPUT_CONTROL_POINT_COUNT: return 0;
        case D3D11_SB_OPCODE_DCL_TESS_DOMAIN: return 0;
        case D3D11_SB_OPCODE_DCL_TESS_PARTITIONING: return 0;
        case D3D11_SB_OPCODE_DCL_TESS_OUTPUT_PRIMITIVE: return 0;
        case D3D11_SB_OPCODE_DCL_HS_MAX_TESSFACTOR: return 0;
        case D3D11_SB_OPCODE_DCL_HS_FORK_PHASE_INSTANCE_COUNT: return 0;
        case D3D11_SB_OPCODE_DCL_HS_JOIN_PHASE_INSTANCE_COUNT: return 0;
        case D3D11_SB_OPCODE_DCL_THREAD_GROUP: return 0;
        case D3D11_SB_OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED: return 1;
        case D3D11_SB_OPCODE_DCL_UNORDERED_ACCESS_VIEW_RAW: return 1;
        case D3D11_SB_OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED: return 1;
        case D3D11_SB_OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_RAW: return 1;
        case D3D11_SB_OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_STRUCTURED: return 1;
        case D3D11_SB_OPCODE_DCL_RESOURCE_RAW: return 1;
        case D3D11_SB_OPCODE_DCL_RESOURCE_STRUCTURED: return 1;
        case D3D11_SB_OPCODE_LD_UAV_TYPED: return 3;
        case D3D11_SB_OPCODE_STORE_UAV_TYPED: return 3;
        case D3D11_SB_OPCODE_LD_RAW: return 3;
        case D3D11_SB_OPCODE_STORE_RAW: return 3;
        case D3D11_SB_OPCODE_LD_STRUCTURED: return 4;
        case D3D11_SB_OPCODE_STORE_STRUCTURED: return 4;
        case D3D11_SB_OPCODE_ATOMIC_AND: return 3;
        case D3D11_SB_OPCODE_ATOMIC_OR: return 3;
        case D3D11_SB_OPCODE_ATOMIC_XOR: return 3;
        case D3D11_SB_OPCODE_ATOMIC_CMP_STORE: return 4;
        case D3D11_SB_OPCODE_ATOMIC_IADD: return 3;
        case D3D11_SB_OPCODE_ATOMIC_IMAX: return 3;
        case D3D11_SB_OPCODE_ATOMIC_IMIN: return 3;
        case D3D11_SB_OPCODE_ATOMIC_UMAX: return 3;
        case D3D11_SB_OPCODE_ATOMIC_UMIN: return 3;
        case D3D11_SB_OPCODE_IMM_ATOMIC_ALLOC: return 2;
        case D3D11_SB_OPCODE_IMM_ATOMIC_CONSUME: return 2;
        case D3D11_SB_OPCODE_IMM_ATOMIC_IADD: return 4;
        case D3D11_SB_OPCODE_IMM_ATOMIC_AND: return 4;
        case D3D11_SB_OPCODE_IMM_ATOMIC_OR: return 4;
        case D3D11_SB_OPCODE_IMM_ATOMIC_XOR: return 4;
        case D3D11_SB_OPCODE_IMM_ATOMIC_EXCH: return 4;
        case D3D11_SB_OPCODE_IMM_ATOMIC_CMP_EXCH: return 5;
        case D3D11_SB_OPCODE_IMM_ATOMIC_IMAX: return 4;
        case D3D11_SB_OPCODE_IMM_ATOMIC_IMIN: return 4;
        case D3D11_SB_OPCODE_IMM_ATOMIC_UMAX: return 4;
        case D3D11_SB_OPCODE_IMM_ATOMIC_UMIN: return 4;
        case D3D11_SB_OPCODE_SYNC: return 0;
        case D3D11_SB_OPCODE_EVAL_SNAPPED: return 3;
        case D3D11_SB_OPCODE_EVAL_SAMPLE_INDEX: return 3;
        case D3D11_SB_OPCODE_EVAL_CENTROID: return 2;
        case D3D11_SB_OPCODE_DCL_GS_INSTANCE_COUNT: return 0;
        case D3D11_SB_OPCODE_DADD: return 3;
        case D3D11_SB_OPCODE_DMAX: return 3;
        case D3D11_SB_OPCODE_DMIN: return 3;
        case D3D11_SB_OPCODE_DMUL: return 3;
        case D3D11_SB_OPCODE_DEQ: return 3;
        case D3D11_SB_OPCODE_DGE: return 3;
        case D3D11_SB_OPCODE_DLT: return 3;
        case D3D11_SB_OPCODE_DNE: return 3;
        case D3D11_SB_OPCODE_DMOV: return 2;
        case D3D11_SB_OPCODE_DMOVC: return 4;
        case D3D11_SB_OPCODE_DTOF: return 2;
        case D3D11_SB_OPCODE_FTOD: return 2;
        case D3D11_SB_OPCODE_ABORT: return 0;
        case D3D11_SB_OPCODE_DEBUG_BREAK: return 0;
        case D3D11_1_SB_OPCODE_DDIV: return 3;
        case D3D11_1_SB_OPCODE_DFMA: return 4;
        case D3D11_1_SB_OPCODE_DRCP: return 2;
        case D3D11_1_SB_OPCODE_MSAD: return 4;
        case D3D11_1_SB_OPCODE_DTOI: return 2;
        case D3D11_1_SB_OPCODE_DTOU: return 2;
        case D3D11_1_SB_OPCODE_ITOD: return 2;
        case D3D11_1_SB_OPCODE_UTOD: return 2;
        case D3DWDDM1_3_SB_OPCODE_GATHER4_FEEDBACK: return 5;
        case D3DWDDM1_3_SB_OPCODE_GATHER4_C_FEEDBACK: return 6;
        case D3DWDDM1_3_SB_OPCODE_GATHER4_PO_FEEDBACK: return 6;
        case D3DWDDM1_3_SB_OPCODE_GATHER4_PO_C_FEEDBACK: return 7;
        case D3DWDDM1_3_SB_OPCODE_LD_FEEDBACK: return 4;
        case D3DWDDM1_3_SB_OPCODE_LD_MS_FEEDBACK: return 5;
        case D3DWDDM1_3_SB_OPCODE_LD_UAV_TYPED_FEEDBACK: return 4;
        case D3DWDDM1_3_SB_OPCODE_LD_RAW_FEEDBACK: return 4;
        case D3DWDDM1_3_SB_OPCODE_LD_STRUCTURED_FEEDBACK: return 5;
        case D3DWDDM1_3_SB_OPCODE_SAMPLE_L_FEEDBACK: return 6;
        case D3DWDDM1_3_SB_OPCODE_SAMPLE_C_LZ_FEEDBACK: return 6;
        case D3DWDDM1_3_SB_OPCODE_SAMPLE_CLAMP_FEEDBACK: return 6;
        case D3DWDDM1_3_SB_OPCODE_SAMPLE_B_CLAMP_FEEDBACK: return 7;
        case D3DWDDM1_3_SB_OPCODE_SAMPLE_D_CLAMP_FEEDBACK: return 8;
        case D3DWDDM1_3_SB_OPCODE_SAMPLE_C_CLAMP_FEEDBACK: return 7;
        case D3DWDDM1_3_SB_OPCODE_CHECK_ACCESS_FULLY_MAPPED: return 2;
        default:
            UNEXPECTED("Unknown opcode");
            return ~0u;
    }
}

inline bool PatchSpace(ResourceBindingInfo51& Res, ResourceExtendedInfo& Ext, const DXBCUtils::BindInfo& Info)
{
    Ext.SrcSpace = Res.Space;
    Res.Space    = Info.Space;
    return true;
}

inline bool PatchSpace(ResourceBindingInfo50&, ResourceExtendedInfo& Ext, const DXBCUtils::BindInfo& Info)
{
    VERIFY_EXPR(Ext.SrcSpace == ~0u);
    return Info.Space == 0 || Info.Space == ~0U;
}

template <typename ResourceBindingInfoType>
void RemapShaderResources(const DXBCUtils::TResourceBindingMap& ResourceMap, const void* EndPtr, ResourceDefChunkHeader* RDEFHeader, TExtendedResourceMap& ExtResMap, ResourceBindingPerType& BindingsPerType)
{
    VERIFY_EXPR(RDEFHeader->Magic == RDEFFourCC);

    auto* Ptr        = reinterpret_cast<char*>(RDEFHeader) + sizeof(ChunkHeader);
    auto* ResBinding = reinterpret_cast<ResourceBindingInfoType*>(Ptr + RDEFHeader->ResBindingOffset);
    if (ResBinding + RDEFHeader->ResBindingCount > EndPtr)
    {
        LOG_ERROR_AND_THROW("Resource binding data is outside of the specified byte code range. The byte code may be corrupted.");
    }

    String TempName;
    for (Uint32 r = 0; r < RDEFHeader->ResBindingCount; ++r)
    {
        auto&       Res  = ResBinding[r];
        const char* Name = Ptr + Res.NameOffset;
        if (Name + 1 > EndPtr)
        {
            LOG_ERROR_AND_THROW("Resource name pointer is outside of the specified byte code range. The byte code may be corrupted.");
        }

        const auto ResType = ToResType(Res.ShaderInputType);
        if (ResType > BindingsPerType.size())
        {
            LOG_ERROR_AND_THROW("Invalid shader input type.");
        }

        TempName = Name;

        // before SM 5.1
        // Example: g_tex2D_sampler[2]
        Uint32 ArrayInd = 0;
        if (TempName.back() == ']')
        {
            size_t ArrEnd   = TempName.length() - 1;
            size_t ArrStart = ArrEnd - 1;

            for (; ArrStart > 1; --ArrStart)
            {
                if (TempName[ArrStart] == '[')
                    break;
                VERIFY_EXPR(TempName[ArrStart] >= '0' && TempName[ArrStart] <= '9');
            }

            ArrayInd = std::stoi(TempName.substr(ArrStart + 1, ArrEnd - ArrStart - 1));
            TempName.resize(ArrStart);
        }

        auto Iter = ResourceMap.find(HashMapStringKey{TempName.c_str(), false});
        if (Iter == ResourceMap.end())
        {
            LOG_ERROR_AND_THROW("Failed to find '", TempName, "' in the resource mapping.");
        }

        auto& Ext     = ExtResMap[&Iter->second];
        auto& Binding = BindingsPerType[ResType];
        Binding.emplace_back(&Iter->second);

        VERIFY_EXPR(ArrayInd < Iter->second.ArraySize);
        VERIFY_EXPR(Ext.SrcBindPoint == ~0u || Ext.SrcBindPoint == Res.BindPoint - ArrayInd);
        VERIFY_EXPR(Ext.Type == RES_TYPE_COUNT || Ext.Type == ResType);
        VERIFY_EXPR((ResType != RES_TYPE_CBV && Res.BindCount == 0) ||
                    (ResType == RES_TYPE_CBV && Res.BindCount == ~0u) ||
                    Iter->second.ArraySize >= Res.BindCount);

#ifdef DILIGENT_DEBUG
        static_assert(SHADER_RESOURCE_TYPE_LAST == 8, "Please update the switch below to handle the new shader resource type");
        switch (Iter->second.ResType)
        {
            // clang-format off
            case SHADER_RESOURCE_TYPE_CONSTANT_BUFFER:  VERIFY_EXPR(ResType == RES_TYPE_CBV);     break;
            case SHADER_RESOURCE_TYPE_TEXTURE_SRV:      VERIFY_EXPR(ResType == RES_TYPE_SRV);     break;
            case SHADER_RESOURCE_TYPE_BUFFER_SRV:       VERIFY_EXPR(ResType == RES_TYPE_SRV);     break;
            case SHADER_RESOURCE_TYPE_TEXTURE_UAV:      VERIFY_EXPR(ResType == RES_TYPE_UAV);     break;
            case SHADER_RESOURCE_TYPE_BUFFER_UAV:       VERIFY_EXPR(ResType == RES_TYPE_UAV);     break;
            case SHADER_RESOURCE_TYPE_SAMPLER:          VERIFY_EXPR(ResType == RES_TYPE_SAMPLER); break;
            case SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT: VERIFY_EXPR(ResType == RES_TYPE_SRV);     break;
            // clang-format on
            default: UNEXPECTED("Unsupported shader resource type.");
        }
#endif
        Ext.Type         = ResType;
        Ext.SrcBindPoint = Res.BindPoint - ArrayInd;
        Res.BindPoint    = Iter->second.BindPoint + ArrayInd;

        if (!PatchSpace(Res, Ext, Iter->second))
        {
            LOG_ERROR_AND_THROW("Can not change space for resource '", TempName, "' because the shader was not compiled for SM 5.1.");
        }
    }
}


struct ShaderBytecodeRemapper
{
public:
    ShaderBytecodeRemapper(ShaderChunkHeader const& _Header, TExtendedResourceMap& _ExtResMap, ResourceBindingPerType const& _BindingsPerType) :
        Header{_Header}, ExtResourceMap{_ExtResMap}, BindingsPerType{_BindingsPerType}
    {}

    void PatchBytecode(Uint32* Token, const void* EndPtr) noexcept(false);

private:
    ShaderChunkHeader const&      Header;
    TExtendedResourceMap&         ExtResourceMap;
    ResourceBindingPerType const& BindingsPerType;

    static constexpr Uint32 RuntimeSizedArraySize = ~0u;

private:
    void RemapResourceOperand(const OperandToken& Operand, Uint32* Token, const void* Finish);
    void RemapResourceOperandSM50(const OperandToken& Operand, Uint32* Token, const void* Finish);
    void RemapResourceOperandSM51(const OperandToken& Operand, Uint32* Token, const void* Finish);
    void RemapResourceOperandSM51_2(const OperandToken& Operand, Uint32* Token, RES_TYPE Type);

    void RemapResourceBinding(const OpcodeToken& Opcode, Uint32* Token, const void* Finish);
    void RemapResourceBindingSM51(const OpcodeToken& Opcode, Uint32* Token, const void* Finish);

    void ParseOperand(Uint32*& Token, const void* Finish);
    void ParseIndex(D3D10_SB_OPERAND_INDEX_REPRESENTATION IndexType, Uint32*& Token, const void* Finish);
    void ParseCustomData(Uint32*& Token, const void* Finish, D3D10_SB_CUSTOMDATA_CLASS Type);
    void ParseOpcode(Uint32*& Token, const void* Finish);

    bool IsSM51() const { return Header.VersionMajor == 5 && Header.VersionMinor >= 1; }
};

void ShaderBytecodeRemapper::RemapResourceOperand(const OperandToken& Operand, Uint32* Token, const void* Finish)
{
    if (IsSM51())
    {
        RemapResourceOperandSM51(Operand, Token, Finish);
    }
    else
    {
        RemapResourceOperandSM50(Operand, Token, Finish);
    }
}

void ShaderBytecodeRemapper::RemapResourceBinding(const OpcodeToken& Opcode, Uint32* Token, const void* Finish)
{
    if (IsSM51())
    {
        RemapResourceBindingSM51(Opcode, Token, Finish);
    }
}

void ShaderBytecodeRemapper::RemapResourceOperandSM50(const OperandToken& Operand, Uint32* Token, const void* Finish)
{
    const auto FindResourceBindings = [this](RES_TYPE Type, Uint32& Token) //
    {
        auto& Bindings = BindingsPerType[Type];
        for (auto& Info : Bindings)
        {
            const auto& Ext = ExtResourceMap[Info];
            if (Token >= Ext.SrcBindPoint && Token < Ext.SrcBindPoint + Info->ArraySize)
            {
                Token = Info->BindPoint + (Token - Ext.SrcBindPoint);
                VERIFY_EXPR(Ext.Type == Type);
                return true;
            }
        }
        return false;
    };

    switch (Operand.OperandType)
    {
        case D3D10_SB_OPERAND_TYPE_CONSTANT_BUFFER:
        {
            // 0 - cbuffer bind point
            // 1 - row offset (16 bytes per row) | cbuffer size

            VERIFY_EXPR(Token + 2 <= Finish);
            VERIFY_EXPR(Operand.IndexDim == D3D10_SB_OPERAND_INDEX_2D);
            VERIFY_EXPR(Operand.OperandIndex1D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);
            VERIFY_EXPR(Operand.OperandIndex1D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);

            if (!FindResourceBindings(RES_TYPE_CBV, Token[0]))
                LOG_ERROR_AND_THROW("Failed to find cbuffer with bind point (", Token[0], ").");
            break;
        }

        case D3D10_SB_OPERAND_TYPE_SAMPLER:
        {
            // 0 - sampler bind point

            VERIFY_EXPR(Token + 1 <= Finish);
            VERIFY_EXPR(Operand.IndexDim == D3D10_SB_OPERAND_INDEX_1D);
            VERIFY_EXPR(Operand.OperandIndex1D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);

            if (!FindResourceBindings(RES_TYPE_SAMPLER, Token[0]))
                LOG_ERROR_AND_THROW("Failed to find sampler with bind point (", Token[0], ").");
            break;
        }

        case D3D10_SB_OPERAND_TYPE_RESOURCE:
        {
            // 0 - texture bind point

            VERIFY_EXPR(Token + 1 <= Finish);
            VERIFY_EXPR(Operand.IndexDim == D3D10_SB_OPERAND_INDEX_1D);
            VERIFY_EXPR(Operand.OperandIndex1D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);

            if (!FindResourceBindings(RES_TYPE_SRV, Token[0]))
                LOG_ERROR_AND_THROW("Failed to find texture with bind point (", Token[0], ").");
            break;
        }

        case D3D11_SB_OPERAND_TYPE_UNORDERED_ACCESS_VIEW:
        {
            // 0 - UAV bind point

            VERIFY_EXPR(Token + 1 <= Finish);
            VERIFY_EXPR(Operand.IndexDim == D3D10_SB_OPERAND_INDEX_1D);
            VERIFY_EXPR(Operand.OperandIndex1D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);

            if (!FindResourceBindings(RES_TYPE_UAV, Token[0]))
                LOG_ERROR_AND_THROW("Failed to find UAV with bind point (", Token[0], ").");
            break;
        }

        case D3D10_SB_OPERAND_TYPE_IMMEDIATE_CONSTANT_BUFFER:
            break; // ignore

        default:
            break; // ignore
    }
}

void ShaderBytecodeRemapper::RemapResourceOperandSM51_2(const OperandToken& Operand, Uint32* Token, RES_TYPE Type)
{
    const auto& Bindings = BindingsPerType[Type];
    if (Token[0] >= Bindings.size())
        LOG_ERROR_AND_THROW("Invalid ", ResTypeToString(Type), " index (", Token[0], "), the number of resources is (", Bindings.size(), ").");

    const auto& Info = *Bindings[Token[0]];
    const auto& Ext  = ExtResourceMap[&Info];
    switch (Operand.OperandIndex2D)
    {
        case D3D10_SB_OPERAND_INDEX_IMMEDIATE32:
        case D3D10_SB_OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE:
        {
            if (Token[1] < Ext.SrcBindPoint || Token[1] >= Ext.SrcBindPoint + Info.ArraySize)
                LOG_ERROR_AND_THROW("Invalid bind point (", Token[1], "), expected to be in the range (", Ext.SrcBindPoint, "..", Ext.SrcBindPoint + Info.ArraySize - 1, ").");

            Token[1] = Info.BindPoint + (Token[1] - Ext.SrcBindPoint);
            break;
        }
        case D3D10_SB_OPERAND_INDEX_RELATIVE:
        {
            const auto Operand2 = reinterpret_cast<OperandToken&>(Token[1]);
            VERIFY_EXPR(Operand2.OperandType == D3D10_SB_OPERAND_TYPE_TEMP);
            VERIFY_EXPR(Operand2.IndexDim == D3D10_SB_OPERAND_INDEX_1D);
            VERIFY_EXPR(Operand2.OperandIndex1D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);

            if (Token[2] < Ext.SrcBindPoint || Token[2] >= Ext.SrcBindPoint + Info.ArraySize)
                LOG_ERROR_AND_THROW("Invalid bind point (", Token[2], "), expected to be in the range (", Ext.SrcBindPoint, "..", Ext.SrcBindPoint + Info.ArraySize - 1, ").");

            Token[2] = Info.BindPoint + (Token[2] - Ext.SrcBindPoint);
            break;
        }
        default:
            LOG_ERROR_AND_THROW("Unknown OperandIndex (", Uint32{Operand.OperandIndex2D}, ").");
    }
}

void ShaderBytecodeRemapper::RemapResourceOperandSM51(const OperandToken& Operand, Uint32* Token, const void* Finish)
{
    switch (Operand.OperandType)
    {
        case D3D10_SB_OPERAND_TYPE_CONSTANT_BUFFER:
        {
            // 0 - cbuffer index in resource definition
            // 1 - cbuffer bind point
            // 2 - row offset (16 bytes per row)

            VERIFY_EXPR(Token + 3 <= Finish);
            VERIFY_EXPR(Operand.IndexDim == D3D10_SB_OPERAND_INDEX_3D);
            VERIFY_EXPR(Operand.OperandIndex1D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);

            RemapResourceOperandSM51_2(Operand, Token, RES_TYPE_CBV);
            break;
        }

        case D3D10_SB_OPERAND_TYPE_SAMPLER:
        {
            // 0 - sampler index in resource definition
            // 1 - sampler bind point

            VERIFY_EXPR(Token + 2 <= Finish);
            VERIFY_EXPR(Operand.IndexDim >= D3D10_SB_OPERAND_INDEX_2D);
            VERIFY_EXPR(Operand.OperandIndex1D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);

            RemapResourceOperandSM51_2(Operand, Token, RES_TYPE_SAMPLER);
            break;
        }

        case D3D10_SB_OPERAND_TYPE_RESOURCE:
        {
            // 0 - texture index in resource definition
            // 1 - texture bind point

            VERIFY_EXPR(Token + 2 <= Finish);
            VERIFY_EXPR(Operand.IndexDim >= D3D10_SB_OPERAND_INDEX_2D);
            VERIFY_EXPR(Operand.OperandIndex1D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);

            RemapResourceOperandSM51_2(Operand, Token, RES_TYPE_SRV);
            break;
        }

        case D3D11_SB_OPERAND_TYPE_UNORDERED_ACCESS_VIEW:
        {
            // 0 - UAV index in resource definition
            // 1 - UAV bind point

            VERIFY_EXPR(Token + 2 <= Finish);
            VERIFY_EXPR(Operand.IndexDim >= D3D10_SB_OPERAND_INDEX_2D);
            VERIFY_EXPR(Operand.OperandIndex1D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);

            RemapResourceOperandSM51_2(Operand, Token, RES_TYPE_UAV);
            break;
        }

        case D3D10_SB_OPERAND_TYPE_IMMEDIATE_CONSTANT_BUFFER:
            break; // ignore

        default:
            break; // ignore
    }
}

void ShaderBytecodeRemapper::RemapResourceBindingSM51(const OpcodeToken& Opcode, Uint32* Token, const void* Finish)
{
    const auto& Operand = *reinterpret_cast<OperandToken*>(Token);

    switch (Opcode.OpcodeType)
    {
        case D3D10_SB_OPCODE_DCL_CONSTANT_BUFFER:
        {
            // 0 - operand info
            // 1 - cbuffer index
            // 2 - first bind point -- remapped in RemapResourceOperand()
            // 3 - last bind point
            // 4 - cbuffer size
            // 5 - register space

            VERIFY_EXPR(Token + 6 <= Finish);
            VERIFY_EXPR(Opcode.OpcodeLength > 5);
            VERIFY_EXPR(Operand.OperandType == D3D10_SB_OPERAND_TYPE_CONSTANT_BUFFER);
            VERIFY_EXPR(Operand.IndexDim == D3D10_SB_OPERAND_INDEX_3D);
            VERIFY_EXPR(Operand.OperandIndex1D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);
            VERIFY_EXPR(Operand.OperandIndex2D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);
            VERIFY_EXPR(Operand.OperandIndex3D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);

            const auto& Bindings = BindingsPerType[RES_TYPE_CBV];
            if (Token[1] >= Bindings.size())
                LOG_ERROR_AND_THROW("Invalid cbuffer index (", Token[1], "), the number of constant buffers is (", Bindings.size(), ").");

            const auto& Info = *Bindings[Token[1]];
            const auto& Ext  = ExtResourceMap[&Info];
            VERIFY_EXPR(Info.BindPoint == Token[2]);
            VERIFY_EXPR(Ext.Type == RES_TYPE_CBV);

            if (Token[3] != RuntimeSizedArraySize && Token[3] != Ext.SrcBindPoint + Info.ArraySize - 1)
                LOG_ERROR_AND_THROW("Invalid cbuffer bind point (", Token[3], "), expected (", Ext.SrcBindPoint + Info.ArraySize - 1, ").");
            if (Ext.SrcSpace != Token[5])
                LOG_ERROR_AND_THROW("Invalid cbuffer register space (", Token[5], "), expected (", Ext.SrcSpace, ").");

            if (Token[3] != RuntimeSizedArraySize)
                Token[3] = Info.BindPoint + Info.ArraySize - 1;

            Token[5] = Info.Space;
            break;
        }

        case D3D10_SB_OPCODE_DCL_SAMPLER:
        {
            // 0 - operand info
            // 1 - sampler index
            // 2 - first bind point -- remapped in RemapResourceOperand()
            // 3 - last bind point
            // 4 - register space

            VERIFY_EXPR(Token + 5 <= Finish);
            VERIFY_EXPR(Opcode.OpcodeLength > 4);
            VERIFY_EXPR(Operand.OperandType == D3D10_SB_OPERAND_TYPE_SAMPLER);
            VERIFY_EXPR(Operand.IndexDim == D3D10_SB_OPERAND_INDEX_3D);
            VERIFY_EXPR(Operand.OperandIndex1D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);
            VERIFY_EXPR(Operand.OperandIndex2D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);
            VERIFY_EXPR(Operand.OperandIndex3D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);

            const auto& Bindings = BindingsPerType[RES_TYPE_SAMPLER];
            if (Token[1] >= Bindings.size())
                LOG_ERROR_AND_THROW("Invalid sampler index (", Token[1], "), the number of samplers is (", Bindings.size(), ").");

            const auto& Info = *Bindings[Token[1]];
            const auto& Ext  = ExtResourceMap[&Info];
            VERIFY_EXPR(Info.BindPoint == Token[2]);
            VERIFY_EXPR(Ext.Type == RES_TYPE_SAMPLER);

            if (Token[3] != RuntimeSizedArraySize && Token[3] != Ext.SrcBindPoint + Info.ArraySize - 1)
                LOG_ERROR_AND_THROW("Invalid sampler bind point (", Token[3], "), expected (", Ext.SrcBindPoint + Info.ArraySize - 1, ").");
            if (Ext.SrcSpace != Token[4])
                LOG_ERROR_AND_THROW("Invalid sampler register space (", Token[4], "), expected (", Ext.SrcSpace, ").");

            if (Token[3] != RuntimeSizedArraySize)
                Token[3] = Info.BindPoint + Info.ArraySize - 1;

            Token[4] = Info.Space;
            break;
        }

        // Texture
        case D3D10_SB_OPCODE_DCL_RESOURCE:
        case D3D11_SB_OPCODE_DCL_RESOURCE_STRUCTURED:
        {
            // 0 - operand info
            // 1 - texture index
            // 2 - first bind point -- remapped in RemapResourceOperand()
            // 3 - last bind point
            // 4 - format? / struct size
            // 5 - register space

            VERIFY_EXPR(Token + 6 <= Finish);
            VERIFY_EXPR(Opcode.OpcodeLength > 5);
            VERIFY_EXPR(Operand.OperandType == D3D10_SB_OPERAND_TYPE_RESOURCE);
            VERIFY_EXPR(Operand.IndexDim == D3D10_SB_OPERAND_INDEX_3D);
            VERIFY_EXPR(Operand.OperandIndex1D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);
            VERIFY_EXPR(Operand.OperandIndex2D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);
            VERIFY_EXPR(Operand.OperandIndex3D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);

            const auto& Bindings = BindingsPerType[RES_TYPE_SRV];
            if (Token[1] >= Bindings.size())
                LOG_ERROR_AND_THROW("Invalid texture index (", Token[1], "), the number of textures is (", Bindings.size(), ").");

            const auto& Info = *Bindings[Token[1]];
            const auto& Ext  = ExtResourceMap[&Info];
            VERIFY_EXPR(Info.BindPoint == Token[2]);
            VERIFY_EXPR(Ext.Type == RES_TYPE_SRV);

            if (Token[3] != RuntimeSizedArraySize && Token[3] != Ext.SrcBindPoint + Info.ArraySize - 1)
                LOG_ERROR_AND_THROW("Invalid texture bind point (", Token[3], "), expected (", Ext.SrcBindPoint + Info.ArraySize - 1, ").");
            if (Ext.SrcSpace != Token[5])
                LOG_ERROR_AND_THROW("Invalid texture register space (", Token[5], "), expected (", Ext.SrcSpace, ").");

            if (Token[3] != RuntimeSizedArraySize)
                Token[3] = Info.BindPoint + Info.ArraySize - 1;

            Token[5] = Info.Space;
            break;
        }
        case D3D11_SB_OPCODE_DCL_RESOURCE_RAW:
        {
            // 0 - operand info
            // 1 - texture index
            // 2 - first bind point -- remapped in RemapResourceOperand()
            // 3 - last bind point
            // 4 - register space

            VERIFY_EXPR(Token + 5 <= Finish);
            VERIFY_EXPR(Opcode.OpcodeLength > 4);
            VERIFY_EXPR(Operand.OperandType == D3D10_SB_OPERAND_TYPE_RESOURCE);
            VERIFY_EXPR(Operand.IndexDim == D3D10_SB_OPERAND_INDEX_3D);
            VERIFY_EXPR(Operand.OperandIndex1D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);
            VERIFY_EXPR(Operand.OperandIndex2D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);
            VERIFY_EXPR(Operand.OperandIndex3D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);

            const auto& Bindings = BindingsPerType[RES_TYPE_SRV];
            if (Token[1] >= Bindings.size())
                LOG_ERROR_AND_THROW("Invalid texture index (", Token[1], "), the number of textures is (", Bindings.size(), ").");

            const auto& Info = *Bindings[Token[1]];
            const auto& Ext  = ExtResourceMap[&Info];
            VERIFY_EXPR(Info.BindPoint == Token[2]);
            VERIFY_EXPR(Ext.Type == RES_TYPE_SRV);

            if (Token[3] != RuntimeSizedArraySize && Token[3] != Ext.SrcBindPoint + Info.ArraySize - 1)
                LOG_ERROR_AND_THROW("Invalid texture bind point (", Token[3], "), expected (", Ext.SrcBindPoint + Info.ArraySize - 1, ").");
            if (Ext.SrcSpace != Token[4])
                LOG_ERROR_AND_THROW("Invalid texture register space (", Token[4], "), expected (", Ext.SrcSpace, ").");

            if (Token[3] != RuntimeSizedArraySize)
                Token[3] = Info.BindPoint + Info.ArraySize - 1;

            Token[4] = Info.Space;
            break;
        }

        // UAV
        case D3D11_SB_OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED:
        case D3D11_SB_OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED:
        {
            // 0 - operand info
            // 1 - UAV index
            // 2 - first bind point -- remapped in RemapResourceOperand()
            // 3 - last bind point
            // 4 - format? / struct size
            // 5 - register space

            VERIFY_EXPR(Token + 6 <= Finish);
            VERIFY_EXPR(Opcode.OpcodeLength > 5);
            VERIFY_EXPR(Operand.OperandType == D3D11_SB_OPERAND_TYPE_UNORDERED_ACCESS_VIEW);
            VERIFY_EXPR(Operand.IndexDim == D3D10_SB_OPERAND_INDEX_3D);
            VERIFY_EXPR(Operand.OperandIndex1D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);
            VERIFY_EXPR(Operand.OperandIndex2D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);
            VERIFY_EXPR(Operand.OperandIndex3D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);

            const auto& Bindings = BindingsPerType[RES_TYPE_UAV];
            if (Token[1] >= Bindings.size())
                LOG_ERROR_AND_THROW("Invalid UAV index (", Token[1], "), the number of UAVs is (", Bindings.size(), ").");

            const auto& Info = *Bindings[Token[1]];
            const auto& Ext  = ExtResourceMap[&Info];
            VERIFY_EXPR(Info.BindPoint == Token[2]);
            VERIFY_EXPR(Ext.Type == RES_TYPE_UAV);

            if (Token[3] != RuntimeSizedArraySize && Token[3] != Ext.SrcBindPoint + Info.ArraySize - 1)
                LOG_ERROR_AND_THROW("Invalid UAV bind point (", Token[3], "), expected (", Ext.SrcBindPoint + Info.ArraySize - 1, ").");
            if (Ext.SrcSpace != Token[5])
                LOG_ERROR_AND_THROW("Invalid UAV register space (", Token[5], "), expected (", Ext.SrcSpace, ").");

            if (Token[3] != RuntimeSizedArraySize)
                Token[3] = Info.BindPoint + Info.ArraySize - 1;

            Token[5] = Info.Space;
            break;
        }
        case D3D11_SB_OPCODE_DCL_UNORDERED_ACCESS_VIEW_RAW:
        {
            // 0 - operand info
            // 1 - UAV index
            // 2 - first bind point -- remapped in RemapResourceOperand()
            // 3 - last bind point
            // 4 - register space

            VERIFY_EXPR(Token + 5 <= Finish);
            VERIFY_EXPR(Opcode.OpcodeLength > 4);
            VERIFY_EXPR(Operand.OperandType == D3D11_SB_OPERAND_TYPE_UNORDERED_ACCESS_VIEW);
            VERIFY_EXPR(Operand.IndexDim == D3D10_SB_OPERAND_INDEX_3D);
            VERIFY_EXPR(Operand.OperandIndex1D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);
            VERIFY_EXPR(Operand.OperandIndex2D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);
            VERIFY_EXPR(Operand.OperandIndex3D == D3D10_SB_OPERAND_INDEX_IMMEDIATE32);

            const auto& Bindings = BindingsPerType[RES_TYPE_UAV];
            if (Token[1] >= Bindings.size())
                LOG_ERROR_AND_THROW("Invalid UAV index (", Token[1], "), the number of UAVs is (", Bindings.size(), ").");

            const auto& Info = *Bindings[Token[1]];
            const auto& Ext  = ExtResourceMap[&Info];
            VERIFY_EXPR(Info.BindPoint == Token[2]);
            VERIFY_EXPR(Ext.Type == RES_TYPE_UAV);

            if (Token[3] != RuntimeSizedArraySize && Token[3] != Ext.SrcBindPoint + Info.ArraySize - 1)
                LOG_ERROR_AND_THROW("Invalid UAV bind point (", Token[3], "), expected (", Ext.SrcBindPoint + Info.ArraySize - 1, ").");
            if (Ext.SrcSpace != Token[4])
                LOG_ERROR_AND_THROW("Invalid UAV register space (", Token[4], "), expected (", Ext.SrcSpace, ").");

            if (Token[3] != RuntimeSizedArraySize)
                Token[3] = Info.BindPoint + Info.ArraySize - 1;

            Token[4] = Info.Space;
            break;
        }

        default:
            break; // ignore
    }
}

void ShaderBytecodeRemapper::ParseIndex(D3D10_SB_OPERAND_INDEX_REPRESENTATION IndexType, Uint32*& Token, const void* Finish)
{
    switch (IndexType)
    {
        case D3D10_SB_OPERAND_INDEX_IMMEDIATE32:
        {
            Token += 1;
            break;
        }
        case D3D10_SB_OPERAND_INDEX_IMMEDIATE64:
        {
            Token += 2;
            break;
        }
        case D3D10_SB_OPERAND_INDEX_RELATIVE:
        {
            ParseOperand(Token, Finish);
            break;
        }
        case D3D10_SB_OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE:
        {
            Token += 1;
            ParseOperand(Token, Finish);
            break;
        }
        case D3D10_SB_OPERAND_INDEX_IMMEDIATE64_PLUS_RELATIVE:
        {
            Token += 2;
            ParseOperand(Token, Finish);
            break;
        }
        default:
            LOG_ERROR_AND_THROW("Unknown index type");
    }
}

void ShaderBytecodeRemapper::ParseOperand(Uint32*& Token, const void* Finish)
{
    const auto& Operand = *reinterpret_cast<OperandToken*>(Token++);

    Uint32 NumComponents = 0;
    switch (Operand.NumComponents)
    {
        case D3D10_SB_OPERAND_0_COMPONENT: NumComponents = 0; break;
        case D3D10_SB_OPERAND_1_COMPONENT: NumComponents = 1; break;
        case D3D10_SB_OPERAND_4_COMPONENT: NumComponents = 4; break;
        case D3D10_SB_OPERAND_N_COMPONENT:
        default: LOG_ERROR_AND_THROW("Unsupported component count");
    }

    switch (Operand.OperandType)
    {
        case D3D10_SB_OPERAND_TYPE_IMMEDIATE32:
        case D3D10_SB_OPERAND_TYPE_IMMEDIATE64:
            break;
        default:
            if (NumComponents == 4)
            {
                switch (Operand.CompSelection)
                {
                    case D3D10_SB_OPERAND_4_COMPONENT_MASK_MODE:
                    {
                        Uint32 WriteMask = Operand.CompMask;
                        (void)(WriteMask);
                        break;
                    }
                    case D3D10_SB_OPERAND_4_COMPONENT_SWIZZLE_MODE:
                    {
                        Uint32 Swizzle[4];
                        Swizzle[0] = (Operand.CompMask >> 0) & 3;
                        Swizzle[1] = (Operand.CompMask >> 2) & 3;
                        Swizzle[2] = (Operand.CompMask >> 4) & 3;
                        Swizzle[3] = (Operand.CompMask >> 6) & 3;
                        break;
                    }
                    case D3D10_SB_OPERAND_4_COMPONENT_SELECT_1_MODE:
                    {
                        Uint32 Component = Operand.CompMask & 3;
                        (void)(Component);
                        break;
                    }
                    default:
                        LOG_ERROR_AND_THROW("Unknown component selection");
                }
            }
            break;
    }

    if (Operand.Extended)
    {
        Token++; // extended operand type
    }

    switch (Operand.OperandType)
    {
        case D3D10_SB_OPERAND_TYPE_IMMEDIATE32:
        case D3D10_SB_OPERAND_TYPE_IMMEDIATE64:
        {
            Token += NumComponents;
            break;
        }

        case D3D10_SB_OPERAND_TYPE_SAMPLER:
        case D3D10_SB_OPERAND_TYPE_RESOURCE:
        case D3D10_SB_OPERAND_TYPE_CONSTANT_BUFFER:
        case D3D10_SB_OPERAND_TYPE_IMMEDIATE_CONSTANT_BUFFER:
        case D3D11_SB_OPERAND_TYPE_UNORDERED_ACCESS_VIEW:
            RemapResourceOperand(Operand, Token, Finish);
            break;

        default:
            break; // ignore
    }

    if (Operand.IndexDim != D3D10_SB_OPERAND_INDEX_0D)
    {
        if (Operand.IndexDim >= D3D10_SB_OPERAND_INDEX_1D)
            ParseIndex(Operand.OperandIndex1D, Token, Finish);

        if (Operand.IndexDim >= D3D10_SB_OPERAND_INDEX_2D)
            ParseIndex(Operand.OperandIndex2D, Token, Finish);

        if (Operand.IndexDim >= D3D10_SB_OPERAND_INDEX_3D)
            ParseIndex(Operand.OperandIndex3D, Token, Finish);
    }

    VERIFY_EXPR(Token <= Finish);
}

void ShaderBytecodeRemapper::ParseCustomData(Uint32*& Token, const void* Finish, D3D10_SB_CUSTOMDATA_CLASS Type)
{
    switch (Type)
    {
        case D3D10_SB_CUSTOMDATA_COMMENT:
        case D3D10_SB_CUSTOMDATA_DEBUGINFO:
        case D3D10_SB_CUSTOMDATA_OPAQUE:
        case D3D10_SB_CUSTOMDATA_DCL_IMMEDIATE_CONSTANT_BUFFER:
        case D3D11_SB_CUSTOMDATA_SHADER_MESSAGE:
        case D3D11_SB_CUSTOMDATA_SHADER_CLIP_PLANE_CONSTANT_MAPPINGS_FOR_DX9:
            break;
        default:
            LOG_ERROR_AND_THROW("Unknown custom data type");
    }
}

void ShaderBytecodeRemapper::ParseOpcode(Uint32*& Token, const void* Finish)
{
    // based on code from
    // https://github.com/microsoft/D3D12TranslationLayer/blob/master/src/ShaderBinary.cpp
    // Copyright (c) Microsoft Corporation.
    // MIT License

    Uint32*      StartToken        = Token;
    const auto   Opcode            = reinterpret_cast<OpcodeToken&>(*Token++);
    const Uint32 NumOperands       = GetNumOperands(Opcode.OpcodeType);
    Uint32       InstructionLength = Opcode.OpcodeLength;

    VERIFY_EXPR(Opcode.OpcodeType < D3D10_SB_NUM_OPCODES);

    if (Opcode.Extended &&
        (Opcode.OpcodeType == D3D11_SB_OPCODE_DCL_INTERFACE ||
         Opcode.OpcodeType == D3D11_SB_OPCODE_DCL_FUNCTION_TABLE))
    {
        // these instructions may be longer than can fit in the normal instructionlength field
        InstructionLength = *Token++;
    }
    else
    {
        const Uint32 D3D11_SB_MAX_SIMULTANEOUS_EXTENDED_OPCODES = 3;

        bool Extended = (Opcode.Extended > 0);
        for (Uint32 i = 0; i < (Extended ? D3D11_SB_MAX_SIMULTANEOUS_EXTENDED_OPCODES : 0); ++i)
        {
            Uint32 ExtToken = *Token++;
            Extended        = (ExtToken >> 31) > 0;
        }
    }

    switch (Opcode.OpcodeType)
    {
        case D3D10_SB_OPCODE_CUSTOMDATA:
        {
            const auto Type   = static_cast<D3D10_SB_CUSTOMDATA_CLASS>((*(Token - 1) & 0xfffff800) >> 11);
            InstructionLength = *Token;
            VERIFY_EXPR(InstructionLength >= 2);

            ParseCustomData(Token, Finish, Type);
            break;
        }

        // clang-format off
        case D3D11_SB_OPCODE_DCL_FUNCTION_BODY:                Token += 1; break;
        case D3D11_SB_OPCODE_DCL_FUNCTION_TABLE:               Token += 2; break;
        case D3D11_SB_OPCODE_DCL_INTERFACE:                    Token += 3; break;
        case D3D11_SB_OPCODE_INTERFACE_CALL:                   Token += 1; break;
        case D3D10_SB_OPCODE_DCL_TEMPS:                        Token += 1; break;
        case D3D10_SB_OPCODE_DCL_INDEXABLE_TEMP:               Token += 3; break;
        case D3D10_SB_OPCODE_DCL_MAX_OUTPUT_VERTEX_COUNT:      Token += 1; break;
        case D3D11_SB_OPCODE_DCL_GS_INSTANCE_COUNT:            Token += 1; break;
        case D3D11_SB_OPCODE_DCL_HS_MAX_TESSFACTOR:            Token += 1; break;
        case D3D11_SB_OPCODE_DCL_HS_FORK_PHASE_INSTANCE_COUNT: Token += 1; break;
        case D3D11_SB_OPCODE_DCL_HS_JOIN_PHASE_INSTANCE_COUNT: Token += 1; break;
        case D3D11_SB_OPCODE_DCL_THREAD_GROUP:                 Token += 3; break;
            // clang-format on

        case D3D10_SB_OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY:
        case D3D10_SB_OPCODE_DCL_GS_INPUT_PRIMITIVE:
        case D3D10_SB_OPCODE_DCL_GLOBAL_FLAGS:
        case D3D11_SB_OPCODE_DCL_INPUT_CONTROL_POINT_COUNT:
        case D3D11_SB_OPCODE_DCL_OUTPUT_CONTROL_POINT_COUNT:
        case D3D11_SB_OPCODE_DCL_TESS_DOMAIN:
        case D3D11_SB_OPCODE_DCL_TESS_PARTITIONING:
        case D3D11_SB_OPCODE_DCL_TESS_OUTPUT_PRIMITIVE:
        case D3D11_SB_OPCODE_SYNC:
            break;

        default:
            for (Uint32 i = 0; i < NumOperands; ++i)
                ParseOperand(Token, Finish);
            break;
    }

    Uint32* EndToken = StartToken + InstructionLength;
    VERIFY_EXPR(Token <= EndToken);

    RemapResourceBinding(Opcode, StartToken + 1, EndToken);

    Token = EndToken;

    if (Token < Finish)
    {
        const auto NextOpcode = reinterpret_cast<OpcodeToken&>(*Token);
        VERIFY_EXPR(NextOpcode.OpcodeType < D3D10_SB_NUM_OPCODES);
    }
}

void ShaderBytecodeRemapper::PatchBytecode(Uint32* Token, const void* EndPtr) noexcept(false)
{
    VERIFY_EXPR(Token == static_cast<const void*>(&Header + 1));
    const void* Finish = reinterpret_cast<const char*>(&Header) + sizeof(ChunkHeader) + Header.Length;

    VERIFY_EXPR(Header.VersionMajor >= 4);
    VERIFY_EXPR(Header.ProgramType < PROGRAM_TYPE_COUNT_SM5);
    VERIFY_EXPR(Header.NumDWords * 4 == Header.Length);

    if (Finish > EndPtr)
    {
        LOG_ERROR_AND_THROW("Shader code chunk is outside of the specified byte code range. The byte code may be corrupted.");
    }

    for (; Token < Finish;)
    {
        ParseOpcode(Token, Finish);
    }
}

} // namespace


namespace DXBCUtils
{

bool RemapResourceBindings(const TResourceBindingMap& ResourceMap,
                           void*                      pBytecode,
                           size_t                     Size)
{
    if (pBytecode == nullptr)
    {
        LOG_ERROR_MESSAGE("pBytecode must not be null.");
        return false;
    }

    auto* const       Ptr    = static_cast<char*>(pBytecode);
    const void* const EndPtr = Ptr + Size;

    if (Size < sizeof(DXBCHeader))
    {
        LOG_ERROR_MESSAGE("The size of the byte code (", Size, ") is too small to contain the DXBC header. The byte code may be corrupted.");
        return false;
    }

    auto& Header = *reinterpret_cast<DXBCHeader*>(Ptr);
    if (Header.TotalSize != Size)
    {
        LOG_ERROR_MESSAGE("The byte code size (", Header.TotalSize, ") specified in the header does not match the actual size (", Size,
                          "). The byte code may be corrupted.");
        return false;
    }

#ifdef DILIGENT_DEVELOPMENT
    {
        DWORD Checksum[4] = {};
        CalculateDXBCChecksum(reinterpret_cast<BYTE*>(Ptr), static_cast<DWORD>(Size), Checksum);

        DEV_CHECK_ERR((Checksum[0] == Header.Checksum[0] &&
                       Checksum[1] == Header.Checksum[1] &&
                       Checksum[2] == Header.Checksum[2] &&
                       Checksum[3] == Header.Checksum[3]),
                      "Unexpected checksum. The byte code may be corrupted or the container format may have changed.");
    }
#endif

    if (Header.Magic != DXBCFourCC)
    {
        LOG_ERROR_MESSAGE("Bytecode header does not contain the 'DXBC' magic number. The byte code may be corrupted.");
        return false;
    }

    const Uint32*          Chunks = reinterpret_cast<Uint32*>(Ptr + sizeof(Header));
    ResourceBindingPerType BindingsPerType;
    TExtendedResourceMap   ExtResourceMap;

    bool RemapResDef   = false;
    bool RemapBytecode = false;
    try
    {
        for (Uint32 i = 0; i < Header.ChunkCount; ++i)
        {
            auto* pChunk = reinterpret_cast<ChunkHeader*>(Ptr + Chunks[i]);
            if (pChunk + 1 > EndPtr)
            {
                LOG_ERROR_MESSAGE("Not enough space for the chunk header. The byte code may be corrupted.");
                return false;
            }
            if ((Ptr + Chunks[i] + pChunk->Length) > EndPtr)
            {
                LOG_ERROR_MESSAGE("Not enough space for the chunk data. The byte code may be corrupted.");
                return false;
            }

            if (pChunk->Magic == RDEFFourCC)
            {
                auto* RDEFHeader = reinterpret_cast<ResourceDefChunkHeader*>(pChunk);

                if (RDEFHeader->MajorVersion == 5 && RDEFHeader->MinorVersion == 1)
                {
                    RemapShaderResources<ResourceBindingInfo51>(ResourceMap, EndPtr, RDEFHeader, ExtResourceMap, BindingsPerType);
                    RemapResDef = true;
                }
                else if ((RDEFHeader->MajorVersion == 5 && RDEFHeader->MinorVersion == 0) || RDEFHeader->MajorVersion < 5)
                {
                    RemapShaderResources<ResourceBindingInfo50>(ResourceMap, EndPtr, RDEFHeader, ExtResourceMap, BindingsPerType);
                    RemapResDef = true;
                }
                else
                {
                    LOG_ERROR_MESSAGE("Unexpected shader model: ", RDEFHeader->MajorVersion, '.', RDEFHeader->MinorVersion);
                }
            }

            if (pChunk->Magic == SHDRFourCC || pChunk->Magic == SHEXFourCC)
            {
                Uint32*                Token    = reinterpret_cast<Uint32*>(Ptr + Chunks[i] + sizeof(ShaderChunkHeader));
                const auto&            SBHeader = *reinterpret_cast<ShaderChunkHeader*>(pChunk);
                ShaderBytecodeRemapper Remapper{SBHeader, ExtResourceMap, BindingsPerType};

                Remapper.PatchBytecode(Token, EndPtr);
                RemapBytecode = true;
            }
        }
    }
    catch (...)
    {
        return false;
    }

    if (!RemapResDef)
    {
        LOG_ERROR_MESSAGE("Failed to find 'RDEF' chunk with the resource definition.");
        return false;
    }

    if (!RemapBytecode)
    {
        LOG_ERROR_MESSAGE("Failed to find 'SHDR' or 'SHEX' chunk with the shader bytecode.");
        return false;
    }

    // update checksum
    DWORD Checksum[4] = {};
    CalculateDXBCChecksum(reinterpret_cast<BYTE*>(Ptr), static_cast<DWORD>(Size), Checksum);

    static_assert(sizeof(Header.Checksum) == sizeof(Checksum), "Unexpected checksum size");
    memcpy(Header.Checksum, Checksum, sizeof(Header.Checksum));

    return true;
}

} // namespace DXBCUtils

} // namespace Diligent
