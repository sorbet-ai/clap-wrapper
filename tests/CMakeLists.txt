add_subdirectory(clap-first-example)

if (APPLE)
    add_subdirectory(clap-macos-embedded)
endif()

# what the wrapper functions do with their arguments, and the AUv2 at run time: a project of its
# own, configured and built twice (wrapper-regressions/run.cmake). ctest -R wrapper-regressions
enable_testing()
add_test(NAME wrapper-regressions COMMAND ${CMAKE_COMMAND}
    -DCLAP_WRAPPER_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/.. -DCLAP_SDK_ROOT=${CLAP_SDK_ROOT} -DVST3_SDK_ROOT=${VST3_SDK_ROOT}
    -DAUDIOUNIT_SDK_ROOT=${AUDIOUNIT_SDK_ROOT} -DGENERATOR=${CMAKE_GENERATOR}
    -DBUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/wrapper-regressions
    -P ${CMAKE_CURRENT_SOURCE_DIR}/wrapper-regressions/run.cmake)
