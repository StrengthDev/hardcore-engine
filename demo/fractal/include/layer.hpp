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
	hc::uniform input_buffer;
	hc::render_pipeline pipeline;

	struct
	{
		float center[2] = { 0.0f, 0.0f };
		float scale = 1.0f;
		float frame = 0.0f;
		std::uint16_t func = 0;
		std::uint16_t n = 0;
		std::uint16_t pad[2];
	} inputs;

	bool dragging = false;
	float last_mouse_pos[2];
};
