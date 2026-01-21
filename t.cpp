#include <iostream> // Required for std::cout and std::endl

// Function imported from a.cpp
int oort(int x){
    return x*x;
}

// Function imported and converted from b.py
// Python's dynamic typing and automatic type promotion mean that
// x, y, z could be integers or floats. Since the sum includes 0.14 (a float),
// the result will always be a float. Using `double` for parameters and return type
// in C++ mirrors this behavior robustly, allowing for integer or floating-point inputs.
double zX6Ya(double x, double y, double z){
    return x + y + z + 0.14;
}

int main() {
    // Corresponds to PRINT(oort(2))
    std::cout << oort(2) << std::endl;

    // Corresponds to PRINT(zX6Ya(1,2,3))
    // C++ will implicitly convert the integer literals 1, 2, 3 to doubles
    // when passed to the zX6Ya function, matching Python's numeric behavior.
    std::cout << zX6Ya(1, 2, 3) << std::endl;

    return 0;
}
