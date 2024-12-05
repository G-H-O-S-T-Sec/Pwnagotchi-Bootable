#include "pwnagotchi.hpp"
#include "system_config.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>

// Global variables for graceful shutdown
volatile sig_atomic_t g_running = 1;

// Signal handler
void signal_handler(int signum) {
    g_running = 0;
}

class PwnagotchiSystem {
private:
    PwnagotchiAI ai;
    SystemConfig sys_config;
    std::unique_ptr<std::thread> display_thread;
    std::unique_ptr<std::thread> storage_thread;
    
    void displayLoop() {
        while (g_running) {
            if (sys_config.getDisplayConfig().enabled) {
                // Update display with AI status
                std::cout << ai.getStatus() << "\n";
            }
            // Respect display refresh rate
            std::this_thread::sleep_for(
                std::chrono::milliseconds(1000 / sys_config.getDisplayConfig().refresh_rate)
            );
        }
    }
    
    void storageLoop() {
        while (g_running) {
            // Check storage every minute
            sys_config.checkStorage();
            std::this_thread::sleep_for(std::chrono::minutes(1));
        }
    }

public:
    PwnagotchiSystem() {
        // Set up signal handlers
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
        
        // Initialize display
        sys_config.initializeDisplay();
        
        // Start monitoring threads
        display_thread = std::make_unique<std::thread>(&PwnagotchiSystem::displayLoop, this);
        storage_thread = std::make_unique<std::thread>(&PwnagotchiSystem::storageLoop, this);
    }
    
    ~PwnagotchiSystem() {
        g_running = 0;
        if (display_thread && display_thread->joinable()) {
            display_thread->join();
        }
        if (storage_thread && storage_thread->joinable()) {
            storage_thread->join();
        }
    }
    
    void run() {
        NetworkStats stats;
        std::vector<AccessPoint> discovered_aps;

        while (g_running) {
            // Check system resources
            if (sys_config.hasStorageWarning()) {
                std::cout << "Warning: Low storage space\n";
                sys_config.cleanupOldFiles();
            }

            // Update AI state
            ai.updateState(discovered_aps);
            
            // Get target decisions
            auto targets = ai.decideTargets();
            
            // Simulate attacks (replace with real implementation)
            if (!targets.empty()) {
                stats.deauths_sent += targets.size();
                stats.handshakes_captured += targets.size() / 2;
                stats.success_rate = static_cast<float>(stats.handshakes_captured) / stats.deauths_sent;
            }
            
            // Update AI learning
            ai.updateLearning(stats);
            
            // Channel hopping
            uint8_t new_channel = ai.selectNextChannel();
            
            // Save state periodically
            static int epoch = 0;
            if (++epoch % 5 == 0) {
                ai.saveState(sys_config.getPaths().models / "ai_state.bin");
            }
            
            // Sleep for a bit
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
};

int main() {
    try {
        PwnagotchiSystem system;
        std::cout << "Pwnagotchi started. Press Ctrl+C to exit.\n";
        system.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    std::cout << "Pwnagotchi shutting down...\n";
    return 0;
}
