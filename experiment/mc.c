#include <stdio.h>   // Required for printf
#include <stdlib.h>  // Required for rand, srand, RAND_MAX
#include <time.h>    // Required for time (to seed rand) and clock (for timing)

/**
 * @brief Estimates the value of PI using the Monte Carlo simulation method.
 *
 * This function simulates dropping random points into a 2x2 square.
 * It counts how many points fall within a quarter circle inscribed in that square.
 * The ratio of points inside the circle to total points, scaled by 4,
 * approximates PI.
 *
 * @param samples The total number of random points to generate.
 * @return The estimated value of PI as a double-precision floating-point number.
 */
double estimate_pi(long long samples) {
    long long inside = 0; // Counter for points falling inside the quarter circle

    // Loop for the specified number of samples
    for (long long i = 0; i < samples; ++i) {
        // Generate two random double-precision floating-point numbers between 0.0 and 1.0
        // (double)rand() / RAND_MAX converts the integer result of rand()
        // to a double in the range [0.0, 1.0].
        double x = (double)rand() / RAND_MAX;
        double y = (double)rand() / RAND_MAX;

        // Check if the point (x, y) falls within the unit quarter circle.
        // The equation of a circle centered at (0,0) with radius 1 is x^2 + y^2 = 1.
        if (x * x + y * y <= 1.0) {
            inside++; // Increment counter if the point is inside or on the circle's boundary
        }
    }

    // Calculate PI approximation: 4 * (points_inside / total_samples)
    // The cast to (double)inside ensures floating-point division.
    return 4.0 * ((double)inside / samples);
}

int main() {
    // Seed the random number generator using the current time.
    // This ensures a different sequence of random numbers each time the program runs.
    srand((unsigned int)time(NULL));

    // Define the number of samples for the simulation.
    // Use LL suffix for long long literal.
    long long samples = 50000000LL; // 50 million samples

    // Record the starting time for performance measurement.
    // clock() returns the processor time used by the program.
    clock_t start_time = clock();

    // Call the function to estimate PI.
    double pi_estimate = estimate_pi(samples);

    // Record the ending time.
    clock_t end_time = clock();

    // Calculate the elapsed time in seconds.
    // CLOCKS_PER_SEC is a macro defined in <time.h> that converts clock ticks to seconds.
    double elapsed_time_seconds = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    // Print the estimated value of PI.
    // %lf is the format specifier for printing a double.
    printf("Estimated PI: %lf\n", pi_estimate);

    // Print the execution time, formatted to two decimal places.
    printf("Time: %.2f seconds\n", elapsed_time_seconds);

    return 0; // Indicate successful execution
}
