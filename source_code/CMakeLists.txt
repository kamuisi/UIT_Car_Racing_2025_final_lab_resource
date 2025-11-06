cmake_minimum_required(VERSION 3.10)
project(UCR2025)

set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
set(CMAKE_C_COMPILER_LAUNCHER ccache)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16")
	set(CMAKE_GENERATOR_NINJA ON)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -ffast-math -Wall -Wextra")
set(CMAKE_BUILD_TYPE Release)

execute_process(
        COMMAND python3 -c "import torch, os; print(os.path.dirname(torch.__file__))"
	OUTPUT_VARIABLE PYTORCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CMAKE_PREFIX_PATH "${PYTORCH}")

execute_process(
	COMMAND python3 -c "import torch_tensorrt, os; print(os.path.dirname(torch_tensorrt.__file__))"
	OUTPUT_VARIABLE TORCH_TENSORRT
	OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	
message(STATUS "Found PyTorch at ${PYTORCH}")
message(STATUS "Found Torch-TensorRT at ${TORCH_TENSORRT}")
	
find_package(Torch REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

include_directories(${TORCH_TENSORRT}/include ${CMAKE_SOURCE_DIR}/inc)
link_directories(${TORCH_TENSORRT}/lib)

add_subdirectory(module)

add_executable(main main.cpp)

target_link_libraries(main PRIVATE camera servo_oled model motor Threads::Threads)