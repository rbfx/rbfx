// File: crn_dxt_endpoint_refiner.cpp
// See Copyright Notice and license at the end of inc/crnlib.h
#include "crn_core.h"
#include "crn_dxt_endpoint_refiner.h"
#include "crn_dxt1.h"

namespace crnlib {
dxt_endpoint_refiner::dxt_endpoint_refiner()
    : m_pParams(NULL),
      m_pResults(NULL) {
}

bool dxt_endpoint_refiner::refine(const params& p, results& r) {
  if (!p.m_num_pixels)
    return false;

  m_pParams = &p;
  m_pResults = &r;

  r.m_error = cUINT64_MAX;
  r.m_low_color = 0;
  r.m_high_color = 0;

  double alpha2_sum = 0.0f;
  double beta2_sum = 0.0f;
  double alphabeta_sum = 0.0f;

  vec<3, double> alphax_sum(0.0f);
  vec<3, double> betax_sum(0.0f);

  vec<3, double> first_color(0.0f);

  // This linear solver is from Squish.
  for (uint i = 0; i < p.m_num_pixels; ++i) {
    uint8 c = p.m_pSelectors[i];

    double k;
    if (p.m_dxt1_selectors)
      k = g_dxt1_to_linear[c] * 1.0f / 3.0f;
    else
      k = g_dxt5_to_linear[c] * 1.0f / 7.0f;

    double alpha = 1.0f - k;
    double beta = k;

    vec<3, double> x;

    if (p.m_dxt1_selectors)
      x.set(p.m_pPixels[i][0] * 1.0f / 255.0f, p.m_pPixels[i][1] * 1.0f / 255.0f, p.m_pPixels[i][2] * 1.0f / 255.0f);
    else
      x.set(p.m_pPixels[i][p.m_alpha_comp_index] / 255.0f);

    if (!i)
      first_color = x;

    alpha2_sum += alpha * alpha;
    beta2_sum += beta * beta;
    alphabeta_sum += alpha * beta;
    alphax_sum += alpha * x;
    betax_sum += beta * x;
  }

  // zero where non-determinate
  vec<3, double> a, b;
  if (beta2_sum == 0.0f) {
    a = alphax_sum / alpha2_sum;
    b.clear();
  } else if (alpha2_sum == 0.0f) {
    a.clear();
    b = betax_sum / beta2_sum;
  } else {
    double factor = alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum;
    if (factor != 0.0f) {
      a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) / factor;
      b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) / factor;
    } else {
      a = first_color;
      b = first_color;
    }
  }

  vec3F l(0.0f), h(0.0f);
  l = a;
  h = b;

  l.clamp(0.0f, 1.0f);
  h.clamp(0.0f, 1.0f);

  if (p.m_dxt1_selectors)
    optimize_dxt1(l, h);
  else
    optimize_dxt5(l, h);

  return r.m_error < p.m_error_to_beat;
}

void dxt_endpoint_refiner::optimize_dxt5(vec3F low_color, vec3F high_color) {
  uint8 L0 = math::clamp<int>(low_color[0] * 256.0f, 0, 255);
  uint8 H0 = math::clamp<int>(high_color[0] * 256.0f, 0, 255);

  uint64 hist[8] = {}, D2[8] = {}, DD[8] = {};
  for (uint c = m_pParams->m_alpha_comp_index, i = 0; i < m_pParams->m_num_pixels; i++) {
    uint8 a = m_pParams->m_pPixels[i][c];
    uint8 s = m_pParams->m_pSelectors[i];
    hist[s]++;
    D2[s] += a * 2;
    DD[s] += a * a;
  }

  uint16 solutions[529];
  uint solutions_count = 0;
  solutions[solutions_count++] = L0 == H0 ? H0 ? H0 - 1 << 8 | L0 : 1 : L0 > H0 ? H0 << 8 | L0 : L0 << 8 | H0;
  uint8 minL = L0 <= 11 ? 0 : L0 - 11, maxL = L0 >= 244 ? 255 : L0 + 11;
  uint8 minH = H0 <= 11 ? 0 : H0 - 11, maxH = H0 >= 244 ? 255 : H0 + 11;
  for (uint16 L = minL; L <= maxL; L++) {
    for (uint16 H = minH; H <= maxH; H++) {
      if ((maxH < L || L <= H || H < minL) && (L != L0 || H != H0) && (L != H0 || H != L0))
        solutions[solutions_count++] = L == H ? H ? H - 1 << 8 | L : 1 : L > H ? H << 8 | L : L << 8 | H;
    }
  }

  for (uint i = 0; i < solutions_count; i++) {
    uint8 L = solutions[i] & 0xFF;
    uint8 H = solutions[i] >> 8;
    uint values[8];
    dxt5_block::get_block_values8(values, L, H);
    uint64 error = 0;
    for (uint64 s = 0; s < 8; s++)
      error += hist[s] * values[s] * values[s] - D2[s] * values[s] + DD[s];
    if (error < m_pResults->m_error) {
      m_pResults->m_low_color = L;
      m_pResults->m_high_color = H;
      m_pResults->m_error = error;
      if (!m_pResults->m_error)
        return;
    }
  }
}

void dxt_endpoint_refiner::optimize_dxt1(vec3F low_color, vec3F high_color) {
  uint16 L0 = math::clamp<int>(low_color[0] * 32.0f, 0, 31) << 11 | math::clamp<int>(low_color[1] * 64.0f, 0, 63) << 5 | math::clamp<int>(low_color[2] * 32.0f, 0, 31);
  uint16 H0 = math::clamp<int>(high_color[0] * 32.0f, 0, 31) << 11 | math::clamp<int>(high_color[1] * 64.0f, 0, 63) << 5 | math::clamp<int>(high_color[2] * 32.0f, 0, 31);

  uint64 hist[4] = {}, D2[4][3] = {}, DD[4][3] = {};
  for (uint i = 0; i < m_pParams->m_num_pixels; i++) {
    const color_quad_u8& pixel = m_pParams->m_pPixels[i];
    uint8 s = m_pParams->m_pSelectors[i];
    hist[s]++;
    for (uint c = 0; c < 3; c++) {
      D2[s][c] += pixel[c] * 2;
      DD[s][c] += pixel[c] * pixel[c];
    }
  }
  crnlib::vector<uint> solutions(54);
  bool preserveL = hist[0] + hist[2] > hist[1] + hist[3];
  bool improved = true;

  for (uint iterations = 8; improved && iterations; iterations--) {
    improved = false;
    uint solutions_count = 0;
    for (uint16 b0 = L0 & 31, g0 = L0 >> 5 & 63, r0 = L0 >> 11 & 31, b = b0 ? b0 - 1 : b0; b <= b0 + 1 && b <= 31; b++) {
      for (uint16 g = g0 ? g0 - 1 : g0; g <= g0 + 1 && g <= 63; g++) {
        for (uint16 r = r0 ? r0 - 1 : r0; r <= r0 + 1 && r <= 31; r++) {
          uint16 L = r << 11 | g << 5 | b;
          if (L != L0)
            solutions[solutions_count++] = L > H0 ? L | H0 << 16 : H0 | L << 16;
        }
      }
    }
    for (uint16 b0 = H0 & 31, g0 = H0 >> 5 & 63, r0 = H0 >> 11 & 31, b = b0 ? b0 - 1 : b0; b <= b0 + 1 && b <= 31; b++) {
      for (uint16 g = g0 ? g0 - 1 : g0; g <= g0 + 1 && g <= 63; g++) {
        for (uint16 r = r0 ? r0 - 1 : r0; r <= r0 + 1 && r <= 31; r++) {
          uint16 H = r << 11 | g << 5 | b;
          if (H != H0)
            solutions[solutions_count++] = H > L0 ? H | L0 << 16 : L0 | H << 16;
        }
      }
    }
    std::sort(solutions.begin(), solutions.begin() + solutions_count);
    for (uint i = 0; i < solutions_count; i++) {
      if (i && solutions[i] == solutions[i - 1])
        continue;
      uint16 L = solutions[i] & 0xFFFF;
      uint16 H = solutions[i] >> 16;
      if (L == H) {
        L += !preserveL ? ~L & 0x1F ? 0x1 : ~L & 0xF800 ? 0x800 : ~L & 0x7E0 ? 0x20 : 0 : !L ? 0x1 : 0;
        H -= preserveL ? H & 0x1F ? 0x1 : H & 0xF800 ? 0x800 : H & 0x7E0 ? 0x20 : 0 : H == 0xFFFF ? 0x1 : 0;
      }
      color_quad_u8 block_colors[4];
      dxt1_block::get_block_colors4(block_colors, L, H);
      uint64 error = 0;
      for (uint64 s = 0, d[3]; s < 4; s++) {
        for (uint c = 0; c < 3; c++)
          d[c] = hist[s] * block_colors[s][c] * block_colors[s][c] - D2[s][c] * block_colors[s][c] + DD[s][c];
        error += m_pParams->m_perceptual ? d[0] * 8 + d[1] * 25 + d[2] : d[0] + d[1] + d[2];
      }
      if (error < m_pResults->m_error) {
        m_pResults->m_low_color = L0 = L;
        m_pResults->m_high_color = H0 = H;
        m_pResults->m_error = error;
        if (!m_pResults->m_error)
          return;
        improved = true;
      }
    }
  }
}

}  // namespace crnlib
