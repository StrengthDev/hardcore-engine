#pragma once

#include <spiral.hpp>

class ELLayer : public Spiral::Layer
{
public:

	ELLayer();
	~ELLayer();

	void tick() override;

	bool handleEvent(const Spiral::Event &e) override;

private:


};