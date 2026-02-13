# ml_utils.py
import numpy as np

def matrix_multiply(a, b):
    return np.dot(a, b)

class DataProcessor:
    def __init__(self, data):
        self.data = data
    
    def normalize(self):
        return (self.data - np.mean(self.data)) / np.std(self.data)