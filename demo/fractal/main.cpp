#include <hardcore/core/entry_point.hpp>

#include <layer.hpp>

class fractal_demo : public hc::client
{
public:
	fractal_demo() : client("FractalDemo", 0, 0, 0)
	{
		hc::window::set_title("fractal demo");
		push_layer(new fractal_layer());
	}
};

hc::client* hc::start()
{
	return new fractal_demo();
}
