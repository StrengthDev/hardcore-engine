#include <spiral/core/entry_point.hpp>

#include "TestLayer.hpp"
#include "ELLayer.hpp"

class ExampleClient : public spiral::client
{
public:
	ExampleClient() : client("Spiral Application", 1, 0, 0)
	{
		spiral::window::set_title("Chad Engine");

		const char* files[2] =
		{
			"resources/icons/icon1.png",
			"resources/icons/icon2.png"
		};

		spiral::log::set_log_mask_flags(spiral::log::TRACE_BIT ^ 0xff);
		spiral::log::set_log_format_flags(spiral::log::CALLER_BIT);

		spiral::window::set_icon(files, 2);
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

spiral::client* spiral::start()
{
	return new ExampleClient();
}
