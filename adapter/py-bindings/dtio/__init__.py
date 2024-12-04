from time import time
import math
# import torch
# from torch.utils.data import Dataset, DataLoader, RandomSampler, SequentialSampler
# from torch.utils.data.sampler import Sampler
import numpy as np
import ctypes, ctypes.util
import pickle
import dtiopy

# class DTIOObject:
#     def __init__(self):
#         # dtio_lib = ctypes.util.find_library("dtio")
#         if dtio_lib == None:
#             raise RuntimeError("DTIO-Python: cannot find libdtio.so, check LD_LIBRARY_PATH")
#         self.lib = ctypes.CDLL(ctypes.util.find_library("dtio"))
#         self.protected = None

#         self.lib.DTIO_Init()

#     def test_pybindings(self):
#         print("Testing python bindings for DTIO")

#     def write(self, filename, buf, offset, count):
#         self.lib.DTIO_write(filename, buf, offset, count)

#     def read(self, filename, buf, offset, count):
#         self.lib.DTIO_read(filename, buf, offset, count)

class TorchDataset(Dataset):
    def __init__(self, format_type, dataset_type, epoch, num_samples, num_workers, batch_size):
        pass

    def __getitem__(self, idx):
        dtiopy.DTIO_read()

class TorchDataLoader(BaseDataLoader):
    def __init__(self):
        pass
