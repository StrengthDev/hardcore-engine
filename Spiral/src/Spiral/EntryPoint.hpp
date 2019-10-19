#pragma once

extern Spiral::Client* Spiral::Start();

int main(int argc, char** argv)
{
	auto client = Spiral::Start();
	client->Run();
	delete client;
}