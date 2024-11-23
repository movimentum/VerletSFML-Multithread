#include <iostream>

#include "engine/window_context_handler.hpp"
#include "engine/common/color_utils.hpp"

#include "physics/physics_nozzle.hpp"
#include "thread_pool/thread_pool.hpp"
#include "renderer/renderer.hpp"

#include "physics/geometry.hpp"


int main()
{
	srand(0x13b);
	
    const uint32_t window_width  = 1920;
    const uint32_t window_height = 1080;
    WindowContextHandler app("Verlet-MultiThread", sf::Vector2u(window_width, window_height), sf::Style::Default);
    RenderContext& render_context = app.getRenderContext();
    // Initialize solver and renderer

    tp::ThreadPool thread_pool(10);
	const IVec2 world_size{4000, 350};

	TGeometry g({
		{0.0f, 0.0f}, {1700.0f, 0.0f}, {2000.0f, 150.0f}, {2200.0f, 150.0f}, {2500.0f, 0.0f}, {4000.0f, 0.0f},
		{4000.0f, 350.0f}, {2500.0f, 350.0f}, {2200.0f, 200.0f}, {2000.0f, 200.0f}, {1700.0f, 350.0f}, {0.0f, 350.0f}
	};
	
	/* // Just a square box
	const IVec2 world_size{ 100, 100 };
	g.coords = { {0.0f, 0.0f}, {100.0f, 0.0f}, {100.0f, 100.0f}, {0.0f, 100.0f} };
	*/
    
	PhysicSolverNozzle solver{world_size, thread_pool, g};
	solver.gravity = {0.0f, 0.0f};
	
    Renderer renderer(solver, thread_pool);

    const float margin = 0.0f;
    const auto  zoom   = static_cast<float>(window_height - margin) / static_cast<float>(world_size.y) * 0.6;
    render_context.setZoom(zoom);
    render_context.setFocus({world_size.x * 0.64f, world_size.y * 0.64f});
	//render_context.setFocus({ 0.0f, 0.0f });

    bool emit = true;
    app.getEventManager().addKeyPressedCallback(sf::Keyboard::Space, [&](sfev::CstEv) {
        emit = !emit;
    });

    constexpr uint32_t fps_cap = 60;
    int32_t target_fps = fps_cap;
    app.getEventManager().addKeyPressedCallback(sf::Keyboard::S, [&](sfev::CstEv) {
        target_fps = target_fps ? 0 : fps_cap;
        app.setFramerateLimit(target_fps);
    });
	
	// Save image to memory button
	app.getEventManager().addKeyPressedCallback(sf::Keyboard::M, [&](sfev::CstEv) {
        render_context.save_to_memory();
		printf("Saved to memory!\n");
		render_context.save_image_from_memory_to_file("b-from_memory.jpg");
		printf("Saved from memory!\n");
    });
	
	// Save image from memory to file button
	bool is_saving_pics = false;
	app.getEventManager().addKeyPressedCallback(sf::Keyboard::Q, [&](sfev::CstEv) {
		is_saving_pics = !is_saving_pics;
    });
	
	// Print frame info
	app.getEventManager().addKeyPressedCallback(sf::Keyboard::P, [&](sfev::CstEv) {
		float zoom = render_context.getZoom();
		Vec2 center = render_context.getFocus();
		printf("zoom: %f\n", zoom);
		printf("center: %f, %f\n", center.x, center.y);
    });
	
	// Setup
	for (uint32_t i{100000}; i--;) {
		auto x = 1 + (float(rand()) / RAND_MAX * (world_size.x - 2));
		auto y = 1 + (float(rand()) / RAND_MAX * (world_size.y - 2));
		
		// Gas ahead of the nozzle
		if ( ! solver.g.isInside({x,y}) )
			 continue;

		// Vacuum or rarefied gas in the nozzle and downstream
		if (x > 1600 && i%20)
			continue;

		const auto id = solver.createObject({x, y});
		solver.objects[id].last_position.x -= 0.0f;  // bulk velocity
		solver.objects[id].last_position.x += 0.2f * (float(rand()) / RAND_MAX - 0.5f); // chaotic speed: 0 -- for hypersonic; considerably greater than bulk velocity -- for ~subsonic
		solver.objects[id].last_position.y += 0.2f * (float(rand()) / RAND_MAX - 0.5f); //
		solver.objects[id].color = ColorUtils::getRainbow(id * 0.0001f);
	}

    // Main loop
    const float dt = 1.0f / static_cast<float>(fps_cap);
	int i=0;
    while (app.run()) {
        
		solver.update(dt);

        render_context.clear();
        renderer.render(render_context);
        render_context.display();

		
		++i;
		
		if (is_saving_pics){
			std::stringstream s;
			s << "pics/file-" << i << ".jpg";
			render_context.save_display_image(s.str());
		}
		
		// TODO:
		// 1. Evolve to a given time
		// 2. Set stations (zoom and focus)
		// 3. Save pics during a given time
		// 4. Prepare a set of interesting stations to capture
    }
	
    return 0;
}
