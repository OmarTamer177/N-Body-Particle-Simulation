#ifndef N_BODY_CONFIG_HPP
#define N_BODY_CONFIG_HPP

#include <cstddef>

// Shared simulation parameters loaded from the input header and optionally
// overridden by command-line arguments at runtime.
struct SimulationConfig {
    // Total number of particles in the current simulation case.
    std::size_t particle_count = 0;
    // Number of timesteps to execute.
    int iterations = 0;
    // Fixed timestep used in the velocity and position updates.
    double dt = 0.0;
    // Gravitational constant used by the pairwise force equation.
    double gravitational_constant = 1.0;
    // Softening term used to avoid unstable forces at near-zero distance.
    double softening = 1e-9;
};

#endif
