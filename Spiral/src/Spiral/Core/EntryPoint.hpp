#pragma once

extern Spiral::Client* Spiral::Start();

int main(int argc, char** argv)
{
#ifndef NDEBUG
	Spiral::logInit();
#endif
	auto client = Spiral::Start();
	client->Run();
	delete client;
}