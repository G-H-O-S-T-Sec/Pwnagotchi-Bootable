#include "neural_network.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>

// Helper function to save network outputs
void save_predictions(const std::string& filename, 
                     const std::vector<std::vector<double>>& predictions) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    for (const auto& pred : predictions) {
        for (size_t i = 0; i < pred.size(); ++i) {
            file << pred[i];
            if (i < pred.size() - 1) file << ",";
        }
        file << "\n";
    }
}

// Example usage and testing of the neural network
int main() {
    try {
        // Create a neural network for binary classification
        nn::NeuralNetwork network(nn::loss::cross_entropy, nn::loss::cross_entropy_derivative);

        // Add layers
        network.add_layer(std::make_unique<nn::DenseLayer>(4, 64, nn::activation::relu, nn::activation::relu_derivative));
        network.add_layer(std::make_unique<nn::BatchNormLayer>(64));
        network.add_layer(std::make_unique<nn::DropoutLayer>(0.3));
        network.add_layer(std::make_unique<nn::DenseLayer>(64, 32, nn::activation::relu, nn::activation::relu_derivative));
        network.add_layer(std::make_unique<nn::BatchNormLayer>(32));
        network.add_layer(std::make_unique<nn::DropoutLayer>(0.2));
        network.add_layer(std::make_unique<nn::DenseLayer>(32, 2, nn::activation::sigmoid, nn::activation::sigmoid_derivative));

        // Generate some example data (XOR-like problem with noise)
        std::vector<std::vector<double>> inputs = {
            {0, 0, 0, 0}, {0, 0, 1, 1}, {0, 1, 0, 1}, {0, 1, 1, 0},
            {1, 0, 0, 1}, {1, 0, 1, 0}, {1, 1, 0, 0}, {1, 1, 1, 1}
        };

        std::vector<std::vector<double>> targets = {
            {1, 0}, {0, 1}, {0, 1}, {1, 0},
            {0, 1}, {1, 0}, {1, 0}, {0, 1}
        };

        // Train the network
        std::cout << "Training neural network...\n";
        auto start_time = std::chrono::high_resolution_clock::now();

        network.train(inputs, targets, 1000, 0.001, 4);

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "Training completed in " << duration.count() << "ms\n\n";

        // Test the network
        std::cout << "Testing network predictions:\n";
        network.set_training(false);  // Switch to inference mode

        std::vector<std::vector<double>> predictions;
        for (const auto& input : inputs) {
            auto prediction = network.forward(input);
            predictions.push_back(prediction);
            
            std::cout << "Input: ";
            for (auto val : input) std::cout << val << " ";
            std::cout << "| Prediction: ";
            for (auto val : prediction) std::cout << std::fixed << std::setprecision(4) << val << " ";
            std::cout << "\n";
        }

        // Save predictions
        save_predictions("predictions.csv", predictions);
        std::cout << "\nPredictions saved to predictions.csv\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
