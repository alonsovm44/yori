#IMPLEMENT THIS ALGORITH LITERALLY 
#IMPLEMENT CALCULATION OF PI VIA MONTECARLO SIMULATION
import random
import time

def estimate_pi(samples: int) -> float:
    inside = 0

    for _ in range(samples):
        x = random.random()
        y = random.random()
        if x*x + y*y <= 1.0:
            inside += 1

    return 4.0 * inside / samples


if __name__ == "__main__":
    samples = 50_000_000  # 50 millones
    start = time.time()
    pi = estimate_pi(samples)
    end = time.time()

    print(f"Estimated PI: {pi}")
    print(f"Time: {end - start:.2f} seconds")
