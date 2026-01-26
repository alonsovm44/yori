import random
import math
import time

def calcular_pi_montecarlo(puntos_totales):
    """
    Estima el valor de Pi usando simulación de Montecarlo.
    Lanza 'puntos_totales' dardos aleatorios a un cuadrado de 2x2
    y cuenta cuántos caen dentro del círculo inscrito.
    """
    dentro_del_circulo = 0

    for _ in range(puntos_totales):
        # Generar coordenadas aleatorias entre -1 y 1
        x = random.uniform(-1, 1)
        y = random.uniform(-1, 1)

        # Distancia al cuadrado desde el origen (Teorema de Pitágoras)
        distancia = x*x + y*y

        # Si la distancia es menor a 1, está dentro del círculo
        if distancia <= 1.0:
            dentro_del_circulo += 1

    # Pi es 4 veces la probabilidad de caer dentro
    estimacion_pi = 4.0 * dentro_del_circulo / puntos_totales
    return estimacion_pi

if __name__ == "__main__":
    NUM_PUNTOS = 10_000_000 # 10 Millones de iteraciones
    
    print(f"Calculando Pi con {NUM_PUNTOS} puntos...")
    
    inicio = time.time()
    pi_approx = calcular_pi_montecarlo(NUM_PUNTOS)
    fin = time.time()
    
    print(f"Pi estimado: {pi_approx}")
    print(f"Pi real (math.pi): {math.pi}")
    print(f"Tiempo ejecución: {fin - inicio:.4f} segundos")