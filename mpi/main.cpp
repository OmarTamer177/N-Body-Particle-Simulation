#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include <mpi.h>

#include "config.hpp"
#include "io.hpp"
#include "physics.hpp"

namespace {

// MPI executable options mirror the sequential version except for thread count.
struct CommandLineOptions {
    std::string input_path;
    std::string output_path;
    int iterations_override = -1;
    double dt_override = -1.0;
};

// Describes how the global particle array is divided across MPI processes.
struct PartitionInfo {
    // Global begin index of the first particle owned by this rank.
    int begin_index = 0;
    // Number of particles updated by this rank.
    int local_count = 0;
    // Per-rank particle counts used for ownership calculations.
    std::vector<int> particle_counts;
    // Starting particle index of each rank inside the global array.
    std::vector<int> particle_displacements;
    // Number of doubles exchanged by each rank during MPI_Allgatherv.
    std::vector<int> state_counts;
    // Starting offset of each rank inside the packed communication buffer.
    std::vector<int> state_displacements;
};

void print_usage() {
    std::cout
        << "Usage:\n"
        << "  mpiexec -n <processes> nbody_mpi.exe --input <file>\n"
        << "            [--iterations <count>] [--dt <value>]\n"
        << "            [--output <summary_file>]\n";
}

bool parse_arguments(int argc, char* argv[], CommandLineOptions& options, std::string& error_message) {
    // Parse the same override scheme used by the other executables so the
    // benchmark workflow stays consistent across implementations.
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
    // Keep the same validation rules as the non-MPI implementations.
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

PartitionInfo build_partition(std::size_t particle_count, int process_count, int rank) {
    PartitionInfo partition;
    partition.particle_counts.resize(process_count, 0);
    partition.particle_displacements.resize(process_count, 0);
    partition.state_counts.resize(process_count, 0);
    partition.state_displacements.resize(process_count, 0);

    // Split particles as evenly as possible. Any remainder is assigned one by
    // one to the first processes so the imbalance never exceeds one particle.
    const int total_particles = static_cast<int>(particle_count);
    const int base_count = total_particles / process_count;
    const int remainder = total_particles % process_count;

    int running_particle_offset = 0;
    int running_state_offset = 0;

    for (int process = 0; process < process_count; ++process) {
        const int count = base_count + (process < remainder ? 1 : 0);
        // Particle displacements are in particle units, while state
        // displacements are in doubles because the MPI buffer is flattened.
        partition.particle_counts[process] = count;
        partition.particle_displacements[process] = running_particle_offset;
        partition.state_counts[process] = count * 4;
        partition.state_displacements[process] = running_state_offset;

        if (process == rank) {
            // Save the ownership range of the current rank for fast access
            // inside the timestep loop.
            partition.begin_index = running_particle_offset;
            partition.local_count = count;
        }

        running_particle_offset += count;
        running_state_offset += count * 4;
    }

    return partition;
}

void pack_local_state(
    const std::vector<Particle>& particles,
    int begin_index,
    int local_count,
    std::vector<double>& buffer) {
    // Only dynamic state must be exchanged each timestep. Mass and id remain
    // constant, so positions and velocities are sufficient here.
    buffer.resize(static_cast<std::size_t>(local_count) * 4);

    for (int offset = 0; offset < local_count; ++offset) {
        const Particle& particle = particles[static_cast<std::size_t>(begin_index + offset)];
        const std::size_t base = static_cast<std::size_t>(offset) * 4;
        // Flatten the updated local state into:
        // [x, y, vx, vy] for each owned particle.
        buffer[base + 0] = particle.position.x;
        buffer[base + 1] = particle.position.y;
        buffer[base + 2] = particle.velocity.x;
        buffer[base + 3] = particle.velocity.y;
    }
}

void unpack_global_state(std::vector<Particle>& particles, const std::vector<double>& buffer) {
    // Rebuild the shared global snapshot after MPI_Allgatherv so the next
    // timestep reads the newest positions and velocities from every rank.
    for (std::size_t i = 0; i < particles.size(); ++i) {
        const std::size_t base = i * 4;
        particles[i].position.x = buffer[base + 0];
        particles[i].position.y = buffer[base + 1];
        particles[i].velocity.x = buffer[base + 2];
        particles[i].velocity.y = buffer[base + 3];
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    // MPI must be initialized before calling rank- or communicator-related APIs.
    MPI_Init(&argc, &argv);

    int rank = 0;
    int process_count = 1;
    // rank identifies the current process, and process_count is the total
    // number of cooperating MPI processes in this run.
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);

    CommandLineOptions options;
    std::string error_message;

    // Only rank 0 prints most user-facing errors to avoid duplicate messages.
    if (!parse_arguments(argc, argv, options, error_message)) {
        if (rank == 0 && !error_message.empty()) {
            std::cerr << error_message << '\n';
            print_usage();
        }
        MPI_Finalize();
        return error_message.empty() ? 0 : 1;
    }

    if (!validate_runtime_options(options, error_message)) {
        if (rank == 0) {
            std::cerr << error_message << '\n';
        }
        MPI_Finalize();
        return 1;
    }

    SimulationConfig config;
    std::vector<Particle> particles;
    // Every rank loads the same initial particle state so force computation
    // can read a full global snapshot locally.
    if (!load_simulation_input(options.input_path, config, particles, error_message)) {
        if (rank == 0) {
            std::cerr << error_message << '\n';
        }
        MPI_Finalize();
        return 1;
    }

    if (options.iterations_override >= 0) {
        config.iterations = options.iterations_override;
    }

    if (options.dt_override > 0.0) {
        config.dt = options.dt_override;
    }


    // Build the ownership map once because it stays constant for the whole run.
    const PartitionInfo partition = build_partition(particles.size(), process_count, rank);
    std::vector<double> local_state_buffer;
    // Every particle contributes x, y, vx, vy to the exchanged state buffer.
    std::vector<double> global_state_buffer(particles.size() * 4);

    // Synchronize before timing so all processes start measuring the
    // simulation interval from the same point.
    MPI_Barrier(MPI_COMM_WORLD);
    const double start_time = MPI_Wtime();

    for (int iteration = 1; iteration <= config.iterations; ++iteration) {
        // Each process updates only its owned block, then participates in a
        // collective exchange so the next timestep sees a consistent global state.
        reset_forces(
            particles,
            static_cast<std::size_t>(partition.begin_index),
            static_cast<std::size_t>(partition.begin_index + partition.local_count));

        for (int offset = 0; offset < partition.local_count; ++offset) {
            const std::size_t index = static_cast<std::size_t>(partition.begin_index + offset);
            // Force calculation still reads the full particle snapshot because
            // every particle is influenced by every other particle.
            particles[index].force = compute_force_for_particle(particles, index, config);
        }

        update_particles(
            particles,
            static_cast<std::size_t>(partition.begin_index),
            static_cast<std::size_t>(partition.begin_index + partition.local_count),
            config.dt);

        pack_local_state(particles, partition.begin_index, partition.local_count, local_state_buffer);

        // Share the updated local state from every rank so all processes can
        // reconstruct the same global snapshot for the next timestep.
        MPI_Allgatherv(
            local_state_buffer.empty() ? nullptr : local_state_buffer.data(),
            partition.local_count * 4,
            MPI_DOUBLE,
            global_state_buffer.empty() ? nullptr : global_state_buffer.data(),
            partition.state_counts.data(),
            partition.state_displacements.data(),
            MPI_DOUBLE,
            MPI_COMM_WORLD);

        unpack_global_state(particles, global_state_buffer);
    }

    // End timing after all ranks finish the last timestep.
    MPI_Barrier(MPI_COMM_WORLD);
    const double local_elapsed = MPI_Wtime() - start_time;
    double max_elapsed = 0.0;
    // The total runtime of the parallel job is determined by the slowest rank.
    MPI_Reduce(&local_elapsed, &max_elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "MPI N-body simulation completed.\n";
        std::cout << "Particles: " << config.particle_count << '\n';
        std::cout << "Iterations: " << config.iterations << '\n';
        std::cout << "dt: " << config.dt << '\n';
        std::cout << "Processes: " << process_count << '\n';
        std::cout << "Elapsed seconds: " << max_elapsed << '\n';

        if (!options.output_path.empty()) {
            SummaryInfo summary;
            summary.implementation = "mpi";
            summary.elapsed_seconds = max_elapsed;
            summary.threads = 1;
            summary.processes = process_count;
    
            // Only rank 0 writes the summary to avoid multiple processes
            // racing to produce the same output file.
            if (!write_summary_output(options.output_path, config, summary, error_message)) {
                std::cerr << error_message << '\n';
                MPI_Finalize();
                return 1;
            }

            std::cout << "Summary written to: " << options.output_path << '\n';
        }
    }

    MPI_Finalize();
    return 0;
}
