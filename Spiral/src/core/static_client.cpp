#include <pch.hpp>

#include <spiral/core/static_client.hpp>

#include <spiral/core/client.hpp>

namespace Spiral
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
}