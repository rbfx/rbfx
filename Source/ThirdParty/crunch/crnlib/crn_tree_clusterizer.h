// File: crn_tree_clusterizer.h
// See Copyright Notice and license at the end of inc/crnlib.h
#pragma once
#include "crn_matrix.h"
#include "crn_threading.h"
#include <queue>

namespace crnlib {
template <typename VectorType>
class tree_clusterizer {
 public:
  tree_clusterizer() {}

  struct VectorInfo {
    uint index;
    uint weight;
  };

  struct NodeInfo {
    uint m_index;
    float m_variance;
    NodeInfo (uint index, float variance) : m_index(index), m_variance(variance) {}
    bool operator<(const NodeInfo& other) const {
      return m_index < other.m_index ? m_variance < other.m_variance : !(other.m_variance < m_variance);
    }
  };

  struct split_alternative_node_task_params {
    uint main_node;
    uint alternative_node;
    uint max_splits;
  };

  void split_alternative_node_task(uint64, void* pData_ptr) {
    split_alternative_node_task_params* pParams = (split_alternative_node_task_params*)pData_ptr;
    std::priority_queue<NodeInfo> node_queue;
    uint begin_node = pParams->alternative_node, end_node = begin_node, splits = 0;

    m_nodes[end_node] = m_nodes[pParams->main_node];
    node_queue.push(NodeInfo(end_node, m_nodes[end_node].m_variance));
    end_node++;
    splits++;

    while (splits < pParams->max_splits && split_node(node_queue, end_node))
      splits++;

    m_nodes[pParams->main_node] = m_nodes[pParams->alternative_node];
    m_nodes[pParams->main_node].m_alternative = true;
  }


  void generate_codebook(VectorType* vectors, uint* weights, uint size, uint max_splits, bool generate_node_index_map = false, task_pool* pTask_pool = 0) {
    m_vectors = vectors;
    m_vectorsInfo.resize(size);
    m_weightedVectors.resize(size);
    m_weightedDotProducts.resize(size);
    m_vectorsInfoLeft.resize(size);
    m_vectorsInfoRight.resize(size);
    m_vectorComparison.resize(size);
    m_nodes.resize(max_splits << 2);
    m_codebook.clear();
    uint num_tasks = pTask_pool ? pTask_pool->get_num_threads() + 1 : 1;

    vq_node root;
    root.m_begin = 0;
    root.m_end = size;
    double ttsum = 0.0f;
    for (uint i = 0; i < m_vectorsInfo.size(); i++) {
      const VectorType& v = vectors[i];
      m_vectorsInfo[i].index = i;
      const uint weight = m_vectorsInfo[i].weight = weights[i];
      m_weightedVectors[i] = v * (float)weight;
      root.m_centroid += m_weightedVectors[i];
      root.m_total_weight += weight;
      m_weightedDotProducts[i] = v.dot(v) * weight;
      ttsum += m_weightedDotProducts[i];
    }
    root.m_variance = (float)(ttsum - (root.m_centroid.dot(root.m_centroid) / root.m_total_weight));
    root.m_centroid *= (1.0f / root.m_total_weight);

    std::priority_queue<NodeInfo> node_queue;
    uint begin_node = 0, end_node = begin_node, splits = 0;
    m_nodes[end_node] = root;
    node_queue.push(NodeInfo(end_node, root.m_variance));
    end_node++;
    splits++;

    if (num_tasks > 1) {
      while (splits < max_splits && node_queue.size() != num_tasks && split_node(node_queue, end_node, pTask_pool))
        splits++;
      if (node_queue.size() == num_tasks) {
        std::priority_queue<NodeInfo> alternative_node_queue = node_queue;
        uint alternative_node = max_splits << 1, alternative_max_splits = max_splits / num_tasks;
        crnlib::vector<split_alternative_node_task_params> params(num_tasks);
        for (uint task = 0; !alternative_node_queue.empty(); alternative_node_queue.pop(), alternative_node += alternative_max_splits << 1, task++) {
          params[task].main_node = alternative_node_queue.top().m_index;
          params[task].alternative_node = alternative_node;
          params[task].max_splits = alternative_max_splits;
          pTask_pool->queue_object_task(this, &tree_clusterizer::split_alternative_node_task, task, &params[task]);
        }
        pTask_pool->join();
      }
    }

    while (splits < max_splits && split_node(node_queue, end_node, pTask_pool))
      splits++;

    for (uint i = begin_node; i < end_node; i++) {
      vq_node& node = m_nodes[i];
      if (!node.m_alternative && node.m_left != -1)
        continue;
      node.m_codebook_index = m_codebook.size();
      m_codebook.push_back(node.m_centroid);
      if (generate_node_index_map) {
        for (uint j = node.m_begin; j < node.m_end; j++)
          m_node_index_map.insert(std::make_pair(m_vectors[m_vectorsInfo[j].index], node.m_codebook_index));
      }
    }
  }

  inline uint get_node_index(const VectorType& v) {
    return m_node_index_map.find(v)->second;
  }

  inline uint get_codebook_size() const {
    return m_codebook.size();
  }

  inline const VectorType& get_codebook_entry(uint index) const {
    return m_codebook[index];
  }

  typedef crnlib::vector<VectorType> vector_vec_type;
  inline const vector_vec_type& get_codebook() const {
    return m_codebook;
  }

 private:
  VectorType* m_vectors;
  crnlib::vector<VectorType> m_weightedVectors;
  crnlib::vector<double> m_weightedDotProducts;
  crnlib::vector<VectorInfo> m_vectorsInfo, m_vectorsInfoLeft, m_vectorsInfoRight;
  crnlib::vector<bool> m_vectorComparison;
  crnlib::hash_map<VectorType, uint> m_node_index_map;

  struct vq_node {
    vq_node()
        : m_centroid(cClear), m_total_weight(0), m_left(-1), m_right(-1), m_codebook_index(-1), m_unsplittable(false), m_alternative(false), m_processed(false) {}

    VectorType m_centroid;
    uint64 m_total_weight;

    float m_variance;

    uint m_begin;
    uint m_end;

    int m_left;
    int m_right;

    int m_codebook_index;

    bool m_unsplittable;
    bool m_alternative;
    bool m_processed;
  };

  typedef crnlib::vector<vq_node> node_vec_type;

  node_vec_type m_nodes;

  vector_vec_type m_codebook;

  struct distance_comparison_task_params {
    VectorType* left_child;
    VectorType* right_child;
    uint begin;
    uint end;
    uint num_tasks;
  };

  void distance_comparison_task(uint64 data, void* pData_ptr) {
    distance_comparison_task_params* pParams = (distance_comparison_task_params*)pData_ptr;
    const VectorType& left_child = *pParams->left_child;
    const VectorType& right_child = *pParams->right_child;
    uint begin = pParams->begin + (pParams->end - pParams->begin) * data / pParams->num_tasks;
    uint end = pParams->begin + (pParams->end - pParams->begin) * (data + 1) / pParams->num_tasks;
    for (uint i = begin; i < end; i++) {
      const VectorType& v = m_vectors[m_vectorsInfo[i].index];
      double left_dist2 = left_child.squared_distance(v);
      double right_dist2 = right_child.squared_distance(v);
      m_vectorComparison[i] = left_dist2 < right_dist2;
    }
  }

  bool split_node(std::priority_queue<NodeInfo>& node_queue, uint& end_node, task_pool* pTask_pool = 0) {
    if (node_queue.empty())
      return false;

    vq_node& parent_node = m_nodes[node_queue.top().m_index];

    if (parent_node.m_alternative)
      parent_node.m_alternative = false;

    if (parent_node.m_variance <= 0.0f || parent_node.m_begin + 1 == parent_node.m_end)
      return false;

    node_queue.pop();

    if (parent_node.m_processed) {
      if (!parent_node.m_unsplittable) {
        m_nodes[end_node] = m_nodes[parent_node.m_left];
        m_nodes[end_node].m_alternative = true;
        node_queue.push(NodeInfo(end_node, m_nodes[end_node].m_variance));
        parent_node.m_left = end_node++;
        m_nodes[end_node] = m_nodes[parent_node.m_right];
        m_nodes[end_node].m_alternative = true;
        node_queue.push(NodeInfo(end_node, m_nodes[end_node].m_variance));
        parent_node.m_right = end_node++;
      }
      return true;
    }
    parent_node.m_processed = true;

    uint num_blocks = (parent_node.m_end - parent_node.m_begin) >> 9;
    uint num_tasks = num_blocks > 1 && pTask_pool ? math::minimum(num_blocks, pTask_pool->get_num_threads() + 1) : 1;

    VectorType furthest(0);
    double furthest_dist = -1.0f;

    for (uint i = parent_node.m_begin; i < parent_node.m_end; i++) {
      const VectorType& v = m_vectors[m_vectorsInfo[i].index];
      double dist = v.squared_distance(parent_node.m_centroid);
      if (dist > furthest_dist) {
        furthest_dist = dist;
        furthest = v;
      }
    }

    VectorType opposite;
    double opposite_dist = -1.0f;

    for (uint i = parent_node.m_begin; i < parent_node.m_end; i++) {
      const VectorType& v = m_vectors[m_vectorsInfo[i].index];
      double dist = v.squared_distance(furthest);
      if (dist > opposite_dist) {
        opposite_dist = dist;
        opposite = v;
      }
    }

    VectorType left_child((furthest + parent_node.m_centroid) * .5f);
    VectorType right_child((opposite + parent_node.m_centroid) * .5f);

    if (parent_node.m_begin + 2 < parent_node.m_end) {
      const uint N = VectorType::num_elements;

      matrix<N, N, float> covar;
      covar.clear();

      for (uint i = parent_node.m_begin; i < parent_node.m_end; i++) {
        const VectorType& v = m_vectors[m_vectorsInfo[i].index] - parent_node.m_centroid;
        const VectorType w = v * (float)m_vectorsInfo[i].weight;
        for (uint x = 0; x < N; x++) {
          for (uint y = x; y < N; y++)
            covar[x][y] = covar[x][y] + v[x] * w[y];
        }
      }

      float divider = (float)parent_node.m_total_weight;
      for (uint x = 0; x < N; x++) {
        for (uint y = x; y < N; y++) {
          covar[x][y] /= divider;
          covar[y][x] = covar[x][y];
        }
      }

      VectorType axis(1.0f);
      // Starting with an estimate of the principle axis should work better, but doesn't in practice?
      //left_child - right_child);
      //axis.normalize();

      for (uint iter = 0; iter < 10; iter++) {
        VectorType x;

        double max_sum = 0;

        for (uint i = 0; i < N; i++) {
          double sum = 0;

          for (uint j = 0; j < N; j++)
            sum += axis[j] * covar[i][j];

          x[i] = (float)sum;

          max_sum = i ? math::maximum(max_sum, sum) : sum;
        }

        if (max_sum != 0.0f)
          x *= (float)(1.0f / max_sum);

        axis = x;
      }

      axis.normalize();

      VectorType new_left_child(0.0f);
      VectorType new_right_child(0.0f);

      double left_weight = 0.0f;
      double right_weight = 0.0f;

      for (uint i = parent_node.m_begin; i < parent_node.m_end; i++) {
        const VectorInfo& vectorInfo = m_vectorsInfo[i];
        const float weight = (float)vectorInfo.weight;
        double t = (m_vectors[vectorInfo.index] - parent_node.m_centroid) * axis;
        if (t < 0.0f) {
          new_left_child += m_weightedVectors[vectorInfo.index];
          left_weight += weight;
        } else {
          new_right_child += m_weightedVectors[vectorInfo.index];
          right_weight += weight;
        }
      }

      if ((left_weight > 0.0f) && (right_weight > 0.0f)) {
        left_child = new_left_child * (float)(1.0f / left_weight);
        right_child = new_right_child * (float)(1.0f / right_weight);
      }
    }

    uint64 left_weight = 0;
    uint64 right_weight = 0;

    uint left_info_index = 0;
    uint right_info_index = 0;

    float prev_total_variance = 1e+10f;

    float left_variance = 0.0f;
    float right_variance = 0.0f;

    // FIXME: Excessive upper limit
    const uint cMaxLoops = 1024;
    for (uint total_loops = 0; total_loops < cMaxLoops; total_loops++) {
      left_info_index = right_info_index = parent_node.m_begin;

      VectorType new_left_child(cClear);
      VectorType new_right_child(cClear);

      double left_ttsum = 0.0f;
      double right_ttsum = 0.0f;

      left_weight = 0;
      right_weight = 0;

      if (num_tasks > 1) {
        distance_comparison_task_params params;
        params.left_child = &left_child;
        params.right_child = &right_child;
        params.begin = parent_node.m_begin;
        params.end = parent_node.m_end;
        params.num_tasks = num_tasks;

        for (uint task = 0; task < params.num_tasks; task++)
          pTask_pool->queue_object_task(this, &tree_clusterizer::distance_comparison_task, task, &params);
        pTask_pool->join();

        for (uint i = parent_node.m_begin; i < parent_node.m_end; i++) {
          const VectorInfo& vectorInfo = m_vectorsInfo[i];
          if (m_vectorComparison[i]) {
            new_left_child += m_weightedVectors[vectorInfo.index];
            left_ttsum += m_weightedDotProducts[vectorInfo.index];
            left_weight += vectorInfo.weight;
            m_vectorsInfoLeft[left_info_index++] = vectorInfo;
          } else {
            new_right_child += m_weightedVectors[vectorInfo.index];
            right_ttsum += m_weightedDotProducts[vectorInfo.index];
            right_weight += vectorInfo.weight;
            m_vectorsInfoRight[right_info_index++] = vectorInfo;
          }
        }
      } else {
        for (uint i = parent_node.m_begin; i < parent_node.m_end; i++) {
          const VectorInfo& vectorInfo = m_vectorsInfo[i];
          double left_dist2 = left_child.squared_distance(m_vectors[vectorInfo.index]);
          double right_dist2 = right_child.squared_distance(m_vectors[vectorInfo.index]);
          if (left_dist2 < right_dist2) {
            new_left_child += m_weightedVectors[vectorInfo.index];
            left_ttsum += m_weightedDotProducts[vectorInfo.index];
            left_weight += vectorInfo.weight;
            m_vectorsInfoLeft[left_info_index++] = vectorInfo;
          } else {
            new_right_child += m_weightedVectors[vectorInfo.index];
            right_ttsum += m_weightedDotProducts[vectorInfo.index];
            right_weight += vectorInfo.weight;
            m_vectorsInfoRight[right_info_index++] = vectorInfo;
          }
        }
      }

      if ((!left_weight) || (!right_weight)) {
        parent_node.m_unsplittable = true;
        return true;
      }

      left_variance = (float)(left_ttsum - (new_left_child.dot(new_left_child) / left_weight));
      right_variance = (float)(right_ttsum - (new_right_child.dot(new_right_child) / right_weight));

      new_left_child *= (1.0f / left_weight);
      new_right_child *= (1.0f / right_weight);

      left_child = new_left_child;
      right_child = new_right_child;

      float total_variance = left_variance + right_variance;
      if (total_variance < .00001f)
        break;

      if (((prev_total_variance - total_variance) / total_variance) < .00001f)
        break;

      prev_total_variance = total_variance;
    }

    parent_node.m_left = end_node++;
    parent_node.m_right = end_node++;

    node_queue.push(NodeInfo(parent_node.m_left, left_variance));
    node_queue.push(NodeInfo(parent_node.m_right, right_variance));

    vq_node& left_child_node = m_nodes[parent_node.m_left];
    vq_node& right_child_node = m_nodes[parent_node.m_right];

    left_child_node.m_begin = parent_node.m_begin;
    left_child_node.m_end = right_child_node.m_begin = left_info_index;
    right_child_node.m_end = parent_node.m_end;

    memcpy(&m_vectorsInfo[left_child_node.m_begin], &m_vectorsInfoLeft[parent_node.m_begin], (left_child_node.m_end - left_child_node.m_begin) * sizeof(VectorInfo));
    memcpy(&m_vectorsInfo[right_child_node.m_begin], &m_vectorsInfoRight[parent_node.m_begin], (right_child_node.m_end - right_child_node.m_begin) * sizeof(VectorInfo));

    left_child_node.m_centroid = left_child;
    left_child_node.m_total_weight = left_weight;
    left_child_node.m_variance = left_variance;

    right_child_node.m_centroid = right_child;
    right_child_node.m_total_weight = right_weight;
    right_child_node.m_variance = right_variance;

    return true;
  }
};

template<typename VectorType>
void split_vectors(VectorType (&vectors)[64], uint (&weights)[64], uint size, VectorType (&result)[2]) {
  VectorType weightedVectors[64];
  double weightedDotProducts[64];
  VectorType centroid(cClear);
  uint64 total_weight = 0;
  double ttsum = 0.0f;
  for (uint i = 0; i < size; i++) {
    const VectorType& v = vectors[i];
    const uint weight = weights[i];
    weightedVectors[i] = v * (float)weight;
    centroid += weightedVectors[i];
    total_weight += weight;
    weightedDotProducts[i] = v.dot(v) * weight;
    ttsum += weightedDotProducts[i];
  }
  float variance = (float)(ttsum - (centroid.dot(centroid) / total_weight));
  centroid *= (1.0f / total_weight);
  result[0] = result[1] = centroid;
  if (variance <= 0.0f || size == 1)
    return;
  VectorType furthest;
  double furthest_dist = -1.0f;
  for (uint i = 0; i < size; i++) {
    const VectorType& v = vectors[i];
    double dist = v.squared_distance(centroid);
    if (dist > furthest_dist) {
      furthest_dist = dist;
      furthest = v;
    }
  }
  VectorType opposite;
  double opposite_dist = -1.0f;
  for (uint i = 0; i < size; i++) {
    const VectorType& v = vectors[i];
    double dist = v.squared_distance(furthest);
    if (dist > opposite_dist) {
      opposite_dist = dist;
      opposite = v;
    }
  }
  VectorType left_child((furthest + centroid) * .5f);
  VectorType right_child((opposite + centroid) * .5f);
  if (size > 2) {
    const uint N = VectorType::num_elements;
    matrix<N, N, float> covar;
    covar.clear();
    for (uint i = 0; i < size; i++) {
      const VectorType& v = vectors[i] - centroid;
      const VectorType w = v * (float)weights[i];
      for (uint x = 0; x < N; x++) {
        for (uint y = x; y < N; y++)
          covar[x][y] = covar[x][y] + v[x] * w[y];
      }
    }
    float divider = (float)total_weight;
    for (uint x = 0; x < N; x++) {
      for (uint y = x; y < N; y++) {
        covar[x][y] /= divider;
        covar[y][x] = covar[x][y];
      }
    }
    VectorType axis(1.0f);
    for (uint iter = 0; iter < 10; iter++) {
      VectorType x;
      double max_sum = 0;
      for (uint i = 0; i < N; i++) {
        double sum = 0;
        for (uint j = 0; j < N; j++)
          sum += axis[j] * covar[i][j];
        x[i] = (float)sum;
        max_sum = i ? math::maximum(max_sum, sum) : sum;
      }
      if (max_sum != 0.0f)
        x *= (float)(1.0f / max_sum);
      axis = x;
    }
    axis.normalize();
    VectorType new_left_child(0.0f);
    VectorType new_right_child(0.0f);
    double left_weight = 0.0f;
    double right_weight = 0.0f;
    for (uint i = 0; i < size; i++) {
      const VectorType& v = vectors[i];
      const float weight = (float)weights[i];
      double t = (v - centroid) * axis;
      if (t < 0.0f) {
        new_left_child += weightedVectors[i];
        left_weight += weight;
      } else {
        new_right_child += weightedVectors[i];
        right_weight += weight;
      }
    }
    if ((left_weight > 0.0f) && (right_weight > 0.0f)) {
      left_child = new_left_child * (float)(1.0f / left_weight);
      right_child = new_right_child * (float)(1.0f / right_weight);
    }
  }
  uint64 left_weight = 0;
  uint64 right_weight = 0;
  float prev_total_variance = 1e+10f;
  float left_variance = 0.0f;
  float right_variance = 0.0f;
  const uint cMaxLoops = 1024;
  for (uint total_loops = 0; total_loops < cMaxLoops; total_loops++) {
    VectorType new_left_child(cClear);
    VectorType new_right_child(cClear);
    double left_ttsum = 0.0f;
    double right_ttsum = 0.0f;
    left_weight = 0;
    right_weight = 0;
    for (uint i = 0; i < size; i++) {
      const VectorType& v = vectors[i];
      double left_dist2 = left_child.squared_distance(v);
      double right_dist2 = right_child.squared_distance(v);
      if (left_dist2 < right_dist2) {
        new_left_child += weightedVectors[i];
        left_ttsum += weightedDotProducts[i];
        left_weight += weights[i];
      } else {
        new_right_child += weightedVectors[i];
        right_ttsum += weightedDotProducts[i];
        right_weight += weights[i];
      }
    }
    if ((!left_weight) || (!right_weight))
      return;
    left_variance = (float)(left_ttsum - (new_left_child.dot(new_left_child) / left_weight));
    right_variance = (float)(right_ttsum - (new_right_child.dot(new_right_child) / right_weight));
    new_left_child *= (1.0f / left_weight);
    new_right_child *= (1.0f / right_weight);
    left_child = new_left_child;
    right_child = new_right_child;
    float total_variance = left_variance + right_variance;
    if (total_variance < .00001f)
      break;
    if (((prev_total_variance - total_variance) / total_variance) < .00001f)
      break;
    prev_total_variance = total_variance;
  }
  result[0] = left_child;
  result[1] = right_child;
}

}  // namespace crnlib
