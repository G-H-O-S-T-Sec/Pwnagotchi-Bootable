#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <queue>
#include <filesystem>
#include "advanced_neural_net.hpp"
#include "network_intelligence.hpp"
#include "attack_optimizer.hpp"
#include "ai_communication.hpp"
#include "display_system.hpp"
#include "system_config.hpp"

namespace fs = std::filesystem;

class AdvancedPwnagotchi {
private:
    // Core components
    std::unique_ptr<net_intel::NetworkIntelligence> intelligence;
    std::unique_ptr<attack::AttackOptimizer> attack_optimizer;
    std::unique_ptr<ai_comm::AICommunication> ai_comm;
    std::unique_ptr<display::DisplaySystem> display;
    SystemConfig sys_config;
    
    // State management
    struct State {
        bool hunting_mode;
        bool stealth_mode;
        bool learning_mode;
        double energy_level;
        std::chrono::system_clock::time_point last_action;
        std::map<std::string, int> successful_handshakes;
    } state;
    
    // Threading
    std::atomic<bool> running{false};
    std::vector<std::thread> worker_threads;
    std::mutex state_mutex;
    
    // Advanced features
    struct AdvancedFeatures {
        bool adaptive_frequency_hopping;
        bool smart_power_management;
        bool enhanced_stealth;
        bool collaborative_learning;
    } features;
    
    // Performance metrics
    struct Metrics {
        uint64_t packets_processed;
        uint64_t handshakes_captured;
        uint64_t successful_attacks;
        double average_success_rate;
        std::chrono::milliseconds average_capture_time;
    } metrics;
    
    void intelligence_loop() {
        while (running) {
            try {
                process_network_data();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            } catch (const std::exception& e) {
                log_error("Intelligence loop error: " + std::string(e.what()));
            }
        }
    }
    
    void attack_loop() {
        while (running) {
            try {
                if (state.hunting_mode && state.energy_level > 0.2) {
                    execute_attack_strategy();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            } catch (const std::exception& e) {
                log_error("Attack loop error: " + std::string(e.what()));
            }
        }
    }
    
    void communication_loop() {
        while (running) {
            try {
                process_communications();
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            } catch (const std::exception& e) {
                log_error("Communication loop error: " + std::string(e.what()));
            }
        }
    }
    
    void process_network_data() {
        std::lock_guard<std::mutex> lock(state_mutex);
        
        // Get potential targets
        auto targets = intelligence->get_potential_targets();
        
        // Update display
        std::vector<display::NetworkMapWidget::NetworkNode> nodes;
        for (const auto& target : targets) {
            nodes.push_back({
                0, 0,  // Position will be calculated by force-directed layout
                target.bssid,
                target.ssid,
                target.rssi,
                target.is_target,
                target.clients
            });
        }
        display->update_network_map(nodes);
        
        // Update metrics
        metrics.packets_processed++;
    }
    
    void execute_attack_strategy() {
        std::lock_guard<std::mutex> lock(state_mutex);
        
        // Get targets from intelligence
        auto targets = intelligence->get_potential_targets();
        if (targets.empty()) return;
        
        // Get optimized attack strategy
        auto strategy = attack_optimizer->optimize_strategy(targets[0]);
        
        // Execute strategy
        for (const auto& attack : strategy) {
            if (!running || state.energy_level < 0.1) break;
            
            // Apply stealth modifications if needed
            if (state.stealth_mode) {
                modify_attack_for_stealth(attack);
            }
            
            // Execute attack
            bool success = execute_attack(attack, targets[0]);
            
            // Update metrics and learning
            if (success) {
                metrics.successful_attacks++;
                metrics.handshakes_captured++;
                state.successful_handshakes[targets[0].bssid]++;
            }
            
            // Update attack optimizer
            attack_optimizer->update_strategy(attack, success, targets[0].bssid);
            
            // Update energy level
            state.energy_level -= attack.energy_cost;
        }
    }
    
    void process_communications() {
        auto msg = ai_comm->receive_message();
        ai_comm->process_message(msg);
        
        // Update display with new status
        std::string status = generate_status_message();
        display->set_status(status);
    }
    
    void modify_attack_for_stealth(attack::AttackVector& attack) {
        // Implement sophisticated stealth modifications
        attack.parameters["power"] = std::to_string(
            std::min(10, std::stoi(attack.parameters["power"]))
        );
        attack.parameters["interval"] = std::to_string(
            std::max(500, std::stoi(attack.parameters["interval"]))
        );
    }
    
    bool execute_attack(const attack::AttackVector& attack,
                       const net_intel::AccessPoint& target) {
        // Implement actual attack execution
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Simulate attack success (replace with actual implementation)
        bool success = (std::rand() % 100) < (attack.success_rate * 100);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time
        );
        
        // Update metrics
        metrics.average_capture_time = std::chrono::milliseconds(
            (metrics.average_capture_time.count() * metrics.successful_attacks + 
             duration.count()) / (metrics.successful_attacks + 1)
        );
        
        return success;
    }
    
    std::string generate_status_message() {
        std::stringstream ss;
        ss << "Mode: " << (state.hunting_mode ? "Hunting" : "Passive") << "\n"
           << "Energy: " << static_cast<int>(state.energy_level * 100) << "%\n"
           << "Handshakes: " << metrics.handshakes_captured << "\n"
           << "Success Rate: " << static_cast<int>(metrics.average_success_rate * 100) << "%";
        return ss.str();
    }
    
    void log_error(const std::string& error) {
        // Implement error logging
        std::cerr << "Error: " << error << std::endl;
    }

public:
    AdvancedPwnagotchi()
        : state{true, false, true, 1.0,
                std::chrono::system_clock::now(),
                std::map<std::string, int>()},
          features{true, true, true, true},
          metrics{0, 0, 0, 0.0, std::chrono::milliseconds(0)} {
        
        // Initialize components
        intelligence = std::make_unique<net_intel::NetworkIntelligence>();
        attack_optimizer = std::make_unique<attack::AttackOptimizer>();
        ai_comm = std::make_unique<ai_comm::AICommunication>();
        
        // Initialize display
        display::DisplayMetrics metrics{800, 480, 96, 1.0, true, false};
        display::Theme theme{
            {0, 0, 0, 255},      // background
            {255, 255, 255, 255}, // text_primary
            {200, 200, 200, 255}, // text_secondary
            {0, 255, 0, 255},     // accent
            {255, 0, 0, 255},     // warning
            {0, 255, 0, 255},     // success
            10, 5,                // padding, margin
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            14                    // font_size
        };
        display = std::make_unique<display::DisplaySystem>(metrics, theme);
    }
    
    void start() {
        running = true;
        
        // Start display
        display->start();
        
        // Start worker threads
        worker_threads.emplace_back(&AdvancedPwnagotchi::intelligence_loop, this);
        worker_threads.emplace_back(&AdvancedPwnagotchi::attack_loop, this);
        worker_threads.emplace_back(&AdvancedPwnagotchi::communication_loop, this);
    }
    
    void stop() {
        running = false;
        
        // Stop display
        display->stop();
        
        // Join worker threads
        for (auto& thread : worker_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
        // Save state and models
        save_state();
    }
    
    void set_hunting_mode(bool enabled) {
        std::lock_guard<std::mutex> lock(state_mutex);
        state.hunting_mode = enabled;
    }
    
    void set_stealth_mode(bool enabled) {
        std::lock_guard<std::mutex> lock(state_mutex);
        state.stealth_mode = enabled;
        if (enabled) {
            attack_optimizer->set_risk_threshold(0.9);
        } else {
            attack_optimizer->set_risk_threshold(0.7);
        }
    }
    
    void save_state() {
        // Save neural network models
        intelligence->save_models(sys_config.getPaths().models / "intelligence");
        attack_optimizer->save_models(sys_config.getPaths().models / "attacks");
        ai_comm->save_models(sys_config.getPaths().models / "communication");
        
        // Save metrics and state
        nlohmann::json j;
        j["metrics"] = {
            {"packets_processed", metrics.packets_processed},
            {"handshakes_captured", metrics.handshakes_captured},
            {"successful_attacks", metrics.successful_attacks},
            {"average_success_rate", metrics.average_success_rate},
            {"average_capture_time", metrics.average_capture_time.count()}
        };
        j["state"] = {
            {"hunting_mode", state.hunting_mode},
            {"stealth_mode", state.stealth_mode},
            {"learning_mode", state.learning_mode},
            {"energy_level", state.energy_level},
            {"successful_handshakes", state.successful_handshakes}
        };
        
        std::ofstream file(sys_config.getPaths().models / "state.json");
        file << j.dump(4);
    }
    
    void load_state() {
        // Load neural network models
        intelligence->load_models(sys_config.getPaths().models / "intelligence");
        attack_optimizer->load_models(sys_config.getPaths().models / "attacks");
        ai_comm->load_models(sys_config.getPaths().models / "communication");
        
        // Load metrics and state
        std::ifstream file(sys_config.getPaths().models / "state.json");
        if (file) {
            nlohmann::json j;
            file >> j;
            
            metrics.packets_processed = j["metrics"]["packets_processed"];
            metrics.handshakes_captured = j["metrics"]["handshakes_captured"];
            metrics.successful_attacks = j["metrics"]["successful_attacks"];
            metrics.average_success_rate = j["metrics"]["average_success_rate"];
            metrics.average_capture_time = std::chrono::milliseconds(
                j["metrics"]["average_capture_time"].get<int64_t>()
            );
            
            state.hunting_mode = j["state"]["hunting_mode"];
            state.stealth_mode = j["state"]["stealth_mode"];
            state.learning_mode = j["state"]["learning_mode"];
            state.energy_level = j["state"]["energy_level"];
            state.successful_handshakes = j["state"]["successful_handshakes"]
                .get<std::map<std::string, int>>();
        }
    }
};
