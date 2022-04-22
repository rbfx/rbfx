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

#pragma once

#ifndef GL_KHR_shader_subgroup
#    define GL_KHR_shader_subgroup                       1
#    define GL_SUBGROUP_SIZE_KHR                         0x9532
#    define GL_SUBGROUP_SUPPORTED_STAGES_KHR             0x9533
#    define GL_SUBGROUP_SUPPORTED_FEATURES_KHR           0x9534
#    define GL_SUBGROUP_QUAD_ALL_STAGES_KHR              0x9535
#    define GL_SUBGROUP_FEATURE_BASIC_BIT_KHR            0x00000001
#    define GL_SUBGROUP_FEATURE_VOTE_BIT_KHR             0x00000002
#    define GL_SUBGROUP_FEATURE_ARITHMETIC_BIT_KHR       0x00000004
#    define GL_SUBGROUP_FEATURE_BALLOT_BIT_KHR           0x00000008
#    define GL_SUBGROUP_FEATURE_SHUFFLE_BIT_KHR          0x00000010
#    define GL_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT_KHR 0x00000020
#    define GL_SUBGROUP_FEATURE_CLUSTERED_BIT_KHR        0x00000040
#    define GL_SUBGROUP_FEATURE_QUAD_BIT_KHR             0x00000080
#endif /* GL_KHR_shader_subgroup */
