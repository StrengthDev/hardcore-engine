#pragma once

#include <hardcore.hpp>

#include <chrono>

class TestLayer : public hc::Layer
{
public:
	TestLayer();

	~TestLayer();

	void tick() override;

	bool handleEvent(const hc::Event &e) override;

private:
	double timec;
	int framec;
	std::uint32_t color = 0;
	hc::mesh resource;
	hc::dynamic_storage_vector instances;
	hc::render_pipeline pipeline;

	hc::texture_resource tex;
};
