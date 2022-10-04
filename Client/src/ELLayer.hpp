#pragma once

#include <spiral.hpp>

class ELLayer : public spiral::Layer
{
public:

	ELLayer();
	~ELLayer();

	void tick() override;

	bool handleEvent(const spiral::Event &e) override;

private:


};