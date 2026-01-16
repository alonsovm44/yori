#include <iostream>
#include <cmath>

int main() {
    double number, secondNumber;
    
    // Input first number
    std::cout << "Enter the first number: ";
    std::cin >> number;

    // Input second number
    std::cout << "Enter the second number: ";
    std::cin >> secondNumber;

    // Perform operations
    double addition = number + secondNumber;
    double multiplication = number * secondNumber;
    double subtraction = number - secondNumber;
    double division;
    double powerAtoB, powerBtoA;
    long long factorialA = 1, factorialB = 1;

    if (secondNumber != 0) {
        division = number / secondNumber;
    } else {
        std::cout << "Warning: Division by zero is not allowed." << std::endl;
        division = NAN; // Not a Number to indicate invalid operation
    }

    powerAtoB = std::pow(number, secondNumber);
    powerBtoA = std::pow(secondNumber, number);

    for (int i = 1; i <= static_cast<int>(number); ++i) {
        factorialA *= i;
    }
    for (int i = 1; i <= static_cast<int>(secondNumber); ++i) {
        factorialB *= i;
    }

    // Output results
    std::cout << "Results:\n";
    std::cout << "1. Addition: " << addition << "\n";
    std::cout << "2. Multiplication: " << multiplication << "\n";
    std::cout << "3. Subtraction: " << subtraction << "\n";
    if (!std::isnan(division)) {
        std::cout << "4. Division: " << division << "\n";
    }
    std::cout << "5. A^B: " << powerAtoB << "\n";
    std::cout << "6. B^A: " << powerBtoA << "\n";
    std::cout << "7. Factorial of A: " << factorialA << "\n";
    std::cout << "8. Factorial of B: " << factorialB << "\n";

    // Wait for user to exit
    std::cin.ignore();
    std::cout << "Type any key to exit.";

    return 0;
}
