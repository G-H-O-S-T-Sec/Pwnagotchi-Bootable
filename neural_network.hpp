#pragma once

#include <vector>
#include <memory>
#include <random>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <functional>
#include <iostream>
#include <numeric>

namespace nn {

// Forward declarations
class Layer;
class NeuralNetwork;

// Activation functions
namespace activation {
    inline double sigmoid(double x) { return 1.0 / (1.0 + std::exp(-x)); }
    inline double sigmoid_derivative(double x) { 
        double s = sigmoid(x);
        return s * (1.0 - s);
    }
    
    inline double tanh(double x) { return std::tanh(x); }
    inline double tanh_derivative(double x) {
        double t = tanh(x);
        return 1.0 - t * t;
    }
    
    inline double relu(double x) { return std::max(0.0, x); }
    inline double relu_derivative(double x) { return x > 0.0 ? 1.0 : 0.0; }
    
    inline double leaky_relu(double x) { return x > 0.0 ? x : 0.01 * x; }
    inline double leaky_relu_derivative(double x) { return x > 0.0 ? 1.0 : 0.01; }
}

// Loss functions
namespace loss {
    inline double mse(const std::vector<double>& predicted, const std::vector<double>& target) {
        double sum = 0.0;
        for (size_t i = 0; i < predicted.size(); ++i) {
            double diff = predicted[i] - target[i];
            sum += diff * diff;
        }
        return sum / predicted.size();
    }
    
    inline double mse_derivative(double predicted, double target) {
        return 2.0 * (predicted - target) / predicted;
    }
    
    inline double cross_entropy(const std::vector<double>& predicted, const std::vector<double>& target) {
        double sum = 0.0;
        for (size_t i = 0; i < predicted.size(); ++i) {
            sum += target[i] * std::log(predicted[i] + 1e-7);
        }
        return -sum;
    }
    
    inline double cross_entropy_derivative(double predicted, double target) {
        return -target / (predicted + 1e-7);
    }
}

// Base Layer class
class Layer {
protected:
    std::vector<double> output;
    std::vector<double> delta;
    
public:
    virtual ~Layer() = default;
    virtual void forward(const std::vector<double>& input) = 0;
    virtual void backward(const std::vector<double>& prev_delta) = 0;
    virtual void update(double learning_rate) = 0;
    virtual void init(std::mt19937& rng) = 0;
    
    const std::vector<double>& get_output() const { return output; }
    const std::vector<double>& get_delta() const { return delta; }
};

// Dense (Fully Connected) Layer
class DenseLayer : public Layer {
private:
    size_t input_size;
    size_t output_size;
    std::vector<std::vector<double>> weights;
    std::vector<double> biases;
    std::vector<double> input_cache;
    std::function<double(double)> activation_fn;
    std::function<double(double)> activation_derivative;

public:
    DenseLayer(size_t input_size, size_t output_size, 
               std::function<double(double)> activation = activation::sigmoid,
               std::function<double(double)> activation_deriv = activation::sigmoid_derivative)
        : input_size(input_size), output_size(output_size),
          weights(output_size, std::vector<double>(input_size)),
          biases(output_size),
          activation_fn(activation),
          activation_derivative(activation_deriv) {}

    void init(std::mt19937& rng) override {
        std::uniform_real_distribution<double> dist(-1.0, 1.0);
        for (auto& row : weights) {
            for (auto& weight : row) {
                weight = dist(rng) * std::sqrt(2.0 / input_size); // He initialization
            }
        }
        for (auto& bias : biases) {
            bias = 0.0; // Initialize biases to zero
        }
    }

    void forward(const std::vector<double>& input) override {
        input_cache = input;
        output.resize(output_size);
        
        for (size_t i = 0; i < output_size; ++i) {
            double sum = biases[i];
            for (size_t j = 0; j < input_size; ++j) {
                sum += input[j] * weights[i][j];
            }
            output[i] = activation_fn(sum);
        }
    }

    void backward(const std::vector<double>& prev_delta) override {
        delta.resize(output_size);
        for (size_t i = 0; i < output_size; ++i) {
            delta[i] = prev_delta[i] * activation_derivative(output[i]);
        }
    }

    void update(double learning_rate) override {
        for (size_t i = 0; i < output_size; ++i) {
            for (size_t j = 0; j < input_size; ++j) {
                weights[i][j] -= learning_rate * delta[i] * input_cache[j];
            }
            biases[i] -= learning_rate * delta[i];
        }
    }
};

// Dropout Layer
class DropoutLayer : public Layer {
private:
    double dropout_rate;
    std::vector<bool> mask;
    bool is_training;
    std::mt19937 rng;

public:
    DropoutLayer(double rate = 0.5) 
        : dropout_rate(rate), is_training(true), rng(std::random_device{}()) {}

    void init(std::mt19937& rng) override {
        // Nothing to initialize for dropout layer
    }

    void forward(const std::vector<double>& input) override {
        output = input;
        if (is_training) {
            std::bernoulli_distribution dist(1.0 - dropout_rate);
            mask.resize(input.size());
            for (size_t i = 0; i < input.size(); ++i) {
                mask[i] = dist(rng);
                output[i] *= mask[i] ? (1.0 / (1.0 - dropout_rate)) : 0.0;
            }
        }
    }

    void backward(const std::vector<double>& prev_delta) override {
        delta = prev_delta;
        if (is_training) {
            for (size_t i = 0; i < delta.size(); ++i) {
                delta[i] *= mask[i] ? (1.0 / (1.0 - dropout_rate)) : 0.0;
            }
        }
    }

    void update(double learning_rate) override {
        // Nothing to update for dropout layer
    }

    void set_training(bool training) { is_training = training; }
};

// BatchNorm Layer
class BatchNormLayer : public Layer {
private:
    double epsilon;
    double momentum;
    std::vector<double> gamma;
    std::vector<double> beta;
    std::vector<double> running_mean;
    std::vector<double> running_var;
    std::vector<double> input_cache;
    std::vector<double> normalized_cache;
    bool is_training;

public:
    BatchNormLayer(size_t size, double eps = 1e-5, double mom = 0.99)
        : epsilon(eps), momentum(mom), is_training(true) {
        gamma.resize(size, 1.0);
        beta.resize(size, 0.0);
        running_mean.resize(size, 0.0);
        running_var.resize(size, 1.0);
    }

    void init(std::mt19937& rng) override {
        // Nothing to initialize for batch norm
    }

    void forward(const std::vector<double>& input) override {
        input_cache = input;
        output.resize(input.size());
        normalized_cache.resize(input.size());

        if (is_training) {
            // Calculate mean and variance
            double mean = 0.0, var = 0.0;
            for (const auto& val : input) {
                mean += val;
            }
            mean /= input.size();

            for (const auto& val : input) {
                double diff = val - mean;
                var += diff * diff;
            }
            var /= input.size();

            // Update running statistics
            for (size_t i = 0; i < running_mean.size(); ++i) {
                running_mean[i] = momentum * running_mean[i] + (1.0 - momentum) * mean;
                running_var[i] = momentum * running_var[i] + (1.0 - momentum) * var;
            }

            // Normalize and scale
            for (size_t i = 0; i < input.size(); ++i) {
                normalized_cache[i] = (input[i] - mean) / std::sqrt(var + epsilon);
                output[i] = gamma[i] * normalized_cache[i] + beta[i];
            }
        } else {
            // Use running statistics for inference
            for (size_t i = 0; i < input.size(); ++i) {
                output[i] = gamma[i] * (input[i] - running_mean[i]) / 
                           std::sqrt(running_var[i] + epsilon) + beta[i];
            }
        }
    }

    void backward(const std::vector<double>& prev_delta) override {
        delta = prev_delta;
        // Implement backward pass for batch normalization
        // This is a simplified version
        for (size_t i = 0; i < delta.size(); ++i) {
            delta[i] *= gamma[i];
        }
    }

    void update(double learning_rate) override {
        // Update gamma and beta
        for (size_t i = 0; i < gamma.size(); ++i) {
            gamma[i] -= learning_rate * delta[i] * normalized_cache[i];
            beta[i] -= learning_rate * delta[i];
        }
    }

    void set_training(bool training) { is_training = training; }
};

// Neural Network class
class NeuralNetwork {
private:
    std::vector<std::unique_ptr<Layer>> layers;
    std::function<double(const std::vector<double>&, const std::vector<double>&)> loss_fn;
    std::function<double(double, double)> loss_derivative;
    std::mt19937 rng;

public:
    NeuralNetwork(std::function<double(const std::vector<double>&, const std::vector<double>&)> loss = loss::mse,
                 std::function<double(double, double)> loss_deriv = loss::mse_derivative)
        : loss_fn(loss), loss_derivative(loss_deriv), rng(std::random_device()()) {}

    void add_layer(std::unique_ptr<Layer> layer) {
        layer->init(rng);
        layers.push_back(std::move(layer));
    }

    std::vector<double> forward(const std::vector<double>& input) {
        std::vector<double> current = input;
        for (auto& layer : layers) {
            layer->forward(current);
            current = layer->get_output();
        }
        return current;
    }

    void backward(const std::vector<double>& target) {
        const auto& output = layers.back()->get_output();
        std::vector<double> delta(output.size());
        
        // Calculate initial delta using loss derivative
        for (size_t i = 0; i < output.size(); ++i) {
            delta[i] = loss_derivative(output[i], target[i]);
        }

        // Backpropagate through layers
        for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
            (*it)->backward(delta);
            delta = (*it)->get_delta();
        }
    }

    void update(double learning_rate) {
        for (auto& layer : layers) {
            layer->update(learning_rate);
        }
    }

    double get_loss(const std::vector<double>& predicted, const std::vector<double>& target) {
        return loss_fn(predicted, target);
    }

    void train(const std::vector<std::vector<double>>& inputs,
              const std::vector<std::vector<double>>& targets,
              size_t epochs,
              double learning_rate,
              size_t batch_size = 32) {
        
        if (inputs.size() != targets.size()) {
            throw std::runtime_error("Number of inputs must match number of targets");
        }

        for (size_t epoch = 0; epoch < epochs; ++epoch) {
            double total_loss = 0.0;
            
            // Create shuffled indices for mini-batch training
            std::vector<size_t> indices(inputs.size());
            std::iota(indices.begin(), indices.end(), 0);
            std::shuffle(indices.begin(), indices.end(), rng);

            // Mini-batch training
            for (size_t i = 0; i < inputs.size(); i += batch_size) {
                size_t batch_end = std::min(i + batch_size, inputs.size());
                
                // Process each sample in the mini-batch
                for (size_t j = i; j < batch_end; ++j) {
                    size_t idx = indices[j];
                    auto predicted = forward(inputs[idx]);
                    total_loss += get_loss(predicted, targets[idx]);
                    backward(targets[idx]);
                    update(learning_rate);
                }
            }

            // Calculate average loss for the epoch
            total_loss /= inputs.size();
            
            if ((epoch + 1) % 100 == 0) {
                std::cout << "Epoch " << (epoch + 1) << ", Loss: " << total_loss << std::endl;
            }
        }
    }

    void set_training(bool training) {
        for (auto& layer : layers) {
            if (auto dropout = dynamic_cast<DropoutLayer*>(layer.get())) {
                dropout->set_training(training);
            }
            if (auto batchnorm = dynamic_cast<BatchNormLayer*>(layer.get())) {
                batchnorm->set_training(training);
            }
        }
    }
};

} // namespace nn
