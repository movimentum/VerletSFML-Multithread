#include "renderer.hpp"


Renderer::Renderer(PhysicSolverNozzle& solver_, tp::ThreadPool& tp)
    : solver{solver_}
    , world_va{sf::LineStrip, solver_.g.coords.size() + 1 }
    , objects_va{sf::Quads}
    , thread_pool{tp}
{
    initializeWorldVA();

    object_texture.loadFromFile("res/circle.png");
    object_texture.generateMipmap();
    object_texture.setSmooth(true);
}

void Renderer::render(RenderContext& context)
{
    renderHUD(context);
    context.draw(world_va);

    sf::RenderStates states;
    states.texture = &object_texture;
    context.draw(world_va, states);
    // Particles
    updateParticlesVA();
    context.draw(objects_va, states);
}

void Renderer::initializeWorldVA()
{
	auto coords = solver.g.getCoordsInflated(0.5f);

	for (int i{ 0 }; i < world_va.getVertexCount(); ++i) {
		TPoint& pnt = coords[i % coords.size()]; // first & last world_va points should be the same
		world_va[i].position = { pnt.x, pnt.y };
	}

    const uint8_t level = 70;
    const sf::Color background_color{level, level, level};
	for (int i{ 0 }; i<world_va.getVertexCount(); ++i){
		world_va[i].color = background_color;
	}
}

void Renderer::updateParticlesVA()
{
    objects_va.resize(solver.objects.size() * 4);

    const float texture_size = 1024.0f;
    const float radius       = 0.5f;
    thread_pool.dispatch(to<uint32_t>(solver.objects.size()), [&](uint32_t start, uint32_t end) {
        for (uint32_t i{start}; i < end; ++i) {
            const PhysicObject& object = solver.objects.data[i];
            const uint32_t idx = i << 2;
            objects_va[idx + 0].position = object.position + Vec2{-radius, -radius};
            objects_va[idx + 1].position = object.position + Vec2{ radius, -radius};
            objects_va[idx + 2].position = object.position + Vec2{ radius,  radius};
            objects_va[idx + 3].position = object.position + Vec2{-radius,  radius};
            objects_va[idx + 0].texCoords = {0.0f        , 0.0f};
            objects_va[idx + 1].texCoords = {texture_size, 0.0f};
            objects_va[idx + 2].texCoords = {texture_size, texture_size};
            objects_va[idx + 3].texCoords = {0.0f        , texture_size};

            const sf::Color color = object.color;
            objects_va[idx + 0].color = color;
            objects_va[idx + 1].color = color;
            objects_va[idx + 2].color = color;
            objects_va[idx + 3].color = color;
        }
    });
}

void Renderer::renderHUD(RenderContext&)
{
    // HUD
    /*const float margin    = 20.0f;
    float       current_y = margin;
    text_time.setString("Simulation time: " + toString(phys_time.get()) + "ms");
    text_time.setPosition({margin, current_y});
    current_y += text_time.getBounds().y + 0.5f * margin;
    context.renderToHUD(text_time, RenderContext::Mode::Normal);

    text_objects.setString("Objects: " + toString(simulation.solver.objects.size()));
    text_objects.setPosition({margin, current_y});
    current_y += text_objects.getBounds().y + 0.5f * margin;
    context.renderToHUD(text_objects, RenderContext::Mode::Normal);*/
}
