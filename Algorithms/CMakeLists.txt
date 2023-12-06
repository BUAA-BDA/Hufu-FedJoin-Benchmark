cmake_minimum_required(VERSION 3.16)

Project(join)

find_package(Boost REQUIRED COMPONENTS thread system)

add_subdirectory(extern/ABY)

add_executable(join
	join.cpp
	nest_loop_join.cpp
	hash_join.cpp
	sort_merge_join.cpp
	utils.cpp
	obliv_permutation.cpp
	fast_join.cpp
	obliv_join.cpp
	murmur_hash3.cpp
	)

add_executable(test
	test.cpp
	utils.cpp
	obliv_permutation.cpp
	)

target_link_libraries(join PUBLIC
	ABY::aby
	relic_s
	)


target_include_directories(join PUBLIC
	extern/ABY/src/abycore
	extern/ABY/extern/ENCRYPTO_utils/src
	)

target_link_libraries(test PUBLIC
	ABY::aby
	relic_s
	)


target_include_directories(test PUBLIC
	extern/ABY/src/abycore
	extern/ABY/extern/ENCRYPTO_utils/src
	)