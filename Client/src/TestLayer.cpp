#include "TestLayer.hpp"

float verts[] = { 0.0f, -0.5f, 0.0f, -0.5f, 0.5f, 0.0f, 0.5f, 0.5f, 0.0f };
uint16_t idx[] = { 0, 1, 2 };

TestLayer::TestLayer()
{
	timec = 0;
	framec = 0;

	Spiral::shader_library::add(Spiral::shader("resources/shaders/shader.vert.spv", "main"));
	Spiral::shader_library::add(Spiral::shader("resources/shaders/shader.frag.spv", "main"));

	Spiral::Mesh mesh = {};
	mesh.vertices = verts;
	mesh.vSize = sizeof(float) * 9;
	mesh.indices = idx;
	mesh.iSize = 6;

	Spiral::data_layout vert_layout(1);
	vert_layout.set_type(0, Spiral::data_layout::type::VEC3, Spiral::data_layout::component_type::FLOAT32);
	resource = Spiral::object_resource(verts, sizeof(verts), vert_layout);
	instances = Spiral::instance_vector(1, Spiral::data_layout::create<std::uint32_t>());
	pipeline = Spiral::render_pipeline(Spiral::shader_library::get("resources/shaders/shader.vert.spv"),
		Spiral::shader_library::get("resources/shaders/shader.frag.spv"));

	pipeline.link(resource);
}

TestLayer::~TestLayer()
{

}

void TestLayer::tick()
{
	timec += Spiral::delta_time();
	if (timec > 1.)
	{
		timec -= 1.;
		LOGF_INFO("Frame count: ({0})", framec);
		framec = 0;
	}
	framec++;
}

bool TestLayer::handleEvent(const Spiral::Event &e)
{
	return false;
}