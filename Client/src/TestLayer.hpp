#pragma once

#include <spiral.hpp>

#include <chrono>

class TestLayer : public spiral::Layer
{
public:
	TestLayer();

	~TestLayer();

	void tick() override;

	bool handleEvent(const spiral::Event &e) override;

private:
	double timec;
	int framec;
	std::uint32_t color = 0;
	spiral::mesh resource;
	spiral::dynamic_storage_vector instances;
	spiral::render_pipeline pipeline;
};