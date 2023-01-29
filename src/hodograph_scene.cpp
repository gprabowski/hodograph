#include <hodograph_scene.hpp>

#include <vector>

#include <math.hpp>

#include <mock_data.hpp>

namespace pusn {

	void generate_milling_tool(api_agnostic_geometry& out) {}

	bool hodograph_scene::init() {
		// Generate and add milling tool
		model.reset();

		// ADD GRID
		glfw_impl::fill_renderable(grid.geometry.vertices, grid.geometry.indices,
			grid.api_renderable);
		glfw_impl::add_program_to_renderable("resources/grid", grid.api_renderable);

		return true;
	}

	void hodograph_scene::set_light_uniforms(input_state& input,
		glfw_impl::renderable& r) {
		// set light and camera uniforms
		glfw_impl::set_uniform("light_pos", r.program.value(),
			light.placement.position);
		glfw_impl::set_uniform("light_color", r.program.value(), light.color);
		glfw_impl::set_uniform("cam_pos", r.program.value(), input.camera.pos);
	}

	void hodograph_scene::render(input_state& input) {

		// 1. get camera info
		glDepthFunc(GL_LESS);

		const auto view = math::get_view_matrix(
			input.camera.pos, input.camera.pos + input.camera.front, input.camera.up);

		const auto proj = math::get_projection_matrix(
			glm::radians(input.render_info.fov_y),

			glfw_impl::last_frame_info::right_viewport_area.x,

			glfw_impl::last_frame_info::right_viewport_area.y,
			input.render_info.clip_near, input.render_info.clip_far);

		// 2. render grid
		glDisable(GL_CULL_FACE);
		const auto model_grid_m =
			math::get_model_matrix(grid.placement.position, grid.placement.scale,
				math::deg_to_rad(grid.placement.rotation));
		glfw_impl::use_program(grid.api_renderable.program.value());

		set_light_uniforms(input, grid.api_renderable);

		glfw_impl::set_uniform("model", grid.api_renderable.program.value(),
			model_grid_m);
		glfw_impl::set_uniform("view", grid.api_renderable.program.value(), view);
		glfw_impl::set_uniform("proj", grid.api_renderable.program.value(), proj);
		glfw_impl::render(grid.api_renderable, grid.geometry);
		glEnable(GL_CULL_FACE);
		const auto sq = [](float a) {return a * a; };

		// 3. render the model
		static auto last_time = std::chrono::system_clock::now();
		const std::chrono::duration<float> time_delta = std::chrono::system_clock::now() - last_time;
		last_time = std::chrono::system_clock::now();
		// UPDATE HERE
		model.hodograph.angle += time_delta.count() * model.hodograph.omega;

		// FIND L2 CONFIG
		const auto delta = 4 * sq(model.hodograph.l1) * std::cos(model.hodograph.angle) * std::cos(model.hodograph.angle) - 4 * (model.hodograph.l1 * model.hodograph.l1 - model.hodograph.l2 * model.hodograph.l2);
		if (delta < 0) {
			throw;
		}

		const auto sq_delta = std::sqrt(delta);
		const auto first_sol = (2 * model.hodograph.l1 * std::cos(model.hodograph.angle) + sq_delta) / 2;
		const auto second_sol = (2 * model.hodograph.l1 * std::cos(model.hodograph.angle) - sq_delta) / 2;
		const auto l3 = first_sol > 0 ? first_sol : second_sol;

		if (l3 < 0) {
			throw;
		}

		auto beta = std::acos((sq(model.hodograph.l2) + sq(l3) - sq(model.hodograph.l1)) / (2 * model.hodograph.l2 * l3));

		if (std::fmod(model.hodograph.angle, 2 * glm::pi<float>()) < glm::pi<float>()) {
			beta = 2 * glm::pi<float>() - beta;
		}

		auto render_element = [&](auto& renderable, auto& geometry, auto& mmat) {
			glfw_impl::use_program(renderable.program.value());
			set_light_uniforms(input, renderable);
			glfw_impl::set_uniform("model", renderable.program.value(), mmat);
			glfw_impl::set_uniform("view", renderable.program.value(), view);
			glfw_impl::set_uniform("proj", renderable.program.value(), proj);
			glfw_impl::render(renderable, geometry);
			glfw_impl::use_program(0);
		};

		auto mmat =
			math::get_model_matrix({ 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f },
				glm::vec3{ 0.f, 0.f, model.hodograph.angle }) * 
			math::get_model_matrix({ 0.f, 0.f, 0.f }, { model.hodograph.l1, 1.f, 1.f },
				glm::vec3{ 0.f, 0.f, 0.f });
		render_element(model.renderable.unit_cyllinder, model.geometry.unit_cyllinder, mmat);
		
		mmat =
			math::get_model_matrix({ std::cos(model.hodograph.angle) * model.hodograph.l1, std::sin(model.hodograph.angle) * model.hodograph.l1, 0.f }, { 1.f, 1.f, 1.f },
				glm::vec3{ 0.f, 0.f, beta }) * math::get_model_matrix({ 0.f, 0.f, 0.f }, { model.hodograph.l2, 1.f, 1.f },
					glm::vec3{ 0.f, 0.f, 0.f });

		render_element(model.renderable.unit_cyllinder, model.geometry.unit_cyllinder, mmat);

		// update graph data
		model.t += time_delta.count();
		model.xgraph_data.AddPoint(model.t, l3);
		static float last_x = 0.f;
		static float last_dx = 0.f;

		const auto dx = (l3 - last_x) / time_delta.count();
		last_x = l3;
		const auto ddx = (dx - last_dx) / time_delta.count();
		last_dx = dx;
		model.dxgraph_data.AddPoint(model.t, dx);
		model.ddxgraph_data.AddPoint(model.t, ddx);

		model.dxxgraph_data.AddPoint(l3, dx);
	}
} // namespace pusn
