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
		hc::u32 func = n_funcs - 1;
		hc::u32 n = 1;
		hc::u32 pad[2];
		float frame = 0;
	} f_inputs;

	bool dragging = false;
	float last_mouse_pos[2];
	int w, h;
	bool animate = false;
	float time = 0;

	static const hc::u32 n_funcs = 6;
	static const hc::u32 max_iterations = 10;
};
