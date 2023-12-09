## Requirements

* gcc/g++ >= 8
* cmake >= 3.16
* Boost >= 1.75



## Compile

```shell
git clone --recurse-submodules git@github.com:BUAA-BDA/Hufu-FedJoin-Benchmark.git
cd Hufu-FedJoin-Benchmark
make build
cd build
cmake ..
make -j 8
```



## Run

```shell
# build directory
# parameters role(S/C) + input_file + data_size + algorithm_name

# Server:
./join SERVER ../../Data/TPCH/part.txt 20 80 Conclave

# Client:
./join CLIENT ../../Data/TPCH/partsupp.txt 20 80 Conclave
```

