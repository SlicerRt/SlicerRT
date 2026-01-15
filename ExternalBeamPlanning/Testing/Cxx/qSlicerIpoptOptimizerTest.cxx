#include "../../Widgets/qSlicerIpoptOptimizer.h"

#include <QCoreApplication>
#include <QDebug>
#include <iostream>
#include <cmath>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    std::cout << "Testing qSlicerIpoptOptimizer..." << std::endl;

    // Create optimizer
    qSlicerIpoptOptimizer optimizer;

    // Test 1: Simple quadratic function - minimize (x-2)^2 + (y-1)^2
    std::cout << "\n=== Test 1: Simple Quadratic Optimization ===" << std::endl;

    auto objective = [](const qSlicerIpoptOptimizer::Array& x) -> double {
        return (x[0] - 2.0) * (x[0] - 2.0) + (x[1] - 1.0) * (x[1] - 1.0);
    };

    auto gradient = [](const qSlicerIpoptOptimizer::Array& x) -> qSlicerIpoptOptimizer::Array {
        qSlicerIpoptOptimizer::Array grad(2);
        grad[0] = 2.0 * (x[0] - 2.0);  // d/dx
        grad[1] = 2.0 * (x[1] - 1.0);  // d/dy
        return grad;
    };

    optimizer.setObjectiveFunction(objective);
    optimizer.setGradientFunction(gradient);

    // Set stricter tolerance for testing
    optimizer.setAbsoluteObjectiveTolerance(1e-8);
    optimizer.setMaxIterations(100);

    // Starting point
    qSlicerIpoptOptimizer::Array x0 = {0.0, 0.0};

    // Solve
    auto result = optimizer.solveProblem(x0);

    // Check results
    if (result.success) {
        std::cout << "✓ Optimization succeeded!" << std::endl;
        std::cout << "  Solution: [" << result.solution[0] << ", " << result.solution[1] << "]" << std::endl;
        std::cout << "  Expected: [2.0, 1.0]" << std::endl;
        std::cout << "  Final objective: " << result.final_objective_value << std::endl;
        std::cout << "  Iterations: " << result.iteration_count << std::endl;

        // Verify solution accuracy
        double error_x = std::abs(result.solution[0] - 2.0);
        double error_y = std::abs(result.solution[1] - 1.0);
        double max_error = std::max(error_x, error_y);

        if (max_error < 1e-6) {
            std::cout << "✓ Solution accuracy test passed (error: " << max_error << ")" << std::endl;
        } else {
            std::cout << "✗ Solution accuracy test failed (error: " << max_error << ")" << std::endl;
            return 1;
        }
    } else {
        std::cout << "✗ Optimization failed with status: " << result.status << std::endl;
        return 1;
    }

    // Test 2: Rosenbrock function - minimize (1-x)^2 + 100*(y-x^2)^2
    std::cout << "\n=== Test 2: Rosenbrock Function ===" << std::endl;

    auto rosenbrock_obj = [](const qSlicerIpoptOptimizer::Array& x) -> double {
        return (1.0 - x[0]) * (1.0 - x[0]) + 100.0 * (x[1] - x[0]*x[0]) * (x[1] - x[0]*x[0]);
    };

    auto rosenbrock_grad = [](const qSlicerIpoptOptimizer::Array& x) -> qSlicerIpoptOptimizer::Array {
        qSlicerIpoptOptimizer::Array grad(2);
        grad[0] = -2.0 * (1.0 - x[0]) - 400.0 * x[0] * (x[1] - x[0]*x[0]);
        grad[1] = 200.0 * (x[1] - x[0]*x[0]);
        return grad;
    };

    optimizer.setObjectiveFunction(rosenbrock_obj);
    optimizer.setGradientFunction(rosenbrock_grad);
    optimizer.setMaxIterations(1000);

    // Starting point for Rosenbrock
    qSlicerIpoptOptimizer::Array x0_rb = {-1.2, 1.0};

    auto result_rb = optimizer.solveProblem(x0_rb);

    if (result_rb.success) {
        std::cout << "✓ Rosenbrock optimization succeeded!" << std::endl;
        std::cout << "  Solution: [" << result_rb.solution[0] << ", " << result_rb.solution[1] << "]" << std::endl;
        std::cout << "  Expected: [1.0, 1.0]" << std::endl;
        std::cout << "  Final objective: " << result_rb.final_objective_value << std::endl;
        std::cout << "  Iterations: " << result_rb.iteration_count << std::endl;

        // Verify solution (Rosenbrock is harder, so allow larger tolerance)
        double error_x = std::abs(result_rb.solution[0] - 1.0);
        double error_y = std::abs(result_rb.solution[1] - 1.0);
        double max_error = std::max(error_x, error_y);

        if (max_error < 1e-3) {
            std::cout << "✓ Rosenbrock accuracy test passed (error: " << max_error << ")" << std::endl;
        } else {
            std::cout << "✗ Rosenbrock accuracy test failed (error: " << max_error << ")" << std::endl;
            return 1;
        }
    } else {
        std::cout << "✗ Rosenbrock optimization failed with status: " << result_rb.status << std::endl;
        return 1;
    }

    std::cout << "\n✓ All tests passed successfully!" << std::endl;
    return 0;
}
