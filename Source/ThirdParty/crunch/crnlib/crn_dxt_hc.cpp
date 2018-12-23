// File: crn_dxt_hc.cpp
// See Copyright Notice and license at the end of inc/crnlib.h
#include "crn_core.h"
#include "crn_dxt_hc.h"
#include "crn_image_utils.h"
#include "crn_console.h"
#include "crn_dxt_fast.h"
#include "crn_etc.h"

namespace crnlib {

typedef vec<6, float> vec6F;
typedef vec<16, float> vec16F;

static uint8 g_tile_map[8][2][2] = {
  {{ 0, 0 }, { 0, 0 }},
  {{ 0, 0 }, { 1, 1 }},
  {{ 0, 1 }, { 0, 1 }},
  {{ 0, 0 }, { 1, 2 }},
  {{ 1, 2 }, { 0, 0 }},
  {{ 0, 1 }, { 0, 2 }},
  {{ 1, 0 }, { 2, 0 }},
  {{ 0, 1 }, { 2, 3 }},
};

dxt_hc::dxt_hc()
  : m_num_blocks(0),
    m_has_color_blocks(false),
    m_has_etc_color_blocks(false),
    m_num_alpha_blocks(0),
    m_main_thread_id(crn_get_current_thread_id()),
    m_canceled(false),
    m_pTask_pool(NULL),
    m_prev_phase_index(-1),
    m_prev_percentage_complete(-1) {
}

dxt_hc::~dxt_hc() {
}

void dxt_hc::clear() {
  m_blocks = 0;
  m_num_blocks = 0;
  m_num_alpha_blocks = 0;
  m_has_color_blocks = false;

  m_color_clusters.clear();
  m_alpha_clusters.clear();

  m_canceled = false;

  m_prev_phase_index = -1;
  m_prev_percentage_complete = -1;

  m_block_weights.clear();
  m_block_encodings.clear();
  for (uint c = 0; c < 3; c++)
    m_block_selectors[c].clear();
  m_color_selectors.clear();
  m_alpha_selectors.clear();
  m_color_selectors_used.clear();
  m_alpha_selectors_used.clear();
  m_tile_indices.clear();
  m_endpoint_indices.clear();
  m_selector_indices.clear();
  m_tiles.clear();
  m_num_tiles = 0;
}

bool dxt_hc::compress(
    color_quad_u8 (*blocks)[16],
    crnlib::vector<endpoint_indices_details>& endpoint_indices,
    crnlib::vector<selector_indices_details>& selector_indices,
    crnlib::vector<uint32>& color_endpoints,
    crnlib::vector<uint32>& alpha_endpoints,
    crnlib::vector<uint32>& color_selectors,
    crnlib::vector<uint64>& alpha_selectors,
    const params& p
  ) {
  clear();
  m_has_etc_color_blocks = p.m_format == cETC1 || p.m_format == cETC2 || p.m_format == cETC2A;
  m_has_color_blocks = p.m_format == cDXT1 || p.m_format == cDXT5 || m_has_etc_color_blocks;
  m_num_alpha_blocks = p.m_format == cDXT5 || p.m_format == cDXT5A || p.m_format == cETC2A ? 1 : p.m_format == cDXN_XY || p.m_format == cDXN_YX ? 2 : 0;
  if (!m_has_color_blocks && !m_num_alpha_blocks)
    return false;
  m_blocks = blocks;
  m_main_thread_id = crn_get_current_thread_id();
  m_pTask_pool = p.m_pTask_pool;
  m_params = p;

  uint tile_derating[8] = {0, 1, 1, 2, 2, 2, 2, 3};
  for (uint level = 0; level < p.m_num_levels; level++) {
    float adaptive_tile_color_psnr_derating = p.m_adaptive_tile_color_psnr_derating;
    if (level && adaptive_tile_color_psnr_derating > .25f)
      adaptive_tile_color_psnr_derating = math::maximum(.25f, adaptive_tile_color_psnr_derating / powf(3.0f, static_cast<float>(level)));
    for (uint e = 0; e < 8; e++)
      m_color_derating[level][e] = math::lerp(0.0f, adaptive_tile_color_psnr_derating, tile_derating[e] / 3.0f);
  }
  for (uint e = 0; e < 8; e++)
    m_alpha_derating[e] = math::lerp(0.0f, m_params.m_adaptive_tile_alpha_psnr_derating, tile_derating[e] / 3.0f);
  for (uint i = 0; i < 256; i++)
    m_uint8_to_float[i] = i * 1.0f / 255.0f;

  m_num_blocks = m_params.m_num_blocks;
  m_block_weights.resize(m_num_blocks);
  m_block_encodings.resize(m_num_blocks);
  for (uint c = 0; c < 3; c++)
    m_block_selectors[c].resize(m_num_blocks);
  m_tile_indices.resize(m_num_blocks);
  m_endpoint_indices.resize(m_num_blocks);
  m_selector_indices.resize(m_num_blocks);
  m_tiles.resize(m_num_blocks);

  for (uint level = 0; level < p.m_num_levels; level++) {
    float weight = p.m_levels[level].m_weight;
    for (uint b = p.m_levels[level].m_first_block, bEnd = b + p.m_levels[level].m_num_blocks; b < bEnd; b++)
      m_block_weights[b] = weight;
  }

  for (uint i = 0; i <= m_pTask_pool->get_num_threads(); i++)
    m_pTask_pool->queue_object_task(this, m_has_etc_color_blocks ? &dxt_hc::determine_tiles_task_etc : &dxt_hc::determine_tiles_task, i);
  m_pTask_pool->join();

  m_num_tiles = 0;
  for (uint t = 0; t < m_tiles.size(); t++) {
    if (m_tiles[t].pixels.size())
      m_num_tiles++;
  }

  if (m_has_color_blocks)
    determine_color_endpoints();

  if (m_num_alpha_blocks)
    determine_alpha_endpoints();

  if (m_has_color_blocks)
    create_color_selector_codebook();

  if (m_num_alpha_blocks)
    create_alpha_selector_codebook();

  color_endpoints.reserve(color_endpoints.size() + m_color_clusters.size());
  crnlib::vector<uint16> color_endpoints_remap(m_color_clusters.size());
  hash_map<uint32, uint> color_endpoints_map;
  for (uint i = 0; i < m_color_clusters.size(); i++) {
    if (m_color_clusters[i].pixels.size()) {
      uint32 endpoint = m_has_etc_color_blocks ? m_color_clusters[i].first_endpoint :
        dxt1_block::pack_endpoints(m_color_clusters[i].first_endpoint, m_color_clusters[i].second_endpoint);
      hash_map<uint32, uint>::insert_result insert_result = color_endpoints_map.insert(endpoint, color_endpoints.size());
      if (insert_result.second) {
        color_endpoints_remap[i] = color_endpoints.size();
        color_endpoints.push_back(endpoint);
      } else {
        color_endpoints_remap[i] = insert_result.first->second;
      }
    }
  }

  alpha_endpoints.reserve(alpha_endpoints.size() + m_alpha_clusters.size());
  crnlib::vector<uint16> alpha_endpoints_remap(m_alpha_clusters.size());
  hash_map<uint32, uint> alpha_endpoints_map;
  for (uint i = 0; i < m_alpha_clusters.size(); i++) {
    if (m_alpha_clusters[i].pixels.size()) {
      uint32 endpoint = dxt5_block::pack_endpoints(m_alpha_clusters[i].first_endpoint, m_alpha_clusters[i].second_endpoint);
      hash_map<uint32, uint>::insert_result insert_result = alpha_endpoints_map.insert(endpoint, alpha_endpoints.size());
      if (insert_result.second) {
        alpha_endpoints_remap[i] = alpha_endpoints.size();
        alpha_endpoints.push_back(endpoint);
      } else {
        alpha_endpoints_remap[i] = insert_result.first->second;
      }
    }
  }

  color_selectors.reserve(color_selectors.size() + m_color_selectors.size());
  crnlib::vector<uint16> color_selectors_remap(m_color_selectors.size());
  hash_map<uint32, uint> color_selectors_map;
  for (uint i = 0; i < m_color_selectors.size(); i++) {
    if (m_color_selectors_used[i]) {
      hash_map<uint32, uint>::insert_result insert_result = color_selectors_map.insert(m_color_selectors[i], color_selectors.size());
      if (insert_result.second) {
        color_selectors_remap[i] = color_selectors.size();
        color_selectors.push_back(m_color_selectors[i]);
      } else {
        color_selectors_remap[i] = insert_result.first->second;
      }
    }
  }

  alpha_selectors.reserve(alpha_selectors.size() + m_alpha_selectors.size());
  crnlib::vector<uint16> alpha_selectors_remap(m_alpha_selectors.size());
  hash_map<uint64, uint> alpha_selectors_map;
  for (uint i = 0; i < m_alpha_selectors.size(); i++) {
    if (m_alpha_selectors_used[i]) {
      hash_map<uint64, uint>::insert_result insert_result = alpha_selectors_map.insert(m_alpha_selectors[i], alpha_selectors.size());
      if (insert_result.second) {
        alpha_selectors_remap[i] = alpha_selectors.size();
        alpha_selectors.push_back(m_alpha_selectors[i]);
      } else {
        alpha_selectors_remap[i] = insert_result.first->second;
      }
    }
  }

  endpoint_indices.resize(m_num_blocks);
  selector_indices.resize(m_num_blocks);
  for (uint level = 0; level < p.m_num_levels; level++) {
    uint first_block = p.m_levels[level].m_first_block;
    uint end_block = first_block + p.m_levels[level].m_num_blocks;
    uint block_width = p.m_levels[level].m_block_width;
    for (uint by = 0, b = first_block; b < end_block; by++) {
      for (uint bx = 0; bx < block_width; bx++, b++) {
        bool top_match = by != 0;
        bool left_match = top_match || bx;
        bool diag_match = m_has_etc_color_blocks && top_match && bx;
        for (uint c = m_has_color_blocks ? 0 : cAlpha0; c < cAlpha0 + m_num_alpha_blocks; c++) {
          uint16 endpoint_index = (c ? alpha_endpoints_remap : color_endpoints_remap)[m_endpoint_indices[b].component[c]];
          left_match = left_match && endpoint_index == endpoint_indices[b - 1].component[c];
          top_match = top_match && endpoint_index == endpoint_indices[b - block_width].component[c];
          diag_match = diag_match && endpoint_index == endpoint_indices[b - block_width - 1].component[c];
          endpoint_indices[b].component[c] = endpoint_index;
          uint16 selector_index = (c ? alpha_selectors_remap : color_selectors_remap)[m_selector_indices[b].component[c]];
          selector_indices[b].component[c] = selector_index;
        }
        endpoint_indices[b].reference = m_has_etc_color_blocks && b & 1 ? m_endpoint_indices[b].reference : left_match ? 1 : top_match ? 2 : diag_match ? 3 : 0;
      }
    }
  }

  m_pTask_pool = NULL;
  return true;
}

vec6F dxt_hc::palettize_color(color_quad_u8* pixels, uint pixels_count) {
  uint color[64];
  for (uint i = 0; i < pixels_count; i++)
    color[i] = pixels[i][0] << 16 | pixels[i][1] << 8 | pixels[i][2];
  std::sort(color, color + pixels_count);
  vec3F vectors[64];
  uint weights[64];
  uint size = 0;
  for (uint i = 0; i < pixels_count; i++) {
    if (!i || color[i] != color[i - 1]) {
      vectors[size][0] = m_params.m_perceptual ? m_uint8_to_float[color[i] >> 16] * 0.5f : m_uint8_to_float[color[i] >> 16];
      vectors[size][1] = m_uint8_to_float[color[i] >> 8 & 0xFF];
      vectors[size][2] = m_params.m_perceptual ? m_uint8_to_float[color[i] & 0xFF] * 0.25f : m_uint8_to_float[color[i] & 0xFF];
      weights[size] = 1;
      size++;
    } else {
      weights[size - 1]++;
    }
  }
  vec3F result[2];
  split_vectors<vec3F>(vectors, weights, size, result);
  if (result[0].length() > result[1].length())
    utils::swap(result[0], result[1]);
  return *(vec6F*)result;
}

vec2F dxt_hc::palettize_alpha(color_quad_u8* pixels, uint pixels_count, uint comp_index) {
  uint8 alpha[64];
  for (uint p = 0; p < pixels_count; p++)
    alpha[p] = pixels[p][comp_index];
  std::sort(alpha, alpha + pixels_count);
  vec1F vectors[64];
  uint weights[64];
  uint size = 0;
  for (uint i = 0; i < pixels_count; i++) {
    if (!i || alpha[i] != alpha[i - 1]) {
      vectors[size][0] = m_uint8_to_float[alpha[i]];
      weights[size] = 1;
      size++;
    } else {
      weights[size - 1]++;
    }
  }
  vec1F result[2];
  split_vectors<vec1F>(vectors, weights, size, result);
  if (result[0] > result[1])
    utils::swap(result[0], result[1]);
  return *(vec2F*)result;
}

void dxt_hc::determine_tiles_task(uint64 data, void*) {
  uint num_tasks = m_pTask_pool->get_num_threads() + 1;
  uint offsets[9] = {0, 16, 32, 48, 0, 32, 64, 96, 64};
  uint8 tiles[8][4] = {{8}, {6, 7}, {4, 5}, {6, 1, 3}, {7, 0, 2}, {4, 2, 3}, {5, 0, 1}, {0, 2, 1, 3}};
  color_quad_u8 tilePixels[128];
  uint8 selectors[64];
  uint tile_error[3][9];
  uint total_error[3][8];

  for (uint level = 0; level < m_params.m_num_levels; level++) {
    float weight = m_params.m_levels[level].m_weight;
    uint width = m_params.m_levels[level].m_block_width;
    uint height = m_params.m_levels[level].m_num_blocks / width;
    uint faceHeight = height / m_params.m_num_faces;
    uint h = height * data / num_tasks & ~1;
    uint hEnd = height * (data + 1) / num_tasks & ~1;
    uint hFace = h % faceHeight;
    uint b = m_params.m_levels[level].m_first_block + h * width;

    for (; h < hEnd; h += 2, hFace += 2, b += width) {
      uint tile_offset = b;
      uint tile_offset_delta = 4;
      if (hFace == faceHeight) {
        hFace = 0;
      } else if (hFace & 2) {
        tile_offset_delta = -4;
        tile_offset += (width << 1) + tile_offset_delta;
      }
      for (uint bNext = b + width; b < bNext; b += 2, tile_offset += tile_offset_delta) {
        for (int t = 0; t < 64; t += 16)
          memcpy(tilePixels + t, m_blocks[b + (t & 16 ? width : 0) + (t & 32 ? 1 : 0)], 64);
        for (int t = 0; t < 64; t += 4)
          memcpy(tilePixels + 64 + t, m_blocks[b + (t & 32 ? width : 0) + (t & 4 ? 1 : 0)] + (t >> 1 & 12), 16);

        for (uint t = 0; t < 9; t++) {
          color_quad_u8* pixels = tilePixels + offsets[t];
          uint size = 16 << (t >> 2);
          if (m_has_color_blocks) {
            uint low16, high16;
            dxt_fast::compress_color_block(size, pixels, low16, high16, selectors);
            color_quad_u8 block_colors[4];
            dxt1_block::get_block_colors4(block_colors, low16, high16);
            uint error = 0;
            for (uint p = 0; p < size; p++) {
              for (uint8 c = 0; c < 3; c++) {
                uint delta = pixels[p][c] - block_colors[selectors[p]][c];
                error += delta * delta;
              }
            }
            tile_error[cColor][t] = error;
          }
          for (uint a = 0; a < m_num_alpha_blocks; a++) {
            uint8 component = m_params.m_alpha_component_indices[a];
            dxt5_endpoint_optimizer optimizer;
            dxt5_endpoint_optimizer::params params;
            dxt5_endpoint_optimizer::results results;
            params.m_pPixels = pixels;
            params.m_num_pixels = size;
            params.m_comp_index = component;
            params.m_use_both_block_types = false;
            params.m_quality = cCRNDXTQualityNormal;
            results.m_pSelectors = selectors;
            optimizer.compute(params, results);
            uint block_values[cDXT5SelectorValues];
            dxt5_block::get_block_values8(block_values, results.m_first_endpoint, results.m_second_endpoint);
            tile_error[cAlpha0 + a][t] = results.m_error;
          }
        }

        for (uint8 c = m_has_color_blocks ? 0 : cAlpha0; c < cAlpha0 + m_num_alpha_blocks; c++) {
          for (uint8 e = 0; e < 8; e++) {
            total_error[c][e] = 0;
            for (uint8 t = 0, s = e + 1; s; s >>= 1, t++)
              total_error[c][e] += tile_error[c][tiles[e][t]];
          }
        }

        float best_quality = 0.0f;
        uint best_encoding = 0;
        for (uint e = 0; e < 8; e++) {
          float quality = 0;
          if (m_has_color_blocks) {
            double peakSNR = total_error[cColor][e] ? log10(255.0f / sqrt(total_error[cColor][e] / 192.0)) * 20.0f : 999999.0f;
            quality = (float)math::maximum<double>(peakSNR - m_color_derating[level][e], 0.0f);
            if (m_num_alpha_blocks)
              quality *= m_params.m_adaptive_tile_color_alpha_weighting_ratio;
          }
          for (uint a = 0; a < m_num_alpha_blocks; a++) {
            double peakSNR = total_error[cAlpha0 + a][e] ? log10(255.0f / sqrt(total_error[cAlpha0 + a][e] / 64.0)) * 20.0f : 999999.0f;
            quality += (float)math::maximum<double>(peakSNR - m_alpha_derating[e], 0.0f);
          }
          if (quality > best_quality) {
            best_quality = quality;
            best_encoding = e;
          }
        }
    
        for (uint tile_index = 0, s = best_encoding + 1; s; s >>= 1, tile_index++) {
          tile_details& tile = m_tiles[tile_offset | tile_index];
          uint t = tiles[best_encoding][tile_index];
          tile.pixels.append(tilePixels + offsets[t], 16 << (t >> 2));
          tile.weight = weight;
          if (m_has_color_blocks)
            tile.color_endpoint = palettize_color(tile.pixels.get_ptr(), tile.pixels.size());
          for (uint a = 0; a < m_num_alpha_blocks; a++)
            tile.alpha_endpoints[a] = palettize_alpha(tile.pixels.get_ptr(), tile.pixels.size(), m_params.m_alpha_component_indices[a]);
        }

        for (uint by = 0; by < 2; by++) {
          for (uint bx = 0; bx < 2; bx++) {
            m_block_encodings[b + (by ? width : 0) + bx] = best_encoding;
            m_tile_indices[b + (by ? width : 0) + bx] = tile_offset | g_tile_map[best_encoding][by][bx];
          }
        }

      }
    }
  }
}

void dxt_hc::determine_tiles_task_etc(uint64 data, void*) {
  uint num_tasks = m_pTask_pool->get_num_threads() + 1;
  uint offsets[5] = {0, 8, 16, 24, 16};
  uint8 tiles[3][2] = {{4}, {2, 3}, {0, 1}};
  uint8 tile_map[3][2] = {{ 0, 0 }, { 0, 1 }, { 0, 1 }};
  color_quad_u8 tilePixels[32];
  uint8 selectors[32];
  uint tile_error[5];
  uint total_error[3];

  etc1_optimizer optimizer;
  etc1_optimizer::params params;
  params.m_use_color4 = false;
  params.m_constrain_against_base_color5 = false;
  etc1_optimizer::results results;
  results.m_pSelectors = selectors;
  int scan[] = {-1, 0, 1};
  int refine[] = {-3, -2, 2, 3};

  for (uint level = 0; level < m_params.m_num_levels; level++) {
    float weight = m_params.m_levels[level].m_weight;
    uint b = (m_params.m_levels[level].m_first_block + m_params.m_levels[level].m_num_blocks * data / num_tasks) & ~1;
    uint bEnd = (m_params.m_levels[level].m_first_block + m_params.m_levels[level].m_num_blocks * (data + 1) / num_tasks) & ~1;
    for (; b < bEnd; b += 2) {
      for (uint p = 0; p < 16; p++)
        tilePixels[p] = m_blocks[b >> 1][(p << 2 & 12) | p >> 2];
      memcpy(tilePixels + 16, m_blocks[b >> 1], 64);
      for (uint t = 0; t < 5; t++) {
        params.m_pSrc_pixels = tilePixels + offsets[t];
        params.m_num_src_pixels = results.m_n = 8 << (t >> 2);
        optimizer.init(params, results);
        params.m_pScan_deltas = scan;
        params.m_scan_delta_size = sizeof(scan) / sizeof(*scan);
        optimizer.compute();
        if (results.m_error > 375 * params.m_num_src_pixels) {
          params.m_pScan_deltas = refine;
          params.m_scan_delta_size = sizeof(refine) / sizeof(*refine);
          optimizer.compute();
        }
        tile_error[t] = results.m_error;
      }

      for (uint8 e = 0; e < 3; e++) {
        total_error[e] = 0;
        for (uint8 t = 0, s = e + 1; s; s >>= 1, t++)
          total_error[e] += tile_error[tiles[e][t]];
      }

      float best_quality = 0.0f;
      uint best_encoding = 0;
      for (uint e = 0; e < 3; e++) {
        float quality = 0;
        double peakSNR = total_error[e] ? log10(255.0f / sqrt(total_error[e] / 48.0)) * 20.0f : 999999.0f;
        quality = (float)math::maximum<double>(peakSNR - m_color_derating[level][e], 0.0f);
        if (quality > best_quality) {
          best_quality = quality;
          best_encoding = e;
        }
      }

      vec2F alpha_endpoints = m_num_alpha_blocks ? palettize_alpha(tilePixels, 16, 3) : vec2F(cClear);
      for (uint tile_index = 0, s = best_encoding + 1; s; s >>= 1, tile_index++) {
        tile_details& tile = m_tiles[b | tile_index];
        uint t = tiles[best_encoding][tile_index];
        tile.pixels.append(tilePixels + offsets[t], 8 << (t >> 2));
        tile.weight = weight;
        tile.color_endpoint = palettize_color(tile.pixels.get_ptr(), tile.pixels.size());
        if (m_num_alpha_blocks)
          tile.alpha_endpoints[0] = alpha_endpoints;
      }

      for (uint bx = 0; bx < 2; bx++) {
        m_block_encodings[b | bx] = best_encoding;
        m_tile_indices[b | bx] = b | tile_map[best_encoding][bx];
        m_endpoint_indices[b | bx].reference = bx ? best_encoding : 0;
      }
      if (best_encoding >> 1)
        memcpy(m_blocks[b >> 1], tilePixels, 64);
    }
  }
}

void dxt_hc::determine_color_endpoint_codebook_task(uint64 data, void*) {
  const uint num_tasks = m_pTask_pool->get_num_threads() + 1;
  dxt1_endpoint_optimizer optimizer;
  dxt_endpoint_refiner refiner;
  crnlib::vector<uint8> selectors;

  for (uint cluster_index = (uint)data; cluster_index < m_color_clusters.size(); cluster_index += num_tasks) {
    color_cluster& cluster = m_color_clusters[cluster_index];
    if (cluster.pixels.empty())
      continue;

    dxt1_endpoint_optimizer::params params;
    params.m_block_index = cluster_index;
    params.m_pPixels = cluster.pixels.get_ptr();
    params.m_num_pixels = cluster.pixels.size();
    params.m_pixels_have_alpha = false;
    params.m_use_alpha_blocks = false;
    params.m_perceptual = m_params.m_perceptual;
    params.m_quality = cCRNDXTQualityUber;
    params.m_endpoint_caching = false;

    dxt1_endpoint_optimizer::results results;
    selectors.resize(params.m_num_pixels);
    results.m_pSelectors = selectors.get_ptr();

    optimizer.compute(params, results);
    cluster.first_endpoint = results.m_low_color;
    cluster.second_endpoint = results.m_high_color;
    color_quad_u8 block_values[4], color_values[4];
    dxt1_block::get_block_colors4(block_values, cluster.first_endpoint, cluster.second_endpoint);
    for (uint i = 0; i < 4; i++)
      color_values[i] = cluster.color_values[i] = block_values[g_dxt1_from_linear[i]];
    for (uint c = 0; results.m_alternate_rounding && c < 3; c++) {
      color_values[1].c[c] = ((color_values[0].c[c] << 1) + color_values[3].c[c] + 1) / 3;
      color_values[2].c[c] = ((color_values[3].c[c] << 1) + color_values[0].c[c] + 1) / 3;
    }

    uint endpoint_weight = color::color_distance(m_params.m_perceptual, color_values[0], color_values[3], false) / 2000;
    float encoding_weight[8];
    for (uint i = 0; i < 8; i++)
      encoding_weight[i] = math::lerp(1.15f, 1.0f, i / 7.0f);

    crnlib::vector<uint>& blocks = cluster.blocks[cColor];
    for (uint i = 0; i < blocks.size(); i++) {
      uint b = blocks[i];
      uint weight = (uint)(math::clamp<uint>(endpoint_weight * m_block_weights[b], 1, 2048) * encoding_weight[m_block_encodings[b]]);
      uint32 selector = 0;
      for (uint p = 0; p < 16; p++) {
        uint error_best = cUINT32_MAX;
        uint8 s_best = 0;
        for (uint8 t = 0; t < 4; t++) {
          uint8 s = results.m_reordered ? 3 - g_dxt1_to_linear[t] : g_dxt1_to_linear[t];
          uint error = color::color_distance(m_params.m_perceptual, (color_quad_u8&)m_blocks[b][p], color_values[s], false);
          if (error < error_best) {
            s_best = s;
            error_best = error;
          }
        }
        selector = selector << 2 | s_best;
      }
      m_block_selectors[cColor][b] = (uint64)selector << 32 | weight;
    }

    dxt_endpoint_refiner::params refinerParams;
    dxt_endpoint_refiner::results refinerResults;
    refinerParams.m_perceptual = m_params.m_perceptual;
    refinerParams.m_pSelectors = selectors.get_ptr();
    refinerParams.m_pPixels = cluster.pixels.get_ptr();
    refinerParams.m_num_pixels = cluster.pixels.size();
    refinerParams.m_dxt1_selectors = true;
    refinerParams.m_error_to_beat = results.m_error;
    refinerParams.m_block_index = cluster_index;
    if (refiner.refine(refinerParams, refinerResults)) {
      cluster.first_endpoint = refinerResults.m_low_color;
      cluster.second_endpoint = refinerResults.m_high_color;
    }
  }
}

void dxt_hc::determine_color_endpoint_codebook_task_etc(uint64 data, void*) {
  uint num_tasks = m_pTask_pool->get_num_threads() + 1;
  uint8 delta[8][2] = { {2, 8}, {5, 17}, {9, 29}, {13, 42}, {18, 60}, {24, 80}, {33, 106}, {47, 183} };
  int scan[] = {-1, 0, 1};
  int refine[] = {-3, -2, 2, 3};
  for (uint iCluster = m_color_clusters.size() * data / num_tasks, iEnd = m_color_clusters.size() * (data + 1) / num_tasks; iCluster < iEnd; iCluster++) {
    color_cluster& cluster = m_color_clusters[iCluster];
    if (cluster.pixels.size()) {
      etc1_optimizer optimizer;
      etc1_optimizer::params params;
      params.m_use_color4 = false;
      params.m_constrain_against_base_color5 = false;
      etc1_optimizer::results results;
      crnlib::vector<uint8> selectors(cluster.pixels.size());
      params.m_pSrc_pixels = cluster.pixels.get_ptr();
      results.m_pSelectors = selectors.get_ptr();
      results.m_n = params.m_num_src_pixels = cluster.pixels.size();
      optimizer.init(params, results);
      params.m_pScan_deltas = scan;
      params.m_scan_delta_size = sizeof(scan) / sizeof(*scan);
      optimizer.compute();
      if (results.m_error > 375 * params.m_num_src_pixels) {
        params.m_pScan_deltas = refine;
        params.m_scan_delta_size = sizeof(refine) / sizeof(*refine);
        optimizer.compute();
      }
      color_quad_u8 endpoint;
      for (int c = 0; c < 3; c++)
        endpoint.c[c] = results.m_block_color_unscaled.c[c] << 3 | results.m_block_color_unscaled.c[c] >> 2;
      endpoint.c[3] = results.m_block_inten_table;
      cluster.first_endpoint = endpoint.m_u32;
      for (uint8 d0 = delta[endpoint.c[3]][0], d1 = delta[endpoint.c[3]][1], c = 0; c < 3; c++) {
        uint8 q = endpoint.c[c];
        cluster.color_values[0].c[c] = q <= d1 ? 0 : q - d1;
        cluster.color_values[1].c[c] = q <= d0 ? 0 : q - d0;
        cluster.color_values[2].c[c] = q >= 255 - d0 ? 255 : q + d0;
        cluster.color_values[3].c[c] = q >= 255 - d1 ? 255 : q + d1;
      }
      for (int t = 0; t < 4; t++)
        cluster.color_values[t].c[3] = 0xFF;
      float endpoint_weight = powf(math::minimum((cluster.color_values[3].get_luma() - cluster.color_values[0].get_luma()) / 100.0f, 1.0f), 2.7f);

      crnlib::vector<uint>& blocks = cluster.blocks[cColor];
      for (uint i = 0; i < blocks.size(); i++) {
        uint b = blocks[i];
        uint weight = (uint)(math::clamp<uint>(0x8000 * endpoint_weight * m_block_weights[b] * (m_block_encodings[b] ? 0.972f : 1.0f), 1, 0xFFFF));
        uint32 selector = 0;
        for (uint p = 0; p < 8; p++) {
          uint error_best = cUINT32_MAX;
          uint8 s_best = 0;
          for (uint8 s = 0; s < 4; s++) {
            uint error = color::color_distance(m_params.m_perceptual, ((color_quad_u8(*)[8])m_blocks)[b][p], cluster.color_values[s], false);
            if (error < error_best) {
              s_best = s;
              error_best = error;
            }
          }
          selector = selector << 2 | s_best;
        }
        m_block_selectors[cColor][b] = (uint64)selector << ((b & 1) ? 32 : 48) | weight;
      }
    }
  }
}

void dxt_hc::determine_color_endpoint_clusters_task(uint64 data, void* pData_ptr) {
  tree_clusterizer<vec6F>* vq = (tree_clusterizer<vec6F>*)pData_ptr;
  const crnlib::vector<vec6F>& codebook = vq->get_codebook();
  uint num_tasks = m_pTask_pool->get_num_threads() + 1;
  for (uint t = m_tiles.size() * data / num_tasks, tEnd = m_tiles.size() * (data + 1) / num_tasks; t < tEnd; t++) {
    if (m_tiles[t].pixels.size()) {
      const vec6F& v = m_tiles[t].color_endpoint;
      float node_dist = codebook[vq->get_node_index(v)].squared_distance(v);
      float best_dist = math::cNearlyInfinite;
      uint best_index = 0;
      for (uint i = 0; i < codebook.size(); i++) {
        const vec6F& c = codebook[i];
        float dist = 0;
        float d0 = c[0] - v[0]; dist += d0 * d0;
        float d1 = c[1] - v[1]; dist += d1 * d1;
        if (dist > node_dist)
          continue;
        float d2 = c[2] - v[2]; dist += d2 * d2;
        float d3 = c[3] - v[3]; dist += d3 * d3;
        if (dist > node_dist)
          continue;
        float d4 = c[4] - v[4]; dist += d4 * d4;
        float d5 = c[5] - v[5]; dist += d5 * d5;
        if (dist < best_dist) {
          best_dist = dist;
          best_index = i;
          if (best_dist == 0.0f)
            break;
        }
      }
      m_tiles[t].cluster_indices[cColor] = best_index;
    }
  }
}

void dxt_hc::determine_color_endpoints() {
  uint num_tasks = m_pTask_pool->get_num_threads() + 1;
  crnlib::vector<std::pair<vec6F, uint> > endpoints;
  for (uint t = 0; t < m_tiles.size(); t++) {
    if (m_tiles[t].pixels.size())
      endpoints.push_back(std::make_pair(m_tiles[t].color_endpoint, (uint)(m_tiles[t].pixels.size() * m_tiles[t].weight)));
  }

  struct Node {
    std::pair<vec6F, uint> *p, *pEnd;
    Node (std::pair<vec6F, uint>* begin, std::pair<vec6F, uint>* end) : p(begin), pEnd(end) {}
    bool operator<(const Node& other) const { return *p > *other.p; }
    static void sort_task(uint64 data, void* ptr) { std::sort(((Node*)ptr)->p, ((Node*)ptr)->pEnd); }
  };

  crnlib::vector<Node> nodes;
  Node node(0, endpoints.get_ptr());
  for (uint i = 0; i < num_tasks; i++) {
    node.p = node.pEnd;
    node.pEnd = endpoints.get_ptr() + endpoints.size() * (i + 1) / num_tasks;
    if (node.p != node.pEnd)
      nodes.push_back(node);
  }

  for (uint i = 0; i < nodes.size(); i++)
    m_pTask_pool->queue_task(&Node::sort_task, i, &nodes[i]);
  m_pTask_pool->join();

  std::priority_queue<Node> queue;
  for (uint i = 0; i < nodes.size(); i++)
    queue.push(nodes[i]);

  crnlib::vector<vec6F> vectors;
  crnlib::vector<uint> weights;
  vectors.reserve(endpoints.size());
  weights.reserve(endpoints.size());
  while (queue.size()) {
    Node node = queue.top();
    std::pair<vec6F, uint>* endpoint = node.p++;
    queue.pop();
    if (node.p != node.pEnd)
      queue.push(node);
    if (!vectors.size() || endpoint->first != vectors.back()) {
      vectors.push_back(endpoint->first);
      weights.push_back(endpoint->second);
    } else if (weights.back() > UINT_MAX - endpoint->second) {
      weights.back() = UINT_MAX;
    } else {
      weights.back() += endpoint->second;
    }
  }

  tree_clusterizer<vec6F> vq;
  vq.generate_codebook(vectors.get_ptr(), weights.get_ptr(), vectors.size(), math::minimum<uint>(m_num_tiles, m_params.m_color_endpoint_codebook_size), true, m_pTask_pool);
  m_color_clusters.resize(vq.get_codebook_size());

  for (uint i = 0; i <= m_pTask_pool->get_num_threads(); i++)
    m_pTask_pool->queue_object_task(this, &dxt_hc::determine_color_endpoint_clusters_task, i, &vq);
  m_pTask_pool->join();

  for (uint t = 0; t < m_num_blocks; t++) {
    if (m_tiles[t].pixels.size())
      m_color_clusters[m_tiles[t].cluster_indices[cColor]].pixels.append(m_tiles[t].pixels);
  }

  for (uint b = 0; b < m_num_blocks; b++) {
    uint cluster_index = m_tiles[m_tile_indices[b]].cluster_indices[cColor];
    m_endpoint_indices[b].component[cColor] = cluster_index;
    m_color_clusters[cluster_index].blocks[cColor].push_back(b);
    if (m_has_etc_color_blocks && m_endpoint_indices[b].reference && cluster_index == m_endpoint_indices[b - 1].component[cColor]) {
      if (m_endpoint_indices[b].reference >> 1) {
        color_quad_u8 mirror[16];
        for (uint p = 0; p < 16; p++)
          mirror[p] = m_blocks[b >> 1][(p << 2 & 12) | p >> 2];
        memcpy(m_blocks[b >> 1], mirror, 64);
      }
      m_endpoint_indices[b].reference = 0;
    }
  }

  for (uint i = 0; i <= m_pTask_pool->get_num_threads(); i++)
    m_pTask_pool->queue_object_task(this, m_has_etc_color_blocks ? &dxt_hc::determine_color_endpoint_codebook_task_etc : &dxt_hc::determine_color_endpoint_codebook_task, i, NULL);
  m_pTask_pool->join();
}

void dxt_hc::determine_alpha_endpoint_codebook_task(uint64 data, void*) {
  const uint num_tasks = m_pTask_pool->get_num_threads() + 1;
  dxt5_endpoint_optimizer optimizer;
  dxt_endpoint_refiner refiner;
  crnlib::vector<uint8> selectors;

  for (uint cluster_index = (uint)data; cluster_index < m_alpha_clusters.size(); cluster_index += num_tasks) {
    alpha_cluster& cluster = m_alpha_clusters[cluster_index];
    if (cluster.pixels.empty())
      continue;

    dxt5_endpoint_optimizer::params params;
    params.m_pPixels = cluster.pixels.get_ptr();
    params.m_num_pixels = cluster.pixels.size();
    params.m_comp_index = 0;
    params.m_quality = cCRNDXTQualityUber;
    params.m_use_both_block_types = false;

    dxt5_endpoint_optimizer::results results;
    selectors.resize(params.m_num_pixels);
    results.m_pSelectors = selectors.get_ptr();

    optimizer.compute(params, results);
    cluster.first_endpoint = results.m_first_endpoint;
    cluster.second_endpoint = results.m_second_endpoint;
    uint block_values[8], alpha_values[8];
    dxt5_block::get_block_values(block_values, cluster.first_endpoint, cluster.second_endpoint);
    for (uint i = 0; i < 8; i++)
      alpha_values[i] = cluster.alpha_values[i] = block_values[g_dxt5_from_linear[i]];
    int delta = cluster.first_endpoint - cluster.second_endpoint;
    uint encoding_weight[8];
    for (uint endpoint_weight = math::clamp<uint>(delta * delta >> 3, 1, 2048), i = 0; i < 8; i++)
      encoding_weight[i] = (uint)(endpoint_weight * math::lerp(1.15f, 1.0f, i / 7.0f));

    if (m_has_etc_color_blocks) {
      static const int stripped_modifier_table[2][8] = {
        {-10, -7, -5, -2, 1, 4, 6, 9},
        {-10, -3, -2, -1, 0, 1, 2, 9}
      };
      int base_codeword = (results.m_first_endpoint + results.m_second_endpoint + 1) >> 1;
      int modifier_index = delta <= 6 ? 13 : 11;
      int multiplier = delta <= 6 ? 1 : math::clamp<int>((delta + 12) / 18, 1, 15);
      const int* modifier = stripped_modifier_table[modifier_index == 11 ? 0 : 1];
      for (int i = 0; i < 8; i++)
        alpha_values[i] = cluster.alpha_values[i] = math::clamp<int>(base_codeword + modifier[i] * multiplier, 0, 255);
      cluster.first_endpoint = base_codeword;
      cluster.second_endpoint = multiplier << 4 | modifier_index;
    }

    for (uint a = 0; a < m_num_alpha_blocks; a++) {
      uint component_index = m_params.m_alpha_component_indices[a];
      crnlib::vector<uint>& blocks = cluster.blocks[cAlpha0 + a];
      for (uint i = 0; i < blocks.size(); i++) {
        uint b = blocks[i];
        uint weight = encoding_weight[m_block_encodings[b]];
        uint64 selector = 0;
        for (uint p = 0; p < 16; p++) {
          uint error_best = cUINT32_MAX;
          uint8 s_best = 0;
          for (uint8 t = 0; t < 8; t++) {
            uint8 s = m_has_etc_color_blocks ? t : results.m_reordered ? 7 - g_dxt5_to_linear[t] : g_dxt5_to_linear[t];
            int delta = m_blocks[m_has_etc_color_blocks ? b >> 1 : b][p][component_index] - alpha_values[s];
            uint error = delta >= 0 ? delta : -delta;
            if (error < error_best) {
              s_best = s;
              error_best = error;
            }
          }
          selector = selector << 3 | s_best;
        }
        m_block_selectors[cAlpha0 + a][b] = selector << 16 | weight;
      }
    }

    dxt_endpoint_refiner::params refinerParams;
    dxt_endpoint_refiner::results refinerResults;
    refinerParams.m_perceptual = m_params.m_perceptual;
    refinerParams.m_pSelectors = selectors.get_ptr();
    refinerParams.m_pPixels = cluster.pixels.get_ptr();
    refinerParams.m_num_pixels = cluster.pixels.size();
    refinerParams.m_dxt1_selectors = false;
    refinerParams.m_error_to_beat = results.m_error;
    refinerParams.m_block_index = cluster_index;
    cluster.refined_alpha = !m_has_etc_color_blocks && refiner.refine(refinerParams, refinerResults);
    if (cluster.refined_alpha) {
      cluster.first_endpoint = refinerResults.m_low_color;
      cluster.second_endpoint = refinerResults.m_high_color;
      dxt5_block::get_block_values(block_values, cluster.first_endpoint, cluster.second_endpoint);
      for (uint i = 0; i < 8; i++)
        cluster.refined_alpha_values[i] = block_values[g_dxt5_from_linear[i]];
    } else {
      memcpy(cluster.refined_alpha_values, cluster.alpha_values, sizeof(cluster.refined_alpha_values));
    }
  }
}

void dxt_hc::determine_alpha_endpoint_clusters_task(uint64 data, void* pData_ptr) {
  tree_clusterizer<vec2F>* vq = (tree_clusterizer<vec2F>*)pData_ptr;
  const crnlib::vector<vec2F>& codebook = vq->get_codebook();
  uint num_tasks = m_pTask_pool->get_num_threads() + 1;
  for (uint t = m_tiles.size() * data / num_tasks, tEnd = m_tiles.size() * (data + 1) / num_tasks; t < tEnd; t++) {
    if (m_tiles[t].pixels.size()) {
      for (uint a = 0; a < m_num_alpha_blocks; a++) {
        const vec2F& v = m_tiles[t].alpha_endpoints[a];
        float best_dist = math::cNearlyInfinite;
        uint best_index = 0;
        for (uint i = 0; i < codebook.size(); i++) {
          float dist = (codebook[i][0] - v[0]) * (codebook[i][0] - v[0]) + (codebook[i][1] - v[1]) * (codebook[i][1] - v[1]);
          if (dist < best_dist) {
            best_dist = dist;
            best_index = i;
            if (best_dist == 0.0f)
              break;
          }
        }
        m_tiles[t].cluster_indices[cAlpha0 + a] = best_index;
      }
    }
  }
}

void dxt_hc::determine_alpha_endpoints() {
  uint num_tasks = m_pTask_pool->get_num_threads() + 1;
  crnlib::vector<std::pair<vec2F, uint> > endpoints;
  for (uint a = 0; a < m_num_alpha_blocks; a++) {
    for (uint t = 0; t < m_tiles.size(); t++) {
      if (m_tiles[t].pixels.size())
        endpoints.push_back(std::make_pair(m_tiles[t].alpha_endpoints[a], m_tiles[t].pixels.size()));
    }
  }

  struct Node {
    std::pair<vec2F, uint> *p, *pEnd;
    Node (std::pair<vec2F, uint>* begin, std::pair<vec2F, uint>* end) : p(begin), pEnd(end) {}
    bool operator<(const Node& other) const { return *p > *other.p; }
    static void sort_task(uint64 data, void* ptr) { std::sort(((Node*)ptr)->p, ((Node*)ptr)->pEnd); }
  };

  crnlib::vector<Node> nodes;
  Node node(0, endpoints.get_ptr());
  for (uint i = 0; i < num_tasks; i++) {
    node.p = node.pEnd;
    node.pEnd = endpoints.get_ptr() + endpoints.size() * (i + 1) / num_tasks;
    if (node.p != node.pEnd)
      nodes.push_back(node);
  }

  for (uint i = 0; i < nodes.size(); i++)
    m_pTask_pool->queue_task(&Node::sort_task, i, &nodes[i]);
  m_pTask_pool->join();

  std::priority_queue<Node> queue;
  for (uint i = 0; i < nodes.size(); i++)
    queue.push(nodes[i]);

  crnlib::vector<vec2F> vectors;
  crnlib::vector<uint> weights;
  vectors.reserve(endpoints.size());
  weights.reserve(endpoints.size());
  while (queue.size()) {
    Node node = queue.top();
    std::pair<vec2F, uint>* endpoint = node.p++;
    queue.pop();
    if (node.p != node.pEnd)
      queue.push(node);
    if (!vectors.size() || endpoint->first != vectors.back()) {
      vectors.push_back(endpoint->first);
      weights.push_back(endpoint->second);
    } else if (weights.back() > UINT_MAX - endpoint->second) {
      weights.back() = UINT_MAX;
    } else {
      weights.back() += endpoint->second;
    }
  }

  tree_clusterizer<vec2F> vq;
  vq.generate_codebook(vectors.get_ptr(), weights.get_ptr(), vectors.size(), math::minimum<uint>(m_num_tiles, m_params.m_alpha_endpoint_codebook_size), false, m_pTask_pool);
  m_alpha_clusters.resize(vq.get_codebook_size());

  for (uint i = 0; i < num_tasks; i++)
    m_pTask_pool->queue_object_task(this, &dxt_hc::determine_alpha_endpoint_clusters_task, i, &vq);
  m_pTask_pool->join();

  for (uint a = 0; a < m_num_alpha_blocks; a++) {
    uint component_index = m_params.m_alpha_component_indices[a];
    for (uint t = 0; t < m_num_blocks; t++) {
      crnlib::vector<color_quad_u8>& source = m_tiles[t].pixels;
      if (source.size()) {
        crnlib::vector<color_quad_u8>& destination = m_alpha_clusters[m_tiles[t].cluster_indices[cAlpha0 + a]].pixels;
        for (uint p = 0; p < source.size(); p++)
          destination.push_back(color_quad_u8(source[p][component_index]));
      }
    }
  }

  for (uint b = 0; b < m_num_blocks; b++) {
    for (uint a = 0; a < m_num_alpha_blocks; a++) {
      uint cluster_index = m_tiles[m_tile_indices[b]].cluster_indices[cAlpha0 + a];
      m_endpoint_indices[b].component[cAlpha0 + a] = cluster_index;
      if (!(m_has_etc_color_blocks && b & 1))
        m_alpha_clusters[cluster_index].blocks[cAlpha0 + a].push_back(b);
    }
  }

  for (uint i = 0; i < num_tasks; i++)
    m_pTask_pool->queue_object_task(this, &dxt_hc::determine_alpha_endpoint_codebook_task, i, NULL);
  m_pTask_pool->join();
}

struct color_selector_details {
  color_selector_details() { utils::zero_object(*this); }
  uint error[16][4];
  bool used;
};

void dxt_hc::create_color_selector_codebook_task(uint64 data, void* pData_ptr) {
  crnlib::vector<color_selector_details>& selector_details = *static_cast<crnlib::vector<color_selector_details>*>(pData_ptr);
  uint num_tasks = m_pTask_pool->get_num_threads() + 1;
  uint E2[16][4];
  uint E4[8][16];
  uint E8[4][256];
  for (uint n = m_has_etc_color_blocks ? m_num_blocks >> 1 : m_num_blocks, b = n * data / num_tasks, bEnd = n * (data + 1) / num_tasks; b < bEnd; b++) {
    color_cluster& cluster = m_color_clusters[m_endpoint_indices[b].color];
    color_quad_u8* endpoint_colors = cluster.color_values;
    for (uint p = 0; p < 16; p++) {
      for (uint s = 0; s < 4; s++)
        E2[p][s] = m_has_etc_color_blocks ? color::color_distance(m_params.m_perceptual, m_blocks[b][p], m_color_clusters[m_endpoint_indices[b << 1 | p >> 3].color].color_values[s], false) :
          color::color_distance(m_params.m_perceptual, m_blocks[b][p], endpoint_colors[s], false);
    }
    for (uint p = 0; p < 8; p++) {
      for (uint s = 0; s < 16; s++)
        E4[p][s] = E2[p << 1][s & 3] + E2[p << 1 | 1][s >> 2];
    }
    for (uint p = 0; p < 4; p++) {
      for (uint s = 0; s < 256; s++)
        E8[p][s] = E4[p << 1][s & 15] + E4[p << 1 | 1][s >> 4];
    }
    uint best_index = 0;
    for (uint best_error = cUINT32_MAX, s = 0; s < m_color_selectors.size(); s++) {
      uint32 selector = m_color_selectors[s];
      uint error = E8[0][selector & 255] + E8[1][selector >> 8 & 255] + E8[2][selector >> 16 & 255] + E8[3][selector >> 24 & 255];
      if (error < best_error) {
        best_error = error;
        best_index = s;
      }
    }
    uint (&total_errors)[16][4] = selector_details[best_index].error;
    for (uint p = 0; p < 16; p++) {
      for (uint s = 0; s < 4; s++)
        total_errors[p][s] += E2[p][s];
    }
    selector_details[best_index].used = true;
    m_selector_indices[m_has_etc_color_blocks ? b << 1 : b].color = best_index;
  }
}

struct SelectorNode {
  uint64 *p, *pEnd;
  SelectorNode (uint64* begin, uint64* end) : p(begin), pEnd(end) {}
  bool operator<(const SelectorNode& other) const { return *p > *other.p; }
  static void sort_task(uint64 data, void* ptr) { std::sort(((SelectorNode*)ptr)->p, ((SelectorNode*)ptr)->pEnd); }
};

void dxt_hc::create_color_selector_codebook() {
  uint num_tasks = m_pTask_pool->get_num_threads() + 1;
  crnlib::vector<uint64> selectors(m_has_etc_color_blocks ? m_num_blocks >> 1 : m_num_blocks);
  for (uint i = 0, b = 0, step = m_has_etc_color_blocks ? 2 : 1; b < m_num_blocks; b += step)
    selectors[i++] = m_block_selectors[cColor][b] + (m_has_etc_color_blocks ? m_block_selectors[cColor][b + 1] : 0);

  crnlib::vector<SelectorNode> nodes;
  SelectorNode node(0, selectors.get_ptr());
  for (uint i = 0; i < num_tasks; i++) {
    node.p = node.pEnd;
    node.pEnd = selectors.get_ptr() + selectors.size() * (i + 1) / num_tasks;
    if (node.p != node.pEnd)
      nodes.push_back(node);
  }

  for (uint i = 0; i < nodes.size(); i++)
    m_pTask_pool->queue_task(&SelectorNode::sort_task, i, &nodes[i]);
  m_pTask_pool->join();

  std::priority_queue<SelectorNode> queue;
  for (uint i = 0; i < nodes.size(); i++)
    queue.push(nodes[i]);

  float v[4];
  for (uint s = 0; s < 4; s++)
    v[s] = (s + 0.5f) * 0.25f;

  crnlib::vector<vec16F> vectors;
  crnlib::vector<uint> weights;
  vectors.reserve(selectors.size());
  weights.reserve(selectors.size());
  for (uint64 prev_selector = 0; queue.size();) {
    SelectorNode node = queue.top();
    uint64 selector = *node.p++;
    queue.pop();
    if (node.p != node.pEnd)
      queue.push(node);
    uint weight = (uint)selector;
    selector >>= 32;
    if (!vectors.size() || selector != prev_selector) {
      prev_selector = selector;
      vec16F vector;
      for (uint p = 0; p < 16; p++, selector >>= 2)
        vector[15 - p] = v[selector & 3];
      vectors.push_back(vector);
      weights.push_back(weight);
    } else if (weights.back() > UINT_MAX - weight) {
      weights.back() = UINT_MAX;
    } else {
      weights.back() += weight;
    }
  }

  tree_clusterizer<vec16F> selector_vq;
  selector_vq.generate_codebook(vectors.get_ptr(), weights.get_ptr(), vectors.size(), m_params.m_color_selector_codebook_size, false, m_pTask_pool);
  m_color_selectors.resize(selector_vq.get_codebook_size());
  m_color_selectors_used.resize(selector_vq.get_codebook_size());
  for (uint i = 0; i < selector_vq.get_codebook_size(); i++) {
    const vec16F& v = selector_vq.get_codebook_entry(i);
    m_color_selectors[i] = 0;
    for (uint sh = 0, j = 0; j < 16; j++, sh += 2)
      m_color_selectors[i] |= (uint)(v[j] * 4.0f) << sh;
  }

  crnlib::vector<crnlib::vector<color_selector_details> > selector_details(num_tasks);
  for (uint t = 0; t < num_tasks; t++) {
    selector_details[t].resize(m_color_selectors.size());
    m_pTask_pool->queue_object_task(this, &dxt_hc::create_color_selector_codebook_task, t, &selector_details[t]);
  }
  m_pTask_pool->join();

  for (uint t = 1; t < num_tasks; t++) {
    for (uint i = 0; i < m_color_selectors.size(); i++) {
      for (uint8 p = 0; p < 16; p++) {
        for (uint8 s = 0; s < 4; s++)
          selector_details[0][i].error[p][s] += selector_details[t][i].error[p][s];
      }
      selector_details[0][i].used = selector_details[0][i].used || selector_details[t][i].used;
    }
  }

  for (uint i = 0; i < m_color_selectors.size(); i++) {
    m_color_selectors_used[i] = selector_details[0][i].used;
    uint (&errors)[16][4] = selector_details[0][i].error;
    m_color_selectors[i] = 0;
    for (uint sh = 0, p = 0; p < 16; p++, sh += 2) {
      uint* e = errors[p];
      uint8 s03 = e[3] < e[0] ? 3 : 0;
      uint8 s12 = e[2] < e[1] ? 2 : 1;
      m_color_selectors[i] |= (e[s12] < e[s03] ? s12 : s03) << sh;
    }
  }
}

struct alpha_selector_details {
  alpha_selector_details() { utils::zero_object(*this); }
  uint error[16][8];
  bool used;
};

void dxt_hc::create_alpha_selector_codebook_task(uint64 data, void* pData_ptr) {
  crnlib::vector<alpha_selector_details>& selector_details = *static_cast<crnlib::vector<alpha_selector_details>*>(pData_ptr);
  uint num_tasks = m_pTask_pool->get_num_threads() + 1;
  uint E3[16][8];
  uint E6[8][64];
  for (uint n = m_has_etc_color_blocks ? m_num_blocks >> 1 : m_num_blocks, b = n * data / num_tasks, bEnd = n * (data + 1) / num_tasks; b < bEnd; b++) {
    for (uint c = cAlpha0; c < cAlpha0 + m_num_alpha_blocks; c++) {
      const uint alpha_pixel_comp = m_params.m_alpha_component_indices[c - cAlpha0];
      alpha_cluster& cluster = m_alpha_clusters[m_endpoint_indices[m_has_etc_color_blocks ? b << 1 : b].component[c]];
      uint* block_values = cluster.alpha_values;
      for (uint p = 0; p < 16; p++) {
        for (uint s = 0; s < 8; s++) {
          int delta = m_blocks[b][p][alpha_pixel_comp] - block_values[s];
          E3[p][s] = delta * delta;
        }
      }
      for (uint p = 0; p < 8; p++) {
        for (uint s = 0; s < 64; s++)
          E6[p][s] = E3[p << 1][s & 7] + E3[p << 1 | 1][s >> 3];
      }
      uint best_index = 0;
      for (uint best_error = cUINT32_MAX, s = 0; s < m_alpha_selectors.size(); s++) {
        uint64 selector = m_alpha_selectors[s];
        uint error = E6[0][selector & 63];
        error += E6[1][selector >> 6 & 63];
        error += E6[2][selector >> 12 & 63];
        error += E6[3][selector >> 18 & 63];
        error += E6[4][selector >> 24 & 63];
        error += E6[5][selector >> 30 & 63];
        error += E6[6][selector >> 36 & 63];
        error += E6[7][selector >> 42 & 63];
        if (error < best_error) {
          best_error = error;
          best_index = s;
        }
      }
      if (cluster.refined_alpha) {
        block_values = cluster.refined_alpha_values;
        for (uint p = 0; p < 16; p++) {
          for (uint s = 0; s < 8; s++) {
            int delta = m_blocks[b][p][alpha_pixel_comp] - block_values[s];
            E3[p][s] = delta * delta;
          }
        }
      }
      uint (&total_errors)[16][8] = selector_details[best_index].error;
      for (uint p = 0; p < 16; p++) {
        for (uint s = 0; s < 8; s++)
         total_errors[p][s] += E3[p][s];
      }
      selector_details[best_index].used = true;
      m_selector_indices[m_has_etc_color_blocks ? b << 1 : b].component[c] = best_index;
    }
  }
}

void dxt_hc::create_alpha_selector_codebook() {
  uint num_tasks = m_pTask_pool->get_num_threads() + 1;
  crnlib::vector<uint64> selectors(m_num_alpha_blocks * (m_has_etc_color_blocks ? m_num_blocks >> 1 : m_num_blocks));
  for (uint i = 0, c = cAlpha0; c < cAlpha0 + m_num_alpha_blocks; c++) {
    for (uint b = 0, step = m_has_etc_color_blocks ? 2 : 1; b < m_num_blocks; b += step)
      selectors[i++] = m_block_selectors[c][b];
  }

  crnlib::vector<SelectorNode> nodes;
  SelectorNode node(0, selectors.get_ptr());
  for (uint i = 0; i < num_tasks; i++) {
    node.p = node.pEnd;
    node.pEnd = selectors.get_ptr() + selectors.size() * (i + 1) / num_tasks;
    if (node.p != node.pEnd)
      nodes.push_back(node);
  }

  for (uint i = 0; i < nodes.size(); i++)
    m_pTask_pool->queue_task(&SelectorNode::sort_task, i, &nodes[i]);
  m_pTask_pool->join();

  std::priority_queue<SelectorNode> queue;
  for (uint i = 0; i < nodes.size(); i++)
    queue.push(nodes[i]);

  float v[8];
  for (uint s = 0; s < 8; s++)
    v[s] = (s + 0.5f) * 0.125f;

  crnlib::vector<vec16F> vectors;
  crnlib::vector<uint> weights;
  vectors.reserve(selectors.size());
  weights.reserve(selectors.size());
  for (uint64 prev_selector = 0; queue.size();) {
    SelectorNode node = queue.top();
    uint64 selector = *node.p++;
    queue.pop();
    if (node.p != node.pEnd)
      queue.push(node);
    uint weight = (uint16)selector;
    selector >>= 16;
    if (!vectors.size() || selector != prev_selector) {
      prev_selector = selector;
      vec16F vector;
      for (uint p = 0; p < 16; p++, selector >>= 3)
        vector[15 - p] = v[selector & 7];
      vectors.push_back(vector);
      weights.push_back(weight);
    } else if (weights.back() > UINT_MAX - weight) {
      weights.back() = UINT_MAX;
    } else {
      weights.back() += weight;
    }
  }

  tree_clusterizer<vec16F> selector_vq;
  selector_vq.generate_codebook(vectors.get_ptr(), weights.get_ptr(), vectors.size(), m_params.m_alpha_selector_codebook_size, false, m_pTask_pool);
  m_alpha_selectors.resize(selector_vq.get_codebook_size());
  m_alpha_selectors_used.resize(selector_vq.get_codebook_size());
  for (uint i = 0; i < selector_vq.get_codebook_size(); i++) {
    const vec16F& v = selector_vq.get_codebook_entry(i);
    m_alpha_selectors[i] = 0;
    for (uint sh = 0, j = 0; j < 16; j++, sh += 3)
      m_alpha_selectors[i] |= (uint64)(v[j] * 8.0f) << sh;
  }

  crnlib::vector<crnlib::vector<alpha_selector_details> > selector_details(num_tasks);
  for (uint t = 0; t < num_tasks; t++) {
    selector_details[t].resize(m_alpha_selectors.size());
    m_pTask_pool->queue_object_task(this, &dxt_hc::create_alpha_selector_codebook_task, t, &selector_details[t]);
  }
  m_pTask_pool->join();

  for (uint t = 1; t < num_tasks; t++) {
    for (uint i = 0; i < m_alpha_selectors.size(); i++) {
      for (uint8 p = 0; p < 16; p++) {
        for (uint8 s = 0; s < 8; s++)
          selector_details[0][i].error[p][s] += selector_details[t][i].error[p][s];
      }
      selector_details[0][i].used = selector_details[0][i].used || selector_details[t][i].used;
    }
  }

  for (uint i = 0; i < m_alpha_selectors.size(); i++) {
    m_alpha_selectors_used[i] = selector_details[0][i].used;
    uint (&errors)[16][8] = selector_details[0][i].error;
    m_alpha_selectors[i] = 0;
    for (uint sh = 0, p = 0; p < 16; p++, sh += 3) {
      uint* e = errors[p];
      uint8 s07 = e[7] < e[0] ? 7 : 0;
      uint8 s12 = e[2] < e[1] ? 2 : 1;
      uint8 s34 = e[4] < e[3] ? 4 : 3;
      uint8 s56 = e[6] < e[5] ? 6 : 5;
      uint8 s02 = e[s12] < e[s07] ? s12 : s07;
      uint8 s36 = e[s56] < e[s34] ? s56 : s34;
      m_alpha_selectors[i] |= (uint64)(e[s36] < e[s02] ? s36 : s02) << sh;
    }
  }
}

bool dxt_hc::update_progress(uint phase_index, uint subphase_index, uint subphase_total) {
  CRNLIB_ASSERT(crn_get_current_thread_id() == m_main_thread_id);

  if (!m_params.m_pProgress_func)
    return true;

  const int percentage_complete = (subphase_total > 1) ? ((100 * subphase_index) / (subphase_total - 1)) : 100;
  if (((int)phase_index == m_prev_phase_index) && (m_prev_percentage_complete == percentage_complete))
    return !m_canceled;

  m_prev_percentage_complete = percentage_complete;

  bool status = (*m_params.m_pProgress_func)(phase_index, cTotalCompressionPhases, subphase_index, subphase_total, m_params.m_pProgress_func_data) != 0;
  if (!status) {
    m_canceled = true;
    return false;
  }

  return true;
}

}  // namespace crnlib
