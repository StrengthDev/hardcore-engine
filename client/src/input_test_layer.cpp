#include "input_test_layer.hpp"

ELLayer::ELLayer()
{

}

ELLayer::~ELLayer()
{

}

void ELLayer::tick()
{

}

bool ELLayer::handleEvent(const hc::Event& e)
{
	switch (e.type)
	{
	case hc::EventType::WindowResize:
		LOGF_INFO("[EVENT] Window resized ({0}, {1})", e.x.i, e.y.i);
		return false;
	case hc::EventType::KeyPressed:
		LOGF_INFO("[EVENT] Key pressed (Key: {0} ; Scancode: {1})", e.x.i, e.y.i);
		return false;
	case hc::EventType::KeyReleased:
		LOGF_INFO("[EVENT] Key released (Key: {0} ; Scancode: {1})", e.x.i, e.y.i);
		return false;
	case hc::EventType::KeyTyped:
		LOGF_INFO("[EVENT] Character typed ('{0}')", e.x.c);
		return false;
	case hc::EventType::MouseButtonPressed:
		LOGF_INFO("[EVENT] Mouse button pressed (Button: {0})", e.x.i);
		return false;
	case hc::EventType::MouseButtonReleased:
		LOGF_INFO("[EVENT] Mouse button released (Button: {0})", e.x.i);
		LOG_DEBUG("Main thread: " << std::this_thread::get_id());
		hc::parallel::immediate_async<void>([]()
			{
				LOG_DEBUG("Worker thread: " << std::this_thread::get_id());
			});
		return false;
	case hc::EventType::MouseMoved:
		//LOGF_INFO("[EVENT] Mouse moved ({0}, {1})", e.x.f, e.y.f);
		return false;
	case hc::EventType::MouseScrolled:
		LOGF_INFO("[EVENT] Mouse scrolled ({0}, {1})", e.x.f, e.y.f);
		hc::duration d;
		LOG_DEBUG(d);
		d += 2;
		LOG_DEBUG(d);
		hc::duration t = 1.23 + d;
		LOG_DEBUG(d);
		LOG_DEBUG(t);
		d = t;
		LOG_DEBUG(d % 2);
		LOG_DEBUG(d % 0.1);
		LOG_DEBUG(hc::elapsed_time())
		return false;
	}
	return false;
}