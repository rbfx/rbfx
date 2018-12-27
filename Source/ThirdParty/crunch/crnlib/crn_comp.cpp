// File: crn_comp.cpp
// See Copyright Notice and license at the end of inc/crnlib.h
#include "crn_core.h"
#include "crn_console.h"
#include "crn_comp.h"
#include "crn_checksum.h"

#define CRNLIB_CREATE_DEBUG_IMAGES 0
#define CRNLIB_ENABLE_DEBUG_MESSAGES 0

namespace crnlib {

crn_comp::crn_comp()
    : m_pParams(NULL) {
}

crn_comp::~crn_comp() {
}

bool crn_comp::pack_color_endpoints(crnlib::vector<uint8>& packed_data, const crnlib::vector<uint16>& remapping) {
  crnlib::vector<uint> remapped_endpoints(m_color_endpoints.size());

  for (uint i = 0; i < m_color_endpoints.size(); i++)
    remapped_endpoints[remapping[i]] = m_color_endpoints[i];

  const uint component_limits[6] = {31, 63, 31, 31, 63, 31};

  symbol_histogram hist[2];
  hist[0].resize(32);
  hist[1].resize(64);

  crnlib::vector<uint> residual_syms;
  residual_syms.reserve(m_color_endpoints.size() * 2 * 3);

  color_quad_u8 prev[2];
  prev[0].clear();
  prev[1].clear();

  int total_residuals = 0;

  for (uint endpoint_index = 0; endpoint_index < m_color_endpoints.size(); endpoint_index++) {
    const uint endpoint = remapped_endpoints[endpoint_index];

    color_quad_u8 cur[2];
    cur[0] = dxt1_block::unpack_color((uint16)(endpoint & 0xFFFF), false);
    cur[1] = dxt1_block::unpack_color((uint16)((endpoint >> 16) & 0xFFFF), false);

    for (uint j = 0; j < 2; j++) {
      for (uint k = 0; k < 3; k++) {
        int delta = cur[j][k] - prev[j][k];
        total_residuals += delta * delta;
        int sym = delta & component_limits[j * 3 + k];
        int table = (k == 1) ? 1 : 0;
        hist[table].inc_freq(sym);
        residual_syms.push_back(sym);
      }
    }

    prev[0] = cur[0];
    prev[1] = cur[1];
  }

  static_huffman_data_model residual_dm[2];

  symbol_codec codec;
  codec.start_encoding(1024 * 1024);

  // Transmit residuals
  for (uint i = 0; i < 2; i++) {
    if (!residual_dm[i].init(true, hist[i], 15))
      return false;

    if (!codec.encode_transmit_static_huffman_data_model(residual_dm[i], false))
      return false;
  }

  for (uint i = 0; i < residual_syms.size(); i++) {
    const uint sym = residual_syms[i];
    const uint table = ((i % 3) == 1) ? 1 : 0;
    codec.encode(sym, residual_dm[table]);
  }

  codec.stop_encoding(false);

  packed_data.swap(codec.get_encoding_buf());

  return true;
}

bool crn_comp::pack_color_endpoints_etc(crnlib::vector<uint8>& packed_data, const crnlib::vector<uint16>& remapping) {
  crnlib::vector<uint32> remapped_endpoints(m_color_endpoints.size());
  for (uint i = 0; i < m_color_endpoints.size(); i++)
    remapped_endpoints[remapping[i]] = (m_color_endpoints[i] & 0x07000000) | (m_color_endpoints[i] >> 3 & 0x001F1F1F);

  symbol_histogram hist(32);
  for (uint32 prev_endpoint = 0, p = 0; p < remapped_endpoints.size(); p++) {
    for (uint32 _e = prev_endpoint, e = prev_endpoint = remapped_endpoints[p], c = 0; c < 4; c++, _e >>= 8, e >>= 8)
      hist.inc_freq((e - _e) & 0x1F);
  }
  static_huffman_data_model dm;
  dm.init(true, hist, 15);
  symbol_codec codec;
  codec.start_encoding(1024 * 1024);
  codec.encode_transmit_static_huffman_data_model(dm, false);
  for (uint32 prev_endpoint = 0, p = 0; p < remapped_endpoints.size(); p++) {
    for (uint32 _e = prev_endpoint, e = prev_endpoint = remapped_endpoints[p], c = 0; c < 4; c++, _e >>= 8, e >>= 8)
      codec.encode((e - _e) & 0x1F, dm);
  }
  codec.stop_encoding(false);
  packed_data.swap(codec.get_encoding_buf());
  return true;
}

bool crn_comp::pack_alpha_endpoints(crnlib::vector<uint8>& packed_data, const crnlib::vector<uint16>& remapping) {
  crnlib::vector<uint> remapped_endpoints(m_alpha_endpoints.size());

  for (uint i = 0; i < m_alpha_endpoints.size(); i++)
    remapped_endpoints[remapping[i]] = m_alpha_endpoints[i];

  symbol_histogram hist;
  hist.resize(256);

  crnlib::vector<uint> residual_syms;
  residual_syms.reserve(m_alpha_endpoints.size() * 2 * 3);

  uint prev[2];
  utils::zero_object(prev);

  int total_residuals = 0;

  for (uint endpoint_index = 0; endpoint_index < m_alpha_endpoints.size(); endpoint_index++) {
    const uint endpoint = remapped_endpoints[endpoint_index];

    uint cur[2];
    cur[0] = dxt5_block::unpack_endpoint(endpoint, 0);
    cur[1] = dxt5_block::unpack_endpoint(endpoint, 1);

    for (uint j = 0; j < 2; j++) {
      int delta = cur[j] - prev[j];
      total_residuals += delta * delta;

      int sym = delta & 255;

      hist.inc_freq(sym);

      residual_syms.push_back(sym);
    }

    prev[0] = cur[0];
    prev[1] = cur[1];
  }

  static_huffman_data_model residual_dm;

  symbol_codec codec;
  codec.start_encoding(1024 * 1024);

  // Transmit residuals
  if (!residual_dm.init(true, hist, 15))
    return false;

  if (!codec.encode_transmit_static_huffman_data_model(residual_dm, false))
    return false;

  for (uint i = 0; i < residual_syms.size(); i++) {
    const uint sym = residual_syms[i];
    codec.encode(sym, residual_dm);
  }

  codec.stop_encoding(false);

  packed_data.swap(codec.get_encoding_buf());

  return true;
}

bool crn_comp::pack_color_selectors(crnlib::vector<uint8>& packed_data, const crnlib::vector<uint16>& remapping) {
  crnlib::vector<uint32> remapped_selectors(m_color_selectors.size());
  for (uint i = 0; i < m_color_selectors.size(); i++)
    remapped_selectors[remapping[i]] = m_color_selectors[i];
  symbol_histogram hist(16);
  for (uint32 c, selector, prev_selector = 0, i = 0; i < remapped_selectors.size(); i++) {
    for (selector = prev_selector ^ remapped_selectors[i], prev_selector ^= selector, c = 8; c; c--, selector >>= 4)
      hist.inc_freq(selector & 0xF);
  }
  static_huffman_data_model dm;
  dm.init(true, hist, 15);
  symbol_codec codec;
  codec.start_encoding(1024 * 1024);
  codec.encode_transmit_static_huffman_data_model(dm, false);
  for (uint32 c, selector, prev_selector = 0, i = 0; i < remapped_selectors.size(); i++) {
    for (selector = prev_selector ^ remapped_selectors[i], prev_selector ^= selector, c = 8; c; c--, selector >>= 4)
      codec.encode(selector & 0xF, dm);
  }
  codec.stop_encoding(false);
  packed_data.swap(codec.get_encoding_buf());
  return true;
}

bool crn_comp::pack_alpha_selectors(crnlib::vector<uint8>& packed_data, const crnlib::vector<uint16>& remapping) {
  crnlib::vector<uint64> remapped_selectors(m_alpha_selectors.size());
  for (uint i = 0; i < m_alpha_selectors.size(); i++)
    remapped_selectors[remapping[i]] = m_alpha_selectors[i];
  symbol_histogram hist(64);
  for (uint64 c, selector, prev_selector = 0, i = 0; i < remapped_selectors.size(); i++) {
    for (selector = prev_selector ^ remapped_selectors[i], prev_selector ^= selector, c = 8; c; c--, selector >>= 6)
      hist.inc_freq(selector & 0x3F);
  }
  static_huffman_data_model dm;
  dm.init(true, hist, 15);
  symbol_codec codec;
  codec.start_encoding(1024 * 1024);
  codec.encode_transmit_static_huffman_data_model(dm, false);
  for (uint64 c, selector, prev_selector = 0, i = 0; i < remapped_selectors.size(); i++) {
    for (selector = prev_selector ^ remapped_selectors[i], prev_selector ^= selector, c = 8; c; c--, selector >>= 6)
      codec.encode(selector & 0x3F, dm);
  }
  codec.stop_encoding(false);
  packed_data.swap(codec.get_encoding_buf());
  return true;
}

bool crn_comp::pack_blocks(
    uint group,
    bool clear_histograms,
    symbol_codec* pCodec,
    const crnlib::vector<uint16>* pColor_endpoint_remap,
    const crnlib::vector<uint16>* pColor_selector_remap,
    const crnlib::vector<uint16>* pAlpha_endpoint_remap,
    const crnlib::vector<uint16>* pAlpha_selector_remap
  ) {
  if (!pCodec) {
    m_reference_hist.resize(256);
    if (clear_histograms)
      m_reference_hist.set_all(0);

    if (pColor_endpoint_remap) {
      m_endpoint_index_hist[0].resize(pColor_endpoint_remap->size());
      if (clear_histograms)
        m_endpoint_index_hist[0].set_all(0);
    }

    if (pColor_selector_remap) {
      m_selector_index_hist[0].resize(pColor_selector_remap->size());
      if (clear_histograms)
        m_selector_index_hist[0].set_all(0);
    }

    if (pAlpha_endpoint_remap) {
      m_endpoint_index_hist[1].resize(pAlpha_endpoint_remap->size());
      if (clear_histograms)
        m_endpoint_index_hist[1].set_all(0);
    }

    if (pAlpha_selector_remap) {
      m_selector_index_hist[1].resize(pAlpha_selector_remap->size());
      if (clear_histograms)
        m_selector_index_hist[1].set_all(0);
    }
  }

  uint endpoint_index[cNumComps] = {};
  const crnlib::vector<uint16>* endpoint_remap[cNumComps] = {};
  const crnlib::vector<uint16>* selector_remap[cNumComps] = {};
  for (uint c = 0; c < cNumComps; c++) {
    if (m_has_comp[c]) {
      endpoint_remap[c] = c ? pAlpha_endpoint_remap : pColor_endpoint_remap;
      selector_remap[c] = c ? pAlpha_selector_remap : pColor_selector_remap;
    }
  }

  uint block_width = m_levels[group].block_width;
  for (uint by = 0, b = m_levels[group].first_block, bEnd = b + m_levels[group].num_blocks; b < bEnd; by++) {
    for (uint bx = 0; bx < block_width; bx++, b++) {
      const bool secondary_etc_subblock = m_has_etc_color_blocks && bx & 1;
      if (!(by & 1) && !(bx & 1)) {
        uint8 reference_group = m_endpoint_indices[b].reference | m_endpoint_indices[b + block_width].reference << 2 |
          m_endpoint_indices[b + 1].reference << 4 | m_endpoint_indices[b + block_width + 1].reference << 6;
        if (pCodec)
          pCodec->encode(reference_group, m_reference_dm);
        else
          m_reference_hist.inc_freq(reference_group);
      }
      for (uint c = 0, cEnd = secondary_etc_subblock ? cAlpha0 : cNumComps; c < cEnd; c++) {
        if (endpoint_remap[c]) {
          uint index = (*endpoint_remap[c])[m_endpoint_indices[b].component[c]];
          if (secondary_etc_subblock ? m_endpoint_indices[b].reference : !m_endpoint_indices[b].reference) {
            int sym = index - endpoint_index[c];
            if (sym < 0)
              sym += endpoint_remap[c]->size();
            if (!pCodec)
              m_endpoint_index_hist[c ? 1 : 0].inc_freq(sym);
            else
              pCodec->encode(sym, m_endpoint_index_dm[c ? 1 : 0]);
          }
          endpoint_index[c] = index;
        }
      }
      for (uint c = 0, cEnd = secondary_etc_subblock ? 0 : cNumComps; c < cEnd; c++) {
        if (selector_remap[c]) {
          uint index = (*selector_remap[c])[m_selector_indices[b].component[c]];
          if (!pCodec)
            m_selector_index_hist[c ? 1 : 0].inc_freq(index);
          else
            pCodec->encode(index, m_selector_index_dm[c ? 1 : 0]);
        }
      }
    }
  }
  return true;
}

bool crn_comp::alias_images() {
  for (uint face_index = 0; face_index < m_pParams->m_faces; face_index++) {
    for (uint level_index = 0; level_index < m_pParams->m_levels; level_index++) {
      const uint width = math::maximum(1U, m_pParams->m_width >> level_index);
      const uint height = math::maximum(1U, m_pParams->m_height >> level_index);
      if (!m_pParams->m_pImages[face_index][level_index])
        return false;
      m_images[face_index][level_index].alias((color_quad_u8*)m_pParams->m_pImages[face_index][level_index], width, height);
    }
  }

  image_utils::conversion_type conv_type = image_utils::get_image_conversion_type_from_crn_format((crn_format)m_pParams->m_format);
  if (conv_type != image_utils::cConversion_Invalid) {
    for (uint face_index = 0; face_index < m_pParams->m_faces; face_index++) {
      for (uint level_index = 0; level_index < m_pParams->m_levels; level_index++) {
        image_u8 cooked_image(m_images[face_index][level_index]);
        image_utils::convert_image(cooked_image, conv_type);
        m_images[face_index][level_index].swap(cooked_image);
      }
    }
  }

  m_levels.resize(m_pParams->m_levels);
  m_total_blocks = 0;
  for (uint level = 0; level < m_pParams->m_levels; level++) {
    uint blockHeight = ((math::maximum(1U, m_pParams->m_height >> level) + 7) & ~7) >> 2;
    m_levels[level].block_width = ((math::maximum(1U, m_pParams->m_width >> level) + 7) & ~7) >> (m_has_etc_color_blocks ? 1 : 2);
    m_levels[level].first_block = m_total_blocks;
    m_levels[level].num_blocks = m_pParams->m_faces * m_levels[level].block_width * blockHeight;
    m_total_blocks += m_levels[level].num_blocks;
  }

  return true;
}

void crn_comp::clear() {
  m_pParams = NULL;

  for (uint f = 0; f < cCRNMaxFaces; f++)
    for (uint l = 0; l < cCRNMaxLevels; l++)
      m_images[f][l].clear();

  utils::zero_object(m_has_comp);
  m_has_etc_color_blocks = false;

  m_levels.clear();

  m_total_blocks = 0;
  m_color_endpoints.clear();
  m_alpha_endpoints.clear();
  m_color_selectors.clear();
  m_alpha_selectors.clear();
  m_endpoint_indices.clear();
  m_selector_indices.clear();

  utils::zero_object(m_crn_header);

  m_comp_data.clear();

  m_hvq.clear();

  m_reference_hist.clear();
  m_reference_dm.clear();
  for (uint i = 0; i < 2; i++) {
    m_endpoint_remaping[i].clear();
    m_endpoint_index_hist[i].clear();
    m_endpoint_index_dm[i].clear();
    m_selector_remaping[i].clear();
    m_selector_index_hist[i].clear();
    m_selector_index_dm[i].clear();
  }

  for (uint i = 0; i < cCRNMaxLevels; i++)
    m_packed_blocks[i].clear();

  m_packed_data_models.clear();

  m_packed_color_endpoints.clear();
  m_packed_color_selectors.clear();
  m_packed_alpha_endpoints.clear();
  m_packed_alpha_selectors.clear();
}

bool crn_comp::quantize_images() {
  dxt_hc::params params;

  params.m_adaptive_tile_alpha_psnr_derating = m_pParams->m_crn_adaptive_tile_alpha_psnr_derating;
  params.m_adaptive_tile_color_psnr_derating = m_pParams->m_crn_adaptive_tile_color_psnr_derating;

  if (m_pParams->m_flags & cCRNCompFlagManualPaletteSizes) {
    params.m_color_endpoint_codebook_size = math::clamp<int>(m_pParams->m_crn_color_endpoint_palette_size, cCRNMinPaletteSize, cCRNMaxPaletteSize);
    params.m_color_selector_codebook_size = math::clamp<int>(m_pParams->m_crn_color_selector_palette_size, cCRNMinPaletteSize, cCRNMaxPaletteSize);
    params.m_alpha_endpoint_codebook_size = math::clamp<int>(m_pParams->m_crn_alpha_endpoint_palette_size, cCRNMinPaletteSize, cCRNMaxPaletteSize);
    params.m_alpha_selector_codebook_size = math::clamp<int>(m_pParams->m_crn_alpha_selector_palette_size, cCRNMinPaletteSize, cCRNMaxPaletteSize);
  } else {
    uint max_codebook_entries = ((m_pParams->m_width + 3) / 4) * ((m_pParams->m_height + 3) / 4);

    max_codebook_entries = math::clamp<uint>(max_codebook_entries, cCRNMinPaletteSize, cCRNMaxPaletteSize);

    float quality = math::clamp<float>((float)m_pParams->m_quality_level / cCRNMaxQualityLevel, 0.0f, 1.0f);
    float color_quality_power_mul = 1.0f;
    float alpha_quality_power_mul = 1.0f;
    if (m_has_etc_color_blocks) {
      color_quality_power_mul = 1.31f;
      params.m_adaptive_tile_color_psnr_derating = 5.0f;
    }
    if (m_pParams->m_format == cCRNFmtDXT5_CCxY) {
      color_quality_power_mul = 3.5f;
      alpha_quality_power_mul = .35f;
      params.m_adaptive_tile_color_psnr_derating = 5.0f;
    } else if (m_pParams->m_format == cCRNFmtDXT5) {
      color_quality_power_mul = .75f;
    } else if (m_pParams->m_format == cCRNFmtETC2A) {
      alpha_quality_power_mul = .9f;
    }

    float color_endpoint_quality = powf(quality, 1.8f * color_quality_power_mul);
    float color_selector_quality = powf(quality, 1.65f * color_quality_power_mul);
    params.m_color_endpoint_codebook_size = math::clamp<uint>(math::float_to_uint(.5f + math::lerp<float>(math::maximum<float>(64, cCRNMinPaletteSize), (float)max_codebook_entries, color_endpoint_quality)), cCRNMinPaletteSize, cCRNMaxPaletteSize);
    params.m_color_selector_codebook_size = math::clamp<uint>(math::float_to_uint(.5f + math::lerp<float>(math::maximum<float>(96, cCRNMinPaletteSize), (float)max_codebook_entries, color_selector_quality)), cCRNMinPaletteSize, cCRNMaxPaletteSize);

    float alpha_endpoint_quality = powf(quality, 2.1f * alpha_quality_power_mul);
    float alpha_selector_quality = powf(quality, 1.65f * alpha_quality_power_mul);
    params.m_alpha_endpoint_codebook_size = math::clamp<uint>(math::float_to_uint(.5f + math::lerp<float>(math::maximum<float>(24, cCRNMinPaletteSize), (float)max_codebook_entries, alpha_endpoint_quality)), cCRNMinPaletteSize, cCRNMaxPaletteSize);    
    params.m_alpha_selector_codebook_size = math::clamp<uint>(math::float_to_uint(.5f + math::lerp<float>(math::maximum<float>(48, cCRNMinPaletteSize), (float)max_codebook_entries, alpha_selector_quality)), cCRNMinPaletteSize, cCRNMaxPaletteSize);
  }

  if (m_pParams->m_flags & cCRNCompFlagDebugging) {
    console::debug("Color endpoints: %u", params.m_color_endpoint_codebook_size);
    console::debug("Color selectors: %u", params.m_color_selector_codebook_size);
    console::debug("Alpha endpoints: %u", params.m_alpha_endpoint_codebook_size);
    console::debug("Alpha selectors: %u", params.m_alpha_selector_codebook_size);
  }

  params.m_hierarchical = (m_pParams->m_flags & cCRNCompFlagHierarchical) != 0;
  params.m_perceptual = (m_pParams->m_flags & cCRNCompFlagPerceptual) != 0;

  params.m_pProgress_func = m_pParams->m_pProgress_func;
  params.m_pProgress_func_data = m_pParams->m_pProgress_func_data;

  switch (m_pParams->m_format) {
    case cCRNFmtDXT1: {
      params.m_format = cDXT1;
      m_has_comp[cColor] = true;
      break;
    }
    case cCRNFmtDXT3: {
      m_has_comp[cAlpha0] = true;
      return false;
    }
    case cCRNFmtDXT5: {
      params.m_format = cDXT5;
      params.m_alpha_component_indices[0] = m_pParams->m_alpha_component;
      m_has_comp[cColor] = true;
      m_has_comp[cAlpha0] = true;
      break;
    }
    case cCRNFmtDXT5_CCxY: {
      params.m_format = cDXT5;
      params.m_alpha_component_indices[0] = 3;
      m_has_comp[cColor] = true;
      m_has_comp[cAlpha0] = true;
      params.m_perceptual = false;

      //params.m_adaptive_tile_color_alpha_weighting_ratio = 1.0f;
      params.m_adaptive_tile_color_alpha_weighting_ratio = 1.5f;
      break;
    }
    case cCRNFmtDXT5_xGBR:
    case cCRNFmtDXT5_AGBR:
    case cCRNFmtDXT5_xGxR: {
      params.m_format = cDXT5;
      params.m_alpha_component_indices[0] = 3;
      m_has_comp[cColor] = true;
      m_has_comp[cAlpha0] = true;
      params.m_perceptual = false;
      break;
    }
    case cCRNFmtDXN_XY: {
      params.m_format = cDXN_XY;
      params.m_alpha_component_indices[0] = 0;
      params.m_alpha_component_indices[1] = 1;
      m_has_comp[cAlpha0] = true;
      m_has_comp[cAlpha1] = true;
      params.m_perceptual = false;
      break;
    }
    case cCRNFmtDXN_YX: {
      params.m_format = cDXN_YX;
      params.m_alpha_component_indices[0] = 1;
      params.m_alpha_component_indices[1] = 0;
      m_has_comp[cAlpha0] = true;
      m_has_comp[cAlpha1] = true;
      params.m_perceptual = false;
      break;
    }
    case cCRNFmtDXT5A: {
      params.m_format = cDXT5A;
      params.m_alpha_component_indices[0] = m_pParams->m_alpha_component;
      m_has_comp[cAlpha0] = true;
      params.m_perceptual = false;
      break;
    }
    case cCRNFmtETC1: {
      params.m_format = cETC1;
      m_has_comp[cColor] = true;
      break;
    }
    case cCRNFmtETC2: {
      params.m_format = cETC2;
      m_has_comp[cColor] = true;
      break;
    }
    case cCRNFmtETC2A: {
      params.m_format = cETC2A;
      params.m_alpha_component_indices[0] = m_pParams->m_alpha_component;
      m_has_comp[cColor] = true;
      m_has_comp[cAlpha0] = true;
      break;
    }
    default: {
      return false;
    }
  }
  params.m_debugging = (m_pParams->m_flags & cCRNCompFlagDebugging) != 0;
  params.m_pTask_pool = &m_task_pool;

  params.m_num_levels = m_pParams->m_levels;
  for (uint i = 0; i < m_pParams->m_levels; i++) {
    params.m_levels[i].m_first_block = m_levels[i].first_block;
    params.m_levels[i].m_num_blocks = m_levels[i].num_blocks;
    params.m_levels[i].m_block_width = m_levels[i].block_width;
    params.m_levels[i].m_weight = math::minimum(12.0f, powf(1.3f, (float)i));
  }
  params.m_num_faces = m_pParams->m_faces;
  params.m_num_blocks = m_total_blocks;
  color_quad_u8 (*blocks)[16] = (color_quad_u8(*)[16])crnlib_malloc(params.m_num_blocks * 16 * sizeof(color_quad_u8));
  for (uint b = 0, level = 0; level < m_pParams->m_levels; level++) {
    for (uint face = 0; face < m_pParams->m_faces; face++) {
      image_u8& image = m_images[face][level];
      uint width = image.get_width();
      uint height = image.get_height();
      uint blockWidth = ((width + 7) & ~7) >> 2;
      uint blockHeight = ((height + 7) & ~7) >> 2;
      for (uint by = 0; by < blockHeight; by++) {
        for (uint y0 = by << 2, bx = 0; bx < blockWidth; bx++, b++) {
          for (uint t = 0, x0 = bx << 2, dy = 0; dy < 4; dy++) {
            for (uint y = math::minimum<uint>(y0 + dy, height - 1), dx = 0; dx < 4; dx++, t++)
              blocks[b][t] = image(math::minimum<uint>(x0 + dx, width - 1), y);
          }
        }
      }
    }
  }
  bool result = m_hvq.compress(blocks, m_endpoint_indices, m_selector_indices, m_color_endpoints, m_alpha_endpoints, m_color_selectors, m_alpha_selectors, params);
  crnlib_free(blocks);

  return result;
}

struct optimize_color_params {
  struct unpacked_endpoint {
    color_quad_u8 low, high;
  };
  const unpacked_endpoint* unpacked_endpoints;
  const uint* hist;
  uint16 n;
  uint16 selected;
  float weight;
  struct result {
    crnlib::vector<uint16> endpoint_remapping;
    crnlib::vector<uint8> packed_endpoints;
    uint total_bits;
  } *pResult;
};

static void sort_color_endpoints(crnlib::vector<uint16>& remapping, const optimize_color_params::unpacked_endpoint* unpacked_endpoints, uint16 n) {
  remapping.resize(n);
  crnlib::vector<optimize_color_params::unpacked_endpoint> endpoints(n);
  crnlib::vector<uint16> indices(n);
  for (uint16 i = 0; i < n; i++) {
    endpoints[i] = unpacked_endpoints[i];
    indices[i] = i;
  }
  optimize_color_params::unpacked_endpoint selected_endpoint = {color_quad_u8(0), color_quad_u8(0)};
  for (uint16 left = n; left;) {
    uint16 selected_index = 0;
    uint min_error = cUINT32_MAX;
    for (uint16 i = 0; i < left; i++) {
      optimize_color_params::unpacked_endpoint& endpoint = endpoints[i];
      uint error = color::elucidian_distance(endpoint.low, selected_endpoint.low, false) + color::elucidian_distance(endpoint.high, selected_endpoint.high, false);
      if (error < min_error) {
        min_error = error;
        selected_index = i;
      }
    }
    selected_endpoint = endpoints[selected_index];
    remapping[indices[selected_index]] = n - left;
    left--;
    endpoints[selected_index] = endpoints[left];
    indices[selected_index] = indices[left];
  }
}

static void remap_color_endpoints(uint16* remapping, const optimize_color_params::unpacked_endpoint* unpacked_endpoints, const uint* hist, uint16 n, uint16 selected, float weight) {
  struct Node {
    uint index, frequency, front_similarity, back_similarity;
    optimize_color_params::unpacked_endpoint e;
    Node() { utils::zero_object(*this); }
  };
  crnlib::vector<Node> remaining(n);
  for (uint16 i = 0; i < n; i++) {
    remaining[i].index = i;
    remaining[i].e = unpacked_endpoints[i];
  }
  crnlib::vector<uint16> chosen(n << 1);
  uint remaining_count = n, chosen_front = n, chosen_back = chosen_front;
  chosen[chosen_front] = selected;
  optimize_color_params::unpacked_endpoint front_e = remaining[selected].e, back_e = front_e;
  bool front_updated = true, back_updated = true;
  remaining[selected] = remaining[--remaining_count];
  const uint* frequency = hist + selected * n;

  for (uint similarity_base = (uint)(4000 * (1.0f + weight)), frequency_normalizer = 0; remaining_count;) {
    uint64 best_value = 0;
    uint best_index = 0;
    for (uint i = 0; i < remaining_count; i++) {
      Node& node = remaining[i];
      node.frequency += frequency[node.index];
      if (front_updated)
        node.front_similarity = similarity_base - math::minimum<uint>(4000, color::elucidian_distance(node.e.low, front_e.low, false) + color::elucidian_distance(node.e.high, front_e.high, false));
      if (back_updated)
        node.back_similarity = similarity_base - math::minimum<uint>(4000, color::elucidian_distance(node.e.low, back_e.low, false) + color::elucidian_distance(node.e.high, back_e.high, false));
      uint64 value = math::maximum(node.front_similarity, node.back_similarity) * (node.frequency + frequency_normalizer) + 1;
      if (value > best_value || (value == best_value && node.index < selected)) {
        best_value = value;
        best_index = i;
        selected = node.index;
      }
    }
    frequency = hist + selected * n;
    uint frequency_front = 0, frequency_back = 0;
    for (int front = chosen_front, back = chosen_back, scale = back - front; scale > 0; front++, back--, scale -= 2) {
      frequency_front += scale * frequency[chosen[front]];
      frequency_back += scale * frequency[chosen[back]];
    }
    front_updated = back_updated = false;
    Node& best_node = remaining[best_index];
    frequency_normalizer = best_node.frequency << 3;
    if ((uint64)best_node.front_similarity * frequency_front > (uint64)best_node.back_similarity * frequency_back) {
      chosen[--chosen_front] = selected;
      front_e = best_node.e;
      front_updated = true;
    } else {
      chosen[++chosen_back] = selected;
      back_e = best_node.e;
      back_updated = true;
    }
    best_node = remaining[--remaining_count];
  }

  for (uint16 i = chosen_front; i <= chosen_back; i++)
    remapping[chosen[i]] = i - chosen_front;
}

void crn_comp::optimize_color_endpoints_task(uint64 data, void* pData_ptr) {
  optimize_color_params* pParams = reinterpret_cast<optimize_color_params*>(pData_ptr);
  crnlib::vector<uint16>& remapping = pParams->pResult->endpoint_remapping;
  uint16 n = pParams->n;
  remapping.resize(n);

  if (data) {
    remap_color_endpoints(remapping.get_ptr(), pParams->unpacked_endpoints, pParams->hist, n, pParams->selected, pParams->weight);
  } else {
    sort_color_endpoints(remapping, pParams->unpacked_endpoints, n);
    optimize_color_selectors();
  }

  m_has_etc_color_blocks ? pack_color_endpoints_etc(pParams->pResult->packed_endpoints, remapping) : pack_color_endpoints(pParams->pResult->packed_endpoints, remapping);
  uint total_bits = pParams->pResult->packed_endpoints.size() << 3;

  crnlib::vector<uint> hist(n);
  for (uint level = 0; level < m_levels.size(); level++) {
    for (uint endpoint_index = 0, b = m_levels[level].first_block, bEnd = b + m_levels[level].num_blocks; b < bEnd; b++) {
      uint index = remapping[m_endpoint_indices[b].component[cColor]];
      if (m_has_etc_color_blocks && b & 1 ? m_endpoint_indices[b].reference : !m_endpoint_indices[b].reference) {
        int sym = index - endpoint_index;
        hist[sym < 0 ? sym + n : sym]++;
      }
      endpoint_index = index;
    }
  }

  static_huffman_data_model dm;
  dm.init(true, n, hist.get_ptr(), 16);
  const uint8* code_sizes = dm.get_code_sizes();
  for (uint16 s = 0; s < n; s++)
    total_bits += hist[s] * code_sizes[s];

  symbol_codec codec;
  codec.start_encoding(64 * 1024);
  codec.encode_enable_simulation(true);
  codec.encode_transmit_static_huffman_data_model(dm, false);
  codec.stop_encoding(false);
  total_bits += codec.encode_get_total_bits_written();

  pParams->pResult->total_bits = total_bits;

  crnlib_delete(pParams);
}

void crn_comp::optimize_color_selectors() {
  crnlib::vector<uint16>& remapping = m_selector_remaping[cColor];
  uint16 n = m_color_selectors.size();
  remapping.resize(n);

  uint8 d[] = {0, 5, 14, 10};

  uint8 D4[0x100];
  for (uint16 i = 0; i < 0x100; i++)
    D4[i] = d[(i ^ i >> 4) & 3] + d[(i >> 2 ^ i >> 6) & 3];
  uint8 D8[0x10000];
  for (uint32 i = 0; i < 0x10000; i++)
    D8[i] = D4[(i >> 8 & 0xF0) | (i >> 4 & 0xF)] + D4[(i >> 4 & 0xF0) | (i & 0xF)];

  crnlib::vector<uint32> selectors(n);
  crnlib::vector<uint16> indices(n);
  for (uint16 i = 0; i < n; i++) {
    selectors[i] = m_color_selectors[i];
    indices[i] = i;
  }
  uint32 selected_selector = 0;
  for (uint16 left = n; left;) {
    uint16 selected_index = 0;
    uint min_error = cUINT32_MAX;
    for (uint16 i = 0; i < left; i++) {
      uint32 selector = selectors[i];
      uint8 d0 = D8[(selector >> 16 & 0xFF00) | (selected_selector >> 24 & 0xFF)];
      uint8 d1 = D8[(selector >> 8 & 0xFF00) | (selected_selector >> 16 & 0xFF)];
      uint8 d2 = D8[(selector & 0xFF00) | (selected_selector >> 8 & 0xFF)];
      uint8 d3 = D8[(selector << 8 & 0xFF00) | (selected_selector & 0xFF)];
      uint error = d0 + d1 + d2 + d3;
      if (error < min_error) {
        min_error = error;
        selected_index = i;
      }
    }
    selected_selector = selectors[selected_index];
    remapping[indices[selected_index]] = n - left;
    left--;
    selectors[selected_index] = selectors[left];
    indices[selected_index] = indices[left];
  }

  pack_color_selectors(m_packed_color_selectors, remapping);
}

void crn_comp::optimize_color() {
  uint16 n = m_color_endpoints.size();
  crnlib::vector<uint> hist(n * n);
  crnlib::vector<uint> sum(n);
  for (uint i, i_prev = 0, b = 0; b < m_endpoint_indices.size(); b++, i_prev = i) {
    i = m_endpoint_indices[b].color;
    if ((m_has_etc_color_blocks && b & 1 ? m_endpoint_indices[b].reference : !m_endpoint_indices[b].reference) && i != i_prev) {
      hist[i * n + i_prev]++;
      hist[i_prev * n + i]++;
      sum[i]++;
      sum[i_prev]++;
    }
  }
  uint16 selected = 0;
  uint best_sum = 0;
  for (uint16 i = 0; i < n; i++) {
    if (best_sum < sum[i]) {
      best_sum = sum[i];
      selected = i;
    }
  }
  crnlib::vector<optimize_color_params::unpacked_endpoint> unpacked_endpoints(n);
  for (uint16 i = 0; i < n; i++) {
    unpacked_endpoints[i].low.m_u32 = m_has_etc_color_blocks ? m_color_endpoints[i] & 0xFFFFFF : dxt1_block::unpack_color(m_color_endpoints[i] & 0xFFFF, true).m_u32;
    unpacked_endpoints[i].high.m_u32 = m_has_etc_color_blocks ? m_color_endpoints[i] >> 24 : dxt1_block::unpack_color(m_color_endpoints[i] >> 16, true).m_u32;
  }

  optimize_color_params::result remapping_trial[4];
  float weights[4] = {0, 0, 1.0f / 6.0f, 0.5f};
  for (uint i = 0; i < 4; i++) {
    optimize_color_params* pParams = crnlib_new<optimize_color_params>();
    pParams->unpacked_endpoints = unpacked_endpoints.get_ptr();
    pParams->hist = hist.get_ptr();
    pParams->n = n;
    pParams->selected = selected;
    pParams->weight = weights[i];
    pParams->pResult = remapping_trial + i;
    m_task_pool.queue_object_task(this, &crn_comp::optimize_color_endpoints_task, i, pParams);
  }
  m_task_pool.join();

  for (uint best_bits = cUINT32_MAX, i = 0; i < 4; i++) {
    if (remapping_trial[i].total_bits < best_bits) {
      m_packed_color_endpoints.swap(remapping_trial[i].packed_endpoints);
      m_endpoint_remaping[cColor].swap(remapping_trial[i].endpoint_remapping);
      best_bits = remapping_trial[i].total_bits;
    }
  }
}

struct optimize_alpha_params {
  struct unpacked_endpoint {
    uint8 low, high;
  };
  const unpacked_endpoint* unpacked_endpoints;
  const uint* hist;
  uint16 n;
  uint16 selected;
  float weight;
  struct result {
    crnlib::vector<uint16> endpoint_remapping;
    crnlib::vector<uint8> packed_endpoints;
    uint total_bits;
  } *pResult;
};

static void sort_alpha_endpoints(crnlib::vector<uint16>& remapping, const optimize_alpha_params::unpacked_endpoint* unpacked_endpoints, uint16 n) {
  remapping.resize(n);
  crnlib::vector<optimize_alpha_params::unpacked_endpoint> endpoints(n);
  crnlib::vector<uint16> indices(n);
  for (uint16 i = 0; i < n; i++) {
    endpoints[i] = unpacked_endpoints[i];
    indices[i] = i;
  }
  optimize_alpha_params::unpacked_endpoint selected_endpoint = {0, 0};
  for (uint16 left = n; left;) {
    uint16 selected_index = 0;
    uint min_error = cUINT32_MAX;
    for (uint16 i = 0; i < left; i++) {
      optimize_alpha_params::unpacked_endpoint& endpoint = endpoints[i];
      uint error = math::square(endpoint.low - selected_endpoint.low) + math::square(endpoint.high - selected_endpoint.high);
      if (error < min_error) {
        min_error = error;
        selected_index = i;
      }
    }
    selected_endpoint = endpoints[selected_index];
    remapping[indices[selected_index]] = n - left;
    left--;
    endpoints[selected_index] = endpoints[left];
    indices[selected_index] = indices[left];
  }
}

static void remap_alpha_endpoints(uint16* remapping, const optimize_alpha_params::unpacked_endpoint* unpacked_endpoints, const uint* hist, uint16 n, uint16 selected, float weight) {
  const uint* frequency = hist + selected * n;
  crnlib::vector<uint16> chosen, remaining;
  crnlib::vector<uint> total_frequency(n);
  chosen.push_back(selected);
  for (uint16 i = 0; i < n; i++) {
    if (i != selected) {
      remaining.push_back(i);
      total_frequency[i] = frequency[i];
    }
  }
  for (uint similarity_base = (uint)(1000 * (1.0f + weight)), total_frequency_normalizer = 0; remaining.size();) {
    const optimize_alpha_params::unpacked_endpoint& e_front = unpacked_endpoints[chosen.front()];
    const optimize_alpha_params::unpacked_endpoint& e_back = unpacked_endpoints[chosen.back()];
    uint16 selected_index = 0;
    uint64 best_value = 0, selected_similarity_front = 0, selected_similarity_back = 0;
    for (uint16 i = 0; i < remaining.size(); i++) {
      uint remaining_index = remaining[i];
      const optimize_alpha_params::unpacked_endpoint& e_remaining = unpacked_endpoints[remaining_index];
      uint error_front = math::square(e_remaining.low - e_front.low) + math::square(e_remaining.high - e_front.high);
      uint error_back = math::square(e_remaining.low - e_back.low) + math::square(e_remaining.high - e_back.high);
      uint64 similarity_front = similarity_base - math::minimum<uint>(error_front, 1000);
      uint64 similarity_back = similarity_base - math::minimum<uint>(error_back, 1000);
      uint64 value = math::maximum(similarity_front, similarity_back) * (total_frequency[remaining_index] + total_frequency_normalizer) + 1;
      if (value > best_value) {
        best_value = value;
        selected_index = i;
        selected_similarity_front = similarity_front;
        selected_similarity_back = similarity_back;
      }
    }
    selected = remaining[selected_index];
    frequency = hist + selected * n;
    total_frequency_normalizer = total_frequency[selected];
    uint frequency_front = 0, frequency_back = 0;
    for (int front = 0, back = chosen.size() - 1, scale = back; scale > 0; front++, back--, scale -= 2) {
      frequency_front += scale * frequency[chosen[front]];
      frequency_back += scale * frequency[chosen[back]];
    }
    if (selected_similarity_front * frequency_front > selected_similarity_back * frequency_back) {
      chosen.push_front(selected);
    } else {
      chosen.push_back(selected);
    }
    remaining.erase(remaining.begin() + selected_index);
    for (uint16 i = 0; i < remaining.size(); i++)
      total_frequency[remaining[i]] += frequency[remaining[i]];
  }
  for (uint16 i = 0; i < n; i++)
    remapping[chosen[i]] = i;
}

void crn_comp::optimize_alpha_endpoints_task(uint64 data, void* pData_ptr) {
  optimize_alpha_params* pParams = reinterpret_cast<optimize_alpha_params*>(pData_ptr);
  crnlib::vector<uint16>& remapping = pParams->pResult->endpoint_remapping;
  uint16 n = pParams->n;
  remapping.resize(n);

  if (data) {
    remap_alpha_endpoints(remapping.get_ptr(), pParams->unpacked_endpoints, pParams->hist, n, pParams->selected, pParams->weight);
  } else {
    sort_alpha_endpoints(remapping, pParams->unpacked_endpoints, n);
    optimize_alpha_selectors();
  }

  pack_alpha_endpoints(pParams->pResult->packed_endpoints, remapping);
  uint total_bits = pParams->pResult->packed_endpoints.size() << 3;

  crnlib::vector<uint> hist(n);
  bool hasAlpha0 = m_has_comp[cAlpha0], hasAlpha1 = m_has_comp[cAlpha1];
  for (uint level = 0; level < m_levels.size(); level++) {
    for (uint alpha0_index = 0, alpha1_index = 0, b = m_levels[level].first_block, bEnd = b + m_levels[level].num_blocks; b < bEnd; b++) {
      if (hasAlpha0) {
        uint index = remapping[m_endpoint_indices[b].component[cAlpha0]];
        if (!m_endpoint_indices[b].reference) {
          int sym = index - alpha0_index;
          hist[sym < 0 ? sym + n : sym]++;
        }
        alpha0_index = index;
      }
      if (hasAlpha1) {
        uint index = remapping[m_endpoint_indices[b].component[cAlpha1]];
        if (!m_endpoint_indices[b].reference) {
          int sym = index - alpha1_index;
          hist[sym < 0 ? sym + n : sym]++;
        }
        alpha1_index = index;
      }
    }
  }

  static_huffman_data_model dm;
  dm.init(true, n, hist.get_ptr(), 16);
  const uint8* code_sizes = dm.get_code_sizes();
  for (uint16 s = 0; s < n; s++)
    total_bits += hist[s] * code_sizes[s];

  symbol_codec codec;
  codec.start_encoding(64 * 1024);
  codec.encode_enable_simulation(true);
  codec.encode_transmit_static_huffman_data_model(dm, false);
  codec.stop_encoding(false);
  total_bits += codec.encode_get_total_bits_written();

  pParams->pResult->total_bits = total_bits;

  crnlib_delete(pParams);
}

void crn_comp::optimize_alpha_selectors() {
  crnlib::vector<uint16>& remapping = m_selector_remaping[cAlpha0];
  uint16 n = m_alpha_selectors.size();
  remapping.resize(n);

  uint8 d[] = {0, 2, 3, 3, 5, 5, 4, 4};

  uint8 D6[0x1000];
  for (uint16 i = 0; i < 0x1000; i++)
    D6[i] = d[(i ^ i >> 6) & 7] + d[(i >> 3 ^ i >> 9) & 7];

  crnlib::vector<uint64> selectors(n);
  crnlib::vector<uint16> indices(n);
  for (uint16 i = 0; i < n; i++) {
    selectors[i] = m_alpha_selectors[i];
    indices[i] = i;
  }
  uint64 selected_selector = 0;
  for (uint16 left = n; left;) {
    uint16 selected_index = 0;
    uint min_error = cUINT32_MAX;
    for (uint16 i = 0; i < left; i++) {
      uint error = 0;
      for (uint64 selector = selectors[i] << 6, delta_selector = selected_selector, j = 0; j < 8; j++, selector >>= 6, delta_selector >>= 6)
        error += D6[(selector & 0xFC0) | (delta_selector & 0x3F)];
      if (error < min_error) {
        min_error = error;
        selected_index = i;
      }
    }
    selected_selector = selectors[selected_index];
    remapping[indices[selected_index]] = n - left;
    left--;
    selectors[selected_index] = selectors[left];
    indices[selected_index] = indices[left];
  }

  pack_alpha_selectors(m_packed_alpha_selectors, remapping);
}

void crn_comp::optimize_alpha() {
  uint16 n = m_alpha_endpoints.size();
  crnlib::vector<uint> hist(n * n);
  crnlib::vector<uint> sum(n);
  bool hasAlpha0 = m_has_comp[cAlpha0], hasAlpha1 = m_has_comp[cAlpha1];
  for (uint i0, i1, i0_prev = 0, i1_prev = 0, b = 0; b < m_endpoint_indices.size(); b++, i0_prev = i0, i1_prev = i1) {
    i0 = m_endpoint_indices[b].alpha0;
    i1 = m_endpoint_indices[b].alpha1;    
    if (!m_endpoint_indices[b].reference) {
      if (hasAlpha0 && i0 != i0_prev) {
        hist[i0 * n + i0_prev]++;
        hist[i0_prev * n + i0]++;
        sum[i0]++;
        sum[i0_prev]++;
      }
      if (hasAlpha1 && i1 != i1_prev) {
        hist[i1 * n + i1_prev]++;
        hist[i1_prev * n + i1]++;
        sum[i1]++;
        sum[i1_prev]++;
      }
    }
  }
  uint16 selected = 0;
  uint best_sum = 0;
  for (uint16 i = 0; i < n; i++) {
    if (best_sum < sum[i]) {
      best_sum = sum[i];
      selected = i;
    }
  }
  crnlib::vector<optimize_alpha_params::unpacked_endpoint> unpacked_endpoints(n);
  for (uint16 i = 0; i < n; i++) {
    unpacked_endpoints[i].low = dxt5_block::unpack_endpoint(m_alpha_endpoints[i], 0);
    unpacked_endpoints[i].high = dxt5_block::unpack_endpoint(m_alpha_endpoints[i], 1);
  }

  optimize_alpha_params::result remapping_trial[4];
  float weights[4] = {0, 0, 1.0f / 6.0f, 0.5f};
  for (uint i = 0; i < 4; i++) {
    optimize_alpha_params* pParams = crnlib_new<optimize_alpha_params>();
    pParams->unpacked_endpoints = unpacked_endpoints.get_ptr();
    pParams->hist = hist.get_ptr();
    pParams->n = n;
    pParams->selected = selected;
    pParams->weight = weights[i];
    pParams->pResult = remapping_trial + i;
    m_task_pool.queue_object_task(this, &crn_comp::optimize_alpha_endpoints_task, i, pParams);
  }
  m_task_pool.join();

  for (uint best_bits = cUINT32_MAX, i = 0; i < 4; i++) {
    if (remapping_trial[i].total_bits < best_bits) {
      m_packed_alpha_endpoints.swap(remapping_trial[i].packed_endpoints);
      m_endpoint_remaping[cAlpha0].swap(remapping_trial[i].endpoint_remapping);
      best_bits = remapping_trial[i].total_bits;
    }
  }
}

bool crn_comp::pack_data_models() {
  symbol_codec codec;
  codec.start_encoding(1024 * 1024);

  if (!codec.encode_transmit_static_huffman_data_model(m_reference_dm, false))
    return false;

  for (uint i = 0; i < 2; i++) {
    if (m_endpoint_index_dm[i].get_total_syms()) {
      if (!codec.encode_transmit_static_huffman_data_model(m_endpoint_index_dm[i], false))
        return false;
    }

    if (m_selector_index_dm[i].get_total_syms()) {
      if (!codec.encode_transmit_static_huffman_data_model(m_selector_index_dm[i], false))
        return false;
    }
  }

  codec.stop_encoding(false);

  m_packed_data_models.swap(codec.get_encoding_buf());

  return true;
}

void crn_comp::append_vec(crnlib::vector<uint8>& a, const void* p, uint size) {
  if (size) {
    uint ofs = a.size();
    a.resize(ofs + size);

    memcpy(&a[ofs], p, size);
  }
}

void crn_comp::append_vec(crnlib::vector<uint8>& a, const crnlib::vector<uint8>& b) {
  if (!b.empty()) {
    uint ofs = a.size();
    a.resize(ofs + b.size());

    memcpy(&a[ofs], &b[0], b.size());
  }
}

bool crn_comp::create_comp_data() {
  utils::zero_object(m_crn_header);

  m_crn_header.m_width = static_cast<uint16>(m_pParams->m_width);
  m_crn_header.m_height = static_cast<uint16>(m_pParams->m_height);
  m_crn_header.m_levels = static_cast<uint8>(m_pParams->m_levels);
  m_crn_header.m_faces = static_cast<uint8>(m_pParams->m_faces);
  m_crn_header.m_format = static_cast<uint8>(m_pParams->m_format);
  m_crn_header.m_userdata0 = m_pParams->m_userdata0;
  m_crn_header.m_userdata1 = m_pParams->m_userdata1;

  m_comp_data.clear();
  m_comp_data.reserve(2 * 1024 * 1024);
  append_vec(m_comp_data, &m_crn_header, sizeof(m_crn_header));
  // tack on the rest of the variable size m_level_ofs array
  m_comp_data.resize(m_comp_data.size() + sizeof(m_crn_header.m_level_ofs[0]) * (m_pParams->m_levels - 1));

  if (m_packed_color_endpoints.size()) {
    m_crn_header.m_color_endpoints.m_num = static_cast<uint16>(m_color_endpoints.size());
    m_crn_header.m_color_endpoints.m_size = m_packed_color_endpoints.size();
    m_crn_header.m_color_endpoints.m_ofs = m_comp_data.size();
    append_vec(m_comp_data, m_packed_color_endpoints);
  }

  if (m_packed_color_selectors.size()) {
    m_crn_header.m_color_selectors.m_num = static_cast<uint16>(m_color_selectors.size());
    m_crn_header.m_color_selectors.m_size = m_packed_color_selectors.size();
    m_crn_header.m_color_selectors.m_ofs = m_comp_data.size();
    append_vec(m_comp_data, m_packed_color_selectors);
  }

  if (m_packed_alpha_endpoints.size()) {
    m_crn_header.m_alpha_endpoints.m_num = static_cast<uint16>(m_alpha_endpoints.size());
    m_crn_header.m_alpha_endpoints.m_size = m_packed_alpha_endpoints.size();
    m_crn_header.m_alpha_endpoints.m_ofs = m_comp_data.size();
    append_vec(m_comp_data, m_packed_alpha_endpoints);
  }

  if (m_packed_alpha_selectors.size()) {
    m_crn_header.m_alpha_selectors.m_num = static_cast<uint16>(m_alpha_selectors.size());
    m_crn_header.m_alpha_selectors.m_size = m_packed_alpha_selectors.size();
    m_crn_header.m_alpha_selectors.m_ofs = m_comp_data.size();
    append_vec(m_comp_data, m_packed_alpha_selectors);
  }

  m_crn_header.m_tables_ofs = m_comp_data.size();
  m_crn_header.m_tables_size = m_packed_data_models.size();
  append_vec(m_comp_data, m_packed_data_models);

  uint level_ofs[cCRNMaxLevels];
  for (uint i = 0; i < m_levels.size(); i++) {
    level_ofs[i] = m_comp_data.size();
    append_vec(m_comp_data, m_packed_blocks[i]);
  }

  crnd::crn_header& dst_header = *(crnd::crn_header*)&m_comp_data[0];
  // don't change the m_comp_data vector - or dst_header will be invalidated!

  memcpy(&dst_header, &m_crn_header, sizeof(dst_header));

  for (uint i = 0; i < m_levels.size(); i++)
    dst_header.m_level_ofs[i] = level_ofs[i];

  const uint actual_header_size = sizeof(crnd::crn_header) + sizeof(dst_header.m_level_ofs[0]) * (m_levels.size() - 1);

  dst_header.m_sig = crnd::crn_header::cCRNSigValue;

  dst_header.m_data_size = m_comp_data.size();
  dst_header.m_data_crc16 = crc16(&m_comp_data[actual_header_size], m_comp_data.size() - actual_header_size);

  dst_header.m_header_size = actual_header_size;
  dst_header.m_header_crc16 = crc16(&dst_header.m_data_size, actual_header_size - (uint)((uint8*)&dst_header.m_data_size - (uint8*)&dst_header));

  return true;
}

bool crn_comp::update_progress(uint phase_index, uint subphase_index, uint subphase_total) {
  if (!m_pParams->m_pProgress_func)
    return true;

#if CRNLIB_ENABLE_DEBUG_MESSAGES
  if (m_pParams->m_flags & cCRNCompFlagDebugging)
    return true;
#endif

  return (*m_pParams->m_pProgress_func)(phase_index, cTotalCompressionPhases, subphase_index, subphase_total, m_pParams->m_pProgress_func_data) != 0;
}

bool crn_comp::compress_internal() {
  if (!alias_images())
    return false;
  if (!quantize_images())
    return false;

  m_reference_hist.clear();
  for (uint i = 0; i < 2; i++) {
    m_endpoint_remaping[i].clear();
    m_endpoint_index_hist[i].clear();
    m_endpoint_index_dm[i].clear();
    m_selector_remaping[i].clear();
    m_selector_index_hist[i].clear();
    m_selector_index_dm[i].clear();
  }

  if (m_has_comp[cColor])
    optimize_color();

  if (m_has_comp[cAlpha0])
    optimize_alpha();

  for (uint pass = 0; pass < 2; pass++) {
    for (uint level = 0; level < m_levels.size(); level++) {
      symbol_codec codec;
      codec.start_encoding(2 * 1024 * 1024);

      if (!pack_blocks(
          level,
          !pass && !level, pass ? &codec : NULL,
          m_has_comp[cColor] ? &m_endpoint_remaping[cColor] : NULL, m_has_comp[cColor] ? &m_selector_remaping[cColor] : NULL,
          m_has_comp[cAlpha0] ? &m_endpoint_remaping[cAlpha0] : NULL, m_has_comp[cAlpha0] ? &m_selector_remaping[cAlpha0] : NULL)) {
        return false;
      }

      codec.stop_encoding(false);

      if (pass)
        m_packed_blocks[level].swap(codec.get_encoding_buf());
    }

    if (!pass) {
      m_reference_dm.init(true, m_reference_hist, 16);

      for (uint i = 0; i < 2; i++) {
        if (m_endpoint_index_hist[i].size())
          m_endpoint_index_dm[i].init(true, m_endpoint_index_hist[i], 16);

        if (m_selector_index_hist[i].size())
          m_selector_index_dm[i].init(true, m_selector_index_hist[i], 16);
      }
    }
  }

  if (!pack_data_models())
    return false;

  if (!create_comp_data())
    return false;

  if (!update_progress(24, 1, 1))
    return false;

  if (m_pParams->m_flags & cCRNCompFlagDebugging) {
    crnlib_print_mem_stats();
  }

  return true;
}

bool crn_comp::compress_pass(const crn_comp_params& params, float* pEffective_bitrate) {
  clear();

  if (pEffective_bitrate)
    *pEffective_bitrate = 0.0f;

  m_pParams = &params;
  m_has_etc_color_blocks = params.m_format == cCRNFmtETC1 || params.m_format == cCRNFmtETC2 || params.m_format == cCRNFmtETC2A;

  if ((math::minimum(m_pParams->m_width, m_pParams->m_height) < 1) || (math::maximum(m_pParams->m_width, m_pParams->m_height) > cCRNMaxLevelResolution))
    return false;

  if (!m_task_pool.init(params.m_num_helper_threads))
    return false;

  bool status = compress_internal();

  m_task_pool.deinit();

  if ((status) && (pEffective_bitrate)) {
    uint total_pixels = 0;

    for (uint f = 0; f < m_pParams->m_faces; f++)
      for (uint l = 0; l < m_pParams->m_levels; l++)
        total_pixels += m_images[f][l].get_total_pixels();

    *pEffective_bitrate = (m_comp_data.size() * 8.0f) / total_pixels;
  }

  return status;
}

void crn_comp::compress_deinit() {
}

}  // namespace crnlib
