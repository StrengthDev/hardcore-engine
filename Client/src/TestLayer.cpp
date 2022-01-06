#include "TestLayer.hpp"

float verts[] = { 0.0f, -0.5f, 0.0f, -0.5f, 0.5f, 0.0f, 0.5f, 0.5f, 0.0f };
uint16_t idx[] = { 0, 1, 2 };

TestLayer::TestLayer()
{
	start = std::chrono::steady_clock::now();
	timec = 0;
	framec = 0;

	uint32_t vi = Spiral::ShaderLibrary::load("resources/shaders/vert.spv", Spiral::ShaderType::Vertex);
	uint32_t fi = Spiral::ShaderLibrary::load("resources/shaders/frag.spv", Spiral::ShaderType::Fragment);

	Spiral::Mesh mesh = {};
	mesh.vertices = verts;
	mesh.vSize = sizeof(float) * 9;
	mesh.indices = idx;
	mesh.iSize = 6;

	Spiral::Renderer::loadMesh(mesh, vi, fi);
	//Spiral::Renderer::Pipeline pl();
	//Spiral::Renderer::Resource res(mesh);
	//Spiral::Renderer::Instance obj(res, pl);
}

TestLayer::~TestLayer()
{

}

void TestLayer::tick()
{
	std::chrono::nanoseconds duration = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start);
	timec += duration.count();
	if (timec > 1000000000)
	{
		timec -= 1000000000;
		SPRL_TRACE("Frame count: ({0})", framec);
		framec = 0;
	}
	framec++;
	start = std::chrono::steady_clock::now();
}

bool TestLayer::handleEvent(const Spiral::Event &e)
{
	return false;
}