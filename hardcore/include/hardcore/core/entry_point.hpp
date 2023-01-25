#pragma once

#include "core.hpp"
#include "entry_point_internal.hpp"
#include "client.hpp"

extern ENGINE_NAMESPACE::client* ENGINE_NAMESPACE::start();

int main(int argc, char** argv)
{
	ENGINE_NAMESPACE::internal::init();
	auto client = ENGINE_NAMESPACE::start();
	client->run();
	delete client;
	ENGINE_NAMESPACE::internal::terminate();
}
