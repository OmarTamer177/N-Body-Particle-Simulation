#ifndef N_BODY_IO_HPP
#define N_BODY_IO_HPP

#include <string>
#include <vector>

#include "config.hpp"
#include "particle.hpp"

// Small metadata block written beside every timing result.
struct SummaryInfo {
    std::string implementation = "unknown";
    double elapsed_seconds = 0.0;
    int threads = 1;
    int processes = 1;
};

// Read one simulation case from disk into the shared config and particle list.
bool load_simulation_input(
    const std::string& path,
    SimulationConfig& config,
    std::vector<Particle>& particles,
    std::string& error_message);

// Write a plain-text timing summary that can be reused in experiments and reports.
bool write_summary_output(
    const std::string& path,
    const SimulationConfig& config,
    const SummaryInfo& summary,
    std::string& error_message);

#endif
