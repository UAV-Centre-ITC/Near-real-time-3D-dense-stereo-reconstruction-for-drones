"""
PFM (Portable FloatMap) format utilities for stereo disparity output.

This module provides functions to save and load PFM files in the format
expected by Middlebury and other stereo evaluation benchmarks.
"""

import numpy as np
import os


def save_pfm(filename, data, scale=-1.0):
    """
    Save data to PFM (Portable FloatMap) format.
    
    PFM format specification:
    - Header: "Pf" for grayscale, "PF" for color
    - Dimensions: width height (space separated)
    - Scale factor: typically -1.0 for little-endian
    - Data: little-endian float32 values
    
    Args:
        filename (str): Output filename
        data (np.ndarray): 2D array of float32 values to save
        scale (float): Scale factor, -1.0 indicates little-endian
    """
    if len(data.shape) != 2:
        raise ValueError("Data must be 2D array for PFM format")
    
    height, width = data.shape
    
    with open(filename, 'wb') as f:
        # Write header
        f.write(b'Pf\n')
        f.write(f'{width} {height}\n'.encode())
        f.write(f'{scale}\n'.encode())
        
        # Write data in little-endian float32 format
        data.astype(np.float32).newbyteorder('<').tofile(f)


def load_pfm(filename):
    """
    Load data from PFM (Portable FloatMap) format.
    
    Args:
        filename (str): Input filename
        
    Returns:
        np.ndarray: Loaded data as float32 array
        float: Scale factor from header
    """
    with open(filename, 'rb') as f:
        # Read header
        header = f.readline().decode().strip()
        if header != 'Pf':
            raise ValueError(f"Invalid PFM header: {header}")
        
        # Read dimensions
        dimensions = f.readline().decode().strip()
        width, height = map(int, dimensions.split())
        
        # Read scale factor
        scale = float(f.readline().decode().strip())
        
        # Read data
        data = np.fromfile(f, dtype=np.float32)
        data = data.reshape((height, width))
        
        # Convert to native byte order if needed
        if scale < 0:
            data = data.newbyteorder('<')
        
        return data, scale


def save_disparity_pfm(filename, disparity_map):
    """
    Save disparity map to PFM format with standard parameters.
    
    Args:
        filename (str): Output filename
        disparity_map (np.ndarray): Disparity map as 2D float32 array
    """
    save_pfm(filename, disparity_map, scale=-1.0)


def save_occlusion_pfm(filename, occlusion_map):
    """
    Save occlusion map to PFM format.
    
    Occlusion maps typically use 0 for occluded pixels and 1 for visible pixels.
    
    Args:
        filename (str): Output filename
        occlusion_map (np.ndarray): Occlusion map as 2D float32 array
    """
    save_pfm(filename, occlusion_map, scale=-1.0)


def save_confidence_pfm(filename, confidence_map):
    """
    Save confidence map to PFM format.
    
    Confidence maps typically use values between 0 and 1.
    
    Args:
        filename (str): Output filename
        confidence_map (np.ndarray): Confidence map as 2D float32 array
    """
    save_pfm(filename, confidence_map, scale=-1.0)


def verify_pfm_file(filename):
    """
    Verify that a PFM file can be read correctly.
    
    Args:
        filename (str): PFM filename to verify
        
    Returns:
        tuple: (width, height, scale) if successful, None if failed
    """
    try:
        if not os.path.exists(filename):
            return None
        
        with open(filename, 'rb') as f:
            header = f.readline().decode().strip()
            if header != 'Pf':
                return None
            
            dimensions = f.readline().decode().strip()
            width, height = map(int, dimensions.split())
            
            scale = float(f.readline().decode().strip())
            
            return width, height, scale
            
    except Exception:
        return None


def get_pfm_info(filename):
    """
    Get information about a PFM file without loading the data.
    
    Args:
        filename (str): PFM filename
        
    Returns:
        dict: File information including dimensions, scale, and data size
    """
    info = verify_pfm_file(filename)
    if info is None:
        return None
    
    width, height, scale = info
    data_size_bytes = width * height * 4  # float32 = 4 bytes
    
    return {
        'width': width,
        'height': height,
        'scale': scale,
        'data_size_bytes': data_size_bytes,
        'data_size_mb': data_size_bytes / (1024 * 1024)
    }
