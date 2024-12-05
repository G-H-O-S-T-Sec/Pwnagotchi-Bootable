#pragma once

#include <vector>
#include <map>
#include <memory>
#include <random>
#include <algorithm>
#include <chrono>
#include "advanced_neural_net.hpp"
#include "network_intelligence.hpp"

namespace attack {

struct AttackVector {
    enum class Type {
        DEAUTH,
        PMKID,
        PASSIVE_MONITOR,
        CLIENT_PROBE,
        EVIL_TWIN,
        KARMA
    };

    Type type;
    double success_rate;
    double stealth_score;
    double energy_cost;
    std::chrono::milliseconds duration;
    std::vector<std::string> prerequisites;
    std::map<std::string, double> parameters;
};

class AttackOptimizer {
private:
    // Neural networks for attack optimization
    std::unique_ptr<ann::AdvancedNeuralNetwork> strategy_optimizer;
    std::unique_ptr<ann::AdvancedNeuralNetwork> timing_predictor;
    std::unique_ptr<ann::AdvancedNeuralNetwork> success_estimator;
    
    // Attack vectors and their effectiveness
    std::map<AttackVector::Type, std::vector<double>> attack_history;
    std::map<std::string, std::vector<AttackVector>> ap_specific_strategies;
    
    // Environmental factors
    struct Environment {
        double noise_level;
        double channel_utilization;
        double client_density;
        double interference_level;
        std::chrono::system_clock::time_point time_of_day;
    } env;
    
    // Reinforcement learning parameters
    struct RLParams {
        double learning_rate;
        double discount_factor;
        double exploration_rate;
        std::vector<double> q_table;
    } rl_params;
    
    // Advanced features
    bool adaptive_mode;
    double risk_threshold;
    double energy_budget;
    
    std::random_device rd;
    std::mt19937 gen;

    double calculate_attack_score(const AttackVector& attack, const net_intel::AccessPoint& target) {
        std::vector<double> features;
        
        // Attack features
        features.push_back(static_cast<double>(attack.type));
        features.push_back(attack.success_rate);
        features.push_back(attack.stealth_score);
        features.push_back(attack.energy_cost);
        
        // Target features
        features.push_back(target.vulnerability_score);
        features.push_back(target.rssi);
        features.push_back(target.entropy);
        
        // Environmental features
        features.push_back(env.noise_level);
        features.push_back(env.channel_utilization);
        features.push_back(env.client_density);
        
        // Get prediction
        auto prediction = strategy_optimizer->predict(features);
        return prediction[0];  // Overall attack score
    }
    
    AttackVector generate_evil_twin_attack(const net_intel::AccessPoint& target) {
        AttackVector attack;
        attack.type = AttackVector::Type::EVIL_TWIN;
        attack.prerequisites = {"hostapd", "dnsmasq"};
        
        // Configure attack parameters based on target
        attack.parameters["ssid"] = target.ssid;
        attack.parameters["channel"] = std::to_string(target.channel);
        attack.parameters["power"] = std::to_string(std::min(20, -target.rssi));  // Match power to avoid detection
        
        // Estimate success rate and stealth
        attack.success_rate = success_estimator->predict({
            static_cast<double>(target.clients.size()),
            target.vulnerability_score,
            env.client_density
        })[0];
        
        attack.stealth_score = 0.7;  // Evil twin attacks are moderately stealthy
        attack.energy_cost = 0.6;    // Moderate energy consumption
        
        return attack;
    }
    
    AttackVector generate_pmkid_attack(const net_intel::AccessPoint& target) {
        AttackVector attack;
        attack.type = AttackVector::Type::PMKID;
        attack.prerequisites = {"hcxdumptool"};
        
        // Configure for optimal PMKID capture
        attack.parameters["timeout"] = "30";
        attack.parameters["channel"] = std::to_string(target.channel);
        
        // Higher success rate for newer APs
        attack.success_rate = 0.8 * target.vulnerability_score;
        attack.stealth_score = 0.9;  // Very stealthy
        attack.energy_cost = 0.3;    // Low energy consumption
        
        return attack;
    }
    
    void update_environment() {
        // Update environmental factors based on observations
        env.time_of_day = std::chrono::system_clock::now();
        
        // Calculate noise level from recent measurements
        std::normal_distribution<> noise_dist(0.3, 0.1);
        env.noise_level = std::clamp(noise_dist(gen), 0.0, 1.0);
        
        // Update channel utilization
        std::normal_distribution<> util_dist(0.5, 0.2);
        env.channel_utilization = std::clamp(util_dist(gen), 0.0, 1.0);
    }
    
    void update_rl_parameters(const AttackVector& attack, double reward) {
        // Update Q-table using Q-learning
        size_t state_idx = static_cast<size_t>(attack.type);
        size_t action_idx = static_cast<size_t>(attack.type);
        
        double old_value = rl_params.q_table[state_idx * 6 + action_idx];
        double max_future_value = *std::max_element(
            rl_params.q_table.begin() + state_idx * 6,
            rl_params.q_table.begin() + (state_idx + 1) * 6
        );
        
        rl_params.q_table[state_idx * 6 + action_idx] = old_value + 
            rl_params.learning_rate * (reward + rl_params.discount_factor * max_future_value - old_value);
        
        // Decay exploration rate
        rl_params.exploration_rate *= 0.995;
    }

public:
    AttackOptimizer() 
        : gen(rd()),
          adaptive_mode(true),
          risk_threshold(0.7),
          energy_budget(1.0) {
        
        // Initialize neural networks
        strategy_optimizer = std::make_unique<ann::AdvancedNeuralNetwork>(0.001, 0.9, 0.1);
        timing_predictor = std::make_unique<ann::AdvancedNeuralNetwork>(0.001, 0.9, 0.1);
        success_estimator = std::make_unique<ann::AdvancedNeuralNetwork>(0.001, 0.9, 0.1);
        
        // Initialize RL parameters
        rl_params.learning_rate = 0.1;
        rl_params.discount_factor = 0.95;
        rl_params.exploration_rate = 0.1;
        rl_params.q_table.resize(36, 0.0);  // 6 states x 6 actions
        
        // Initialize environment
        update_environment();
    }
    
    std::vector<AttackVector> optimize_strategy(const net_intel::AccessPoint& target) {
        std::vector<AttackVector> strategy;
        
        // Check if we have historical data for this AP
        if (ap_specific_strategies.count(target.bssid)) {
            auto& history = ap_specific_strategies[target.bssid];
            // Use successful historical attacks as a base
            for (const auto& past_attack : history) {
                if (past_attack.success_rate > 0.7) {
                    strategy.push_back(past_attack);
                }
            }
        }
        
        // Generate new attack vectors
        std::vector<AttackVector> candidates;
        candidates.push_back(generate_evil_twin_attack(target));
        candidates.push_back(generate_pmkid_attack(target));
        
        // Score and filter candidates
        std::vector<std::pair<double, AttackVector>> scored_attacks;
        for (const auto& attack : candidates) {
            double score = calculate_attack_score(attack, target);
            if (score > risk_threshold) {
                scored_attacks.emplace_back(score, attack);
            }
        }
        
        // Sort by score
        std::sort(scored_attacks.begin(), scored_attacks.end(),
                 std::greater<std::pair<double, AttackVector>>());
        
        // Add top attacks to strategy
        for (const auto& [score, attack] : scored_attacks) {
            if (strategy.size() >= 3) break;  // Limit to top 3 attacks
            strategy.push_back(attack);
        }
        
        return strategy;
    }
    
    void update_strategy(const AttackVector& attack, bool success, 
                        const std::string& target_bssid) {
        // Update attack history
        attack_history[attack.type].push_back(success ? 1.0 : 0.0);
        
        // Update AP-specific strategies
        if (success) {
            ap_specific_strategies[target_bssid].push_back(attack);
        }
        
        // Calculate reward
        double reward = success ? 1.0 : -0.5;
        reward *= attack.stealth_score;  // Adjust reward based on stealth
        
        // Update RL parameters
        update_rl_parameters(attack, reward);
        
        // Update neural networks
        std::vector<std::vector<double>> inputs{{
            static_cast<double>(attack.type),
            attack.success_rate,
            attack.stealth_score,
            env.noise_level,
            env.channel_utilization
        }};
        std::vector<std::vector<double>> targets{{reward}};
        
        strategy_optimizer->train(inputs, targets, 1, 1);
        success_estimator->train(inputs, targets, 1, 1);
    }
    
    void set_risk_threshold(double threshold) {
        risk_threshold = std::clamp(threshold, 0.0, 1.0);
    }
    
    void set_energy_budget(double budget) {
        energy_budget = std::clamp(budget, 0.0, 1.0);
    }
    
    void enable_adaptive_mode(bool enable) {
        adaptive_mode = enable;
    }
    
    // Save and load models
    void save_models(const std::string& prefix) {
        strategy_optimizer->save(prefix + "_strategy.model");
        timing_predictor->save(prefix + "_timing.model");
        success_estimator->save(prefix + "_success.model");
        
        // Save Q-table and parameters
        nlohmann::json j;
        j["q_table"] = rl_params.q_table;
        j["learning_rate"] = rl_params.learning_rate;
        j["exploration_rate"] = rl_params.exploration_rate;
        
        std::ofstream file(prefix + "_rl.json");
        file << j.dump(4);
    }
    
    void load_models(const std::string& prefix) {
        strategy_optimizer->load(prefix + "_strategy.model");
        timing_predictor->load(prefix + "_timing.model");
        success_estimator->load(prefix + "_success.model");
        
        // Load Q-table and parameters
        std::ifstream file(prefix + "_rl.json");
        nlohmann::json j;
        file >> j;
        
        rl_params.q_table = j["q_table"].get<std::vector<double>>();
        rl_params.learning_rate = j["learning_rate"];
        rl_params.exploration_rate = j["exploration_rate"];
    }
};

} // namespace attack
