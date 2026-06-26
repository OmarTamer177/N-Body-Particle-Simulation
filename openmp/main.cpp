#include <iostream>
#include <string>
#include <vector>

#include <omp.h>

#include "config.hpp"
#include "io.hpp"
#include "physics.hpp"
#include "timer.hpp"

namespace {

// Command-line options for the shared-memory OpenMP executable.
struct CommandLineOptions {
    std::string input_path;
    std::string output_path;
    int iterations_override = -1;
    double dt_override = -1.0;
    int threads = 0;
};

void print_usage() {
    std::cout
        << "Usage:\n"
        << "  nbody_omp --input <file> [--iterations <count>] [--dt <value>]\n"
        << "            [--threads <count>] [--output <summary_file>]\n";
}

bool parse_arguments(int argc, char* argv[], CommandLineOptions& options, std::string& error_message) {
    // Parse runtime controls for the benchmark script and manual testing.
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];

        auto require_value = [&](const char* flag, std::string& target) -> bool {
            if (i + 1 >= argc) {
                error_message = std::string("Missing value for ") + flag;
                return false;
            }
            target = argv[++i];
            return true;
        };

        if (argument == "--input") {
            if (!require_value("--input", options.input_path)) {
                return false;
            }
        } else if (argument == "--output") {
            if (!require_value("--output", options.output_path)) {
                return false;
            }
        } else if (argument == "--iterations") {
            if (i + 1 >= argc) {
                error_message = "Missing value for --iterations";
                return false;
            }
            options.iterations_override = std::stoi(argv[++i]);
        } else if (argument == "--dt") {
            if (i + 1 >= argc) {
                error_message = "Missing value for --dt";
                return false;
            }
            options.dt_override = std::stod(argv[++i]);
        } else if (argument == "--threads") {
            if (i + 1 >= argc) {
                error_message = "Missing value for --threads";
                return false;
            }
            options.threads = std::stoi(argv[++i]);
        } else if (argument == "--help" || argument == "-h") {
            print_usage();
            return false;
        } else {
            error_message = "Unknown argument: " + argument;
            return false;
        }
    }

    if (options.input_path.empty()) {
        error_message = "The --input argument is required.";
        return false;
    }

    return true;
}

bool validate_runtime_options(const CommandLineOptions& options, std::string& error_message) {
    // Zero threads means "leave thread count to the OpenMP runtime".
    if (options.iterations_override < -1) {
        error_message = "--iterations must be -1 or greater.";
        return false;
    }

    if (options.dt_override != -1.0 && options.dt_override <= 0.0) {
        error_message = "--dt must be greater than zero.";
        return false;
    }

    if (options.threads < 0) {
        error_message = "--threads must be zero or greater.";
        return false;
    }

    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    CommandLineOptions options;
    std::string error_message;

    // Validate command-line structure before touching the input file.
    if (!parse_arguments(argc, argv, options, error_message)) {
        if (!error_message.empty()) {
            std::cerr << error_message << '\n';
            print_usage();
            return 1;
        }
        return 0;
    }

    if (!validate_runtime_options(options, error_message)) {
        std::cerr << error_message << '\n';
        return 1;
    }

    SimulationConfig config;
    std::vector<Particle> particles;
    // Load the same particle layout used by the sequential baseline.
    if (!load_simulation_input(options.input_path, config, particles, error_message)) {
        std::cerr << error_message << '\n';
        return 1;
    }

    // Apply optional command-line overrides after file loading succeeds.
    if (options.iterations_override >= 0) {
        config.iterations = options.iterations_override;
    }

    if (options.dt_override > 0.0) {
        config.dt = options.dt_override;
    }


    if (options.threads > 0) {
        // Disable automatic thread-count changes so benchmark runs respect
        // the requested worker count.
        omp_set_dynamic(0);
        omp_set_num_threads(options.threads);
    }

    // Measure only the simulation phase, not argument parsing or file loading.
    Timer timer;

    for (int iteration = 1; iteration <= config.iterations; ++iteration) {
        // Each loop is parallelized over particles, which is safe because
        // threads write to disjoint particle slots for a stable timestep state.
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < static_cast<int>(particles.size()); ++i) {
            // Each thread resets a separate particle slot.
            particles[static_cast<std::size_t>(i)].force = Vec2{};
        }

        #pragma omp parallel for schedule(static)
        for (int i = 0; i < static_cast<int>(particles.size()); ++i) {
            // Force for particle i is independent as long as all threads read
            // the same unchanged particle snapshot for this timestep.
            particles[static_cast<std::size_t>(i)].force =
                compute_force_for_particle(particles, static_cast<std::size_t>(i), config);
        }

        #pragma omp parallel for schedule(static)
        for (int i = 0; i < static_cast<int>(particles.size()); ++i) {
            // Once forces are finalized, each particle update is independent.
            Particle& particle = particles[static_cast<std::size_t>(i)];
            const double ax = particle.force.x / particle.mass;
            const double ay = particle.force.y / particle.mass;

            particle.velocity.x += ax * config.dt;
            particle.velocity.y += ay * config.dt;
            particle.position.x += particle.velocity.x * config.dt;
            particle.position.y += particle.velocity.y * config.dt;
        }

    }

    const double elapsed_seconds = timer.elapsed_seconds();
    int actual_threads = 1;

    // Query the number of threads that actually participated in the run for
    // accurate reporting in the benchmark summary.
    #pragma omp parallel
    {
        #pragma omp single
        actual_threads = omp_get_num_threads();
    }

    std::cout << "OpenMP N-body simulation completed.\n";
    std::cout << "Particles: " << config.particle_count << '\n';
    std::cout << "Iterations: " << config.iterations << '\n';
    std::cout << "dt: " << config.dt << '\n';
    std::cout << "Threads: " << actual_threads << '\n';
    std::cout << "Elapsed seconds: " << elapsed_seconds << '\n';

    if (!options.output_path.empty()) {
        SummaryInfo summary;
        summary.implementation = "openmp";
        summary.elapsed_seconds = elapsed_seconds;
        summary.threads = actual_threads;
        summary.processes = 1;

        // Persist timing metadata in the same format as the other versions.
        if (!write_summary_output(options.output_path, config, summary, error_message)) {
            std::cerr << error_message << '\n';
            return 1;
        }

        std::cout << "Summary written to: " << options.output_path << '\n';
    }

    return 0;
}
