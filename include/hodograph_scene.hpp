#pragma once

#include <chrono>
#include <vector>

#include <glad/glad.h>

#include <geometry.hpp>
#include <glfw_impl.hpp>
#include <math.hpp>
#include <mock_data.hpp>

#include <atomic>

#include <graph_structs.hpp>

namespace pusn {

namespace internal {
// each of them needs to store:
//    * geometry
//    * API object reference

struct hodograph_state {
    float l1{ 5.f };
    float l2{ 10.f };
    float omega{ 1.f };
    float angle{ 0.f };
};

struct light {
  scene_object_info placement{{200.f, 100.f, 200.f}, {}, {}};
  math::vec3 color{1.f, 1.f, 1.f};
};

struct scene_grid {
  api_agnostic_geometry geometry{{{math::vec3(-1.0, 0.0, -1.0), {}, {}},
                                  {math::vec3(1.0, 0.0, -1.0), {}, {}},
                                  {math::vec3(1.0, 0.0, 1.0), {}, {}},
                                  {math::vec3(-1.0, 0.0, 1.0), {}, {}}},
                                 {0, 1, 2, 2, 3, 0}};
  scene_object_info placement;
  glfw_impl::renderable api_renderable;
};

struct hodograph_geometry {
  api_agnostic_geometry unit_cyllinder;


  void clear() {
    unit_cyllinder.vertices.clear();
    unit_cyllinder.indices.clear();
  }
};

struct hodograph_renderable {
  glfw_impl::renderable unit_cyllinder;
};

struct model {

  hodograph_state hodograph{};
  hodograph_geometry geometry;
  hodograph_renderable renderable;

  RollingBuffer xgraph_data;
  RollingBuffer dxgraph_data;
  RollingBuffer ddxgraph_data;
  ScrollingBuffer dxxgraph_data;

  float t{ 0.f };

  inline void reset() {
    geometry.clear();

    auto rot_m = glm::toMat4(glm::quat({ 0.f, -glm::pi<float>() / 2.f, 0.f }));
    mock_data::build_vertices_helper(
        50, 1.f, 1.f, geometry.unit_cyllinder.vertices, geometry.unit_cyllinder.indices,
        rot_m, {0.8f, 0.8f, 0.8f});

    
    glfw_impl::fill_renderable(geometry.unit_cyllinder.vertices, geometry.unit_cyllinder.indices,
                               renderable.unit_cyllinder);
    glfw_impl::add_program_to_renderable("resources/model", renderable.unit_cyllinder);
  }
};

} // namespace internal

struct hodograph_scene {
  internal::model model;
  internal::scene_grid grid;
  internal::light light;

  bool init();
  void render(input_state &input);
  void set_light_uniforms(input_state &input, glfw_impl::renderable &r);
};

} // namespace pusn
