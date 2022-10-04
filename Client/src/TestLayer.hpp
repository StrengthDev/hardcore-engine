#pragma once

#include <spiral.hpp>

#include <chrono>

class TestLayer : public Spiral::Layer
{
public:
	TestLayer();

	~TestLayer();

	void tick() override;

	bool handleEvent(const Spiral::Event &e) override;

private:
	double timec;
	int framec;

	Spiral::object_resource resource;
	Spiral::instance_vector instances;
	Spiral::render_pipeline pipeline;
};