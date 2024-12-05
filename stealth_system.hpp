#pragma once

#include <array>
#include <cstdint>
#include <algorithm>
#include <chrono>
#include <random>
#include <map>
#include <fstream>
#include "advanced_neural_net.hpp"

namespace stealth {

// Use fixed-point arithmetic for better performance on Pi Zero W
using fixed_point_t = int32_t;
constexpr int32_t FIXED_POINT_SCALE = 1000;

inline fixed_point_t to_fixed(double val) {
    return static_cast<fixed_point_t>(val * FIXED_POINT_SCALE);
}

inline double from_fixed(fixed_point_t val) {
    return static_cast<double>(val) / FIXED_POINT_SCALE;
}

// Reduced size signature profile
struct SignatureProfile {
    std::array<fixed_point_t, 5> timing_pattern;
    std::array<fixed_point_t, 5> power_levels;
    std::array<uint16_t, 5> frame_sizes;
    uint8_t protocol_ratios[3];  // management, control, data
    fixed_point_t entropy;
};

class LightweightStealthSystem {
private:
    // Simplified neural network - single layer for resource conservation
    std::unique_ptr<ann::AdvancedNeuralNetwork> behavior_model;
    
    // Stealth parameters using fixed-point arithmetic
    struct StealthParams {
        fixed_point_t timing_rand;
        fixed_point_t power_var;
        fixed_point_t frame_var;
        fixed_point_t protocol_mix;
        fixed_point_t burst_prob;
    } params;
    
    // Simplified traffic shaping
    struct TrafficShaper {
        uint16_t min_interval_ms;
        uint16_t max_interval_ms;
        std::array<uint16_t, 3> timing_windows;
    } shaper;
    
    // Efficient random number generation
    std::random_device rd;
    std::mt19937 gen;
    
    // Limited signature database
    std::array<SignatureProfile, 3> known_signatures;
    SignatureProfile current_profile;
    
    // Power management
    bool low_power_mode;
    uint32_t operation_count;
    static constexpr uint32_t POWER_CHECK_INTERVAL = 1000;

    fixed_point_t calculate_signature_similarity(const SignatureProfile& a,
                                              const SignatureProfile& b) {
        fixed_point_t similarity = 0;
        
        // Compare timing patterns
        for (size_t i = 0; i < a.timing_pattern.size(); ++i) {
            fixed_point_t diff = std::abs(a.timing_pattern[i] - b.timing_pattern[i]);
            similarity += FIXED_POINT_SCALE - (diff / 10);
        }
        
        // Compare power levels
        for (size_t i = 0; i < a.power_levels.size(); ++i) {
            fixed_point_t diff = std::abs(a.power_levels[i] - b.power_levels[i]);
            similarity += FIXED_POINT_SCALE - (diff / 5);
        }
        
        // Compare protocol ratios
        for (size_t i = 0; i < 3; ++i) {
            uint8_t diff = std::abs(a.protocol_ratios[i] - b.protocol_ratios[i]);
            similarity += to_fixed(1.0 - (diff / 255.0));
        }
        
        return similarity / to_fixed(3.0);
    }

public:
    LightweightStealthSystem() : 
        gen(rd()),
        low_power_mode(false),
        operation_count(0) {
        initialize_stealth_params();
        load_known_signatures();
    }
    
    void initialize_stealth_params() {
        params = {
            to_fixed(0.3),  // timing_rand
            to_fixed(0.2),  // power_var
            to_fixed(0.15), // frame_var
            to_fixed(0.25), // protocol_mix
            to_fixed(0.1)   // burst_prob
        };
        
        shaper = {
            100,  // min_interval_ms
            1000, // max_interval_ms
            {200, 500, 800} // timing_windows
        };
    }
    
    void load_known_signatures() {
        // Load minimal set of signatures
        SignatureProfile normal_traffic = {};
        for (size_t i = 0; i < 5; ++i) {
            normal_traffic.timing_pattern[i] = to_fixed(0.2);
            normal_traffic.power_levels[i] = to_fixed(-70.0);
            normal_traffic.frame_sizes[i] = 200 + i * 100;
        }
        normal_traffic.protocol_ratios[0] = 51;  // 20%
        normal_traffic.protocol_ratios[1] = 76;  // 30%
        normal_traffic.protocol_ratios[2] = 128; // 50%
        normal_traffic.entropy = to_fixed(0.7);
        
        known_signatures[0] = normal_traffic;
    }
    
    uint16_t get_next_timing_window() {
        if (low_power_mode) {
            return shaper.max_interval_ms;
        }
        
        std::uniform_int_distribution<uint16_t> dis(
            shaper.min_interval_ms,
            shaper.max_interval_ms
        );
        return dis(gen);
    }
    
    int8_t get_power_level(int8_t base_power) {
        if (low_power_mode) {
            return base_power;
        }
        
        std::uniform_int_distribution<int16_t> power_dis(-5, 5);
        return base_power + power_dis(gen);
    }
    
    uint16_t get_frame_size(uint16_t base_size) {
        if (low_power_mode) {
            return base_size;
        }
        
        std::uniform_int_distribution<uint16_t> size_dis(
            base_size / 2,
            base_size * 2
        );
        return size_dis(gen);
    }
    
    bool should_burst() {
        if (low_power_mode) return false;
        
        std::uniform_int_distribution<uint8_t> dis(0, 99);
        return dis(gen) < from_fixed(params.burst_prob) * 100;
    }
    
    void update_power_state() {
        operation_count++;
        if (operation_count >= POWER_CHECK_INTERVAL) {
            // Check battery level here
            // For now, just toggle low power mode based on operation count
            low_power_mode = !low_power_mode;
            operation_count = 0;
        }
    }
    
    void adapt_stealth_params(fixed_point_t detection_risk) {
        if (low_power_mode) return;
        
        if (detection_risk > to_fixed(0.7)) {
            params.timing_rand += to_fixed(0.1);
            params.power_var -= to_fixed(0.05);
            params.frame_var += to_fixed(0.05);
            params.protocol_mix += to_fixed(0.1);
            params.burst_prob -= to_fixed(0.05);
        } else if (detection_risk < to_fixed(0.3)) {
            params.timing_rand -= to_fixed(0.05);
            params.power_var += to_fixed(0.02);
            params.frame_var -= to_fixed(0.02);
            params.protocol_mix -= to_fixed(0.05);
            params.burst_prob += to_fixed(0.02);
        }
        
        // Clamp parameters
        auto clamp_fixed = [](fixed_point_t val, fixed_point_t min, fixed_point_t max) {
            return std::min(std::max(val, min), max);
        };
        
        params.timing_rand = clamp_fixed(params.timing_rand, to_fixed(0.1), to_fixed(0.9));
        params.power_var = clamp_fixed(params.power_var, to_fixed(0.05), to_fixed(0.4));
        params.frame_var = clamp_fixed(params.frame_var, to_fixed(0.05), to_fixed(0.3));
        params.protocol_mix = clamp_fixed(params.protocol_mix, to_fixed(0.1), to_fixed(0.5));
        params.burst_prob = clamp_fixed(params.burst_prob, to_fixed(0.01), to_fixed(0.2));
    }
    
    bool is_low_power_mode() const {
        return low_power_mode;
    }
};

} // namespace stealth
