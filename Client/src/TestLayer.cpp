#include "TestLayer.hpp"

float verts[] = { 0.0f, -0.5f, 0.0f, -0.5f, 0.5f, 0.0f, 0.5f, 0.5f, 0.0f };
uint16_t idx[] = { 0, 1, 2 };

TestLayer::TestLayer()
{
	timec = 0;
	framec = 0;

	spiral::shader_library::add(spiral::shader("resources/shaders/shader.vert.spv", "main"));
	spiral::shader_library::add(spiral::shader("resources/shaders/shader.frag.spv", "main"));

	spiral::data_layout vert_layout(1);
	vert_layout.set_type(0, spiral::data_layout::type::VEC3, spiral::data_layout::component_type::FLOAT32);
	resource = spiral::object_resource(verts, sizeof(verts), vert_layout);
	instances = spiral::instance_vector(1, spiral::data_layout::create<std::uint32_t>());
	pipeline = spiral::render_pipeline(spiral::shader_library::get("resources/shaders/shader.vert.spv"),
		spiral::shader_library::get("resources/shaders/shader.frag.spv"));

	pipeline.link(resource);
}

TestLayer::~TestLayer()
{

}

void TestLayer::tick()
{
	timec += spiral::delta_time();
	if (timec > 1.)
	{
		timec -= 1.;
		LOGF_INFO("Frame count: ({0})", framec);
		framec = 0;
	}
	framec++;
}

bool TestLayer::handleEvent(const spiral::Event &e)
{
	return false;
}