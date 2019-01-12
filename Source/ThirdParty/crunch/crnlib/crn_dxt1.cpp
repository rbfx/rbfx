// File: crn_dxt1.cpp
// See Copyright Notice and license at the end of inc/crnlib.h
//
// Notes:
// This class is not optimized for performance on small blocks, unlike typical DXT1 compressors. It's optimized for scalability and quality:
// - Very high quality in terms of avg. RMSE or Luma RMSE. Goal is to always match or beat every other known offline DXTc compressor: ATI_Compress, squish, NVidia texture tools, nvdxt.exe, etc.
// - Reasonable scalability and stability with hundreds to many thousands of input colors (including inputs with many thousands of equal/nearly equal colors).
// - Any quality optimization which results in even a tiny improvement is worth it -- as long as it's either a constant or linear slowdown.
//   Tiny quality improvements can be extremely valuable in large clusters.
// - Quality should scale well vs. CPU time cost, i.e. the more time you spend the higher the quality.
#include "crn_core.h"
#include "crn_dxt1.h"
#include "crn_ryg_dxt.hpp"
#include "crn_dxt_fast.h"
#include "crn_intersect.h"
#include "crn_vec_interval.h"

namespace crnlib {
//-----------------------------------------------------------------------------------------------------------------------------------------

static const int16 g_fast_probe_table[] = {0, 1, 2, 3};
static const uint cFastProbeTableSize = sizeof(g_fast_probe_table) / sizeof(g_fast_probe_table[0]);

static const int16 g_normal_probe_table[] = {0, 1, 3, 5, 7};
static const uint cNormalProbeTableSize = sizeof(g_normal_probe_table) / sizeof(g_normal_probe_table[0]);

static const int16 g_better_probe_table[] = {0, 1, 2, 3, 5, 9, 15, 19, 27, 43};
static const uint cBetterProbeTableSize = sizeof(g_better_probe_table) / sizeof(g_better_probe_table[0]);

static const int16 g_uber_probe_table[] = {0, 1, 2, 3, 5, 7, 9, 10, 13, 15, 19, 27, 43, 59, 91};
static const uint cUberProbeTableSize = sizeof(g_uber_probe_table) / sizeof(g_uber_probe_table[0]);

struct unique_color_projection {
  unique_color color;
  int64 projection;
};
static struct {
  bool operator()(unique_color_projection a, unique_color_projection b) const { return a.projection < b.projection; }
} g_unique_color_projection_sort;

//-----------------------------------------------------------------------------------------------------------------------------------------

dxt1_endpoint_optimizer::dxt1_endpoint_optimizer()
    : m_pParams(NULL),
      m_pResults(NULL),
      m_perceptual(false),
      m_num_prev_results(0) {
  m_low_coords.reserve(512);
  m_high_coords.reserve(512);

  m_unique_colors.reserve(512);
  m_temp_unique_colors.reserve(512);
  m_unique_packed_colors.reserve(512);

  m_norm_unique_colors.reserve(512);
  m_norm_unique_colors_weighted.reserve(512);

  m_lo_cells.reserve(128);
  m_hi_cells.reserve(128);
}

// All selectors are equal. Try compressing as if it was solid, using the block's average color, using ryg's optimal single color compression tables.
bool dxt1_endpoint_optimizer::try_average_block_as_solid() {
  uint64 tot_r = 0;
  uint64 tot_g = 0;
  uint64 tot_b = 0;

  uint total_weight = 0;
  for (uint i = 0; i < m_unique_colors.size(); i++) {
    uint weight = m_unique_colors[i].m_weight;
    total_weight += weight;

    tot_r += m_unique_colors[i].m_color.r * static_cast<uint64>(weight);
    tot_g += m_unique_colors[i].m_color.g * static_cast<uint64>(weight);
    tot_b += m_unique_colors[i].m_color.b * static_cast<uint64>(weight);
  }

  const uint half_total_weight = total_weight >> 1;
  uint ave_r = static_cast<uint>((tot_r + half_total_weight) / total_weight);
  uint ave_g = static_cast<uint>((tot_g + half_total_weight) / total_weight);
  uint ave_b = static_cast<uint>((tot_b + half_total_weight) / total_weight);

  uint low_color = (ryg_dxt::OMatch5[ave_r][0] << 11) | (ryg_dxt::OMatch6[ave_g][0] << 5) | ryg_dxt::OMatch5[ave_b][0];
  uint high_color = (ryg_dxt::OMatch5[ave_r][1] << 11) | (ryg_dxt::OMatch6[ave_g][1] << 5) | ryg_dxt::OMatch5[ave_b][1];
  bool improved = evaluate_solution(dxt1_solution_coordinates((uint16)low_color, (uint16)high_color));

  if ((m_pParams->m_use_alpha_blocks) && (m_best_solution.m_error)) {
    low_color = (ryg_dxt::OMatch5_3[ave_r][0] << 11) | (ryg_dxt::OMatch6_3[ave_g][0] << 5) | ryg_dxt::OMatch5_3[ave_b][0];
    high_color = (ryg_dxt::OMatch5_3[ave_r][1] << 11) | (ryg_dxt::OMatch6_3[ave_g][1] << 5) | ryg_dxt::OMatch5_3[ave_b][1];
    improved |= evaluate_solution(dxt1_solution_coordinates((uint16)low_color, (uint16)high_color));
  }

  if (m_pParams->m_quality == cCRNDXTQualityUber) {
    // Try compressing as all-solid using the other (non-average) colors in the block in uber.
    for (uint i = 0; i < m_unique_colors.size(); i++) {
      uint r = m_unique_colors[i].m_color[0];
      uint g = m_unique_colors[i].m_color[1];
      uint b = m_unique_colors[i].m_color[2];
      if ((r == ave_r) && (g == ave_g) && (b == ave_b))
        continue;

      uint low_color = (ryg_dxt::OMatch5[r][0] << 11) | (ryg_dxt::OMatch6[g][0] << 5) | ryg_dxt::OMatch5[b][0];
      uint high_color = (ryg_dxt::OMatch5[r][1] << 11) | (ryg_dxt::OMatch6[g][1] << 5) | ryg_dxt::OMatch5[b][1];
      improved |= evaluate_solution(dxt1_solution_coordinates((uint16)low_color, (uint16)high_color));

      if ((m_pParams->m_use_alpha_blocks) && (m_best_solution.m_error)) {
        low_color = (ryg_dxt::OMatch5_3[r][0] << 11) | (ryg_dxt::OMatch6_3[g][0] << 5) | ryg_dxt::OMatch5_3[b][0];
        high_color = (ryg_dxt::OMatch5_3[r][1] << 11) | (ryg_dxt::OMatch6_3[g][1] << 5) | ryg_dxt::OMatch5_3[b][1];
        improved |= evaluate_solution(dxt1_solution_coordinates((uint16)low_color, (uint16)high_color));
      }
    }
  }

  return improved;
}

void dxt1_endpoint_optimizer::compute_vectors(const vec3F& perceptual_weights) {
  m_norm_unique_colors.resize(0);
  m_norm_unique_colors_weighted.resize(0);

  m_mean_norm_color.clear();
  m_mean_norm_color_weighted.clear();

  for (uint i = 0; i < m_unique_colors.size(); i++) {
    const color_quad_u8& color = m_unique_colors[i].m_color;
    const uint weight = m_unique_colors[i].m_weight;

    vec3F norm_color(color.r * 1.0f / 255.0f, color.g * 1.0f / 255.0f, color.b * 1.0f / 255.0f);
    vec3F norm_color_weighted(vec3F::mul_components(perceptual_weights, norm_color));

    m_norm_unique_colors.push_back(norm_color);
    m_norm_unique_colors_weighted.push_back(norm_color_weighted);

    m_mean_norm_color += norm_color * (float)weight;
    m_mean_norm_color_weighted += norm_color_weighted * (float)weight;
  }

  if (m_total_unique_color_weight) {
    m_mean_norm_color *= (1.0f / m_total_unique_color_weight);
    m_mean_norm_color_weighted *= (1.0f / m_total_unique_color_weight);
  }

  for (uint i = 0; i < m_unique_colors.size(); i++) {
    m_norm_unique_colors[i] -= m_mean_norm_color;
    m_norm_unique_colors_weighted[i] -= m_mean_norm_color_weighted;
  }
}

// Compute PCA (principle axis, i.e. direction of largest variance) of input vectors.
void dxt1_endpoint_optimizer::compute_pca(vec3F& axis, const vec3F_array& norm_colors, const vec3F& def) {
  double cov[6] = {0, 0, 0, 0, 0, 0};
  for (uint i = 0; i < norm_colors.size(); i++) {
    const vec3F& v = norm_colors[i];
    float r = v[0];
    float g = v[1];
    float b = v[2];
    if (m_unique_colors[i].m_weight > 1) {
      const double weight = m_unique_colors[i].m_weight;
      cov[0] += r * r * weight;
      cov[1] += r * g * weight;
      cov[2] += r * b * weight;
      cov[3] += g * g * weight;
      cov[4] += g * b * weight;
      cov[5] += b * b * weight;
    } else {
      cov[0] += r * r;
      cov[1] += r * g;
      cov[2] += r * b;
      cov[3] += g * g;
      cov[4] += g * b;
      cov[5] += b * b;
    }
  }
  double vfr = .9f;
  double vfg = 1.0f;
  double vfb = .7f;
  for (uint iter = 0; iter < 8; iter++) {
    double r = vfr * cov[0] + vfg * cov[1] + vfb * cov[2];
    double g = vfr * cov[1] + vfg * cov[3] + vfb * cov[4];
    double b = vfr * cov[2] + vfg * cov[4] + vfb * cov[5];
    double m = math::maximum(fabs(r), fabs(g), fabs(b));
    if (m > 1e-10) {
      m = 1.0f / m;
      r *= m;
      g *= m;
      b *= m;
    }
    double delta = math::square(vfr - r) + math::square(vfg - g) + math::square(vfb - b);
    vfr = r;
    vfg = g;
    vfb = b;
    if ((iter > 2) && (delta < 1e-8))
      break;
  }
  double len = vfr * vfr + vfg * vfg + vfb * vfb;
  if (len < 1e-10) {
    axis = def;
  } else {
    len = 1.0f / sqrt(len);
    axis.set(static_cast<float>(vfr * len), static_cast<float>(vfg * len), static_cast<float>(vfb * len));
  }
}

static const uint8 g_invTableNull[4] = {0, 1, 2, 3};
static const uint8 g_invTableAlpha[4] = {1, 0, 2, 3};
static const uint8 g_invTableColor[4] = {1, 0, 3, 2};

// Computes a valid (encodable) DXT1 solution (low/high colors, swizzled selectors) from input.
void dxt1_endpoint_optimizer::return_solution() {
  compute_selectors();
  bool invert_selectors;

  if (m_best_solution.m_alpha_block)
    invert_selectors = (m_best_solution.m_coords.m_low_color > m_best_solution.m_coords.m_high_color);
  else {
    CRNLIB_ASSERT(m_best_solution.m_coords.m_low_color != m_best_solution.m_coords.m_high_color);

    invert_selectors = (m_best_solution.m_coords.m_low_color < m_best_solution.m_coords.m_high_color);
  }

  m_pResults->m_alternate_rounding = m_best_solution.m_alternate_rounding;
  m_pResults->m_enforce_selector = m_best_solution.m_enforce_selector;
  m_pResults->m_enforced_selector = m_best_solution.m_enforced_selector;
  m_pResults->m_reordered = invert_selectors;
  if (invert_selectors) {
    m_pResults->m_low_color = m_best_solution.m_coords.m_high_color;
    m_pResults->m_high_color = m_best_solution.m_coords.m_low_color;
  } else {
    m_pResults->m_low_color = m_best_solution.m_coords.m_low_color;
    m_pResults->m_high_color = m_best_solution.m_coords.m_high_color;
  }

  const uint8* pInvert_table = g_invTableNull;
  if (invert_selectors)
    pInvert_table = m_best_solution.m_alpha_block ? g_invTableAlpha : g_invTableColor;

  const uint alpha_thresh = m_pParams->m_pixels_have_alpha ? (m_pParams->m_dxt1a_alpha_threshold << 24U) : 0;

  const uint32* pSrc_pixels = reinterpret_cast<const uint32*>(m_pParams->m_pPixels);
  uint8* pDst_selectors = m_pResults->m_pSelectors;

  if ((m_unique_colors.size() == 1) && (!m_pParams->m_pixels_have_alpha)) {
    uint32 c = utils::read_le32(pSrc_pixels);

    CRNLIB_ASSERT(c >= alpha_thresh);

    c |= 0xFF000000U;

    unique_color_hash_map::const_iterator it(m_unique_color_hash_map.find(c));
    CRNLIB_ASSERT(it != m_unique_color_hash_map.end());

    uint unique_color_index = it->second;

    uint selector = pInvert_table[m_best_solution.m_selectors[unique_color_index]];

    memset(pDst_selectors, selector, m_pParams->m_num_pixels);
  } else {
    uint8* pDst_selectors_end = pDst_selectors + m_pParams->m_num_pixels;

    uint8 prev_selector = 0;
    uint32 prev_color = 0;

    do {
      uint32 c = utils::read_le32(pSrc_pixels);
      pSrc_pixels++;

      uint8 selector = 3;

      if (c >= alpha_thresh) {
        c |= 0xFF000000U;

        if (c == prev_color)
          selector = prev_selector;
        else {
          unique_color_hash_map::const_iterator it(m_unique_color_hash_map.find(c));

          CRNLIB_ASSERT(it != m_unique_color_hash_map.end());

          uint unique_color_index = it->second;

          selector = pInvert_table[m_best_solution.m_selectors[unique_color_index]];

          prev_color = c;
          prev_selector = selector;
        }
      }

      *pDst_selectors++ = selector;

    } while (pDst_selectors != pDst_selectors_end);
  }

  m_pResults->m_alpha_block = m_best_solution.m_alpha_block;
  m_pResults->m_error = m_best_solution.m_error;
}

// Per-component 1D endpoint optimization.

void dxt1_endpoint_optimizer::compute_endpoint_component_errors(uint comp_index, uint64 (&error)[4][256], uint64 (&best_remaining_error)[4]) {
  uint64 W[4] = {}, WP2[4] = {}, WPP[4] = {};
  for (uint i = 0; i < m_unique_colors.size(); i++) {
    uint p = m_unique_colors[i].m_color[comp_index];
    uint w = m_unique_colors[i].m_weight;
    uint8 s = m_best_solution.m_selectors[i];
    W[s] += (int64)w;
    WP2[s] += (int64)w * p * 2;
    WPP[s] += (int64)w * p * p;
  }
  const uint comp_limit = comp_index == 1 ? 64 : 32;
  for (uint8 s = 0; s < 2; s++) {
    uint64 best_error = error[s][0] = WPP[s];
    for (uint8 c = 1; c < comp_limit; c++) {
      uint8 p = comp_index == 1 ? c << 2 | c >> 4 : c << 3 | c >> 2;
      error[s][c] = W[s] * p * p - WP2[s] * p + WPP[s];
      if (error[s][c] < best_error)
        best_error = error[s][c];
    }
    best_remaining_error[s] = best_error;
  }
  for (uint8 s = 2; s < 4; s++) {
    uint64 best_error = error[s][0] = WPP[s], d = W[s] - WP2[s], dd = W[s] << 1, e = WPP[s] + d;
    for (uint p = 1; p < 256; p++, d += dd, e += d) {
      error[s][p] = e;
      if (e < best_error)
        best_error = e;
    }
    best_remaining_error[s] = best_error;
  }
  for (uint8 s = 3; s; s--)
    best_remaining_error[s - 1] += best_remaining_error[s];
}

void dxt1_endpoint_optimizer::optimize_endpoint_comps() {
  compute_selectors();
  if (m_best_solution.m_alpha_block || !m_best_solution.m_error)
    return;
  color_quad_u8 source_low(dxt1_block::unpack_color(m_best_solution.m_coords.m_low_color, true));
  color_quad_u8 source_high(dxt1_block::unpack_color(m_best_solution.m_coords.m_high_color, true));
  uint64 error[4][256], best_remaining_error[4];
  for (uint comp_index = 0; comp_index < 3; comp_index++) {
    uint8 p0 = source_low[comp_index];
    uint8 p1 = source_high[comp_index];
    color_quad_u8 low(dxt1_block::unpack_color(m_best_solution.m_coords.m_low_color, false));
    color_quad_u8 high(dxt1_block::unpack_color(m_best_solution.m_coords.m_high_color, false));
    compute_endpoint_component_errors(comp_index, error, best_remaining_error);
    uint64 best_error = error[0][low[comp_index]] + error[1][high[comp_index]] + error[2][(p0 * 2 + p1) / 3] + error[3][(p0 + p1 * 2) / 3];
    if (best_remaining_error[0] >= best_error)
      continue;
    const uint comp_limit = comp_index == 1 ? 64 : 32;
    for (uint8 c0 = 0; c0 < comp_limit; c0++) {
      uint64 e0 = error[0][c0];
      if (e0 + best_remaining_error[1] >= best_error)
        continue;
      low[comp_index] = c0;
      uint16 packed_low = dxt1_block::pack_color(low, false);
      p0 = comp_index == 1 ? c0 << 2 | c0 >> 4 : c0 << 3 | c0 >> 2;
      for (uint8 c1 = 0; c1 < comp_limit; c1++) {
        uint64 e = e0 + error[1][c1];
        if (e + best_remaining_error[2] >= best_error)
          continue;
        p1 = comp_index == 1 ? c1 << 2 | c1 >> 4 : c1 << 3 | c1 >> 2;
        e += error[2][(p0 * 2 + p1) / 3];
        if (e + best_remaining_error[3] >= best_error)
          continue;
        e += error[3][(p0 + p1 * 2) / 3];
        if (e >= best_error)
          continue;
        high[comp_index] = c1;
        if (!evaluate_solution(dxt1_solution_coordinates(packed_low, dxt1_block::pack_color(high, false))))
          continue;
        if (!m_best_solution.m_error)
          return;
        compute_selectors();
        compute_endpoint_component_errors(comp_index, error, best_remaining_error);
        best_error = error[0][c0] + error[1][c1] + error[2][(p0 * 2 + p1) / 3] + error[3][(p0 + p1 * 2) / 3];
        e0 = error[0][c0];
        if (e0 + best_remaining_error[1] >= best_error)
          break;
      }
    }
  }
}

// Voxel adjacency delta coordinations.
static const struct adjacent_coords {
  int8 x, y, z;
} g_adjacency[26] = {
    {-1, -1, -1},
    {0, -1, -1},
    {1, -1, -1},
    {-1, 0, -1},
    {0, 0, -1},
    {1, 0, -1},
    {-1, 1, -1},
    {0, 1, -1},

    {1, 1, -1},
    {-1, -1, 0},
    {0, -1, 0},
    {1, -1, 0},
    {-1, 0, 0},
    {1, 0, 0},
    {-1, 1, 0},
    {0, 1, 0},

    {1, 1, 0},
    {-1, -1, 1},
    {0, -1, 1},
    {1, -1, 1},
    {-1, 0, 1},
    {0, 0, 1},
    {1, 0, 1},
    {-1, 1, 1},

    {0, 1, 1},
    {1, 1, 1}};

// Attempt to refine current solution's endpoints given the current selectors using least squares.
bool dxt1_endpoint_optimizer::refine_solution(int refinement_level) {
  compute_selectors();

  static const int w1Tab[4] = {3, 0, 2, 1};

  static const int prods_0[4] = {0x00, 0x00, 0x02, 0x02};
  static const int prods_1[4] = {0x00, 0x09, 0x01, 0x04};
  static const int prods_2[4] = {0x09, 0x00, 0x04, 0x01};

  double akku_0 = 0;
  double akku_1 = 0;
  double akku_2 = 0;
  double At1_r, At1_g, At1_b;
  double At2_r, At2_g, At2_b;

  At1_r = At1_g = At1_b = 0;
  At2_r = At2_g = At2_b = 0;
  for (uint i = 0; i < m_unique_colors.size(); i++) {
    const color_quad_u8& c = m_unique_colors[i].m_color;
    const double weight = m_unique_colors[i].m_weight;

    double r = c.r * weight;
    double g = c.g * weight;
    double b = c.b * weight;
    int step = m_best_solution.m_selectors[i] ^ 1;

    int w1 = w1Tab[step];

    akku_0 += prods_0[step] * weight;
    akku_1 += prods_1[step] * weight;
    akku_2 += prods_2[step] * weight;
    At1_r += w1 * r;
    At1_g += w1 * g;
    At1_b += w1 * b;
    At2_r += r;
    At2_g += g;
    At2_b += b;
  }

  At2_r = 3 * At2_r - At1_r;
  At2_g = 3 * At2_g - At1_g;
  At2_b = 3 * At2_b - At1_b;

  double xx = akku_2;
  double yy = akku_1;
  double xy = akku_0;

  double t = xx * yy - xy * xy;
  if (!yy || !xx || (fabs(t) < .0000125f))
    return false;

  double frb = (3.0f * 31.0f / 255.0f) / t;
  double fg = frb * (63.0f / 31.0f);

  bool improved = false;

  if (refinement_level == 0) {
    uint max16;
    max16 = math::clamp<int>(static_cast<int>((At1_r * yy - At2_r * xy) * frb + 0.5f), 0, 31) << 11;
    max16 |= math::clamp<int>(static_cast<int>((At1_g * yy - At2_g * xy) * fg + 0.5f), 0, 63) << 5;
    max16 |= math::clamp<int>(static_cast<int>((At1_b * yy - At2_b * xy) * frb + 0.5f), 0, 31) << 0;

    uint min16;
    min16 = math::clamp<int>(static_cast<int>((At2_r * xx - At1_r * xy) * frb + 0.5f), 0, 31) << 11;
    min16 |= math::clamp<int>(static_cast<int>((At2_g * xx - At1_g * xy) * fg + 0.5f), 0, 63) << 5;
    min16 |= math::clamp<int>(static_cast<int>((At2_b * xx - At1_b * xy) * frb + 0.5f), 0, 31) << 0;

    dxt1_solution_coordinates nc((uint16)min16, (uint16)max16);
    nc.canonicalize();
    improved |= evaluate_solution(nc);
  } else if (refinement_level == 1) {
    // Try exploring the local lattice neighbors of the least squares optimized result.
    color_quad_u8 e[2];

    e[0].clear();
    e[0][0] = (uint8)math::clamp<int>(static_cast<int>((At1_r * yy - At2_r * xy) * frb + 0.5f), 0, 31);
    e[0][1] = (uint8)math::clamp<int>(static_cast<int>((At1_g * yy - At2_g * xy) * fg + 0.5f), 0, 63);
    e[0][2] = (uint8)math::clamp<int>(static_cast<int>((At1_b * yy - At2_b * xy) * frb + 0.5f), 0, 31);

    e[1].clear();
    e[1][0] = (uint8)math::clamp<int>(static_cast<int>((At2_r * xx - At1_r * xy) * frb + 0.5f), 0, 31);
    e[1][1] = (uint8)math::clamp<int>(static_cast<int>((At2_g * xx - At1_g * xy) * fg + 0.5f), 0, 63);
    e[1][2] = (uint8)math::clamp<int>(static_cast<int>((At2_b * xx - At1_b * xy) * frb + 0.5f), 0, 31);

    for (uint i = 0; i < 2; i++) {
      for (int rr = -1; rr <= 1; rr++) {
        for (int gr = -1; gr <= 1; gr++) {
          for (int br = -1; br <= 1; br++) {
            dxt1_solution_coordinates nc;

            color_quad_u8 c[2];
            c[0] = e[0];
            c[1] = e[1];

            c[i][0] = (uint8)math::clamp<int>(c[i][0] + rr, 0, 31);
            c[i][1] = (uint8)math::clamp<int>(c[i][1] + gr, 0, 63);
            c[i][2] = (uint8)math::clamp<int>(c[i][2] + br, 0, 31);

            nc.m_low_color = dxt1_block::pack_color(c[0], false);
            nc.m_high_color = dxt1_block::pack_color(c[1], false);

            nc.canonicalize();
            improved |= evaluate_solution(nc);
          }
        }
      }
    }
  } else {
    // Try even harder to explore the local lattice neighbors of the least squares optimized result.
    color_quad_u8 e[2];
    e[0].clear();
    e[0][0] = (uint8)math::clamp<int>(static_cast<int>((At1_r * yy - At2_r * xy) * frb + 0.5f), 0, 31);
    e[0][1] = (uint8)math::clamp<int>(static_cast<int>((At1_g * yy - At2_g * xy) * fg + 0.5f), 0, 63);
    e[0][2] = (uint8)math::clamp<int>(static_cast<int>((At1_b * yy - At2_b * xy) * frb + 0.5f), 0, 31);

    e[1].clear();
    e[1][0] = (uint8)math::clamp<int>(static_cast<int>((At2_r * xx - At1_r * xy) * frb + 0.5f), 0, 31);
    e[1][1] = (uint8)math::clamp<int>(static_cast<int>((At2_g * xx - At1_g * xy) * fg + 0.5f), 0, 63);
    e[1][2] = (uint8)math::clamp<int>(static_cast<int>((At2_b * xx - At1_b * xy) * frb + 0.5f), 0, 31);

    for (int orr = -1; orr <= 1; orr++) {
      for (int ogr = -1; ogr <= 1; ogr++) {
        for (int obr = -1; obr <= 1; obr++) {
          dxt1_solution_coordinates nc;

          color_quad_u8 c[2];
          c[0] = e[0];
          c[1] = e[1];

          c[0][0] = (uint8)math::clamp<int>(c[0][0] + orr, 0, 31);
          c[0][1] = (uint8)math::clamp<int>(c[0][1] + ogr, 0, 63);
          c[0][2] = (uint8)math::clamp<int>(c[0][2] + obr, 0, 31);

          for (int rr = -1; rr <= 1; rr++) {
            for (int gr = -1; gr <= 1; gr++) {
              for (int br = -1; br <= 1; br++) {
                c[1][0] = (uint8)math::clamp<int>(c[1][0] + rr, 0, 31);
                c[1][1] = (uint8)math::clamp<int>(c[1][1] + gr, 0, 63);
                c[1][2] = (uint8)math::clamp<int>(c[1][2] + br, 0, 31);

                nc.m_low_color = dxt1_block::pack_color(c[0], false);
                nc.m_high_color = dxt1_block::pack_color(c[1], false);
                nc.canonicalize();

                improved |= evaluate_solution(nc);
              }
            }
          }
        }
      }
    }
  }

  return improved;
}

//-----------------------------------------------------------------------------------------------------------------------------------------

// Primary endpoint optimization entrypoint.
void dxt1_endpoint_optimizer::optimize_endpoints(vec3F& low_color, vec3F& high_color) {
  vec3F orig_low_color(low_color);
  vec3F orig_high_color(high_color);

  m_trial_solution.clear();

  uint num_passes;
  const int16* pProbe_table = g_uber_probe_table;
  uint probe_range;
  float dist_per_trial = .015625f;

  // How many probes, and the distance between each probe depends on the quality level.
  switch (m_pParams->m_quality) {
    case cCRNDXTQualitySuperFast:
      pProbe_table = g_fast_probe_table;
      probe_range = cFastProbeTableSize;
      dist_per_trial = .027063293f;
      num_passes = 1;
      break;
    case cCRNDXTQualityFast:
      pProbe_table = g_fast_probe_table;
      probe_range = cFastProbeTableSize;
      dist_per_trial = .027063293f;
      num_passes = 2;
      break;
    case cCRNDXTQualityNormal:
      pProbe_table = g_normal_probe_table;
      probe_range = cNormalProbeTableSize;
      dist_per_trial = .027063293f;
      num_passes = 2;
      break;
    case cCRNDXTQualityBetter:
      pProbe_table = g_better_probe_table;
      probe_range = cBetterProbeTableSize;
      num_passes = 2;
      break;
    default:
      pProbe_table = g_uber_probe_table;
      probe_range = cUberProbeTableSize;
      num_passes = 4;
      break;
  }

  if (m_pParams->m_endpoint_caching) {
    // Try the previous X winning endpoints. This may not give us optimal results, but it may increase the probability of early outs while evaluating potential solutions.
    const uint num_prev_results = math::minimum<uint>(cMaxPrevResults, m_num_prev_results);
    for (uint i = 0; i < num_prev_results; i++)
      evaluate_solution(m_prev_results[i]);

    if (!m_best_solution.m_error) {
      // Got lucky - one of the previous endpoints is optimal.
      return_solution();
      return;
    }
  }

  if (m_pParams->m_quality >= cCRNDXTQualityBetter) {
    //evaluate_solution(dxt1_solution_coordinates(low_color, high_color), true, &m_best_solution);
    //refine_solution();

    try_median4(orig_low_color, orig_high_color);
  }

  uint probe_low[cUberProbeTableSize * 2 + 1];
  uint probe_high[cUberProbeTableSize * 2 + 1];

  vec3F scaled_principle_axis[2];

  scaled_principle_axis[1] = m_principle_axis * dist_per_trial;
  scaled_principle_axis[1][0] *= 31.0f;
  scaled_principle_axis[1][1] *= 63.0f;
  scaled_principle_axis[1][2] *= 31.0f;

  scaled_principle_axis[0] = -scaled_principle_axis[1];

  //vec3F initial_ofs(scaled_principle_axis * (float)-probe_range);
  //initial_ofs[0] += .5f;
  //initial_ofs[1] += .5f;
  //initial_ofs[2] += .5f;

  low_color[0] = math::clamp(low_color[0] * 31.0f, 0.0f, 31.0f);
  low_color[1] = math::clamp(low_color[1] * 63.0f, 0.0f, 63.0f);
  low_color[2] = math::clamp(low_color[2] * 31.0f, 0.0f, 31.0f);

  high_color[0] = math::clamp(high_color[0] * 31.0f, 0.0f, 31.0f);
  high_color[1] = math::clamp(high_color[1] * 63.0f, 0.0f, 63.0f);
  high_color[2] = math::clamp(high_color[2] * 31.0f, 0.0f, 31.0f);

  int d[3];
  for (uint c = 0; c < 3; c++)
    d[c] = math::float_to_int_round((high_color[c] - low_color[c]) * (c == 0 ? m_perceptual ? 16 : 2 : c == 1 ? m_perceptual ? 25 : 1 : 2));
  crnlib::vector<unique_color_projection> evaluated_color_projections(m_evaluated_colors.size());
  int64 average_projection = d[0] * (high_color[0] + low_color[0]) * 4 + d[1] * (high_color[1] + low_color[1]) * 2 + d[2] * (high_color[2] + low_color[2]) * 4;
  for (uint i = 0; i < m_evaluated_colors.size(); i++) {
    int64 delta = d[0] * m_evaluated_colors[i].m_color[0] + d[1] * m_evaluated_colors[i].m_color[1] + d[2] * m_evaluated_colors[i].m_color[2] - average_projection;
    evaluated_color_projections[i].projection = delta * m_evaluated_colors[i].m_weight;
    evaluated_color_projections[i].color = m_evaluated_colors[i];
  }
  std::sort(evaluated_color_projections.begin(), evaluated_color_projections.end(), g_unique_color_projection_sort);
  for (uint i = 0, iEnd = m_evaluated_colors.size(); i < iEnd; i++)
    m_evaluated_colors[i] = evaluated_color_projections[i & 1 ? i >> 1 : iEnd - 1 - (i >> 1)].color;

  for (uint pass = 0; pass < num_passes; pass++) {
    // Now separately sweep or probe the low and high colors along the principle axis, both positively and negatively.
    // This results in two arrays of candidate low/high endpoints. Every unique combination of candidate endpoints is tried as a potential solution.
    // In higher quality modes, the various nearby lattice neighbors of each candidate endpoint are also explored, which allows the current solution to "wobble" or "migrate"
    // to areas with lower error.
    // This entire process can be repeated up to X times (depending on the quality level) until a local minimum is established.
    // This method is very stable and scalable. It could be implemented more elegantly, but I'm now very cautious of touching this code.
    if (pass) {
      color_quad_u8 low(dxt1_block::unpack_color(m_best_solution.m_coords.m_low_color, false));
      low_color = vec3F(low.r, low.g, low.b);
      color_quad_u8 high(dxt1_block::unpack_color(m_best_solution.m_coords.m_high_color, false));
      high_color = vec3F(high.r, high.g, high.b);
    }

    const uint64 prev_best_error = m_best_solution.m_error;
    if (!prev_best_error)
      break;

    // Sweep low endpoint along principle axis, record positions
    int prev_packed_color[2] = {-1, -1};
    uint num_low_trials = 0;
    vec3F initial_probe_low_color(low_color + vec3F(.5f));
    for (uint i = 0; i < probe_range; i++) {
      const int ls = i ? 0 : 1;
      int x = pProbe_table[i];

      for (int s = ls; s < 2; s++) {
        vec3F probe_low_color(initial_probe_low_color + scaled_principle_axis[s] * (float)x);

        int r = math::clamp((int)floor(probe_low_color[0]), 0, 31);
        int g = math::clamp((int)floor(probe_low_color[1]), 0, 63);
        int b = math::clamp((int)floor(probe_low_color[2]), 0, 31);

        int packed_color = b | (g << 5U) | (r << 11U);
        if (packed_color != prev_packed_color[s]) {
          probe_low[num_low_trials++] = packed_color;
          prev_packed_color[s] = packed_color;
        }
      }
    }

    prev_packed_color[0] = -1;
    prev_packed_color[1] = -1;

    // Sweep high endpoint along principle axis, record positions
    uint num_high_trials = 0;
    vec3F initial_probe_high_color(high_color + vec3F(.5f));
    for (uint i = 0; i < probe_range; i++) {
      const int ls = i ? 0 : 1;
      int x = pProbe_table[i];

      for (int s = ls; s < 2; s++) {
        vec3F probe_high_color(initial_probe_high_color + scaled_principle_axis[s] * (float)x);

        int r = math::clamp((int)floor(probe_high_color[0]), 0, 31);
        int g = math::clamp((int)floor(probe_high_color[1]), 0, 63);
        int b = math::clamp((int)floor(probe_high_color[2]), 0, 31);

        int packed_color = b | (g << 5U) | (r << 11U);
        if (packed_color != prev_packed_color[s]) {
          probe_high[num_high_trials++] = packed_color;
          prev_packed_color[s] = packed_color;
        }
      }
    }

    // Now try all unique combinations.
    for (uint i = 0; i < num_low_trials; i++) {
      for (uint j = 0; j < num_high_trials; j++) {
        dxt1_solution_coordinates coords((uint16)probe_low[i], (uint16)probe_high[j]);
        coords.canonicalize();
        evaluate_solution(coords);
      }
    }

    if (m_pParams->m_quality >= cCRNDXTQualityNormal) {
      // Generate new candidates by exploring the low color's direct lattice neighbors
      color_quad_u8 lc(dxt1_block::unpack_color(m_best_solution.m_coords.m_low_color, false));

      for (int i = 0; i < 26; i++) {
        int r = lc.r + g_adjacency[i].x;
        if ((r < 0) || (r > 31))
          continue;

        int g = lc.g + g_adjacency[i].y;
        if ((g < 0) || (g > 63))
          continue;

        int b = lc.b + g_adjacency[i].z;
        if ((b < 0) || (b > 31))
          continue;

        dxt1_solution_coordinates coords(dxt1_block::pack_color(r, g, b, false), m_best_solution.m_coords.m_high_color);
        coords.canonicalize();
        evaluate_solution(coords);
      }

      if (m_pParams->m_quality == cCRNDXTQualityUber) {
        // Generate new candidates by exploring the low color's direct lattice neighbors - this time, explore much further separately on each axis.
        lc = dxt1_block::unpack_color(m_best_solution.m_coords.m_low_color, false);

        for (int a = 0; a < 3; a++) {
          int limit = (a == 1) ? 63 : 31;

          for (int s = -2; s <= 2; s += 4) {
            color_quad_u8 c(lc);
            int q = c[a] + s;
            if ((q < 0) || (q > limit))
              continue;

            c[a] = (uint8)q;

            dxt1_solution_coordinates coords(dxt1_block::pack_color(c, false), m_best_solution.m_coords.m_high_color);
            coords.canonicalize();
            evaluate_solution(coords);
          }
        }
      }

      // Generate new candidates by exploring the high color's direct lattice neighbors
      color_quad_u8 hc(dxt1_block::unpack_color(m_best_solution.m_coords.m_high_color, false));

      for (int i = 0; i < 26; i++) {
        int r = hc.r + g_adjacency[i].x;
        if ((r < 0) || (r > 31))
          continue;

        int g = hc.g + g_adjacency[i].y;
        if ((g < 0) || (g > 63))
          continue;

        int b = hc.b + g_adjacency[i].z;
        if ((b < 0) || (b > 31))
          continue;

        dxt1_solution_coordinates coords(m_best_solution.m_coords.m_low_color, dxt1_block::pack_color(r, g, b, false));
        coords.canonicalize();
        evaluate_solution(coords);
      }

      if (m_pParams->m_quality == cCRNDXTQualityUber) {
        // Generate new candidates by exploring the high color's direct lattice neighbors - this time, explore much further separately on each axis.
        hc = dxt1_block::unpack_color(m_best_solution.m_coords.m_high_color, false);

        for (int a = 0; a < 3; a++) {
          int limit = (a == 1) ? 63 : 31;

          for (int s = -2; s <= 2; s += 4) {
            color_quad_u8 c(hc);
            int q = c[a] + s;
            if ((q < 0) || (q > limit))
              continue;

            c[a] = (uint8)q;

            dxt1_solution_coordinates coords(m_best_solution.m_coords.m_low_color, dxt1_block::pack_color(c, false));
            coords.canonicalize();
            evaluate_solution(coords);
          }
        }
      }
    }

    if ((!m_best_solution.m_error) || ((pass) && (m_best_solution.m_error == prev_best_error)))
      break;

    if (m_pParams->m_quality >= cCRNDXTQualityUber) {
      // Attempt to refine current solution's endpoints given the current selectors using least squares.
      refine_solution(1);
    }
  }

  if (m_pParams->m_quality >= cCRNDXTQualityNormal) {
    if ((m_best_solution.m_error) && (!m_pParams->m_pixels_have_alpha)) {
      bool choose_solid_block = false;
      if (m_best_solution.are_selectors_all_equal()) {
        // All selectors equal - try various solid-block optimizations
        choose_solid_block = try_average_block_as_solid();
      }

      if ((!choose_solid_block) && (m_pParams->m_quality == cCRNDXTQualityUber)) {
        // Per-component 1D endpoint optimization.
        optimize_endpoint_comps();
      }
    }

    if (m_pParams->m_quality == cCRNDXTQualityUber) {
      if (m_best_solution.m_error) {
        // The pixels may have already been DXTc compressed by another compressor.
        // It's usually possible to recover the endpoints used to previously pack the block.
        try_combinatorial_encoding();
      }
    }
  }

  return_solution();

  if (m_pParams->m_endpoint_caching) {
    // Remember result for later reruse.
    m_prev_results[m_num_prev_results & (cMaxPrevResults - 1)] = m_best_solution.m_coords;
    m_num_prev_results++;
  }
}

void dxt1_endpoint_optimizer::handle_multicolor_block() {
  uint num_passes = 1;
  vec3F perceptual_weights(1.0f);

  if (m_perceptual) {
    // Compute RGB weighting for use in perceptual mode.
    // The more saturated the block, the more the weights deviate from (1,1,1).
    float ave_redness = 0;
    float ave_blueness = 0;
    float ave_l = 0;

    for (uint i = 0; i < m_unique_colors.size(); i++) {
      const color_quad_u8& c = m_unique_colors[i].m_color;
      int l = (c.r + c.g + c.b + 1) / 3;
      float scale = (float)m_unique_colors[i].m_weight / math::maximum<float>(1.0f, l);
      ave_redness += scale * c.r;
      ave_blueness += scale * c.b;
      ave_l += l;
    }

    ave_redness /= m_total_unique_color_weight;
    ave_blueness /= m_total_unique_color_weight;
    ave_l /= m_total_unique_color_weight;
    ave_l = math::minimum(1.0f, ave_l * 16.0f / 255.0f);

    float p = ave_l * powf(math::saturate(math::maximum(ave_redness, ave_blueness) * 1.0f / 3.0f), 2.75f);

    if (p >= 1.0f)
      num_passes = 1;
    else {
      num_passes = 2;
      perceptual_weights = vec3F::lerp(vec3F(.212f, .72f, .072f), perceptual_weights, p);
    }
  }

  for (uint pass_index = 0; pass_index < num_passes; pass_index++) {
    compute_vectors(perceptual_weights);
    compute_pca(m_principle_axis, m_norm_unique_colors_weighted, vec3F(.2837149f, 0.9540631f, 0.096277453f));
    m_principle_axis[0] /= perceptual_weights[0];
    m_principle_axis[1] /= perceptual_weights[1];
    m_principle_axis[2] /= perceptual_weights[2];
    m_principle_axis.normalize_in_place();
    if (num_passes > 1) {
      // Check for obviously wild principle axes and try to compensate by backing off the component weightings.
      if (fabs(m_principle_axis[0]) >= .795f)
        perceptual_weights.set(.424f, .6f, .072f);
      else if (fabs(m_principle_axis[2]) >= .795f)
        perceptual_weights.set(.212f, .6f, .212f);
      else
        break;
    }
  }

  // Find bounds of projection onto (potentially skewed) principle axis.
  float l = 1e+9;
  float h = -1e+9;

  for (uint i = 0; i < m_norm_unique_colors.size(); i++) {
    float d = m_norm_unique_colors[i] * m_principle_axis;
    l = math::minimum(l, d);
    h = math::maximum(h, d);
  }

  vec3F low_color(m_mean_norm_color + l * m_principle_axis);
  vec3F high_color(m_mean_norm_color + h * m_principle_axis);

  if (!low_color.is_within_bounds(0.0f, 1.0f)) {
    // Low color is outside the lattice, so bring it back in by casting a ray.
    vec3F coord;
    float t;
    aabb3F bounds(vec3F(0.0f), vec3F(1.0f));
    intersection::result res = intersection::ray_aabb(coord, t, ray3F(low_color, m_principle_axis), bounds);
    if (res == intersection::cSuccess)
      low_color = coord;
  }

  if (!high_color.is_within_bounds(0.0f, 1.0f)) {
    // High color is outside the lattice, so bring it back in by casting a ray.
    vec3F coord;
    float t;
    aabb3F bounds(vec3F(0.0f), vec3F(1.0f));
    intersection::result res = intersection::ray_aabb(coord, t, ray3F(high_color, -m_principle_axis), bounds);
    if (res == intersection::cSuccess)
      high_color = coord;
  }

  // Now optimize the endpoints using the projection bounds on the (potentially skewed) principle axis as a starting point.
  optimize_endpoints(low_color, high_color);
}

// Tries quantizing the block to 4 colors using vanilla LBG. It tries all combinations of the quantized results as potential endpoints.
bool dxt1_endpoint_optimizer::try_median4(const vec3F& low_color, const vec3F& high_color) {
  vec3F means[4];

  if (m_unique_colors.size() <= 4) {
    for (uint i = 0; i < 4; i++)
      means[i] = m_norm_unique_colors[math::minimum<int>(m_norm_unique_colors.size() - 1, i)];
  } else {
    means[0] = low_color - m_mean_norm_color;
    means[3] = high_color - m_mean_norm_color;
    means[1] = vec3F::lerp(means[0], means[3], 1.0f / 3.0f);
    means[2] = vec3F::lerp(means[0], means[3], 2.0f / 3.0f);

    fast_random rm;

    const uint cMaxIters = 8;
    uint reassign_rover = 0;
    float prev_total_dist = math::cNearlyInfinite;
    for (uint iter = 0; iter < cMaxIters; iter++) {
      vec3F new_means[4];
      float new_weights[4];
      utils::zero_object(new_means);
      utils::zero_object(new_weights);

      float total_dist = 0;

      for (uint i = 0; i < m_unique_colors.size(); i++) {
        const vec3F& v = m_norm_unique_colors[i];

        float best_dist = means[0].squared_distance(v);
        int best_index = 0;

        for (uint j = 1; j < 4; j++) {
          float dist = means[j].squared_distance(v);
          if (dist < best_dist) {
            best_dist = dist;
            best_index = j;
          }
        }

        total_dist += best_dist;

        new_means[best_index] += v * (float)m_unique_colors[i].m_weight;
        new_weights[best_index] += (float)m_unique_colors[i].m_weight;
      }

      uint highest_index = 0;
      float highest_weight = 0;
      bool empty_cell = false;
      for (uint j = 0; j < 4; j++) {
        if (new_weights[j] > 0.0f) {
          means[j] = new_means[j] / new_weights[j];
          if (new_weights[j] > highest_weight) {
            highest_weight = new_weights[j];
            highest_index = j;
          }
        } else
          empty_cell = true;
      }

      if (!empty_cell) {
        if (fabs(total_dist - prev_total_dist) < .00001f)
          break;

        prev_total_dist = total_dist;
      } else
        prev_total_dist = math::cNearlyInfinite;

      if ((empty_cell) && (iter != (cMaxIters - 1))) {
        const uint ri = (highest_index + reassign_rover) & 3;
        reassign_rover++;

        for (uint j = 0; j < 4; j++) {
          if (new_weights[j] == 0.0f) {
            means[j] = means[ri];
            means[j] += vec3F::make_random(rm, -.00196f, .00196f);
          }
        }
      }
    }
  }

  bool improved = false;

  for (uint i = 0; i < 3; i++) {
    for (uint j = i + 1; j < 4; j++) {
      const vec3F v0(means[i] + m_mean_norm_color);
      const vec3F v1(means[j] + m_mean_norm_color);

      dxt1_solution_coordinates sc(
          color_quad_u8((int)floor(.5f + v0[0] * 31.0f), (int)floor(.5f + v0[1] * 63.0f), (int)floor(.5f + v0[2] * 31.0f), 255),
          color_quad_u8((int)floor(.5f + v1[0] * 31.0f), (int)floor(.5f + v1[1] * 63.0f), (int)floor(.5f + v1[2] * 31.0f), 255), false);

      sc.canonicalize();
      improved |= evaluate_solution(sc);
    }
  }

  improved |= refine_solution((m_pParams->m_quality == cCRNDXTQualityUber) ? 1 : 0);

  return improved;
}

// Given candidate low/high endpoints, find the optimal selectors for 3 and 4 color blocks, compute the resulting error,
// and use the candidate if it results in less error than the best found result so far.
bool dxt1_endpoint_optimizer::evaluate_solution(const dxt1_solution_coordinates& coords, bool alternate_rounding) {
  color_quad_u8 c0 = dxt1_block::unpack_color(coords.m_low_color, false);
  color_quad_u8 c1 = dxt1_block::unpack_color(coords.m_high_color, false);
  uint64 rError = c0.r < c1.r ? m_rDist[c0.r].low + m_rDist[c1.r].high : m_rDist[c0.r].high + m_rDist[c1.r].low;
  uint64 gError = c0.g < c1.g ? m_gDist[c0.g].low + m_gDist[c1.g].high : m_gDist[c0.g].high + m_gDist[c1.g].low;
  uint64 bError = c0.b < c1.b ? m_bDist[c0.b].low + m_bDist[c1.b].high : m_bDist[c0.b].high + m_bDist[c1.b].low;
  if (rError + gError + bError >= m_best_solution.m_error)
    return false;
  if (!alternate_rounding) {
    solution_hash_map::insert_result solution_res(m_solutions_tried.insert(coords.m_low_color | coords.m_high_color << 16));
    if (!solution_res.second)
      return false;
  }
  if (m_evaluate_hc)
    return m_perceptual ? evaluate_solution_hc_perceptual(coords, alternate_rounding) : evaluate_solution_hc_uniform(coords, alternate_rounding);
  if (m_pParams->m_quality >= cCRNDXTQualityBetter)
    return evaluate_solution_uber(coords, alternate_rounding);
  return evaluate_solution_fast(coords, alternate_rounding);
}

inline uint dxt1_endpoint_optimizer::color_distance(bool perceptual, const color_quad_u8& e1, const color_quad_u8& e2, bool alpha) {
  if (perceptual) {
    return color::color_distance(true, e1, e2, alpha);
  } else if (m_pParams->m_grayscale_sampling) {
    // Computes error assuming shader will be converting the result to grayscale.
    int y0 = color::RGB_to_Y(e1);
    int y1 = color::RGB_to_Y(e2);
    int yd = y0 - y1;
    if (alpha) {
      int da = (int)e1[3] - (int)e2[3];
      return yd * yd + da * da;
    } else {
      return yd * yd;
    }
  } else {
    return color::color_distance(false, e1, e2, alpha);
  }
}

bool dxt1_endpoint_optimizer::evaluate_solution_uber(const dxt1_solution_coordinates& coords, bool alternate_rounding) {
  m_trial_solution.m_coords = coords;
  m_trial_solution.m_selectors.resize(m_unique_colors.size());
  m_trial_solution.m_error = m_best_solution.m_error;
  m_trial_solution.m_alpha_block = false;

  uint first_block_type = 0;
  uint last_block_type = 1;

  if ((m_pParams->m_pixels_have_alpha) || (m_pParams->m_force_alpha_blocks))
    first_block_type = 1;
  else if (!m_pParams->m_use_alpha_blocks)
    last_block_type = 0;

  m_trial_selectors.resize(m_unique_colors.size());

  color_quad_u8 colors[cDXT1SelectorValues];

  colors[0] = dxt1_block::unpack_color(coords.m_low_color, true);
  colors[1] = dxt1_block::unpack_color(coords.m_high_color, true);

  for (uint block_type = first_block_type; block_type <= last_block_type; block_type++) {
    uint64 trial_error = 0;

    if (!block_type) {
      colors[2].set_noclamp_rgba((colors[0].r * 2 + colors[1].r + alternate_rounding) / 3, (colors[0].g * 2 + colors[1].g + alternate_rounding) / 3, (colors[0].b * 2 + colors[1].b + alternate_rounding) / 3, 0);
      colors[3].set_noclamp_rgba((colors[1].r * 2 + colors[0].r + alternate_rounding) / 3, (colors[1].g * 2 + colors[0].g + alternate_rounding) / 3, (colors[1].b * 2 + colors[0].b + alternate_rounding) / 3, 0);

      if (m_perceptual) {
        for (int unique_color_index = (int)m_unique_colors.size() - 1; unique_color_index >= 0; unique_color_index--) {
          const color_quad_u8& c = m_unique_colors[unique_color_index].m_color;

          uint best_error = color_distance(true, c, colors[0], false);
          uint best_color_index = 0;

          uint err = color_distance(true, c, colors[1], false);
          if (err < best_error) {
            best_error = err;
            best_color_index = 1;
          }

          err = color_distance(true, c, colors[2], false);
          if (err < best_error) {
            best_error = err;
            best_color_index = 2;
          }

          err = color_distance(true, c, colors[3], false);
          if (err < best_error) {
            best_error = err;
            best_color_index = 3;
          }

          trial_error += best_error * static_cast<uint64>(m_unique_colors[unique_color_index].m_weight);
          if (trial_error >= m_trial_solution.m_error)
            break;

          m_trial_selectors[unique_color_index] = static_cast<uint8>(best_color_index);
        }
      } else {
        for (int unique_color_index = (int)m_unique_colors.size() - 1; unique_color_index >= 0; unique_color_index--) {
          const color_quad_u8& c = m_unique_colors[unique_color_index].m_color;

          uint best_error = color_distance(false, c, colors[0], false);
          uint best_color_index = 0;

          uint err = color_distance(false, c, colors[1], false);
          if (err < best_error) {
            best_error = err;
            best_color_index = 1;
          }

          err = color_distance(false, c, colors[2], false);
          if (err < best_error) {
            best_error = err;
            best_color_index = 2;
          }

          err = color_distance(false, c, colors[3], false);
          if (err < best_error) {
            best_error = err;
            best_color_index = 3;
          }

          trial_error += best_error * static_cast<uint64>(m_unique_colors[unique_color_index].m_weight);
          if (trial_error >= m_trial_solution.m_error)
            break;

          m_trial_selectors[unique_color_index] = static_cast<uint8>(best_color_index);
        }
      }
    } else {
      colors[2].set_noclamp_rgba((colors[0].r + colors[1].r + alternate_rounding) >> 1, (colors[0].g + colors[1].g + alternate_rounding) >> 1, (colors[0].b + colors[1].b + alternate_rounding) >> 1, 255U);

      if (m_perceptual) {
        for (int unique_color_index = (int)m_unique_colors.size() - 1; unique_color_index >= 0; unique_color_index--) {
          const color_quad_u8& c = m_unique_colors[unique_color_index].m_color;

          uint best_error = color_distance(true, c, colors[0], false);
          uint best_color_index = 0;

          uint err = color_distance(true, c, colors[1], false);
          if (err < best_error) {
            best_error = err;
            best_color_index = 1;
          }

          err = color_distance(true, c, colors[2], false);
          if (err < best_error) {
            best_error = err;
            best_color_index = 2;
          }

          trial_error += best_error * static_cast<uint64>(m_unique_colors[unique_color_index].m_weight);
          if (trial_error >= m_trial_solution.m_error)
            break;

          m_trial_selectors[unique_color_index] = static_cast<uint8>(best_color_index);
        }
      } else {
        for (int unique_color_index = (int)m_unique_colors.size() - 1; unique_color_index >= 0; unique_color_index--) {
          const color_quad_u8& c = m_unique_colors[unique_color_index].m_color;

          uint best_error = color_distance(false, c, colors[0], false);
          uint best_color_index = 0;

          uint err = color_distance(false, c, colors[1], false);
          if (err < best_error) {
            best_error = err;
            best_color_index = 1;
          }

          err = color_distance(false, c, colors[2], false);
          if (err < best_error) {
            best_error = err;
            best_color_index = 2;
          }

          trial_error += best_error * static_cast<uint64>(m_unique_colors[unique_color_index].m_weight);
          if (trial_error >= m_trial_solution.m_error)
            break;

          m_trial_selectors[unique_color_index] = static_cast<uint8>(best_color_index);
        }
      }
    }

    if (trial_error < m_trial_solution.m_error) {
      m_trial_solution.m_error = trial_error;
      m_trial_solution.m_alpha_block = (block_type != 0);
      m_trial_solution.m_selectors = m_trial_selectors;
      m_trial_solution.m_alternate_rounding = alternate_rounding;
    }
  }

  m_trial_solution.m_enforce_selector = !m_trial_solution.m_alpha_block && m_trial_solution.m_coords.m_low_color == m_trial_solution.m_coords.m_high_color;
  if (m_trial_solution.m_enforce_selector) {
    uint s;
    if ((m_trial_solution.m_coords.m_low_color & 31) != 31) {
      m_trial_solution.m_coords.m_low_color++;
      s = 1;
    } else {
      m_trial_solution.m_coords.m_high_color--;
      s = 0;
    }

    for (uint i = 0; i < m_unique_colors.size(); i++)
      m_trial_solution.m_selectors[i] = static_cast<uint8>(s);
    m_trial_solution.m_enforced_selector = s;
  }

  if (m_trial_solution.m_error < m_best_solution.m_error) {
    m_best_solution = m_trial_solution;
    return true;
  }

  return false;
}

bool dxt1_endpoint_optimizer::evaluate_solution_fast(const dxt1_solution_coordinates& coords, bool alternate_rounding) {
  m_trial_solution.m_coords = coords;
  m_trial_solution.m_selectors.resize(m_unique_colors.size());
  m_trial_solution.m_error = m_best_solution.m_error;
  m_trial_solution.m_alpha_block = false;

  uint first_block_type = 0;
  uint last_block_type = 1;

  if ((m_pParams->m_pixels_have_alpha) || (m_pParams->m_force_alpha_blocks))
    first_block_type = 1;
  else if (!m_pParams->m_use_alpha_blocks)
    last_block_type = 0;

  m_trial_selectors.resize(m_unique_colors.size());

  color_quad_u8 colors[cDXT1SelectorValues];
  colors[0] = dxt1_block::unpack_color(coords.m_low_color, true);
  colors[1] = dxt1_block::unpack_color(coords.m_high_color, true);

  int vr = colors[1].r - colors[0].r;
  int vg = colors[1].g - colors[0].g;
  int vb = colors[1].b - colors[0].b;
  if (m_perceptual) {
    vr *= 8;
    vg *= 24;
  }

  int stops[4];
  stops[0] = colors[0].r * vr + colors[0].g * vg + colors[0].b * vb;
  stops[1] = colors[1].r * vr + colors[1].g * vg + colors[1].b * vb;

  int dirr = vr * 2;
  int dirg = vg * 2;
  int dirb = vb * 2;

  for (uint block_type = first_block_type; block_type <= last_block_type; block_type++) {
    uint64 trial_error = 0;

    if (!block_type) {
      colors[2].set_noclamp_rgba((colors[0].r * 2 + colors[1].r + alternate_rounding) / 3, (colors[0].g * 2 + colors[1].g + alternate_rounding) / 3, (colors[0].b * 2 + colors[1].b + alternate_rounding) / 3, 255U);
      colors[3].set_noclamp_rgba((colors[1].r * 2 + colors[0].r + alternate_rounding) / 3, (colors[1].g * 2 + colors[0].g + alternate_rounding) / 3, (colors[1].b * 2 + colors[0].b + alternate_rounding) / 3, 255U);

      stops[2] = colors[2].r * vr + colors[2].g * vg + colors[2].b * vb;
      stops[3] = colors[3].r * vr + colors[3].g * vg + colors[3].b * vb;

      // 0 2 3 1
      int c0Point = stops[1] + stops[3];
      int halfPoint = stops[3] + stops[2];
      int c3Point = stops[2] + stops[0];

      for (int unique_color_index = (int)m_unique_colors.size() - 1; unique_color_index >= 0; unique_color_index--) {
        const color_quad_u8& c = m_unique_colors[unique_color_index].m_color;

        int dot = c.r * dirr + c.g * dirg + c.b * dirb;

        uint8 best_color_index;
        if (dot < halfPoint)
          best_color_index = (dot < c3Point) ? 0 : 2;
        else
          best_color_index = (dot < c0Point) ? 3 : 1;

        uint best_error = color_distance(m_perceptual, c, colors[best_color_index], false);

        trial_error += best_error * static_cast<uint64>(m_unique_colors[unique_color_index].m_weight);
        if (trial_error >= m_trial_solution.m_error)
          break;

        m_trial_selectors[unique_color_index] = static_cast<uint8>(best_color_index);
      }
    } else {
      colors[2].set_noclamp_rgba((colors[0].r + colors[1].r + alternate_rounding) >> 1, (colors[0].g + colors[1].g + alternate_rounding) >> 1, (colors[0].b + colors[1].b + alternate_rounding) >> 1, 255U);

      stops[2] = colors[2].r * vr + colors[2].g * vg + colors[2].b * vb;

      // 0 2 1
      int c02Point = stops[0] + stops[2];
      int c21Point = stops[2] + stops[1];

      for (int unique_color_index = (int)m_unique_colors.size() - 1; unique_color_index >= 0; unique_color_index--) {
        const color_quad_u8& c = m_unique_colors[unique_color_index].m_color;

        int dot = c.r * dirr + c.g * dirg + c.b * dirb;

        uint8 best_color_index;
        if (dot < c02Point)
          best_color_index = 0;
        else if (dot < c21Point)
          best_color_index = 2;
        else
          best_color_index = 1;

        uint best_error = color_distance(m_perceptual, c, colors[best_color_index], false);

        trial_error += best_error * static_cast<uint64>(m_unique_colors[unique_color_index].m_weight);
        if (trial_error >= m_trial_solution.m_error)
          break;

        m_trial_selectors[unique_color_index] = static_cast<uint8>(best_color_index);
      }
    }

    if (trial_error < m_trial_solution.m_error) {
      m_trial_solution.m_error = trial_error;
      m_trial_solution.m_alpha_block = (block_type != 0);
      m_trial_solution.m_selectors = m_trial_selectors;
    }
  }

  if ((!m_trial_solution.m_alpha_block) && (m_trial_solution.m_coords.m_low_color == m_trial_solution.m_coords.m_high_color)) {
    uint s;
    if ((m_trial_solution.m_coords.m_low_color & 31) != 31) {
      m_trial_solution.m_coords.m_low_color++;
      s = 1;
    } else {
      m_trial_solution.m_coords.m_high_color--;
      s = 0;
    }

    for (uint i = 0; i < m_unique_colors.size(); i++)
      m_trial_solution.m_selectors[i] = static_cast<uint8>(s);
  }

  if (m_trial_solution.m_error < m_best_solution.m_error) {
    m_best_solution = m_trial_solution;
    return true;
  }

  return false;
}

bool dxt1_endpoint_optimizer::evaluate_solution_hc_perceptual(const dxt1_solution_coordinates& coords, bool alternate_rounding) {
  color_quad_u8 c0 = dxt1_block::unpack_color(coords.m_low_color, true);
  color_quad_u8 c1 = dxt1_block::unpack_color(coords.m_high_color, true);
  color_quad_u8 c2((c0.r * 2 + c1.r + alternate_rounding) / 3, (c0.g * 2 + c1.g + alternate_rounding) / 3, (c0.b * 2 + c1.b + alternate_rounding) / 3, 0);
  color_quad_u8 c3((c1.r * 2 + c0.r + alternate_rounding) / 3, (c1.g * 2 + c0.g + alternate_rounding) / 3, (c1.b * 2 + c0.b + alternate_rounding) / 3, 0);
  uint64 error = 0;
  unique_color* color = m_evaluated_colors.get_ptr();
  for (uint count = m_evaluated_colors.size(); count; color++, error < m_best_solution.m_error ? count-- : count = 0) {
    uint e01 = math::minimum(color::color_distance(true, color->m_color, c0, false), color::color_distance(true, color->m_color, c1, false));
    uint e23 = math::minimum(color::color_distance(true, color->m_color, c2, false), color::color_distance(true, color->m_color, c3, false));
    error += math::minimum(e01, e23) * (uint64)color->m_weight;
  }
  if (error >= m_best_solution.m_error)
    return false;
  m_best_solution.m_coords = coords;
  m_best_solution.m_error = error;
  m_best_solution.m_alpha_block = false;
  m_best_solution.m_alternate_rounding = alternate_rounding;
  m_best_solution.m_enforce_selector = m_best_solution.m_coords.m_low_color == m_best_solution.m_coords.m_high_color;
  if (m_best_solution.m_enforce_selector) {
    if ((m_best_solution.m_coords.m_low_color & 31) != 31) {
      m_best_solution.m_coords.m_low_color++;
      m_best_solution.m_enforced_selector = 1;
    } else {
      m_best_solution.m_coords.m_high_color--;
      m_best_solution.m_enforced_selector = 0;
    }
  }
  return true;
}

bool dxt1_endpoint_optimizer::evaluate_solution_hc_uniform(const dxt1_solution_coordinates& coords, bool alternate_rounding) {
  color_quad_u8 c0 = dxt1_block::unpack_color(coords.m_low_color, true);
  color_quad_u8 c1 = dxt1_block::unpack_color(coords.m_high_color, true);
  color_quad_u8 c2((c0.r * 2 + c1.r + alternate_rounding) / 3, (c0.g * 2 + c1.g + alternate_rounding) / 3, (c0.b * 2 + c1.b + alternate_rounding) / 3, 0);
  color_quad_u8 c3((c1.r * 2 + c0.r + alternate_rounding) / 3, (c1.g * 2 + c0.g + alternate_rounding) / 3, (c1.b * 2 + c0.b + alternate_rounding) / 3, 0);
  uint64 error = 0;
  unique_color* color = m_evaluated_colors.get_ptr();
  for (uint count = m_evaluated_colors.size(); count; color++, error < m_best_solution.m_error ? count-- : count = 0) {
    uint e01 = math::minimum(color::color_distance(false, color->m_color, c0, false), color::color_distance(false, color->m_color, c1, false));
    uint e23 = math::minimum(color::color_distance(false, color->m_color, c2, false), color::color_distance(false, color->m_color, c3, false));
    error += math::minimum(e01, e23) * (uint64)color->m_weight;
  }
  if (error >= m_best_solution.m_error)
    return false;
  m_best_solution.m_coords = coords;
  m_best_solution.m_error = error;
  m_best_solution.m_alpha_block = false;
  m_best_solution.m_alternate_rounding = alternate_rounding;
  m_best_solution.m_enforce_selector = m_best_solution.m_coords.m_low_color == m_best_solution.m_coords.m_high_color;
  if (m_best_solution.m_enforce_selector) {
    if ((m_best_solution.m_coords.m_low_color & 31) != 31) {
      m_best_solution.m_coords.m_low_color++;
      m_best_solution.m_enforced_selector = 1;
    } else {
      m_best_solution.m_coords.m_high_color--;
      m_best_solution.m_enforced_selector = 0;
    }
  }
  return true;
}

void dxt1_endpoint_optimizer::compute_selectors() {
  if (m_evaluate_hc)
    compute_selectors_hc();
}

void dxt1_endpoint_optimizer::compute_selectors_hc() {
  m_best_solution.m_selectors.resize(m_unique_colors.size());
  if (m_best_solution.m_enforce_selector) {
    memset(m_best_solution.m_selectors.get_ptr(), m_best_solution.m_enforced_selector, m_best_solution.m_selectors.size());
    return;
  }
  color_quad_u8 c0 = dxt1_block::unpack_color(m_best_solution.m_coords.m_low_color, true);
  color_quad_u8 c1 = dxt1_block::unpack_color(m_best_solution.m_coords.m_high_color, true);
  color_quad_u8 c2((c0.r * 2 + c1.r + m_best_solution.m_alternate_rounding) / 3, (c0.g * 2 + c1.g + m_best_solution.m_alternate_rounding) / 3, (c0.b * 2 + c1.b + m_best_solution.m_alternate_rounding) / 3, 0);
  color_quad_u8 c3((c1.r * 2 + c0.r + m_best_solution.m_alternate_rounding) / 3, (c1.g * 2 + c0.g + m_best_solution.m_alternate_rounding) / 3, (c1.b * 2 + c0.b + m_best_solution.m_alternate_rounding) / 3, 0);
  for (uint i = 0, iEnd = m_unique_colors.size(); i < iEnd; i++) {
    const color_quad_u8& c = m_unique_colors[i].m_color;
    uint e0 = color::color_distance(m_perceptual, c, c0, false);
    uint e1 = color::color_distance(m_perceptual, c, c1, false);
    uint e2 = color::color_distance(m_perceptual, c, c2, false);
    uint e3 = color::color_distance(m_perceptual, c, c3, false);
    uint e01 = math::minimum(e0, e1);
    uint e23 = math::minimum(e2, e3);
    m_best_solution.m_selectors[i] = e01 <= e23 ? e01 == e0 ? 0 : 1 : e23 == e2 ? 2 : 3;
  }
}

unique_color dxt1_endpoint_optimizer::lerp_color(const color_quad_u8& a, const color_quad_u8& b, float f, int rounding) {
  color_quad_u8 res;

  float r = rounding ? 1.0f : 0.0f;
  res[0] = static_cast<uint8>(math::clamp(math::float_to_int(r + math::lerp<float>(a[0], b[0], f)), 0, 255));
  res[1] = static_cast<uint8>(math::clamp(math::float_to_int(r + math::lerp<float>(a[1], b[1], f)), 0, 255));
  res[2] = static_cast<uint8>(math::clamp(math::float_to_int(r + math::lerp<float>(a[2], b[2], f)), 0, 255));
  res[3] = 255;

  return unique_color(res, 1);
}

// The block may have been already compressed using another DXTc compressor, such as squish, ATI_Compress, ryg_dxt, etc.
// Attempt to recover the endpoints used by that block compressor.
void dxt1_endpoint_optimizer::try_combinatorial_encoding() {
  if ((m_unique_colors.size() < 2) || (m_unique_colors.size() > 4))
    return;

  m_temp_unique_colors = m_unique_colors;

  if (m_temp_unique_colors.size() == 2) {
    // a    b    c   d
    // 0.0  1/3 2/3  1.0

    for (uint k = 0; k < 2; k++) {
      for (uint q = 0; q < 2; q++) {
        const uint r = q ^ 1;

        // a b
        m_temp_unique_colors.push_back(lerp_color(m_temp_unique_colors[q].m_color, m_temp_unique_colors[r].m_color, 2.0f, k));
        m_temp_unique_colors.push_back(lerp_color(m_temp_unique_colors[q].m_color, m_temp_unique_colors[r].m_color, 3.0f, k));

        // a c
        m_temp_unique_colors.push_back(lerp_color(m_temp_unique_colors[q].m_color, m_temp_unique_colors[r].m_color, .5f, k));
        m_temp_unique_colors.push_back(lerp_color(m_temp_unique_colors[q].m_color, m_temp_unique_colors[r].m_color, 1.5f, k));

        // a d

        // b c
        m_temp_unique_colors.push_back(lerp_color(m_temp_unique_colors[q].m_color, m_temp_unique_colors[r].m_color, -1.0f, k));
        m_temp_unique_colors.push_back(lerp_color(m_temp_unique_colors[q].m_color, m_temp_unique_colors[r].m_color, 2.0f, k));

        // b d
        m_temp_unique_colors.push_back(lerp_color(m_temp_unique_colors[q].m_color, m_temp_unique_colors[r].m_color, -.5f, k));
        m_temp_unique_colors.push_back(lerp_color(m_temp_unique_colors[q].m_color, m_temp_unique_colors[r].m_color, .5f, k));

        // c d
        m_temp_unique_colors.push_back(lerp_color(m_temp_unique_colors[q].m_color, m_temp_unique_colors[r].m_color, -2.0f, k));
        m_temp_unique_colors.push_back(lerp_color(m_temp_unique_colors[q].m_color, m_temp_unique_colors[r].m_color, -1.0f, k));
      }
    }
  } else if (m_temp_unique_colors.size() == 3) {
    // a    b    c   d
    // 0.0  1/3 2/3  1.0

    for (uint i = 0; i <= 2; i++) {
      for (uint j = 0; j <= 2; j++) {
        if (i == j)
          continue;

        // a b c
        m_temp_unique_colors.push_back(lerp_color(m_temp_unique_colors[i].m_color, m_temp_unique_colors[j].m_color, 1.5f));

        // a b d
        m_temp_unique_colors.push_back(lerp_color(m_temp_unique_colors[i].m_color, m_temp_unique_colors[j].m_color, 2.0f / 3.0f));

        // a c d
        m_temp_unique_colors.push_back(lerp_color(m_temp_unique_colors[i].m_color, m_temp_unique_colors[j].m_color, 1.0f / 3.0f));

        // b c d
        m_temp_unique_colors.push_back(lerp_color(m_temp_unique_colors[i].m_color, m_temp_unique_colors[j].m_color, -.5f));
      }
    }
  }

  m_unique_packed_colors.resize(0);

  for (uint i = 0; i < m_temp_unique_colors.size(); i++) {
    const color_quad_u8& unique_color = m_temp_unique_colors[i].m_color;
    const uint16 packed_color = dxt1_block::pack_color(unique_color, true);

    if (std::find(m_unique_packed_colors.begin(), m_unique_packed_colors.end(), packed_color) != m_unique_packed_colors.end())
      continue;

    m_unique_packed_colors.push_back(packed_color);
  }

  for (uint i = 0; m_best_solution.m_error && i < m_unique_packed_colors.size() - 1; i++) {
    for (uint j = i + 1; m_best_solution.m_error && j < m_unique_packed_colors.size(); j++)
      evaluate_solution(dxt1_solution_coordinates(m_unique_packed_colors[i], m_unique_packed_colors[j]));
  }
  uint64 error = m_best_solution.m_error;
  if (error)
    m_best_solution.m_error = 1;
  for (uint i = 0; m_best_solution.m_error && i < m_unique_packed_colors.size() - 1; i++) {
    for (uint j = i + 1; m_best_solution.m_error && j < m_unique_packed_colors.size(); j++)
      evaluate_solution(dxt1_solution_coordinates(m_unique_packed_colors[i], m_unique_packed_colors[j]), true);
  }
  if (m_best_solution.m_error)
    m_best_solution.m_error = error;
  
}

// The fourth (transparent) color in 3 color "transparent" blocks is black, which can be optionally exploited for small gains in DXT1 mode if the caller
// doesn't actually use alpha. (But not in DXT5 mode, because 3-color blocks aren't permitted by GPU's for DXT5.)
bool dxt1_endpoint_optimizer::try_alpha_as_black_optimization() {
  results* pOrig_results = m_pResults;

  uint num_dark_colors = 0;

  for (uint i = 0; i < m_unique_colors.size(); i++)
    if ((m_unique_colors[i].m_color[0] <= 4) && (m_unique_colors[i].m_color[1] <= 4) && (m_unique_colors[i].m_color[2] <= 4))
      num_dark_colors++;

  if ((!num_dark_colors) || (num_dark_colors == m_unique_colors.size()))
    return true;

  params trial_params(*m_pParams);
  crnlib::vector<color_quad_u8> trial_colors;
  trial_colors.insert(0, m_pParams->m_pPixels, m_pParams->m_num_pixels);

  trial_params.m_pPixels = trial_colors.get_ptr();
  trial_params.m_pixels_have_alpha = true;

  for (uint i = 0; i < trial_colors.size(); i++)
    if ((trial_colors[i][0] <= 4) && (trial_colors[i][1] <= 4) && (trial_colors[i][2] <= 4))
      trial_colors[i][3] = 0;

  results trial_results;

  crnlib::vector<uint8> trial_selectors(m_pParams->m_num_pixels);
  trial_results.m_pSelectors = trial_selectors.get_ptr();

  compute_internal(trial_params, trial_results);

  CRNLIB_ASSERT(trial_results.m_alpha_block);

  color_quad_u8 c[4];
  dxt1_block::get_block_colors3(c, trial_results.m_low_color, trial_results.m_high_color);

  uint64 trial_error = 0;

  for (uint i = 0; i < trial_colors.size(); i++) {
    if (trial_colors[i][3] == 0) {
      CRNLIB_ASSERT(trial_selectors[i] == 3);
    } else {
      CRNLIB_ASSERT(trial_selectors[i] != 3);
    }

    trial_error += color_distance(m_perceptual, trial_colors[i], c[trial_selectors[i]], false);
  }

  if (trial_error < pOrig_results->m_error) {
    pOrig_results->m_error = trial_error;

    pOrig_results->m_low_color = trial_results.m_low_color;
    pOrig_results->m_high_color = trial_results.m_high_color;

    if (pOrig_results->m_pSelectors)
      memcpy(pOrig_results->m_pSelectors, trial_results.m_pSelectors, m_pParams->m_num_pixels);

    pOrig_results->m_alpha_block = true;
  }

  return true;
}

void dxt1_endpoint_optimizer::compute_internal(const params& p, results& r) {
  m_pParams = &p;
  m_pResults = &r;
  m_evaluate_hc = m_pParams->m_quality == cCRNDXTQualityUber && !m_pParams->m_pixels_have_alpha && !m_pParams->m_force_alpha_blocks
    && !m_pParams->m_use_alpha_blocks && !m_pParams->m_grayscale_sampling;
  m_perceptual = m_pParams->m_perceptual && !m_pParams->m_grayscale_sampling;
  if (m_unique_color_hash_map.get_table_size() > 8192)
    m_unique_color_hash_map.clear();
  else
    m_unique_color_hash_map.reset();
  if (m_solutions_tried.get_table_size() > 8192)
    m_solutions_tried.clear();
  else
    m_solutions_tried.reset();
  m_unique_colors.clear();
  m_norm_unique_colors.clear();
  m_mean_norm_color.clear();
  m_norm_unique_colors_weighted.clear();
  m_mean_norm_color_weighted.clear();
  m_principle_axis.clear();
  m_best_solution.clear();

  m_total_unique_color_weight = 0;
  m_unique_colors.reserve(m_pParams->m_num_pixels);
  unique_color color(color_quad_u8(0), 1);
  for (uint i = 0; i < m_pParams->m_num_pixels; i++) {
    if (!m_pParams->m_pixels_have_alpha || m_pParams->m_pPixels[i].a >= m_pParams->m_dxt1a_alpha_threshold) {
      color.m_color.m_u32 = m_pParams->m_pPixels[i].m_u32 | 0xFF000000;
      unique_color_hash_map::insert_result ins_result(m_unique_color_hash_map.insert(color.m_color.m_u32, m_unique_colors.size()));
      if (ins_result.second) {
        m_unique_colors.push_back(color);
      } else {
        m_unique_colors[ins_result.first->second].m_weight++;
      }
      m_total_unique_color_weight++;
    }
  }
  m_has_transparent_pixels = m_total_unique_color_weight != m_pParams->m_num_pixels;
  m_evaluated_colors = m_unique_colors;

  struct {
    uint64 weight, weightedColor, weightedSquaredColor;
  } rPlane[32] = {}, gPlane[64] = {}, bPlane[32] = {};

  for (uint i = 0; i < m_unique_colors.size(); i++) {
    const unique_color& color = m_unique_colors[i];
    uint8 R = color.m_color.r, r = (R >> 3) + ((R & 7) > (R >> 5) ? 1 : 0);
    rPlane[r].weight += color.m_weight;
    rPlane[r].weightedColor += (uint64)color.m_weight * R;
    rPlane[r].weightedSquaredColor += (uint64)color.m_weight * R * R;
    uint8 G = color.m_color.g, g = (G >> 2) + ((G & 3) > (G >> 6) ? 1 : 0);
    gPlane[g].weight += color.m_weight;
    gPlane[g].weightedColor += (uint64)color.m_weight * G;
    gPlane[g].weightedSquaredColor += (uint64)color.m_weight * G * G;
    uint8 B = color.m_color.b, b = (B >> 3) + ((B & 7) > (B >> 5) ? 1 : 0);
    bPlane[b].weight += color.m_weight;
    bPlane[b].weightedColor += (uint64)color.m_weight * B;
    bPlane[b].weightedSquaredColor += (uint64)color.m_weight * B * B;
  }

  if (m_perceptual) {
    for (uint c = 0; c < 32; c++) {
      rPlane[c].weight *= 8;
      rPlane[c].weightedColor *= 8;
      rPlane[c].weightedSquaredColor *= 8;
    }
    for (uint c = 0; c < 64; c++) {
      gPlane[c].weight *= 25;
      gPlane[c].weightedColor *= 25;
      gPlane[c].weightedSquaredColor *= 25;
    }
  }

   for (uint c = 1; c < 32; c++) {
    rPlane[c].weight += rPlane[c - 1].weight;
    rPlane[c].weightedColor += rPlane[c - 1].weightedColor;
    rPlane[c].weightedSquaredColor += rPlane[c - 1].weightedSquaredColor;
    bPlane[c].weight += bPlane[c - 1].weight;
    bPlane[c].weightedColor += bPlane[c - 1].weightedColor;
    bPlane[c].weightedSquaredColor += bPlane[c - 1].weightedSquaredColor;
  }

  for (uint c = 1; c < 64; c++) {
    gPlane[c].weight += gPlane[c - 1].weight;
    gPlane[c].weightedColor += gPlane[c - 1].weightedColor;
    gPlane[c].weightedSquaredColor += gPlane[c - 1].weightedSquaredColor;
  }

  for (uint c = 0; c < 32; c++) {
    uint8 C = c << 3 | c >> 2;
    m_rDist[c].low = rPlane[c].weightedSquaredColor + C * C * rPlane[c].weight - 2 * C * rPlane[c].weightedColor;
    m_rDist[c].high = rPlane[31].weightedSquaredColor + C * C * rPlane[31].weight - 2 * C * rPlane[31].weightedColor - m_rDist[c].low;
    m_bDist[c].low = bPlane[c].weightedSquaredColor + C * C * bPlane[c].weight - 2 * C * bPlane[c].weightedColor;
    m_bDist[c].high = bPlane[31].weightedSquaredColor + C * C * bPlane[31].weight - 2 * C * bPlane[31].weightedColor - m_bDist[c].low;
  }

  for (uint c = 0; c < 64; c++) {
    uint8 C = c << 2 | c >> 4;
    m_gDist[c].low = gPlane[c].weightedSquaredColor + C * C * gPlane[c].weight - 2 * C * gPlane[c].weightedColor;
    m_gDist[c].high = gPlane[63].weightedSquaredColor + C * C * gPlane[63].weight - 2 * C * gPlane[63].weightedColor - m_gDist[c].low;
  }

  if (!m_unique_colors.size()) {
    m_pResults->m_low_color = 0;
    m_pResults->m_high_color = 0;
    m_pResults->m_alpha_block = true;
    memset(m_pResults->m_pSelectors, 3, m_pParams->m_num_pixels);
  } else if (m_unique_colors.size() == 1 && !m_has_transparent_pixels) {
    int r = m_unique_colors[0].m_color.r;
    int g = m_unique_colors[0].m_color.g;
    int b = m_unique_colors[0].m_color.b;
    uint low_color = (ryg_dxt::OMatch5[r][0] << 11) | (ryg_dxt::OMatch6[g][0] << 5) | ryg_dxt::OMatch5[b][0];
    uint high_color = (ryg_dxt::OMatch5[r][1] << 11) | (ryg_dxt::OMatch6[g][1] << 5) | ryg_dxt::OMatch5[b][1];
    evaluate_solution(dxt1_solution_coordinates((uint16)low_color, (uint16)high_color));
    if (m_pParams->m_use_alpha_blocks && m_best_solution.m_error) {
      low_color = (ryg_dxt::OMatch5_3[r][0] << 11) | (ryg_dxt::OMatch6_3[g][0] << 5) | ryg_dxt::OMatch5_3[b][0];
      high_color = (ryg_dxt::OMatch5_3[r][1] << 11) | (ryg_dxt::OMatch6_3[g][1] << 5) | ryg_dxt::OMatch5_3[b][1];
      evaluate_solution(dxt1_solution_coordinates((uint16)low_color, (uint16)high_color));
    }
    return_solution();
  } else {
    handle_multicolor_block();
  }
}

bool dxt1_endpoint_optimizer::compute(const params& p, results& r) {
  if (!p.m_pPixels)
    return false;
  compute_internal(p, r);
  if (m_pParams->m_use_alpha_blocks && m_pParams->m_use_transparent_indices_for_black && !m_pParams->m_pixels_have_alpha)
    return try_alpha_as_black_optimization();
  return true;
}

}  // namespace crnlib
