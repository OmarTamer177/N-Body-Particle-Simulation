#ifndef N_BODY_PARTICLE_HPP
#define N_BODY_PARTICLE_HPP

#include "vec2.hpp"

// Particle state shared by all implementations.
struct Particle {
    // Stable identifier read from the input file.
    int id = -1;
    // Particle mass. Must remain positive for a valid simulation.
    double mass = 1.0;
    // Current 2D position.
    Vec2 position{};
    // Current 2D velocity.
    Vec2 velocity{};
    // Net force accumulated for the current timestep only.
    Vec2 force{};
};

#endif
