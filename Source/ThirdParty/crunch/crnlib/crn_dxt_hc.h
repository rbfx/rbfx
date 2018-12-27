// File: crn_dxt_hc.h
// See Copyright Notice and license at the end of inc/crnlib.h
#pragma once
#include "crn_dxt1.h"
#include "crn_dxt5a.h"
#include "crn_dxt_endpoint_refiner.h"
#include "crn_image.h"
#include "crn_dxt.h"
#include "crn_image.h"
#include "crn_dxt_hc_common.h"
#include "crn_tree_clusterizer.h"
#include "crn_threading.h"

#define CRN_NO_FUNCTION_DEFINITIONS
#include "crunch/crnlib.h"

namespace crnlib {
const uint cTotalCompressionPhases = 25;

class dxt_hc {
 public:
  dxt_hc();
  ~dxt_hc();

  struct endpoint_indices_details {
    union {
      struct {
        uint16 color;
        uint16 alpha0;
        uint16 alpha1;
      };
      uint16 component[3];
    };
    uint8 reference;
    endpoint_indices_details() { utils::zero_object(*this); }
  };

  struct selector_indices_details {
    union {
      struct {
        uint16 color;
        uint16 alpha0;
        uint16 alpha1;
      };
      uint16 component[3];
    };
    selector_indices_details() { utils::zero_object(*this); }
  };

  struct tile_details {
    crnlib::vector<color_quad_u8> pixels;
    float weight;
    vec<6, float> color_endpoint;
    vec<2, float> alpha_endpoints[2];
    uint16 cluster_indices[3];
  };
  crnlib::vector<tile_details> m_tiles;
  uint m_num_tiles;
  float m_color_derating[cCRNMaxLevels][8];
  float m_alpha_derating[8];
  float m_uint8_to_float[256];

  color_quad_u8 (*m_blocks)[16];
  uint m_num_blocks;
  crnlib::vector<float> m_block_weights;
  crnlib::vector<uint8> m_block_encodings;
  crnlib::vector<uint64> m_block_selectors[3];
  crnlib::vector<uint32> m_color_selectors;
  crnlib::vector<uint64> m_alpha_selectors;
  crnlib::vector<bool> m_color_selectors_used;
  crnlib::vector<bool> m_alpha_selectors_used;
  crnlib::vector<uint> m_tile_indices;
  crnlib::vector<endpoint_indices_details> m_endpoint_indices;
  crnlib::vector<selector_indices_details> m_selector_indices;

  struct params {
    params()
        : m_num_blocks(0),
          m_num_levels(0),
          m_num_faces(0),
          m_format(cDXT1),
          m_perceptual(true),
          m_hierarchical(true),
          m_color_endpoint_codebook_size(3072),
          m_color_selector_codebook_size(3072),
          m_alpha_endpoint_codebook_size(3072),
          m_alpha_selector_codebook_size(3072),
          m_adaptive_tile_color_psnr_derating(2.0f),
          m_adaptive_tile_alpha_psnr_derating(2.0f),
          m_adaptive_tile_color_alpha_weighting_ratio(3.0f),
          m_debugging(false),
          m_pProgress_func(0),
          m_pProgress_func_data(0) {
      m_alpha_component_indices[0] = 3;
      m_alpha_component_indices[1] = 0;
      for (uint i = 0; i < cCRNMaxLevels; i++) {
        m_levels[i].m_first_block = 0;
        m_levels[i].m_num_blocks = 0;
        m_levels[i].m_block_width = 0;
      }
    }

    uint m_num_blocks;
    uint m_num_levels;
    uint m_num_faces;

    struct {
      uint m_first_block;
      uint m_num_blocks;
      uint m_block_width;
      float m_weight;
    } m_levels[cCRNMaxLevels];

    dxt_format m_format;
    bool m_perceptual;
    bool m_hierarchical;

    uint m_color_endpoint_codebook_size;
    uint m_color_selector_codebook_size;
    uint m_alpha_endpoint_codebook_size;
    uint m_alpha_selector_codebook_size;

    float m_adaptive_tile_color_psnr_derating;
    float m_adaptive_tile_alpha_psnr_derating;
    float m_adaptive_tile_color_alpha_weighting_ratio;
    uint m_alpha_component_indices[2];

    task_pool* m_pTask_pool;
    bool m_debugging;
    crn_progress_callback_func m_pProgress_func;
    void* m_pProgress_func_data;
  };

  void clear();
  bool compress(
    color_quad_u8 (*blocks)[16],
    crnlib::vector<endpoint_indices_details>& endpoint_indices,
    crnlib::vector<selector_indices_details>& selector_indices,
    crnlib::vector<uint32>& color_endpoints,
    crnlib::vector<uint32>& alpha_endpoints,
    crnlib::vector<uint32>& color_selectors,
    crnlib::vector<uint64>& alpha_selectors,
    const params& p
  );

 private:
  params m_params;

  uint m_num_alpha_blocks;
  bool m_has_color_blocks;
  bool m_has_etc_color_blocks;

  enum {
    cColor = 0,
    cAlpha0 = 1,
    cAlpha1 = 2,
    cNumComps = 3
  };

  struct color_cluster {
    color_cluster() : first_endpoint(0), second_endpoint(0) {}
    crnlib::vector<uint> blocks[3];
    crnlib::vector<color_quad_u8> pixels;
    uint first_endpoint;
    uint second_endpoint;
    color_quad_u8 color_values[4];
  };
  crnlib::vector<color_cluster> m_color_clusters;

  struct alpha_cluster {
    alpha_cluster() : first_endpoint(0), second_endpoint(0) {}
    crnlib::vector<uint> blocks[3];
    crnlib::vector<color_quad_u8> pixels;
    uint first_endpoint;
    uint second_endpoint;
    uint alpha_values[8];
    bool refined_alpha;
    uint refined_alpha_values[8];
  };
  crnlib::vector<alpha_cluster> m_alpha_clusters;

  crn_thread_id_t m_main_thread_id;
  bool m_canceled;
  task_pool* m_pTask_pool;

  int m_prev_phase_index;
  int m_prev_percentage_complete;

  vec<6, float> palettize_color(color_quad_u8* pixels, uint pixels_count);
  vec<2, float> palettize_alpha(color_quad_u8* pixels, uint pixels_count, uint comp_index);
  void determine_tiles_task(uint64 data, void* pData_ptr);
  void determine_tiles_task_etc(uint64 data, void* pData_ptr);

  void determine_color_endpoint_codebook_task(uint64 data, void* pData_ptr);
  void determine_color_endpoint_codebook_task_etc(uint64 data, void* pData_ptr);
  void determine_color_endpoint_clusters_task(uint64 data, void* pData_ptr);
  void determine_color_endpoints();

  void determine_alpha_endpoint_codebook_task(uint64 data, void* pData_ptr);
  void determine_alpha_endpoint_clusters_task(uint64 data, void* pData_ptr);
  void determine_alpha_endpoints();

  void create_color_selector_codebook_task(uint64 data, void* pData_ptr);
  void create_color_selector_codebook();

  void create_alpha_selector_codebook_task(uint64 data, void* pData_ptr);
  void create_alpha_selector_codebook();

  bool update_progress(uint phase_index, uint subphase_index, uint subphase_total);
};

}  // namespace crnlib
