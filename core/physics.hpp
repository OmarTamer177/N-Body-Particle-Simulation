#ifndef N_BODY_PHYSICS_HPP
#define N_BODY_PHYSICS_HPP

#include <cstddef>
#include <vector>

#include "config.hpp"
#include "particle.hpp"

// Compute the net gravitational force acting on one particle using the
// current full particle snapshot.
Vec2 compute_force_for_particle(
    const std::vector<Particle>& particles,
    std::size_t particle_index,
    const SimulationConfig& config);

// Clear force accumulators for a particle range before recomputing forces.
void reset_forces(std::vector<Particle>& particles, std::size_t begin_index, std::size_t end_index);

// Apply acceleration, velocity, and position updates for a particle range.
void update_particles(std::vector<Particle>& particles, std::size_t begin_index, std::size_t end_index, double dt);

#endif
