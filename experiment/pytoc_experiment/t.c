#include <stdio.h> // Required for standard input/output functions like printf
#include <math.h>  // Required for mathematical functions like sqrt and pow
#include <stdlib.h> // Not strictly necessary for this specific problem with static arrays,
                    // but often useful in C programs (e.g., for malloc/free)

// --- Helper Functions for Data Processing ---

/**
 * @brief Calculates the arithmetic mean (average) of an array of double-precision floating-point numbers.
 * @param data A pointer to the array of numbers.
 * @param size The number of elements in the array.
 * @return The calculated mean, or 0.0 if the array is empty.
 */
double calculate_mean(const double* data, int size) {
    if (size == 0) {
        return 0.0;
    }
    double sum = 0.0;
    for (int i = 0; i < size; ++i) {
        sum += data[i];
    }
    return sum / size;
}

/**
 * @brief Calculates the population standard deviation of an array of double-precision floating-point numbers.
 *        Uses N in the denominator (population standard deviation), not N-1 (sample standard deviation).
 * @param data A pointer to the array of numbers.
 * @param size The number of elements in the array.
 * @param mean The pre-calculated mean of the data.
 * @return The calculated standard deviation, or 0.0 if the array has 0 or 1 elements.
 */
double calculate_std_dev(const double* data, int size, double mean) {
    if (size <= 1) {
        return 0.0; // Standard deviation is undefined or 0 for 0 or 1 elements
    }
    double sum_sq_diff = 0.0; // Sum of squared differences from the mean
    for (int i = 0; i < size; ++i) {
        sum_sq_diff += pow(data[i] - mean, 2);
    }
    return sqrt(sum_sq_diff / size);
}

/**
 * @brief Normalizes the input data using the Z-score normalization method: (x - mean) / std_dev.
 *        The results are stored in a separate output array.
 * @param input A pointer to the original input array of numbers.
 * @param output A pointer to the array where normalized data will be stored. Must be pre-allocated.
 * @param size The number of elements in the arrays.
 * @param mean The pre-calculated mean of the input data.
 * @param std_dev The pre-calculated standard deviation of the input data.
 */
void normalize_data(const double* input, double* output, int size, double mean, double std_dev) {
    if (std_dev == 0.0) {
        // If standard deviation is 0, all elements in the input are identical.
        // The normalized value for each element becomes 0.
        for (int i = 0; i < size; ++i) {
            output[i] = 0.0;
        }
    } else {
        for (int i = 0; i < size; ++i) {
            output[i] = (input[i] - mean) / std_dev;
        }
    }
}

/**
 * @brief Prints the elements of a double array to the console.
 * @param data A pointer to the array of numbers to print.
 * @param size The number of elements in the array.
 * @param label A string label to precede the printed array.
 */
void print_data(const double* data, int size, const char* label) {
    printf("%s: [", label);
    for (int i = 0; i < size; ++i) {
        printf("%.4f", data[i]); // Print with 4 decimal places for readability
        if (i < size - 1) {
            printf(", ");
        }
    }
    printf("]\n");
}

// --- Main Program Logic ---

int main() {
    // Create processor with data [1,2,3,4,5]
    // The instructions "INCLUDE x.py" and "IMPORT x.py" are not applicable to C
    // and are ignored as the task is to produce valid C code only.

    // Define the initial integer data set
    int initial_int_data[] = {1, 2, 3, 4, 5};
    int data_size = sizeof(initial_int_data) / sizeof(initial_int_data[0]);

    // Convert integer data to double-precision floats for accurate calculations
    double data[data_size];
    for (int i = 0; i < data_size; ++i) {
        data[i] = (double)initial_int_data[i];
    }

    // Print the original data
    print_data(data, data_size, "Original Data");

    // Calculate the mean of the data
    double mean = calculate_mean(data, data_size);
    printf("Calculated Mean: %.4f\n", mean);

    // Calculate the standard deviation of the data
    double std_dev = calculate_std_dev(data, data_size, mean);
    printf("Calculated Standard Deviation: %.4f\n", std_dev);

    // Create an array to store the normalized data
    double normalized_result[data_size];

    // Normalize the data
    normalize_data(data, normalized_result, data_size, mean, std_dev);

    // Print the result (normalized data)
    print_data(normalized_result, data_size, "Normalized Result");

    return 0; // Indicate successful execution
}
