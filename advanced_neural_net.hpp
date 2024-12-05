#pragma once

#include <vector>
#include <memory>
#include <random>
#include <cmath>
#include <algorithm>
#include <string>
#include <map>
#include <functional>
#include <variant>
#include <nlohmann/json.hpp>

namespace ann {

// Advanced activation functions
struct ActivationFunctions {
    static double swish(double x) { return x / (1.0 + std::exp(-x)); }
    static double swish_derivative(double x) {
        double s = swish(x);
        return s + (1.0 - s) * (1.0 + std::exp(-x));
    }
    
    static double mish(double x) {
        return x * std::tanh(std::log(1.0 + std::exp(x)));
    }
    static double mish_derivative(double x) {
        double ex = std::exp(x);
        double omega = 1.0 + ex;
        double tanh_val = std::tanh(std::log(omega));
        return tanh_val + x * (1.0 - tanh_val * tanh_val) * (ex / omega);
    }
    
    static double gelu(double x) {
        return 0.5 * x * (1.0 + std::tanh(std::sqrt(2.0 / M_PI) * (x + 0.044715 * std::pow(x, 3))));
    }
    static double gelu_derivative(double x) {
        double cdf = 0.5 * (1.0 + std::tanh(std::sqrt(2.0 / M_PI) * (x + 0.044715 * std::pow(x, 3))));
        double pdf = std::exp(-0.5 * x * x) / std::sqrt(2.0 * M_PI);
        return cdf + x * pdf;
    }
};

// Advanced layer types
class Layer {
public:
    virtual ~Layer() = default;
    virtual std::vector<double> forward(const std::vector<double>& input) = 0;
    virtual std::vector<double> backward(const std::vector<double>& gradient) = 0;
    virtual void update(double learning_rate) = 0;
    virtual nlohmann::json to_json() const = 0;
    virtual void from_json(const nlohmann::json& j) = 0;
};

// Transformer attention mechanism
class MultiHeadAttention : public Layer {
private:
    size_t num_heads;
    size_t head_dim;
    std::vector<std::vector<double>> query_weights;
    std::vector<std::vector<double>> key_weights;
    std::vector<std::vector<double>> value_weights;
    std::vector<std::vector<double>> output_weights;
    
    std::vector<std::vector<double>> query_grad;
    std::vector<std::vector<double>> key_grad;
    std::vector<std::vector<double>> value_grad;
    std::vector<std::vector<double>> output_grad;

public:
    MultiHeadAttention(size_t input_dim, size_t num_heads) 
        : num_heads(num_heads), head_dim(input_dim / num_heads) {
        // Initialize weights
        auto init_weights = [this, input_dim](std::vector<std::vector<double>>& w) {
            w.resize(input_dim, std::vector<double>(input_dim));
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<> d(0, std::sqrt(2.0 / input_dim));
            for (auto& row : w) {
                for (auto& val : row) {
                    val = d(gen);
                }
            }
        };

        init_weights(query_weights);
        init_weights(key_weights);
        init_weights(value_weights);
        init_weights(output_weights);
    }

    std::vector<double> forward(const std::vector<double>& input) override {
        // Implement multi-head attention forward pass
        std::vector<double> output(input.size(), 0.0);
        // ... (implementation details)
        return output;
    }

    std::vector<double> backward(const std::vector<double>& gradient) override {
        // Implement multi-head attention backward pass
        std::vector<double> input_gradient(gradient.size(), 0.0);
        // ... (implementation details)
        return input_gradient;
    }

    void update(double learning_rate) override {
        // Update weights using gradients
        auto update_weights = [learning_rate](std::vector<std::vector<double>>& w,
                                           const std::vector<std::vector<double>>& g) {
            for (size_t i = 0; i < w.size(); ++i) {
                for (size_t j = 0; j < w[i].size(); ++j) {
                    w[i][j] -= learning_rate * g[i][j];
                }
            }
        };

        update_weights(query_weights, query_grad);
        update_weights(key_weights, key_grad);
        update_weights(value_weights, value_grad);
        update_weights(output_weights, output_grad);
    }

    nlohmann::json to_json() const override {
        return nlohmann::json{
            {"type", "multi_head_attention"},
            {"num_heads", num_heads},
            {"head_dim", head_dim},
            {"query_weights", query_weights},
            {"key_weights", key_weights},
            {"value_weights", value_weights},
            {"output_weights", output_weights}
        };
    }

    void from_json(const nlohmann::json& j) override {
        num_heads = j["num_heads"];
        head_dim = j["head_dim"];
        query_weights = j["query_weights"];
        key_weights = j["key_weights"];
        value_weights = j["value_weights"];
        output_weights = j["output_weights"];
    }
};

// Advanced residual block with skip connections
class ResidualBlock : public Layer {
private:
    std::vector<std::unique_ptr<Layer>> layers;
    std::vector<double> skip_connection;

public:
    void add_layer(std::unique_ptr<Layer> layer) {
        layers.push_back(std::move(layer));
    }

    std::vector<double> forward(const std::vector<double>& input) override {
        skip_connection = input;
        auto output = input;
        for (auto& layer : layers) {
            output = layer->forward(output);
        }
        // Add skip connection
        for (size_t i = 0; i < output.size(); ++i) {
            output[i] += skip_connection[i];
        }
        return output;
    }

    std::vector<double> backward(const std::vector<double>& gradient) override {
        auto current_gradient = gradient;
        for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
            current_gradient = (*it)->backward(current_gradient);
        }
        // Add skip connection gradient
        for (size_t i = 0; i < current_gradient.size(); ++i) {
            current_gradient[i] += gradient[i];
        }
        return current_gradient;
    }

    void update(double learning_rate) override {
        for (auto& layer : layers) {
            layer->update(learning_rate);
        }
    }

    nlohmann::json to_json() const override {
        nlohmann::json j;
        j["type"] = "residual_block";
        j["layers"] = nlohmann::json::array();
        for (const auto& layer : layers) {
            j["layers"].push_back(layer->to_json());
        }
        return j;
    }

    void from_json(const nlohmann::json& j) override {
        layers.clear();
        for (const auto& layer_json : j["layers"]) {
            // Create and add appropriate layer based on type
            // Implementation depends on layer factory
        }
    }
};

// Advanced neural network with sophisticated architecture
class AdvancedNeuralNetwork {
private:
    std::vector<std::unique_ptr<Layer>> layers;
    double learning_rate;
    double momentum;
    double dropout_rate;
    
    // Advanced features
    bool use_layer_normalization;
    bool use_residual_connections;
    bool use_attention_mechanism;

public:
    AdvancedNeuralNetwork(double lr = 0.001, double m = 0.9, double dropout = 0.2)
        : learning_rate(lr), momentum(m), dropout_rate(dropout),
          use_layer_normalization(true),
          use_residual_connections(true),
          use_attention_mechanism(true) {}

    void add_layer(std::unique_ptr<Layer> layer) {
        layers.push_back(std::move(layer));
    }

    std::vector<double> predict(const std::vector<double>& input) {
        auto current = input;
        for (auto& layer : layers) {
            current = layer->forward(current);
        }
        return current;
    }

    void train(const std::vector<std::vector<double>>& inputs,
              const std::vector<std::vector<double>>& targets,
              size_t epochs,
              size_t batch_size) {
        for (size_t epoch = 0; epoch < epochs; ++epoch) {
            double epoch_loss = 0.0;
            
            // Mini-batch training
            for (size_t i = 0; i < inputs.size(); i += batch_size) {
                size_t batch_end = std::min(i + batch_size, inputs.size());
                auto batch_loss = train_batch(
                    std::vector<std::vector<double>>(inputs.begin() + i, inputs.begin() + batch_end),
                    std::vector<std::vector<double>>(targets.begin() + i, targets.begin() + batch_end)
                );
                epoch_loss += batch_loss;
            }
            
            // Adaptive learning rate
            adjust_learning_rate(epoch, epoch_loss);
        }
    }

    double train_batch(const std::vector<std::vector<double>>& batch_inputs,
                      const std::vector<std::vector<double>>& batch_targets) {
        double batch_loss = 0.0;
        
        // Forward pass
        std::vector<std::vector<double>> predictions;
        for (const auto& input : batch_inputs) {
            predictions.push_back(predict(input));
        }
        
        // Backward pass
        for (size_t i = 0; i < batch_inputs.size(); ++i) {
            auto gradient = compute_loss_gradient(predictions[i], batch_targets[i]);
            for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
                gradient = (*it)->backward(gradient);
            }
            
            // Update weights
            for (auto& layer : layers) {
                layer->update(learning_rate);
            }
            
            batch_loss += compute_loss(predictions[i], batch_targets[i]);
        }
        
        return batch_loss / batch_inputs.size();
    }

    void save(const std::string& filename) {
        nlohmann::json j;
        j["learning_rate"] = learning_rate;
        j["momentum"] = momentum;
        j["dropout_rate"] = dropout_rate;
        j["use_layer_normalization"] = use_layer_normalization;
        j["use_residual_connections"] = use_residual_connections;
        j["use_attention_mechanism"] = use_attention_mechanism;
        
        j["layers"] = nlohmann::json::array();
        for (const auto& layer : layers) {
            j["layers"].push_back(layer->to_json());
        }
        
        std::ofstream file(filename);
        file << j.dump(4);
    }

    void load(const std::string& filename) {
        std::ifstream file(filename);
        nlohmann::json j;
        file >> j;
        
        learning_rate = j["learning_rate"];
        momentum = j["momentum"];
        dropout_rate = j["dropout_rate"];
        use_layer_normalization = j["use_layer_normalization"];
        use_residual_connections = j["use_residual_connections"];
        use_attention_mechanism = j["use_attention_mechanism"];
        
        layers.clear();
        for (const auto& layer_json : j["layers"]) {
            // Create and add appropriate layer based on type
            // Implementation depends on layer factory
        }
    }

private:
    void adjust_learning_rate(size_t epoch, double loss) {
        // Implement adaptive learning rate strategy
        static double prev_loss = std::numeric_limits<double>::max();
        if (loss > prev_loss) {
            learning_rate *= 0.95;  // Reduce learning rate if loss increases
        }
        prev_loss = loss;
    }

    std::vector<double> compute_loss_gradient(const std::vector<double>& prediction,
                                            const std::vector<double>& target) {
        std::vector<double> gradient(prediction.size());
        for (size_t i = 0; i < prediction.size(); ++i) {
            gradient[i] = 2 * (prediction[i] - target[i]);  // MSE gradient
        }
        return gradient;
    }

    double compute_loss(const std::vector<double>& prediction,
                       const std::vector<double>& target) {
        double loss = 0.0;
        for (size_t i = 0; i < prediction.size(); ++i) {
            double diff = prediction[i] - target[i];
            loss += diff * diff;
        }
        return loss / prediction.size();
    }
};

} // namespace ann
