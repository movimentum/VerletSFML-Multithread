#pragma once

#include "physics.hpp"
#include "geometry.hpp"


struct PhysicSolverNozzle : PhysicSolver
{
	TGeometry g;

    PhysicSolverNozzle(IVec2 size, tp::ThreadPool& tp, TGeometry _g ) : PhysicSolver(size, tp), g(_g) { ; }
	
	// Modify atom properties upon reflection from a plane 
	void reflect(PhysicObject& obj, const TFace& face){
		TPoint newVel = face.reflect(obj.getVelocity(), true);
		TPoint newPos = face.reflect(obj.last_position);
		obj.setPosition(newPos.toVec2());
		obj.addVelocity(newVel.toVec2());
	}

	// Two atoms change their velocities upon elastic collision 
	void exchangeVelocities(uint32_t atom_1_idx, uint32_t atom_2_idx){
		PhysicObject& obj_1 = objects.data[atom_1_idx];
        PhysicObject& obj_2 = objects.data[atom_2_idx];

		TFace face = { obj_1.position, obj_2.position, true };

		Vec2 v1 = obj_1.getVelocity();
		Vec2 v2 = obj_2.getVelocity();

		Vec2 v1_tau = face.tangent.toVec2() * (face.tangent * v1);
		Vec2 v1_norm = face.normal.toVec2() * (face.normal * v1);
		Vec2 v2_tau = face.tangent.toVec2() * (face.tangent * v2);
		Vec2 v2_norm = face.normal.toVec2() * (face.normal * v2);

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

				
				if ( g.isInside(pnt) )
					continue;
				if (!g.isInside(pnt_prev)) {
					//removeObject(obj);
					//++cnt;
					//std::cout << "Removed: " << cnt << std::endl;
					continue;
				}

				const TFace& face = g.getClosestFace(pnt);
				reflect(obj, face);
            }
        });
    }
};
