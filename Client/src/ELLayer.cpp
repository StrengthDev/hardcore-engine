#include "ELLayer.hpp"

ELLayer::ELLayer()
{

}

ELLayer::~ELLayer()
{

}

void ELLayer::tick()
{

}

bool ELLayer::handleEvent(const Spiral::Event &e)
{
	switch (e.type)
	{
	case Spiral::EventType::WindowResize:
		SPRL_INFO("[EVENT] Window resized ({0}, {1})", e.x.i, e.y.i);
		return false;
	case Spiral::EventType::KeyPressed:
		SPRL_INFO("[EVENT] Key pressed (Key: {0} ; Scancode: {1})", e.x.i, e.y.i);
		return false;
	case Spiral::EventType::KeyReleased:
		SPRL_INFO("[EVENT] Key released (Key: {0} ; Scancode: {1})", e.x.i, e.y.i);
		return false;
	case Spiral::EventType::KeyTyped:
		SPRL_INFO("[EVENT] Character typed ('{0}')", e.x.c);
		return false;
	case Spiral::EventType::MouseButtonPressed:
		SPRL_INFO("[EVENT] Mouse button pressed (Button: {0})", e.x.i);
		return false;
	case Spiral::EventType::MouseButtonReleased:
		SPRL_INFO("[EVENT] Mouse button released (Button: {0})", e.x.i);
		return false;
	case Spiral::EventType::MouseMoved:
		//SPRL_INFO("[EVENT] Mouse moved ({0}, {1})", e.x.f, e.y.f);
		return false;
	case Spiral::EventType::MouseScrolled:
		SPRL_INFO("[EVENT] Mouse scrolled ({0}, {1})", e.x.f, e.y.f);
		return false;
	}
	return false;
}