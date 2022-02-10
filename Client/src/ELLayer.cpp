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
		LOGF_INFO("[EVENT] Window resized ({0}, {1})", e.x.i, e.y.i);
		return false;
	case Spiral::EventType::KeyPressed:
		LOGF_INFO("[EVENT] Key pressed (Key: {0} ; Scancode: {1})", e.x.i, e.y.i);
		return false;
	case Spiral::EventType::KeyReleased:
		LOGF_INFO("[EVENT] Key released (Key: {0} ; Scancode: {1})", e.x.i, e.y.i);
		return false;
	case Spiral::EventType::KeyTyped:
		LOGF_INFO("[EVENT] Character typed ('{0}')", e.x.c);
		return false;
	case Spiral::EventType::MouseButtonPressed:
		LOGF_INFO("[EVENT] Mouse button pressed (Button: {0})", e.x.i);
		return false;
	case Spiral::EventType::MouseButtonReleased:
		LOGF_INFO("[EVENT] Mouse button released (Button: {0})", e.x.i);
		LOG_DEBUG("Main thread: " << std::this_thread::get_id());
		Spiral::parallel::immediate_task<void>([]()
			{
				LOG_DEBUG("Worker thread: " << std::this_thread::get_id());
			});
		return false;
	case Spiral::EventType::MouseMoved:
		//LOGF_INFO("[EVENT] Mouse moved ({0}, {1})", e.x.f, e.y.f);
		return false;
	case Spiral::EventType::MouseScrolled:
		LOGF_INFO("[EVENT] Mouse scrolled ({0}, {1})", e.x.f, e.y.f);
		return false;
	}
	return false;
}