#pragma once

#include <hardcore.hpp>

class fractal_layer : public hc::Layer
{
public:
	fractal_layer();

	~fractal_layer();

	void tick() override;

	bool handleEvent(const hc::Event& e) override;

private:
	hc::mesh obj;
	hc::uniform vertex_input;
	hc::uniform frag_input;
	hc::render_pipeline pipeline;

	struct
	{
		float center[2] = { 0.0f, 0.0f };
		float scale = 1.0f;
		float ratio = 0.0f;
	} v_inputs;

	struct
	{
		std::uint32_t func = n_funcs - 1;
		std::uint32_t n = 1;
		std::uint32_t pad[2];
		float frame = 0;
	} f_inputs;

	bool dragging = false;
	float last_mouse_pos[2];
	int w, h;
	bool animate = false;
	float time = 0;

	static const std::uint32_t n_funcs = 6;
	static const std::uint32_t max_iterations = 10;
};
