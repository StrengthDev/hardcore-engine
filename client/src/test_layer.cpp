#include "test_layer.hpp"

float verts[] = { 0.0f, -0.5f, 0.0f, -0.5f, 0.5f, 0.0f, 0.5f, 0.5f, 0.0f };
uint16_t idx[] = { 0, 1, 2 };

TestLayer::TestLayer()
{
	timec = 0;
	framec = 0;

	hc::shader_library::add(hc::shader("resources/shaders/shader.vert.spv", "main"));
	hc::shader_library::add(hc::shader("resources/shaders/shader.frag.spv", "main"));

	hc::data_layout vert_layout(1);
	vert_layout.set_type(0, hc::data_layout::type::VEC3, hc::data_layout::component_type::FLOAT32);
	resource = hc::mesh(verts, sizeof(verts), vert_layout);
	instances = hc::dynamic_storage_vector(hc::data_layout::create<hc::u32>(), 10);
	pipeline = hc::render_pipeline(hc::shader_library::get("resources/shaders/shader.vert.spv"),
		hc::shader_library::get("resources/shaders/shader.frag.spv"));

	pipeline.add(resource).set_descriptor(0, instances);
	//pipeline.set_descriptor(resource, 0, instances);

	hc::u32 w = 9, h = 9;
	std::vector<hc::u8> pixels(w * h * 4);
	for (hc::u32 y = 0; y < h; y++)
	{
		for (hc::u32 x = 0; x < h; x++)
		{
			hc::u32 p = (x + y * w) * 4;
			pixels[p] = std::lerp(0, 255, static_cast<float>(x) / w);
			pixels[p + 1] = std::lerp(0, 255, static_cast<float>(y) / h);
			pixels[p + 3] = 255;
		}
	}
	tex = hc::texture_resource(pixels.data(), w, h);
}

TestLayer::~TestLayer()
{

}

void TestLayer::tick()
{
	timec += hc::delta_time();
	if (timec > 1.)
	{
		timec -= 1.;
		LOGF_INFO("Frame count: ({0})", framec);
		framec = 0;
	}
	framec++;
	instances[0].set(&color);
}

bool TestLayer::handleEvent(const hc::Event& e)
{
	switch (e.type)
	{
	case hc::EventType::MouseButtonReleased:
		//LOGF_INFO("[EVENT] Mouse moved ({0}, {1})", e.x.f, e.y.f);
		if (color) color = 0;
		else color = 1;
		return false;
	default:
		return false;
	}
	return false;
}