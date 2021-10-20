# List Scheduling
List Scheduling written in c++

# Requirements
* cmake3.14+
* gcc7+

# Build BLIF parser
```sh
git clone https://github.com/weitunglin/list_scheduling.git
cd list_scheduling
cmake -B build . -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

# Run List Scheduling

1. ML-RCS
```sh
./build/list -l aoi_benchmark/aoi_sample02.blif 2 1 1 
```

2. MR-LCS
```sh
./build/list -r aoi_benchmark/aoi_sample02.blif 5
```

# Worst case

1. ML-RCS
```sh
./build/list -l aoi_benchmark/aoi_worstcase.blif 1 1 1
```

2. MR-LCS
```sh
./build/list -r aoi_benchmark/aoi_worstcase.blif 2
```