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

	spiral::object_resource resource;
	spiral::instance_vector instances;
	spiral::render_pipeline pipeline;
};