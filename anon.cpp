#include "anon_core.hpp"
#include "display_system.hpp"
#include "mesh_network.hpp"
#include "handshake_processor.hpp"
#include "personality_module.hpp"
#include <signal.h>
#include <thread>
#include <iostream>

// Global instance for signal handling
std::unique_ptr<anon::AnonCore> g_anon;
bool g_running = true;

void signal_handler(int signum) {
    std::cout << "\nShutting down Anon..." << std::endl;
    g_running = false;
}

int main() {
    try {
        // Set up signal handling
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);

        // Initialize core components
        g_anon = std::make_unique<anon::AnonCore>();
        auto display = std::make_unique<anon::DisplaySystem>();
        auto mesh = std::make_unique<anon::MeshNetwork>();
        auto processor = std::make_unique<anon::HandshakeProcessor>();
        auto personality = std::make_unique<anon::PersonalityModule>();

        // Start mesh networking in background
        std::thread mesh_thread([&]() {
            mesh->start();
        });

        // Start handshake processing in background
        std::thread processor_thread([&]() {
            processor->start();
        });

        // Start personality module
        personality->bind_to_core(g_anon.get());

        // Main loop
        while (g_running) {
            // Update AI state
            g_anon->update();
            
            // Update display
            display->update(g_anon->get_status());
            
            // Process personality
            personality->process_events();
            
            // Share data with mesh network
            if (g_anon->has_new_data()) {
                mesh->broadcast_data(g_anon->get_shared_data());
            }
            
            // Process received mesh data
            while (mesh->has_pending_data()) {
                g_anon->process_mesh_data(mesh->get_next_data());
            }
            
            // Sleep based on stealth settings
            std::this_thread::sleep_for(
                std::chrono::milliseconds(g_anon->get_update_interval())
            );
        }

        // Clean shutdown
        mesh->stop();
        processor->stop();
        mesh_thread.join();
        processor_thread.join();
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
