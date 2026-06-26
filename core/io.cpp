#include "io.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

// Validate the simulation header before any expensive work begins.
bool validate_config(const SimulationConfig& config, std::string& error_message) {
    if (config.particle_count == 0) {
        error_message = "Particle count must be greater than zero.";
        return false;
    }

    if (config.iterations < 0) {
        error_message = "Iterations must be non-negative.";
        return false;
    }

    if (config.dt <= 0.0) {
        error_message = "Time step dt must be greater than zero.";
        return false;
    }

    if (config.gravitational_constant <= 0.0) {
        error_message = "Gravitational constant G must be greater than zero.";
        return false;
    }

    if (config.softening < 0.0) {
        error_message = "Softening must be non-negative.";
        return false;
    }

    return true;
}

}  // namespace

bool load_simulation_input(
    const std::string& path,
    SimulationConfig& config,
    std::vector<Particle>& particles,
    std::string& error_message) {
    // Load one shared input format so the sequential, OpenMP, and MPI
    // executables all start from the same simulation state.
    std::ifstream input(path);
    if (!input) {
        error_message = "Failed to open input file: " + path;
        return false;
    }

    SimulationConfig loaded_config;
    // First line format:
    // N iterations dt G softening
    if (!(input >> loaded_config.particle_count
                >> loaded_config.iterations
                >> loaded_config.dt
                >> loaded_config.gravitational_constant
                >> loaded_config.softening)) {
        error_message = "Failed to read the input header. Expected: N iterations dt G softening";
        return false;
    }

    if (!validate_config(loaded_config, error_message)) {
        return false;
    }

    std::vector<Particle> loaded_particles;
    // Reserve once so particle pushes do not repeatedly reallocate memory.
    loaded_particles.reserve(loaded_config.particle_count);

    for (std::size_t i = 0; i < loaded_config.particle_count; ++i) {
        Particle particle;
        // Per-particle line format:
        // id mass pos_x pos_y vel_x vel_y
        if (!(input >> particle.id
                    >> particle.mass
                    >> particle.position.x
                    >> particle.position.y
                    >> particle.velocity.x
                    >> particle.velocity.y)) {
            std::ostringstream builder;
            builder << "Failed to read particle line " << (i + 1)
                    << ". Expected: id mass pos_x pos_y vel_x vel_y";
            error_message = builder.str();
            return false;
        }

        if (particle.mass <= 0.0) {
            std::ostringstream builder;
            builder << "Particle " << particle.id << " has non-positive mass.";
            error_message = builder.str();
            return false;
        }

        loaded_particles.push_back(particle);
    }

    // Only publish the loaded state after the full file has been validated.
    config = loaded_config;
    particles = loaded_particles;
    return true;
}

bool write_summary_output(
    const std::string& path,
    const SimulationConfig& config,
    const SummaryInfo& summary,
    std::string& error_message) {
    // Benchmark summaries are written in a simple key-value format so the
    // experiment script and report can reuse the same output structure.
    const std::filesystem::path output_path(path);
    if (output_path.has_parent_path()) {
        std::error_code ec;
        // Create parent folders on demand so report scripts can write into
        // nested result paths without additional setup.
        std::filesystem::create_directories(output_path.parent_path(), ec);
        if (ec) {
            error_message = "Failed to create output directory for summary file.";
            return false;
        }
    }

    std::ofstream output(path);
    if (!output) {
        error_message = "Failed to open summary output file: " + path;
        return false;
    }

    // Keep the summary format simple and stable for automation.
    output << "implementation " << summary.implementation << '\n';
    output << "particles " << config.particle_count << '\n';
    output << "iterations " << config.iterations << '\n';
    output << "dt " << config.dt << '\n';
    output << "gravitational_constant " << config.gravitational_constant << '\n';
    output << "softening " << config.softening << '\n';
    output << "elapsed_seconds " << summary.elapsed_seconds << '\n';
    output << "threads " << summary.threads << '\n';
    output << "processes " << summary.processes << '\n';

    return true;
}
