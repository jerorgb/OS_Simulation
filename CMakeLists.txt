cmake_minimum_required(VERSION 3.10)

project(ActEntrega03 LANGUAGES CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Nice warnings (portable)
if (MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Put executables into bin/ inside build tree
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Sources: kernel simulator executable (main.cpp is in Kernel_sim/)
add_executable(kernel_sim
    Kernel_sim/main.cpp
)

# If you later split code into modules under modules/, add include dirs:
target_include_directories(kernel_sim PRIVATE
    ${CMAKE_SOURCE_DIR}/modules
)

# Optional: allow user to select replacement policy or other defines via cmake -D
# e.g. cmake -DDEFAULT_FRAMES=8 ..
option(BUILD_WITH_EXAMPLES "Build example targets" OFF)

# Installation (optional)
install(TARGETS kernel_sim
    RUNTIME DESTINATION bin
)
```// filepath: d:/CODING_WORK_SPACE_CARPET/EAFIT/C/ActEntrega03/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

project(ActEntrega03 LANGUAGES CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Nice warnings (portable)
if (MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Put executables into bin/ inside build tree
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Sources: kernel simulator executable (main.cpp is in Kernel_sim/)
add_executable(kernel_sim
    Kernel_sim/main.cpp
)

# If you later split code into modules under modules/, add include dirs:
target_include_directories(kernel_sim PRIVATE
    ${CMAKE_SOURCE_DIR}/modules
)

# Optional: allow user to select replacement policy or other defines via cmake -D
# e.g. cmake -DDEFAULT_FRAMES=8 ..
option(BUILD_WITH_EXAMPLES "Build example targets" OFF)

# Installation (optional)
install(TARGETS kernel_sim
    RUNTIME DESTINATION bin
)