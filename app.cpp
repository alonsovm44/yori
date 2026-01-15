#include <iostream>
#include <string>

int main() {
    // Step 1: Pide nombre al usuario
    std::cout << "Ingresa tu nombre: ";
    std::string nombre;
    std::cin >> nombre;

    // Step 2: Imprime un saludo personalizado
    std::cout << "¡Hola, " << nombre << "!" << std::endl;

    // Step 3: Usa un bucle para imprimir una cuenta regresiva del 5 al 0
    for (int i = 5; i >= 0; --i) {
        std::cout << i << std::endl;
    }

    // Step 4: Imprime "Sistema en linea, yori funciona"
    std::cout << "Sistema en línea, Yori funciona" << std::endl;

    return 0;
}
