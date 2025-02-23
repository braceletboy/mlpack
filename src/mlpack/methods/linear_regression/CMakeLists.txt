# Define the files we need to compile
# Anything not in this list will not be compiled into the output library
# Do not include test programs here
set(SOURCES
  linear_regression.hpp
  linear_regression.cpp
)

# add directory name to sources
set(DIR_SRCS)
foreach(file ${SOURCES})
  set(DIR_SRCS ${DIR_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/${file})
endforeach()
# append sources (with directory name) to list of all mlpack sources (used at
# the parent scope)
set(MLPACK_SRCS ${MLPACK_SRCS} ${DIR_SRCS} PARENT_SCOPE)

add_category(linear_regression "regression")
group_bindings(linear_regression "train;predict")
add_python_wrapper(linear_regression)

add_cli_executable(linear_regression)
add_julia_binding(linear_regression)
add_go_binding(linear_regression)
add_r_binding(linear_regression)
add_markdown_docs(linear_regression "cli;python;julia;go;r" "python")
