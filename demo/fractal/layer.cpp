#include "layer.hpp"

float verts[] = { 
	1.0f, 1.0f, 0.0f,
	1.0f, -1.0f, 0.0f, 
	-1.0f, 1.0f, 0.0f, 
	-1.0f, -1.0f, 0.0f, 
};
uint16_t idx[] = { 0, 1, 2, 1, 3, 2 };

fractal_layer::fractal_layer()
{
	hc::shader_library::add(hc::shader("resources/shaders/shader.vert.spv", "main"));
	hc::shader_library::add(hc::shader("resources/shaders/shader.frag.spv", "main"));

	hc::data_layout vert_layout(1);
	vert_layout.set_type(0, hc::data_layout::type::VEC3, hc::data_layout::component_type::FLOAT32);
	obj = hc::mesh(verts, sizeof(verts), idx, sizeof(idx), hc::mesh::index_format::UINT16, vert_layout);
	vertex_input = hc::uniform(hc::data_layout::create<float, float, float, float>(), 1);
	frag_input = hc::uniform(hc::data_layout::create<std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, float>(), 1);
	pipeline = hc::render_pipeline(hc::shader_library::get("resources/shaders/shader.vert.spv"),
		hc::shader_library::get("resources/shaders/shader.frag.spv"));

	pipeline.add(obj).set_descriptor(0, vertex_input).set_descriptor(1, frag_input);
	hc::window::get_dimensions(&w, &h);
	v_inputs.ratio = static_cast<float>(w) / h;
}

fractal_layer::~fractal_layer()
{}

void fractal_layer::tick()
{
	if (animate) time += hc::delta_time();
	f_inputs.frame = std::sin(time);
	vertex_input[0].set(&v_inputs);
	frag_input[0].set(&f_inputs);
}

bool fractal_layer::handleEvent(const hc::Event & e)
{
	switch (e.type)
	{
	case hc::EventType::KeyPressed:
		LOGF_INFO("[EVENT] Key pressed (Key: {0} ; Scancode: {1})", e.x.i, e.y.i);
		switch (e.y.i)
		{
		case 19: //r
			time = 0;
			v_inputs.center[0] = 0.0f;
			v_inputs.center[1] = 0.0f;
			v_inputs.scale = 1.0f;
			f_inputs = { n_funcs - 1, 1, 0, 0, 0 };
			break;
		case 57: //space
			animate = !animate;
			break;
		case 333: //right arrow key
			f_inputs.func = (f_inputs.func + 1) % n_funcs;
			break;
		case 331: //left arrow key
			f_inputs.func = (f_inputs.func + (n_funcs - 1)) % n_funcs;
			break;
		case 328: //up arrow key
			f_inputs.n = f_inputs.n % max_iterations + 1;
			break;
		case 336: //down arrow key
			f_inputs.n = (f_inputs.n + (max_iterations - 2)) % max_iterations + 1;
			break;
		}
		return false;
	case hc::EventType::KeyReleased:
		LOGF_INFO("[EVENT] Key released (Key: {0} ; Scancode: {1})", e.x.i, e.y.i);
		return false;
	case hc::EventType::MouseButtonPressed:
		if (e.x.i == 0)
			dragging = true;
		return false;
	case hc::EventType::MouseButtonReleased:
		if (e.x.i == 0)
			dragging = false;
		return false;
	case hc::EventType::MouseMoved:
		if (dragging)
		{
			float r_x = 1.0, r_y = 1.0;

			if (v_inputs.ratio < 1.0)
				r_y /= v_inputs.ratio;
			else
				r_x *= v_inputs.ratio;

			v_inputs.center[0] += ((e.x.f - last_mouse_pos[0]) / w) * v_inputs.scale * 2 * r_x;
			v_inputs.center[1] += ((e.y.f - last_mouse_pos[1]) / h) * v_inputs.scale * 2 * r_y;
		}
		last_mouse_pos[0] = e.x.f;
		last_mouse_pos[1] = e.y.f;
		return false;
	case hc::EventType::MouseScrolled:
		v_inputs.scale *= std::pow(1.1f, -e.y.f);
		return false;
	case hc::EventType::WindowResize:
		hc::window::get_dimensions(&w, &h);
		v_inputs.ratio = static_cast<float>(w) / h;
		break;
	}
	return false;
}
