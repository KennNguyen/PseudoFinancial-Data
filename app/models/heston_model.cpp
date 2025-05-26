#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <random>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <tuple>
#include <Eigen/Dense>


std::tuple<Eigen::VectorXd, Eigen::VectorXd> simulate_heston_path(
    double initial_asset_price,
    double initial_asset_variance,
    double mean_reversion_speed,
    double long_term_variance,
    double volatility_of_variance,
    double price_variance_correlation,
    double time_step_size,
    const Eigen::MatrixXd& factor_return_increments,
    const Eigen::VectorXd& factor_exposures,
    double idiosyncratic_volatility
) {
    /* 
    Kinda obvious on why you wouldn't on why you shouldn't have a negative or zero time step size,
    but just in case it needs to be spelled out, a negative or zero time step size has no meaning in simulation, time must move forward.
    */
    if (time_step_size <= 0)
        throw std::invalid_argument("time_step_size must be positive");
    /*
    Negative mean-reversion speed would cause the a divergence away from the long-term mean, which would break the intended behaviour of the mean reversion. 
    Additionally, Feller condition requires mean-reversion speed be positive for mean-reversion, so that variance remains well-behaved.
    For reference, I reviewed Steven L. Heston's (1993 - A Closed-Form Solution for Options with Stochastic Volatility with Applications to Bond and Currency Options).
    Also, the Cox-Ingersoll-Ross process forms the mathematical foundation of the variance dynamics used in the Heston model, 
    and if mean-reversion speed was negative, then the variance update would be imaginary or complex-valied due to the square root of a negative number.
    */
    if (mean_reversion_speed < 0)
        throw std::invalid_argument("mean_reversion_speed must be non-negative");
    /*
    Variance can't be negative and must be positive by definition, as its a second moment, and its positive in any stochastic model. 
    */
    if (long_term_variance < 0)
        throw std::invalid_argument("long_term_variance must be non-negative");
    /*
    Standard deviation can't be negative and must be positive, because otherwise you would be modelling noise with an imaginary component.
    */
    if (volatility_of_variance < 0)
        throw std::invalid_argument("volatility_of_variance must be non-negative");
    /*
    Correlation must be in [-1, 1] by definition of the Pearson correlation. 
    For reference, this is to ensure that linear dependence between stochastic processes is valid to preserve the consistency of the Brownian motion construction,
    and the positive and semi-definiteness of the covariance matrix.
    */
    if (price_variance_correlation < -1 || price_variance_correlation > 1)
        throw std::invalid_argument("price_variance_correlation must be between -1 and 1");
    /*
    Idiosyncratic volatility is the standard deviation of the asset-specific noise,
    so it can't be negative and must be positive. Otherwise, you'd be modeling noise with an imaginary component.
    */
    if (idiosyncratic_volatility < 0)
        throw std::invalid_argument("idiosyncratic_volatility must be non-negative");
    if (factor_exposures.size() != factor_return_increments.cols())
        throw std::invalid_argument("Size of factor_exposures must match number of factor columns");

    // Memory Allocation for Price and Variance Paths
    int total_time_steps = factor_return_increments.rows();
    Eigen::VectorXd simulated_prices(total_time_steps + 1);
    Eigen::VectorXd simulated_variances(total_time_steps + 1);
    simulated_prices(0) = initial_asset_price;
    simulated_variances(0) = initial_asset_variance;

    double sqrt_time_step = std::sqrt(time_step_size);

    // TLDR: Mersenne Twister exhibits pattern, so either larger size or switch random generator
    std::mt19937 random_generator(std::random_device{}());
    std::normal_distribution<> standard_normal(0.0, 1.0);

    Eigen::VectorXd random_price_increments(total_time_steps);
    Eigen::VectorXd random_variance_increments(total_time_steps);
    for (int time_step = 0; time_step < total_time_steps; ++time_step) {
        random_price_increments(time_step) = standard_normal(random_generator);
        random_variance_increments(time_step) = standard_normal(random_generator);
    }

    // Heston Dynamics Simulation
    for (int time_step = 0; time_step < total_time_steps; time_step++) {
        Eigen::VectorXd current_factor_increments = factor_return_increments.row(time_step).transpose();
        double factor_drift_contribution = factor_exposures.dot(current_factor_increments);
        double idiosyncratic_shock = idiosyncratic_volatility * standard_normal(random_generator);

        double current_price = simulated_prices(time_step);
        double current_variance = std::max(simulated_variances(time_step), 1e-12); // Ensure variance > 0

        // Brownian Motion components
        double variance_brownian_increment = random_variance_increments(time_step);
        double uncorrelated_price_noise = random_price_increments(time_step);
        double price_brownian_increment = price_variance_correlation * variance_brownian_increment
                                        + std::sqrt(1 - price_variance_correlation * price_variance_correlation) * uncorrelated_price_noise;

        double updated_variance = current_variance
            + mean_reversion_speed * (long_term_variance - current_variance) * time_step_size
            + volatility_of_variance * std::sqrt(current_variance) * variance_brownian_increment * sqrt_time_step;
        // This to prevent any negative variance
        updated_variance = std::max(updated_variance, 1e-12);

        double drift_term = (factor_drift_contribution - 0.5 * current_variance) * time_step_size;
        double diffusion_term = std::sqrt(current_variance) * price_brownian_increment * sqrt_time_step;
        double updated_price = current_price * std::exp(drift_term + diffusion_term);
        // Possible Fix for Asset Price Plateau: Clamp To Ensure Variance Doesn't Explode Or Dampen ie Log 0
        updated_price = std::max(updated_price, 1e-8);

        simulated_prices(time_step + 1) = updated_price;
        simulated_variances(time_step + 1) = updated_variance;
    }

    return std::make_tuple(simulated_prices, simulated_variances);
}

// CSV-Capable Function
#ifdef STANDALONE_BUILD
int main(int argc, char* argv[]) {
    if (argc < 11) {
        std::cerr << "Usage: ./heston_model <initial_price> <initial_variance> <mean_reversion_speed> <long_term_variance> <volatility_of_variance> <correlation> <time_step_size> <idiosyncratic_volatility> <simulation_duration> <factor_exposures...>\n";
        return 1;
    }

    double initial_asset_price = std::stod(argv[1]);
    double initial_asset_variance = std::stod(argv[2]);
    double mean_reversion_speed = std::stod(argv[3]);
    double long_term_variance = std::stod(argv[4]);
    double volatility_of_variance = std::stod(argv[5]);
    double price_variance_correlation = std::stod(argv[6]);
    double time_step_size = std::stod(argv[7]);
    double idiosyncratic_volatility = std::stod(argv[8]);
    int simulation_duration = std::stoi(argv[9]);

    int number_of_factors = argc - 10;
    Eigen::VectorXd factor_exposures(number_of_factors);
    for (int exposure_index = 0; exposure_index < number_of_factors; ++exposure_index) {
        factor_exposures(exposure_index) = std::stod(argv[10 + exposure_index]);
    }

    // Read factor model output and extract last column (factor_level)
    std::ifstream input_file("factor_output.csv");
    if (!input_file.is_open()) {
        std::cerr << "Could not open factor_output.csv\n";
        return 1;
    }

    Eigen::MatrixXd factor_return_increments(simulation_duration, 1);
    std::string current_line;
    std::getline(input_file, current_line); // skip header

    int time_step = 0;
    while (std::getline(input_file, current_line) && time_step < simulation_duration) {
        std::stringstream line_stream(current_line);
        std::string column_value;
        std::vector<std::string> columns;

        while (std::getline(line_stream, column_value, ',')) {
            columns.push_back(column_value);
        }

        if (columns.size() < 3) {
            std::cerr << "Malformed CSV row at time step " << time_step << "\n";
            return 1;
        }

        // Use last column as factor level input
        double factor_increment = std::stod(columns.back());
        factor_return_increments(time_step, 0) = factor_increment;
        ++time_step;
    }
    input_file.close();

    // Simulate Heston dynamics
    Eigen::VectorXd simulated_prices, simulated_variances;
    std::tie(simulated_prices, simulated_variances) = simulate_heston_path(
        initial_asset_price,
        initial_asset_variance,
        mean_reversion_speed,
        long_term_variance,
        volatility_of_variance,
        price_variance_correlation,
        time_step_size,
        factor_return_increments,
        factor_exposures,
        idiosyncratic_volatility
    );

    // Output results
    std::ofstream output_file("heston_output.csv");
    output_file << "time_step,price,variance\n";
    for (int time_step_index = 0; time_step_index <= simulation_duration; ++time_step_index) {
        output_file << time_step_index << "," << simulated_prices(time_step_index) << "," << simulated_variances(time_step_index) << "\n";
    }
    output_file.close();

    return 0;
}
#endif