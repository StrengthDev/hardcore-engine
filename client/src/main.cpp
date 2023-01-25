#include <hardcore/core/entry_point.hpp>

#include "test_layer.hpp"
#include "input_test_layer.hpp"

class ExampleClient : public hc::client
{
public:
	ExampleClient() : client("Hardcore Application", 1, 0, 0)
	{
		hc::window::set_title("Hardcore");

		const char* files[2] =
		{
			"resources/icons/icon1.png",
			"resources/icons/icon2.png"
		};

		hc::log::set_log_mask_flags(hc::log::TRACE_BIT ^ 0xff);
		hc::log::set_log_format_flags(hc::log::CALLER_BIT);

		hc::window::set_icon(files, 2);
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

hc::client* hc::start()
{
	return new ExampleClient();
}
