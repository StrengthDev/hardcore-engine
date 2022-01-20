#include <spiral/core/entry_point.hpp>

#include "TestLayer.hpp"
#include "ELLayer.hpp"

class ExampleClient : public Spiral::Client
{
public:
	ExampleClient() : Client("Spiral Application", 1, 0, 0)
	{
		//TODO: this is stupid, change to loose functions instead of class static methods
		getWindow().setTitle("Chad Engine");

		const char* files[2] =
		{
			"resources/icons/icon1.png",
			"resources/icons/icon2.png"
		};

		Spiral::log::set_log_mask_flags(Spiral::log::TRACE_BIT ^ 0xff);
		Spiral::log::set_log_format_flags(Spiral::log::CALLER_BIT);

		getWindow().setIcon(files, 2);
		//setProperties("Debug Program", 1, 0, 0);
		LOG_TRACE("trace")
		LOG_DEBUG("debug")
		LOG_INFO("information")
		LOG_WARN("warning")
		LOG_ERROR("error")
		LOG_CRIT("critical")
	}

	~ExampleClient()
	{
		
	}

	void pushInitialLayers()
	{
		pushLayer(new TestLayer());
		pushLayer(new ELLayer());
	}
};

Spiral::Client* Spiral::start()
{
	return new ExampleClient();
}