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

bool ELLayer::handleEvent(const spiral::Event &e)
{
	switch (e.type)
	{
	case spiral::EventType::WindowResize:
		LOGF_INFO("[EVENT] Window resized ({0}, {1})", e.x.i, e.y.i);
		return false;
	case spiral::EventType::KeyPressed:
		LOGF_INFO("[EVENT] Key pressed (Key: {0} ; Scancode: {1})", e.x.i, e.y.i);
		return false;
	case spiral::EventType::KeyReleased:
		LOGF_INFO("[EVENT] Key released (Key: {0} ; Scancode: {1})", e.x.i, e.y.i);
		return false;
	case spiral::EventType::KeyTyped:
		LOGF_INFO("[EVENT] Character typed ('{0}')", e.x.c);
		return false;
	case spiral::EventType::MouseButtonPressed:
		LOGF_INFO("[EVENT] Mouse button pressed (Button: {0})", e.x.i);
		return false;
	case spiral::EventType::MouseButtonReleased:
		LOGF_INFO("[EVENT] Mouse button released (Button: {0})", e.x.i);
		LOG_DEBUG("Main thread: " << std::this_thread::get_id());
		spiral::parallel::immediate_task<void>([]()
			{
				LOG_DEBUG("Worker thread: " << std::this_thread::get_id());
			});
		return false;
	case spiral::EventType::MouseMoved:
		//LOGF_INFO("[EVENT] Mouse moved ({0}, {1})", e.x.f, e.y.f);
		return false;
	case spiral::EventType::MouseScrolled:
		LOGF_INFO("[EVENT] Mouse scrolled ({0}, {1})", e.x.f, e.y.f);
		spiral::duration d;
		LOG_DEBUG(d);
		d += 2;
		LOG_DEBUG(d);
		spiral::duration t = 1.23 + d;
		LOG_DEBUG(d);
		LOG_DEBUG(t);
		d = t;
		LOG_DEBUG(d % 2);
		LOG_DEBUG(d % 0.1);
		LOG_DEBUG(spiral::elapsed_time())
		return false;
	}
	return false;
}