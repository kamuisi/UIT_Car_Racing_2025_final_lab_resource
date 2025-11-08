
# Build Instructions

## Installation
**1.** Choose the **Torch-TensortRT** version suitable with your **JetPack**.

**2.** Install the required dependencies.
> [!NOTE]
> - You can use the bazelisk to install Bazel easily.
> - You can find the LibTorch (PyTorch) builds for Jetson devices from [NVIDIA’s official forum](https://forums.developer.nvidia.com/t/pytorch-for-jetson/72048).  
> - The Torch-TensortRT versions i referred are: 
>   - [Jetson Nano](https://github.com/pytorch/TensorRT/releases/tag/v1.0.0). 
>   - [Jetson Xavier NX](https://github.com/pytorch/TensorRT/releases/tag/v1.1.0).

**3.** Install build tools:
```bash
python3 -m pip install pybind11
sudo apt-get update
sudo apt-get install ninja-build libopenblas-dev nvidia-jetpack
```


## Building
**1.** Download the **Source code (zip)** and unzip it.

**2.** Go into the unzipped folder and **uncomment or comment** the lines in your `WORKSPACE` file to match the example version included in this repository.

**3.** Modify `WORKSPACE` in **line 44 and 109** to match your local installation paths.

**4.** Navigate to the `py` folder and open `setup.py`.

**5.** Scroll to the end and find `install-requires` line in `setup()` function.

**6.** Remove the contents inside the square brackets `[]`.

**7.** Save the file, then run this command:
```bash
python3 setup.py bdist_wheel  --release --use-cxx11-abi
```

**8.** Once the build is complete, go to the `dist` folder to find your generated **.whl** (wheel) package.
