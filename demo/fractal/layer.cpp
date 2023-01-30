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
	input_buffer = hc::uniform(hc::data_layout::create<std::uint32_t, std::uint32_t, float, float, float, float>(), 1);
	pipeline = hc::render_pipeline(hc::shader_library::get("resources/shaders/shader.vert.spv"),
		hc::shader_library::get("resources/shaders/shader.frag.spv"));

	pipeline.add(obj).set_descriptor(0, input_buffer);
}

fractal_layer::~fractal_layer()
{}

void fractal_layer::tick()
{
	input_buffer[0].set(&inputs);
}

bool fractal_layer::handleEvent(const hc::Event & e)
{
	switch (e.type)
	{
	case hc::EventType::KeyPressed:
		LOGF_INFO("[EVENT] Key pressed (Key: {0} ; Scancode: {1})", e.x.i, e.y.i);
		return false;
	case hc::EventType::KeyReleased:
		LOGF_INFO("[EVENT] Key released (Key: {0} ; Scancode: {1})", e.x.i, e.y.i);
		return false;
	case hc::EventType::MouseButtonPressed:
		{
			dragging = true;
		}
		return false;
	case hc::EventType::MouseButtonReleased:
		if (e.x.i == 0)
		{
			dragging = false;
		}
		return false;
	case hc::EventType::MouseMoved:
		if (dragging)
		{
			int w, h;
			hc::window::get_dimensions(&w, &h);
			int m = std::min(w, h);
			inputs.center[0] += ((e.x.f - last_mouse_pos[0]) / m) * inputs.scale;
			inputs.center[1] += ((e.y.f - last_mouse_pos[1]) / m) * inputs.scale;
		}
		last_mouse_pos[0] = e.x.f;
		last_mouse_pos[1] = e.y.f;
		return false;
	case hc::EventType::MouseScrolled:
		inputs.scale *= std::pow(1.1f, -e.y.f);
		return false;
	}
	return false;
}
