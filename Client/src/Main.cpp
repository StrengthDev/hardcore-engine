#include <spiral/core/entry_point.hpp>

#include "TestLayer.hpp"
#include "ELLayer.hpp"

class ExampleClient : public Spiral::client
{
public:
	ExampleClient() : client("Spiral Application", 1, 0, 0)
	{
		Spiral::window::set_title("Chad Engine");

		const char* files[2] =
		{
			"resources/icons/icon1.png",
			"resources/icons/icon2.png"
		};

		Spiral::log::set_log_mask_flags(Spiral::log::TRACE_BIT ^ 0xff);
		Spiral::log::set_log_format_flags(Spiral::log::CALLER_BIT);

		Spiral::window::set_icon(files, 2);
		push_layer(new TestLayer());
		push_layer(new ELLayer());
		//LOG_TRACE("trace")
		//LOG_DEBUG("debug")
		//LOG_INFO("information")
		//LOG_WARN("warning")
		//LOG_ERROR("error")
		//LOG_CRIT("critical")
	}

	~ExampleClient()
	{
		
	}
};

Spiral::client* Spiral::start()
{
	return new ExampleClient();
}
