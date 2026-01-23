-- program3.hs - Secuencia Evolutiva
module Evolutive where

evolutionRate :: Int -> Int
evolutionRate 0 = 1
evolutionRate 1 = 1
evolutionRate n = evolutionRate (n-1) + evolutionRate (n-2)

-- Una función simple para procesar listas de puntajes (fitness)
averageFitness :: [Float] -> Float
averageFitness xs = sum xs / fromIntegral (length xs)