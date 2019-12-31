#pragma once

#include <Spiral.hpp>

class TestLayer : public Spiral::Layer
{
public:
	TestLayer();

	~TestLayer() override;

	void tick() override;

	bool handleEvent(Spiral::Event e) override;
};