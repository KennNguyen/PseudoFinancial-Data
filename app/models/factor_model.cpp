#include <random>
#include <stdexcept>
#include <tuple>
#include <Eigen/Dense>

std::tuple<Eigen::MatrixXd, Eigen::VectorXd> simulate_factor_model(
    int simulation_duration,
    double factor_volatility,
    int num_assets,
    const Eigen::VectorXd& beta_vector,
    unsigned int random_seed
) {
    // Error Handling
    if (simulation_duration <= 0 || factor_volatility <= 0.0 || num_assets <= 0)
        throw std::invalid_argument("All input sizes and volatility must be positive.");
    if (beta_vector.size() != num_assets)
        throw std::invalid_argument("Beta vector size must match number of assets.");

    /*
    Althought Mersenne Twister provides good performance and long period, it would still exhibit a pattern in HF applications. 
    We could use a larger state space ie mt19937_64, or use better generators ie PCG or Xoshiro/Xoroshiro.
    Also, check out Intel MKL or Boost.Random library. 
    */
    std::mt19937 random_generator(random_seed);
    std::normal_distribution<> normal_distribution(0.0, factor_volatility);
    std::normal_distribution<> idio_noise(0.0, 1.0);

    // Memory Allocation
    Eigen::VectorXd factor_changes(simulation_duration);
    Eigen::VectorXd cumulative_factor_levels(simulation_duration);
    Eigen::MatrixXd asset_returns(num_assets, simulation_duration);

    // Generation
    for (int time_step = 0; time_step < simulation_duration; ++time_step) {
        double factor_change = normal_distribution(random_generator);
        factor_changes(time_step) = factor_change;
        if (time_step == 0)
            cumulative_factor_levels(time_step) = factor_change;
        else
            cumulative_factor_levels(time_step) = cumulative_factor_levels(time_step - 1) + factor_change;

        for (int asset_index = 0; asset_index < num_assets; ++asset_index) {
            double epsilon = idio_noise(random_generator);
            asset_returns(asset_index, time_step) = beta_vector(asset_index) * factor_change + epsilon;
        }
    }

    return std::make_tuple(asset_returns, cumulative_factor_levels);
}

#ifdef STANDALONE_BUILD
#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: ./factor_model <simulation_duration> <factor_volatility> <num_assets> <random_seed>\n";
        return 1;
    }

    int simulation_duration = std::stoi(argv[1]);
    double factor_volatility = std::stod(argv[2]);
    int num_assets = std::stoi(argv[3]);
    unsigned int random_seed = std::stoul(argv[4]);

    Eigen::VectorXd beta_vector = Eigen::VectorXd::Ones(num_assets); // Can customize this

    Eigen::MatrixXd asset_returns;
    Eigen::VectorXd cumulative_factor_levels;

    std::tie(asset_returns, cumulative_factor_levels) = simulate_factor_model(
        simulation_duration,
        factor_volatility,
        num_assets,
        beta_vector,
        random_seed
    );

    std::ofstream output_file("factor_output.csv");
    output_file << "time_step";
    for (int asset_index = 0; asset_index < num_assets; ++asset_index)
        output_file << ",asset_" << asset_index;
    output_file << ",cumulative_factor_level\n";

    for (int time_step = 0; time_step < simulation_duration; ++time_step) {
        output_file << time_step;
        for (int asset_index = 0; asset_index < num_assets; ++asset_index)
            output_file << "," << asset_returns(asset_index, time_step);
        output_file << "," << cumulative_factor_levels(time_step) << "\n";
    }

    output_file.close();
    return 0;
}
#endif
