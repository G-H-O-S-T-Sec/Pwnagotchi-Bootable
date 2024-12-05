#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <chrono>
#include <thread>
#include <mutex>
#include <functional>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

namespace display {

struct DisplayMetrics {
    int width;
    int height;
    int dpi;
    float scale_factor;
    bool is_hdmi;
    bool is_epaper;
};

struct Theme {
    SDL_Color background;
    SDL_Color text_primary;
    SDL_Color text_secondary;
    SDL_Color accent;
    SDL_Color warning;
    SDL_Color success;
    int padding;
    int margin;
    std::string font_path;
    int font_size;
};

class Widget {
public:
    virtual ~Widget() = default;
    virtual void render(SDL_Renderer* renderer) = 0;
    virtual void update() = 0;
    virtual bool handle_input(SDL_Event& event) = 0;
    
    SDL_Rect bounds;
    bool visible{true};
    bool enabled{true};
};

class StatusWidget : public Widget {
private:
    std::string status_text;
    SDL_Texture* texture;
    TTF_Font* font;
    SDL_Color color;

public:
    void set_status(const std::string& text) {
        status_text = text;
        update();
    }
    
    void render(SDL_Renderer* renderer) override {
        if (visible && texture) {
            SDL_RenderCopy(renderer, texture, nullptr, &bounds);
        }
    }
    
    void update() override {
        // Update texture with new status text
    }
    
    bool handle_input(SDL_Event& event) override {
        return false;  // Status widget doesn't handle input
    }
};

class NetworkMapWidget : public Widget {
private:
    struct NetworkNode {
        float x, y;
        std::string bssid;
        std::string ssid;
        int rssi;
        bool is_target;
        std::vector<std::string> connected_clients;
    };
    
    std::vector<NetworkNode> nodes;
    float zoom_level;
    SDL_Point pan_offset;
    bool dragging;

public:
    void update_network(const std::vector<NetworkNode>& new_nodes) {
        nodes = new_nodes;
        update();
    }
    
    void render(SDL_Renderer* renderer) override {
        if (!visible) return;
        
        // Draw connections between nodes
        for (const auto& node : nodes) {
            for (const auto& client : node.connected_clients) {
                // Draw connection lines
            }
        }
        
        // Draw nodes
        for (const auto& node : nodes) {
            // Draw node circles and labels
            SDL_Rect node_rect{
                static_cast<int>(node.x * zoom_level) + pan_offset.x,
                static_cast<int>(node.y * zoom_level) + pan_offset.y,
                10, 10
            };
            SDL_RenderFillRect(renderer, &node_rect);
        }
    }
    
    void update() override {
        // Update node positions using force-directed layout
        update_layout();
    }
    
    bool handle_input(SDL_Event& event) override {
        switch (event.type) {
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    dragging = true;
                    return true;
                }
                break;
                
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    dragging = false;
                    return true;
                }
                break;
                
            case SDL_MOUSEMOTION:
                if (dragging) {
                    pan_offset.x += event.motion.xrel;
                    pan_offset.y += event.motion.yrel;
                    return true;
                }
                break;
                
            case SDL_MOUSEWHEEL:
                zoom_level *= (1.0f + event.wheel.y * 0.1f);
                zoom_level = std::clamp(zoom_level, 0.1f, 5.0f);
                return true;
        }
        return false;
    }

private:
    void update_layout() {
        // Implement force-directed layout algorithm
        constexpr float k = 0.1f;  // Spring constant
        constexpr float repulsion = 100.0f;
        
        // Calculate forces and update positions
        for (auto& node : nodes) {
            float fx = 0, fy = 0;
            
            // Repulsion between nodes
            for (const auto& other : nodes) {
                if (&other != &node) {
                    float dx = node.x - other.x;
                    float dy = node.y - other.y;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist > 0) {
                        float force = repulsion / (dist * dist);
                        fx += force * dx / dist;
                        fy += force * dy / dist;
                    }
                }
            }
            
            // Attraction to connected nodes
            for (const auto& client : node.connected_clients) {
                auto it = std::find_if(nodes.begin(), nodes.end(),
                    [&client](const NetworkNode& n) { return n.bssid == client; });
                if (it != nodes.end()) {
                    float dx = it->x - node.x;
                    float dy = it->y - node.y;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    fx += k * dx;
                    fy += k * dy;
                }
            }
            
            // Update position
            node.x += fx * 0.1f;
            node.y += fy * 0.1f;
        }
    }
};

class DisplaySystem {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    DisplayMetrics metrics;
    Theme theme;
    
    std::vector<std::unique_ptr<Widget>> widgets;
    std::unique_ptr<StatusWidget> status_widget;
    std::unique_ptr<NetworkMapWidget> network_map;
    
    bool running;
    std::thread render_thread;
    std::mutex state_mutex;
    
    void render_loop() {
        while (running) {
            SDL_SetRenderDrawColor(renderer, 
                theme.background.r, theme.background.g, 
                theme.background.b, theme.background.a);
            SDL_RenderClear(renderer);
            
            // Render all widgets
            std::lock_guard<std::mutex> lock(state_mutex);
            for (auto& widget : widgets) {
                widget->render(renderer);
            }
            
            SDL_RenderPresent(renderer);
            
            // Frame rate control
            SDL_Delay(1000 / 60);  // 60 FPS
        }
    }
    
    void handle_events() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                return;
            }
            
            // Pass event to widgets
            std::lock_guard<std::mutex> lock(state_mutex);
            for (auto& widget : widgets) {
                if (widget->handle_input(event)) {
                    break;
                }
            }
        }
    }

public:
    DisplaySystem(const DisplayMetrics& metrics, const Theme& theme)
        : metrics(metrics), theme(theme), running(false) {
        
        SDL_Init(SDL_INIT_VIDEO);
        TTF_Init();
        
        window = SDL_CreateWindow(
            "Advanced Pwnagotchi",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            metrics.width, metrics.height,
            SDL_WINDOW_SHOWN | (metrics.is_hdmi ? SDL_WINDOW_RESIZABLE : 0)
        );
        
        renderer = SDL_CreateRenderer(
            window, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
        );
        
        // Create widgets
        status_widget = std::make_unique<StatusWidget>();
        network_map = std::make_unique<NetworkMapWidget>();
        
        widgets.push_back(std::move(status_widget));
        widgets.push_back(std::move(network_map));
    }
    
    ~DisplaySystem() {
        stop();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
    }
    
    void start() {
        running = true;
        render_thread = std::thread(&DisplaySystem::render_loop, this);
    }
    
    void stop() {
        running = false;
        if (render_thread.joinable()) {
            render_thread.join();
        }
    }
    
    void update() {
        handle_events();
        
        std::lock_guard<std::mutex> lock(state_mutex);
        for (auto& widget : widgets) {
            widget->update();
        }
    }
    
    void set_status(const std::string& status) {
        std::lock_guard<std::mutex> lock(state_mutex);
        if (status_widget) {
            status_widget->set_status(status);
        }
    }
    
    void update_network_map(const std::vector<NetworkMapWidget::NetworkNode>& nodes) {
        std::lock_guard<std::mutex> lock(state_mutex);
        if (network_map) {
            network_map->update_network(nodes);
        }
    }
};

} // namespace display
