include_directories("${CMAKE_SOURCE_DIR}/src")

# Add Benchmarks
add_executable(cheetah-benchmarks 
    BenchmarkMain.cpp
    FlowTableBenchmarks.cpp
    IntersectionBenchmarks.cpp
)

target_link_libraries(cheetah-benchmarks 
    PumaSDK 
    benchmark
    pthread
    Bitmap
)