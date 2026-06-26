#include <iostream>
#include <string>
#include <vector>

#include "config.hpp"
#include "io.hpp"
#include "physics.hpp"
#include "timer.hpp"

namespace {

// Common command-line options for the sequential executable.
struct CommandLineOptions {
    std::string input_path;
    std::string output_path;
    int iterations_override = -1;
    double dt_override = -1.0;
};

void print_usage() {
    std::cout
        << "Usage:\n"
        << "  nbody_seq --input <file> [--iterations <count>] [--dt <value>]\n"
        << "            [--output <summary_file>]\n";
}

bool parse_arguments(int argc, char* argv[], CommandLineOptions& options, std::string& error_message) {
    // Parse a minimal flag-based CLI so benchmark scripts can override
    // selected runtime parameters without changing the input files.
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
    // -1 means "no override supplied" for these optional arguments.
    if (options.iterations_override < -1) {
        error_message = "--iterations must be -1 or greater.";
        return false;
    }

    if (options.dt_override != -1.0 && options.dt_override <= 0.0) {
        error_message = "--dt must be greater than zero.";
        return false;
    }

    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    CommandLineOptions options;
    std::string error_message;

    // Parse flags first so invalid invocations fail before file I/O begins.
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
    // Load the shared initial state used by all implementations.
    if (!load_simulation_input(options.input_path, config, particles, error_message)) {
        std::cerr << error_message << '\n';
        return 1;
    }

    // Optional runtime overrides are applied after loading the input file.
    if (options.iterations_override >= 0) {
        config.iterations = options.iterations_override;
    }

    if (options.dt_override > 0.0) {
        config.dt = options.dt_override;
    }

    // Start timing only after argument parsing and input validation succeed.
    Timer timer;

    for (int iteration = 1; iteration <= config.iterations; ++iteration) {
        // Keep the reference timestep order identical to the shared algorithm:
        // clear forces, compute all forces, then update all particles.
        reset_forces(particles, 0, particles.size());

        for (std::size_t i = 0; i < particles.size(); ++i) {
            particles[i].force = compute_force_for_particle(particles, i, config);
        }

        update_particles(particles, 0, particles.size(), config.dt);

    }

    // The sequential runtime becomes the baseline for all speedup calculations.
    const double elapsed_seconds = timer.elapsed_seconds();

    std::cout << "Sequential N-body simulation completed.\n";
    std::cout << "Particles: " << config.particle_count << '\n';
    std::cout << "Iterations: " << config.iterations << '\n';
    std::cout << "dt: " << config.dt << '\n';
    std::cout << "G: " << config.gravitational_constant << '\n';
    std::cout << "Softening: " << config.softening << '\n';
    std::cout << "Elapsed seconds: " << elapsed_seconds << '\n';

    if (!options.output_path.empty()) {
        SummaryInfo summary;
        summary.implementation = "sequential";
        summary.elapsed_seconds = elapsed_seconds;
        summary.threads = 1;
        summary.processes = 1;

        // Summary files are used later by the experiment script and report.
        if (!write_summary_output(options.output_path, config, summary, error_message)) {
            std::cerr << error_message << '\n';
            return 1;
        }

        std::cout << "Summary written to: " << options.output_path << '\n';
    }

    return 0;
}
