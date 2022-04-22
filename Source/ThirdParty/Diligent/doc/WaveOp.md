# Wave operations

## Common names

| GLSL | HLSL | MSL | Description |
|------|------|-----|-------------|
| Subgroup      | Wave       | SIMD | a set of lanes (threads) executed simultaneously in the processor. Other names: wavefront (AMD), warp (NVidia). |
| Invocation    | Lane       | -    | a single thread of execution |
| ..Inclusive.. | -          | -    | a function with these suffix includes lanes from 0 to `LaneIndex` |
| ..Exclusive.. | ..Prefix.. | -    | a function with these suffix includes lanes from 0 to `LaneIndex` but current lane is not included |


## WAVE_FEATURE_BASIC

| GLSL | HLSL | MSL | Description |
|------|------|-----|-------------|
| `uint gl_SubgroupSize`         | `uint WaveGetLaneCount()` | `uint [[threads_per_simdgroup]]`     | size of the wave, `WaveSize` will be used as an alias |
| `uint gl_SubgroupInvocationID` | `uint WaveGetLaneIndex()` | `uint [[thread_index_in_simdgroup]]` | lane index within the wave, `LaneIndex` will be used as an alias |
| `bool subgroupElect()`         | `bool WaveIsFirstLane()`  | `bool simd_is_first()`               | exactly one lane within the wave will return true, the others will return false. The lane that returns true is always the one that is active with the lowest `LaneIndex` |
 
 
## WAVE_FEATURE_VOTE

| GLSL | HLSL | MSL | Description |
|------|------|-----|-------------|
| `bool subgroupAny(bool value)`   | `bool WaveActiveAnyTrue(bool value)` | `bool simd_any(bool value)` | returns true if any active lane has `value == true` |
| `bool subgroupAll(bool value)`   | `bool WaveActiveAllTrue(bool value)` | `bool simd_all(bool value)` | returns true if all active lanes have `value == true` |
| `bool subgroupAllEqual(T value)` | `bool WaveActiveAllEqual(T value)`   | -                           | returns true if all active lanes have a `value` that is equal |
 
 
## WAVE_FEATURE_ARITHMETIC

| GLSL | HLSL | MSL | Description |
|------|------|-----|-------------|
| `T subgroupAdd(T value)`          | `T WaveActiveSum(T value)`     | `T simd_sum(T value)`                      | returns the summation of all active lanes `value`'s across the wave |
| `T subgroupMul(T value)`          | `T WaveActiveProduct(T value)` | `T simd_product(T value)`                  | returns the multiplication of all active lanes `value`'s across the wave |
| `T subgroupMin(T value)`          | `T WaveActiveMin(T value)`     | `T simd_min(T value)`                      | returns the minimum value of all active lanes `value`'s across the wave |
| `T subgroupMax(T value)`          | `T WaveActiveMax(T value)`     | `T simd_max(T value)`                      | returns the maximum value of all active lanes `value`'s across the wave |
| `T subgroupAnd(T value)`          | `T WaveActiveBitAnd(T value)`  | `T simd_and(T value)`                      | returns the binary AND of all active lanes `value`'s across the wave |
| `T subgroupOr(T value)`           | `T WaveActiveBitOr(T value)`   | `T simd_or(T value)`                       | returns the binary OR of all active lanes `value`'s across the wave |
| `T subgroupXor(T value)`          | `T WaveActiveBitXor(T value)`  | `T simd_xor(T value)`                      | returns the binary XOR of all active lanes `value`'s across the wave |
| `T subgroupInclusiveMul(T value)` | -                              | `T simd_prefix_inclusive_product(T value)` | returns the inclusive scan the multiplication of all active lanes `value`'s across the wave |
| `T subgroupInclusiveAdd(T value)` | -                              | `T simd_prefix_inclusive_sum(T value)`     | returns the inclusive scan summation of all active lanes `value`'s across the wave |
| `T subgroupInclusiveMin(T value)` | -                              | -                                          | returns the inclusive scan the minimum value of all active lanes `value`'s across the wave |
| `T subgroupInclusiveMax(T value)` | -                              | -                                          | returns the inclusive scan the maximum value of all active lanes `value`'s across the wave |
| `T subgroupInclusiveAnd(T value)` | -                              | -                                          | returns the inclusive scan the binary AND of all active lanes `value`'s across the wave |
| `T subgroupInclusiveOr(T value)`  | -                              | -                                          | returns the inclusive scan the binary OR of all active lanes `value`'s across the wave |
| `T subgroupInclusiveXor(T value)` | -                              | -                                          | returns the inclusive scan the binary XOR of all active lanes `value`'s across the wave |
| `T subgroupExclusiveAdd(T value)` | T WavePrefixSum(T value)       | `T simd_prefix_exclusive_sum(T value)`     | returns the exclusive scan summation of all active lanes `value`'s across the wave |
| `T subgroupExclusiveMul(T value)` | T WavePrefixProduct(T value)   | `T simd_prefix_exclusive_product (T value)`| returns the exclusive scan the multiplication of all active lanes `value`'s across the wave |
| `T subgroupExclusiveMin(T value)` | -                              | -                                          | returns the exclusive scan the minimum value of all active lanes `value`'s across the wave |
| `T subgroupExclusiveMax(T value)` | -                              | -                                          | returns the exclusive scan the maximum value of all active lanes `value`'s across the wave |
| `T subgroupExclusiveAnd(T value)` | -                              | -                                          | returns the exclusive scan the binary AND of all active lanes `value`'s across the wave |
| `T subgroupExclusiveOr(T value)`  | -                              | -                                          | returns the exclusive scan the binary OR of all active lanes `value`'s across the wave |
| `T subgroupExclusiveXor(T value)` | -                              | -                                          | returns the exclusive scan the binary XOR of all active lanes `value`'s across the wave |
 
 
## WAVE_FEATURE_BALLOUT

| GLSL | HLSL | MSL | Description |
|------|------|-----|-------------|
| `uvec4 subgroupBallot(bool value)`                                  | `uint4 WaveActiveBallot(bool value)`   | `simd_vote simd_ballot(bool value)`    | each lane contributes a single bit to the resulting `uvec4` corresponding to `value` |
| `T subgroupBroadcast(T value, uint id)`                             | `T WaveReadLaneAt(T value, uint id)`   | `T simd_broadcast(T value, ushort id)` | broadcasts the `value` whose `LaneIndex == id` to all other lanes (id must be a compile time constant) |
| `T subgroupBroadcastFirst(T value)`                                 | `T WaveReadLaneFirst(T value)`         | `T simd_broadcast_first(T value)`      | broadcasts the `value` whose `LaneIndex` is the lowest active to all other lanes |
| `bool subgroupInverseBallot(uvec4 value)`                           | -                                      | -                                      | returns true if this lanes bit in `value` is true |
| `bool subgroupBallotBitExtract(uvec4 value, uint index)`            | -                                      | -                                      | returns true if the bit corresponding to `index` is set in `value` |
| `uint subgroupBallotBitCount(uvec4 value)`                          | -                                      | -                                      | returns the number of bits set in `value`, only counting the bottom `WaveSize` bits |
| `uint subgroupBallotInclusiveBitCount(uvec4 value)`                 | -                                      | -                                      | returns the inclusive scan of the number of bits set in `value`, only counting the bottom `WaveSize` bits (we'll cover what an inclusive scan is later) |
| `uint subgroupBallotExclusiveBitCount(uvec4 value)`                 | -                                      | -                                      | returns the exclusive scan of the number of bits set in `value`, only counting the bottom `WaveSize` bits (we'll cover what an exclusive scan is later) |
| `uint subgroupBallotFindLSB(uvec4 value)`                           | -                                      | -                                      | returns the lowest bit set in `value`, only counting the bottom `WaveSize` bits |
| `uint subgroupBallotFindMSB(uvec4 value)`                           | -                                      | -                                      | returns the highest bit set in `value`, only counting the bottom `WaveSize` bits |
| `uint subgroupBallotBitCount( subgroupBallot(bool value))`          | `uint WaveActiveCountBits(bool value)` | -                                      | counts the number of boolean variables which evaluate to true across all active lanes in the current wave, and replicates the result to all lanes in the wave |
| `uint subgroupBallotExclusiveBitCount( subgroupBallot(bool value))` | `uint WavePrefixCountBits(bool value)` | -                                      | returns the sum of all the specified boolean variables set to true across all active lanes with indices smaller than the current lane |

## WAVE_FEATURE_SHUFFLE

| GLSL | HLSL | MSL | Description |
|------|------|-----|-------------|
| `T subgroupShuffle(T value, uint index)`   | - | `T simd_shuffle(T value, ushort index)`    | returns the `value` whose `LaneIndex` is equal to `index` |
| `T subgroupShuffleXor(T value, uint mask)` | - | `T simd_shuffle_xor(T value, ushort mask)` | returns the `value` whose `LaneIndex` is equal to the current lanes `LaneIndex` xor'ed with `mask` |
 
 
## WAVE_FEATURE_SHUFFLE_RELATIVE

| GLSL | HLSL | MSL | Description |
|------|---|---|---|
| `T subgroupShuffleUp(T value, uint delta)`   | - | `T simd_shuffle_up(T value, ushort delta)`   | returns the `value` whose `LaneIndex` is equal to the current lanes `LaneIndex` minus `delta` |
| `T subgroupShuffleDown(T value, uint delta)` | - | `T simd_shuffle_down(T value, ushort delta)` | returns the `value` whose `LaneIndex` is equal to the current lanes `LaneIndex` plus `delta` |
 
 
## WAVE_FEATURE_CLUSTERED

| GLSL | HLSL | MSL | Description |
|------|------|-----|-------------|
| `T subgroupClusteredAdd(T value, uint clusterSize)` | - | - | returns the summation of all active lanes `value`'s across clusters of size `clusterSize` |
| `T subgroupClusteredMul(T value, uint clusterSize)` | - | - | returns the multiplication of all active lanes `value`'s across clusters of size `clusterSize` |
| `T subgroupClusteredMin(T value, uint clusterSize)` | - | - | returns the minimum value of all active lanes `value`'s across clusters of size `clusterSize` |
| `T subgroupClusteredMax(T value, uint clusterSize)` | - | - | returns the maximum value of all active lanes `value`'s across clusters of size `clusterSize` |
| `T subgroupClusteredAnd(T value, uint clusterSize)` | - | - | returns the binary AND of all active lanes `value`'s across clusters of size `clusterSize` |
| `T subgroupClusteredOr(T value, uint clusterSize)`  | - | - | returns the binary OR of all active lanes `value`'s across clusters of size `clusterSize` |
| `T subgroupClusteredXor(T value, uint clusterSize)` | - | - | returns the binary XOR of all active lanes `value`'s across clusters of size `clusterSize` |
 
 
## WAVE_FEATURE_QUAD
Quad operations executes on 2x2 grid in pixel and compute shaders.

| GLSL | HLSL | MSL | Description |
|------|------|-----|-------------|
| `T subgroupQuadBroadcast(T value, uint id)` | `T QuadReadLaneAt(T value, uint id)` | `T quad_broadcast(T value, ushort id)` | returns the `value` in the quad whose `LaneIndex` modulus 4 is equal to `id` |
| `T subgroupQuadSwapHorizontal(T value)`     | `T QuadReadAcrossX(T value)`         | -                                      | swaps `value`'s within the quad horizontally |
| `T subgroupQuadSwapVertical(T value)`       | `T QuadReadAcrossY(T value)`         | -                                      | swaps `value`'s within the quad vertically |
| `T subgroupQuadSwapDiagonal(T value)`       | `T QuadReadAcrossDiagonal(T value)`  | -                                      | swaps `value`'s within the quad diagonally |
 
 
## References

GLSL<br/>
https://www.khronos.org/blog/vulkan-subgroup-tutorial<br/>
https://raw.githubusercontent.com/KhronosGroup/GLSL/master/extensions/khr/GL_KHR_shader_subgroup.txt<br/>

HLSL<br/>
https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/hlsl-shader-model-6-0-features-for-direct3d-12<br/>
https://github.com/Microsoft/DirectXShaderCompiler/wiki/Wave-Intrinsics<br/>

MSL<br/>
https://developer.apple.com/metal/Metal-Shading-Language-Specification.pdf (6.8.2 SIMD-group Functions, 6.8.3 Quad-group Functions)
