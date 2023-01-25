#pragma once

#include <hardcore.hpp>

class ELLayer : public hc::Layer
{
public:

	ELLayer();
	~ELLayer();

	void tick() override;

	bool handleEvent(const hc::Event &e) override;

private:


};