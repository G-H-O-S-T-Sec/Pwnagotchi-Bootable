#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <ctime>
#include <chrono>
#include <memory>
#include <random>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <array>
#include <algorithm>
#include <thread>
#include <mutex>
#include "neural_network.hpp"

// Forward declarations
class AccessPoint;
class HandshakeCapture;
class Channel;

// Constants for 32-bit optimization
constexpr uint32_t MAX_CHANNELS = 14;
constexpr uint32_t MAX_CLIENTS = 256;
constexpr uint32_t MAX_APS = 128;
constexpr uint32_t HASH_TABLE_SIZE = 1024;

// Structures optimized for 32-bit systems
struct MacAddress {
    std::array<uint8_t, 6> addr;
    
    std::string toString() const {
        std::stringstream ss;
        for (size_t i = 0; i < 6; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(addr[i]);
            if (i < 5) ss << ":";
        }
        return ss.str();
    }
};

struct NetworkStats {
    uint32_t deauths_sent{0};
    uint32_t associations_sent{0};
    uint32_t handshakes_captured{0};
    uint32_t aps_seen{0};
    uint32_t clients_seen{0};
    float success_rate{0.0f};
};

class AccessPoint {
public:
    MacAddress bssid;
    std::string ssid;
    uint8_t channel;
    int32_t rssi;
    bool has_handshake;
    std::vector<MacAddress> clients;
    std::chrono::system_clock::time_point last_seen;

    AccessPoint() : channel(0), rssi(0), has_handshake(false) {}
};

class PwnagotchiAI {
private:
    // Neural network for decision making
    std::unique_ptr<nn::NeuralNetwork> brain;
    
    // State management
    NetworkStats stats;
    std::map<MacAddress, AccessPoint> access_points;
    std::vector<HandshakeCapture> handshakes;
    uint8_t current_channel;
    bool is_stealthy;
    
    // Random number generation optimized for 32-bit
    std::mt19937 rng;
    
    // Mutex for thread safety
    std::mutex state_mutex;

    // AI State parameters
    float excitement;
    float boredom;
    float tiredness;

public:
    PwnagotchiAI() : current_channel(1), is_stealthy(true),
                     rng(std::random_device{}()),
                     excitement(0.5f), boredom(0.0f), tiredness(0.0f) {
        initializeNeuralNetwork();
    }

    void initializeNeuralNetwork() {
        brain = std::make_unique<nn::NeuralNetwork>(nn::loss::mse, nn::loss::mse_derivative);
        
        // Input layer: [num_aps, num_clients, channel_quality, battery_level, time_since_last_handshake]
        brain->add_layer(std::make_unique<nn::DenseLayer>(5, 32, nn::activation::relu, nn::activation::relu_derivative));
        brain->add_layer(std::make_unique<nn::BatchNormLayer>(32));
        brain->add_layer(std::make_unique<nn::DenseLayer>(32, 16, nn::activation::relu, nn::activation::relu_derivative));
        brain->add_layer(std::make_unique<nn::DenseLayer>(16, 4, nn::activation::sigmoid, nn::activation::sigmoid_derivative));
    }

    // Core functionality
    void updateState(const std::vector<AccessPoint>& new_aps) {
        std::lock_guard<std::mutex> lock(state_mutex);
        for (const auto& ap : new_aps) {
            access_points[ap.bssid] = ap;
        }
        stats.aps_seen = access_points.size();
    }

    std::vector<MacAddress> decideTargets() {
        std::lock_guard<std::mutex> lock(state_mutex);
        std::vector<MacAddress> targets;
        std::vector<double> input = {
            static_cast<double>(stats.aps_seen) / MAX_APS,
            static_cast<double>(stats.clients_seen) / MAX_CLIENTS,
            static_cast<double>(current_channel) / MAX_CHANNELS,
            1.0 - tiredness,  // battery level simulation
            excitement
        };

        auto decision = brain->forward(input);
        
        // Process neural network output
        if (decision[0] > 0.7) {  // Attack threshold
            // Select APs based on RSSI and client count
            for (const auto& [mac, ap] : access_points) {
                if (ap.rssi > -70 && !ap.has_handshake && !ap.clients.empty()) {
                    targets.push_back(mac);
                    if (targets.size() >= 3) break;  // Limit targets for efficiency
                }
            }
        }

        return targets;
    }

    void updateLearning(const NetworkStats& new_stats) {
        std::lock_guard<std::mutex> lock(state_mutex);
        
        // Calculate reward based on success
        float reward = (new_stats.handshakes_captured - stats.handshakes_captured) * 1.0f +
                      (new_stats.deauths_sent > 0 ? 0.1f : 0.0f);

        // Update emotional state
        excitement = std::min(1.0f, excitement + reward);
        boredom = std::max(0.0f, boredom + (reward > 0 ? -0.2f : 0.1f));
        tiredness = std::min(1.0f, tiredness + 0.01f);

        stats = new_stats;
    }

    // Channel management
    uint8_t selectNextChannel() {
        std::uniform_int_distribution<uint8_t> dist(1, MAX_CHANNELS);
        current_channel = dist(rng);
        return current_channel;
    }

    // Stealth mode operations
    void setStealthMode(bool stealth) {
        is_stealthy = stealth;
    }

    bool isStealthy() const {
        return is_stealthy;
    }

    // Status reporting
    std::string getStatus() const {
        std::stringstream ss;
        ss << "Pwnagotchi Status:\n"
           << "APs Seen: " << stats.aps_seen << "\n"
           << "Handshakes: " << stats.handshakes_captured << "\n"
           << "Success Rate: " << (stats.success_rate * 100) << "%\n"
           << "Excitement: " << (excitement * 100) << "%\n"
           << "Channel: " << static_cast<int>(current_channel);
        return ss.str();
    }

    // Save and load functions
    void saveState(const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (file.is_open()) {
            file.write(reinterpret_cast<char*>(&stats), sizeof(NetworkStats));
            file.write(reinterpret_cast<char*>(&current_channel), sizeof(uint8_t));
            file.write(reinterpret_cast<char*>(&excitement), sizeof(float));
            file.write(reinterpret_cast<char*>(&boredom), sizeof(float));
            file.write(reinterpret_cast<char*>(&tiredness), sizeof(float));
        }
    }

    void loadState(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (file.is_open()) {
            file.read(reinterpret_cast<char*>(&stats), sizeof(NetworkStats));
            file.read(reinterpret_cast<char*>(&current_channel), sizeof(uint8_t));
            file.read(reinterpret_cast<char*>(&excitement), sizeof(float));
            file.read(reinterpret_cast<char*>(&boredom), sizeof(float));
            file.read(reinterpret_cast<char*>(&tiredness), sizeof(float));
        }
    }
};
