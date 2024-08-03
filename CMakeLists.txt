cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(
	csd
	DESCRIPTION "sdata for c, encoder/decoder for fast (de)serialization"
	LANGUAGES C
)

project(
	csd-test
	DESCRIPTION "sdata test suite"
	LANGUAGES C
)

add_library(
	csd STATIC
	src/csd_scan.c
	src/csd_parse.c
	src/csd_throw.c
	src/csd_node.c
	src/csd_write.c
)

target_include_directories(
	csd PUBLIC
	${CMAKE_SOURCE_DIR}/include
)
target_include_directories(
	csd PRIVATE
	${CMAKE_SOURCE_DIR}/src
)

set_target_properties(
	csd PROPERTIES
	C_STANDARD 17
	C_STANDARD_REQUIRED YES
	LINKER_LANGUAGE C
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

add_executable(
	csd-test
	src/csd_test.c
)

target_include_directories(
	csd-test PRIVATE
	${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(
	csd-test PRIVATE
	csd
)

set_target_properties(
	csd PROPERTIES
	C_STANDARD 17
	C_STANDARD_REQUIRED YES
	LINKER_LANGUAGE C
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
