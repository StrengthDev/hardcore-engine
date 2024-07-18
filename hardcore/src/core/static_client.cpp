#include <pch.hpp>

#include <core/static_client.hpp>

#include <core/client.hpp>

namespace ENGINE_NAMESPACE
{
	Layer* push_layer(Layer* layer)
	{
		return client::instance->push_layer(layer);
	}

	void pop_layer()
	{
		client::instance->pop_layer();
	}

	Layer* push_overlay(Layer* layer)
	{
		return client::instance->push_overlay(layer);
	}

	void pop_overlay()
	{
		client::instance->pop_overlay();
	}

	void clear_layers()
	{
		client::instance->clear_layers();
	}

	void shutdown()
	{
		client::instance->shutdown();
	}

	time_t delta_time()
	{
		return client::instance->delta_time;
	}

	duration elapsed_time()
	{
		return client::instance->elapsed_time;
	}
}
