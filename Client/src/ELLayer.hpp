#pragma once

#include <Spiral.hpp>

class ELLayer : public Spiral::Layer
{
public:

	ELLayer();
	~ELLayer();

	void tick() override;

	bool handleEvent(Spiral::Event &e) override;

private:


};