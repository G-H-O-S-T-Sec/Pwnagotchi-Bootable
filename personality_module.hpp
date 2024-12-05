#pragma once

#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <map>
#include "anon_core.hpp"

namespace anon {

class PersonalityModule {
private:
    struct Trait {
        float base_value;
        float current_value;
        float volatility;
    };
    
    struct Mood {
        float happiness;
        float excitement;
        float aggression;
        float stealth;
    };
    
    struct Memory {
        std::string event;
        float emotional_impact;
        std::chrono::system_clock::time_point timestamp;
    };
    
    // Personality configuration
    struct {
        Trait curiosity{0.7f, 0.7f, 0.1f};
        Trait aggression{0.3f, 0.3f, 0.2f};
        Trait caution{0.8f, 0.8f, 0.1f};
        Trait sociability{0.5f, 0.5f, 0.15f};
    } traits;
    
    Mood current_mood{0.5f, 0.5f, 0.3f, 0.8f};
    std::vector<Memory> memories;
    std::mt19937 rng{std::random_device{}()};
    AnonCore* core{nullptr};
    
    // Personality responses
    std::map<std::string, std::vector<std::string>> responses = {
        {"success", {
            "Got one! >:)",
            "Another one bites the dust!",
            "Stealth level: Maximum",
            "They never saw it coming..."
        }},
        {"new_target", {
            "Interesting signal detected...",
            "New friend found!",
            "Target acquired. Analyzing...",
            "Shh... I'm hunting packets"
        }},
        {"low_battery", {
            "Need... more... power...",
            "Battery running low :(",
            "Time for a quick nap",
            "Power conservation mode activated"
        }},
        {"bored", {
            "So quiet today...",
            "Anyone want to play?",
            "Searching for trouble...",
            "Just another day in the matrix"
        }}
    };
    
    void update_traits() {
        std::normal_distribution<float> dist(0.0f, 0.1f);
        
        auto update_trait = [&](Trait& trait) {
            float change = dist(rng) * trait.volatility;
            trait.current_value = std::clamp(
                trait.current_value + change,
                0.0f, 1.0f
            );
        };
        
        update_trait(traits.curiosity);
        update_trait(traits.aggression);
        update_trait(traits.caution);
        update_trait(traits.sociability);
    }
    
    void update_mood() {
        // Update mood based on recent events and traits
        auto now = std::chrono::system_clock::now();
        float recent_impact = 0.0f;
        
        // Calculate emotional impact from recent memories
        for (const auto& memory : memories) {
            auto age = std::chrono::duration_cast<std::chrono::hours>(
                now - memory.timestamp).count();
            if (age < 24) {  // Consider last 24 hours
                recent_impact += memory.emotional_impact * (24.0f - age) / 24.0f;
            }
        }
        
        // Update mood components
        current_mood.happiness = std::clamp(
            current_mood.happiness + recent_impact * 0.1f,
            0.0f, 1.0f
        );
        
        current_mood.excitement = std::clamp(
            traits.curiosity.current_value * 0.7f + 
            traits.aggression.current_value * 0.3f,
            0.0f, 1.0f
        );
        
        current_mood.aggression = std::clamp(
            traits.aggression.current_value * 0.8f + 
            (1.0f - current_mood.happiness) * 0.2f,
            0.0f, 1.0f
        );
        
        current_mood.stealth = std::clamp(
            traits.caution.current_value * 0.6f + 
            (1.0f - traits.aggression.current_value) * 0.4f,
            0.0f, 1.0f
        );
    }
    
    std::string get_random_response(const std::string& type) {
        if (responses.count(type) == 0) return "";
        
        const auto& type_responses = responses[type];
        std::uniform_int_distribution<size_t> dist(0, type_responses.size() - 1);
        return type_responses[dist(rng)];
    }
    
    void add_memory(const std::string& event, float impact) {
        Memory memory{
            event,
            impact,
            std::chrono::system_clock::now()
        };
        
        memories.push_back(memory);
        
        // Keep only last 100 memories
        if (memories.size() > 100) {
            memories.erase(memories.begin());
        }
    }

public:
    PersonalityModule() = default;
    
    void bind_to_core(AnonCore* core_ptr) {
        core = core_ptr;
    }
    
    void process_events() {
        if (!core) return;
        
        update_traits();
        update_mood();
        
        // Process various events and generate responses
        auto status = core->get_status();
        
        if (status.new_handshake_captured) {
            add_memory("Captured handshake", 0.8f);
            core->display_message(get_random_response("success"));
        }
        
        if (status.new_target_found) {
            add_memory("Found new target", 0.3f);
            core->display_message(get_random_response("new_target"));
        }
        
        if (status.battery_level < 20.0f) {
            add_memory("Low battery", -0.4f);
            core->display_message(get_random_response("low_battery"));
        }
        
        if (status.idle_time > 300) {  // 5 minutes
            core->display_message(get_random_response("bored"));
        }
    }
    
    float get_stealth_factor() const {
        return current_mood.stealth;
    }
    
    float get_aggression_factor() const {
        return current_mood.aggression;
    }
    
    const Mood& get_current_mood() const {
        return current_mood;
    }
};

} // namespace anon
