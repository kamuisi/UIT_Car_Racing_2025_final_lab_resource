
# UIT Car Racing 2025 final lab resource
This repository contains the final lab source code for the UIT Car Racing 2025 competition. The project is designed to run on NVIDIA Jetson devices, leveraging PyTorch and Torch-TensorRT for model inference.

## Installation

**1. Install I2C libraries and build tools:**
```bash
sudo apt update
sudo apt install libi2c-dev i2c-tools ccache cmake ninja-build
```
**2. Add user to `i2c` and `tty` group**: This allows program to access hardward without requiring `sudo`. 
```bash
sudo usermod -aG i2c $USER
sudo usermod -aG tty $USER
```
> [!CAUTION]
> You must **log out and log back in** (or reboot your Jetson) for these group changes to take effect. Otherwise, you will encounter to permission errors.

**3. Install Pytorch and Torch-TensorRT:**
- Wheel packages (`.whl`) are provided in the `wheel_packages` directory.
- Ensure you select the correct file for your hardware (e.g., `cp36` for Jetson Nano, `cp38` for Jetson Xavier NX).
```bash
# Replace <your-wheel-file.whl> with the correct file from wheel_packages/
python3 -m pip install wheel_packages/<your-wheel-package.whl>
```    
- Alternatively, if you wish to build Torch-TensorRT from source, follow the instructions in the `build_instruction` folder.

## Usage
**1. Prepare Your Model**:
- Move your compiled **TorchScript model** (e.g., `model.ts`) into the `model/` directory.
- You can refer to `save_model_to_TS.py` in the `export_model` folder for an example of how to convert a PyTorch model to TorchScript.

**2. Compile the Project**:
```bash
mkdir build
cd build
cmake -G Ninja ..
```

**3. Build the Project**: This command uses all available processor cores for a faster build.
```bash
ninja -j$(nproc)
```

**4. Run the Program**:
```bash
./main
```

## Reference
- [I2C library usage](https://www.kernel.org/doc/Documentation/i2c/dev-interface)
- [Motor Driver usage](https://www.dropbox.com/scl/fi/o0i4gk3qeexkkjcknd5zf/MSDxx-Manual-v30.pdf?rlkey=8whwse5u8xn4unvxd2ato40sv&e=2&dl=0)
- [Torch-TensorRT](https://github.com/pytorch/TensorRT)

