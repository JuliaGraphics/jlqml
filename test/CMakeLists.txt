add_executable(test_module_load test_module_load.cpp)
target_link_libraries(test_module_load JlCxx::cxxwrap_julia JlCxx::cxxwrap_julia_stl)
add_test(NAME test_module_load COMMAND test_module_load)

get_filename_component(CXXWRAP_ROOT ${JlCxx_location} DIRECTORY)
configure_file(setup-test.jl ${CMAKE_CURRENT_BINARY_DIR}/setup-test.jl @ONLY)

if(WIN32)
  set_property(TEST test_module_load PROPERTY
    ENVIRONMENT
      "JULIA_DEPOT_PATH=${CMAKE_BINARY_DIR}/test-depot"
      "PATH=${JULIA_HOME}\;${JlCxx_location}\;${QtCore_location}"
  )
else()
  set_property(TEST test_module_load PROPERTY
    ENVIRONMENT
      "JULIA_DEPOT_PATH=${CMAKE_BINARY_DIR}/test-depot"
  )
endif()