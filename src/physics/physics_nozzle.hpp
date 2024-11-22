#pragma once

#include "physics.hpp"
#include "geometry.hpp"


struct PhysicSolverNozzle : PhysicSolver
{
	TGeometry g;

    PhysicSolverNozzle(IVec2 size, tp::ThreadPool& tp, TGeometry _g ) : PhysicSolver(size, tp), g(_g) { ; }


	// Get a vector being reflected about a direction 
	Vec2 reflectVec(const Vec2& vector, const Vec2& direction){

		float len = sqrt(direction.x * direction.x + direction.y * direction.y);
		Vec2 l = {direction.x / len, direction.y / len}; // unit tangent
		Vec2 n = {l.y, -l.x}; // unit normal
		
		float vel_norm = vector.x * n.x + vector.y * n.y;
		Vec2 prj_n = {vel_norm * n.x, vel_norm * n.y};
		
		float vel_wall = vector.x * l.x + vector.y * l.y;
		Vec2 prj_l = {vel_wall * l.x, vel_wall * l.y};
		
		Vec2 reflected = prj_l - prj_n;
		return reflected;
	}
	
	// Modify atom properties upon reflection from a plane 
	void reflect(PhysicObject& obj, const Vec2& plane){
		auto reflected_velocity = reflectVec( obj.getVelocity(), plane);
		obj.setPosition(obj.last_position);
		obj.addVelocity(reflected_velocity);
	}

	// Two atoms change their velocities upon elastic collision 
	void exchangeVelocities(uint32_t atom_1_idx, uint32_t atom_2_idx){
		constexpr float response_coef = 1.0f;
        constexpr float eps           = 0.0001f;
		PhysicObject& obj_1 = objects.data[atom_1_idx];
        PhysicObject& obj_2 = objects.data[atom_2_idx];
		Vec2 n  = obj_1.position - obj_2.position;
		const float dist2 = n.x * n.x + n.y * n.y;
		//if (dist2 < 1.0f && dist2 > eps) {
			n /= sqrt(n.x*n.x+n.y*n.y);		
			Vec2 l = {n.y, -n.x};
			
			Vec2 v1 = obj_1.position - obj_1.last_position;
			Vec2 v2 = obj_2.position - obj_2.last_position;
			
			Vec2 v1_tau  = l * (l.x*v1.x + l.y*v1.y);
			Vec2 v1_norm = n * (n.x*v1.x + n.y*v1.y);
			Vec2 v2_tau  = l * (l.x*v2.x + l.y*v2.y);
			Vec2 v2_norm = n * (n.x*v2.x + n.y*v2.y);
			
			Vec2 v1_new = v1_tau + v2_norm;
			Vec2 v2_new = v2_tau + v1_norm;
			
			obj_1.last_position = obj_1.position - v1_new;
			obj_2.last_position = obj_2.position - v2_new;
	}
	
	// Checks if two atoms are colliding and if so create a new contact
	// Overloads base class functionality
    void solveContact(uint32_t atom_1_idx, uint32_t atom_2_idx) override
    {
        constexpr float response_coef = 1.0f;
        constexpr float eps           = 0.0001f;
        PhysicObject& obj_1 = objects.data[atom_1_idx];
        PhysicObject& obj_2 = objects.data[atom_2_idx];
        const Vec2 o2_o1  = obj_1.position - obj_2.position;
        const float dist2 = o2_o1.x * o2_o1.x + o2_o1.y * o2_o1.y;
        if (dist2 < 1.0f && dist2 > eps) {
            const float dist          = sqrt(dist2);
            // Radius are all equal to 1.0f
            const float delta  = response_coef * 0.5f * (1.0f - dist);
            const Vec2 col_vec = (o2_o1 / dist) * delta;
            obj_1.position += col_vec;
            obj_2.position -= col_vec;
			obj_1.last_position += col_vec;
            obj_2.last_position -= col_vec;
			
			exchangeVelocities(atom_1_idx, atom_2_idx);
        }
    }
	
	// Overloads base class functionality
	void updateObjects_multi(float dt) override
    {
        thread_pool.dispatch(to<uint32_t>(objects.size()), [&](uint32_t start, uint32_t end){
            for (uint32_t i{start}; i < end; ++i) {
                PhysicObject& obj = objects.data[i];
                // Add gravity
                obj.acceleration += gravity;
                // Apply Verlet integration
                obj.update(dt);
				

				// Geometry boundaries
				const TPoint pnt = { obj.position.x, obj.position.y };
				const TPoint pnt_prev = { obj.last_position.x, obj.last_position.y };

				for (auto face : g.faces) {

					if (!face.isWall || face.isInside(pnt))
						continue;

					// If obj was not inside on the previous iteration then let it fly away
					if (!face.isInside(pnt_prev))
						continue;

					reflect(obj, { face.tangent.x, face.tangent.y });

					break;
				}
            }
        });
    }
};
