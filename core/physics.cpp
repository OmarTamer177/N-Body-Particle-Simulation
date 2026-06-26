#include "physics.hpp"

#include <cmath>

Vec2 compute_force_for_particle(
    const std::vector<Particle>& particles,
    std::size_t particle_index,
    const SimulationConfig& config) {
    // Source is the particle whose net force we are currently computing.
    const Particle& source = particles[particle_index];

    Vec2 total_force{};
    // Sum the contribution of every other particle using the shared
    // gravitational model for the current timestep snapshot.
    for (std::size_t j = 0; j < particles.size(); ++j) {
        if (j == particle_index) {
            continue;
        }

        const Particle& other = particles[j];
        // Direction from the current particle to the other particle.
        const double dx = other.position.x - source.position.x;
        const double dy = other.position.y - source.position.y;
        // Softening prevents division-by-zero-like behavior when particles
        // become extremely close to one another.
        const double distance_squared = (dx * dx) + (dy * dy) + (config.softening * config.softening);
        const double distance = std::sqrt(distance_squared);
        // The vector force form uses 1 / r^3 after multiplying by dx and dy.
        const double inverse_distance_cubed = 1.0 / (distance_squared * distance);
        const double scale =
            config.gravitational_constant * source.mass * other.mass * inverse_distance_cubed;

        // Add this pairwise contribution to the running net force.
        total_force.x += scale * dx;
        total_force.y += scale * dy;
    }

    return total_force;
}

void reset_forces(std::vector<Particle>& particles, std::size_t begin_index, std::size_t end_index) {
    // Force is recomputed from scratch every iteration, so old values must
    // be cleared before the next accumulation phase starts.
    for (std::size_t i = begin_index; i < end_index; ++i) {
        particles[i].force = Vec2{};
    }
}

void update_particles(std::vector<Particle>& particles, std::size_t begin_index, std::size_t end_index, double dt) {
    // Update motion only after all forces are known to avoid mixing old and
    // new particle states inside the same timestep.
    for (std::size_t i = begin_index; i < end_index; ++i) {
        Particle& particle = particles[i];
        // Newton's second law: a = F / m.
        const double ax = particle.force.x / particle.mass;
        const double ay = particle.force.y / particle.mass;

        // Explicit time integration for velocity, then position.
        particle.velocity.x += ax * dt;
        particle.velocity.y += ay * dt;
        particle.position.x += particle.velocity.x * dt;
        particle.position.y += particle.velocity.y * dt;
    }
}
