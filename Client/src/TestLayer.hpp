#pragma once

#include <Spiral.hpp>

#include <chrono>

class TestLayer : public Spiral::Layer
{
public:
	TestLayer();

	~TestLayer();

	void tick() override;

	bool handleEvent(const Spiral::Event &e) override;

private:
	std::chrono::time_point<std::chrono::steady_clock> start;
	long long timec;
	int framec;
};