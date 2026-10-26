# The wrapper-regressions project: the clap-first example's plugin wrapped the plain way
# (target_add_vst3_wrapper, target_add_auv2_wrapper), as a check of what these functions do with
# their arguments. run.cmake configures and builds it, re-configures and builds it again, and
# checks the results (see there).
cmake_minimum_required(VERSION 3.21)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # (clap-wrapper's own runtime)
project(wrapper-regressions VERSION 1.0.0 LANGUAGES C CXX)
if (APPLE)
    enable_language(OBJC OBJCXX)
endif ()
set(CMAKE_CXX_STANDARD 17)
set(CLAP_WRAPPER_DONT_ADD_TARGETS TRUE)
set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES FALSE)
add_subdirectory(${CLAP_WRAPPER_ROOT} clap-wrapper)

set(ex ${CLAP_WRAPPER_ROOT}/tests/clap-first-example)
add_library(regress-impl STATIC ${ex}/distortion_clap.cpp)
target_link_libraries(regress-impl PUBLIC clap clap-wrapper-extensions)

# a name with spaces, set as the CLAP target's OUTPUT_NAME only (not LIBRARY_OUTPUT_NAME)
set(name "Wrapper Regression")
add_library(regress_clap MODULE ${ex}/distortion_clap_entry.cpp)
target_include_directories(regress_clap PRIVATE ${ex})
target_link_libraries(regress_clap PRIVATE regress-impl)
if (APPLE)
    set_target_properties(regress_clap PROPERTIES OUTPUT_NAME "${name}" BUNDLE TRUE BUNDLE_EXTENSION clap)
else ()
    set_target_properties(regress_clap PROPERTIES OUTPUT_NAME "${name}" PREFIX "" SUFFIX ".clap")
endif ()

# the VST3: a single file on Windows, a bundle on macOS, in ASSET_OUTPUT_DIRECTORY
add_library(regress_vst3 MODULE ${ex}/distortion_clap_entry.cpp)
target_include_directories(regress_vst3 PRIVATE ${ex})
target_link_libraries(regress_vst3 PRIVATE regress-impl)
target_add_vst3_wrapper(TARGET regress_vst3 OUTPUT_NAME "${name}" ASSET_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/assets"
    BUNDLE_IDENTIFIER org.free-audio.wrapper-regression.vst3 BUNDLE_VERSION 1.0.0)

if (APPLE)
    # the AUv2 described from the CLAP target, its name the CLAP target's
    add_library(regress_auv2 MODULE ${ex}/distortion_clap_entry.cpp)
    target_include_directories(regress_auv2 PRIVATE ${ex})
    target_link_libraries(regress_auv2 PRIVATE regress-impl)
    target_add_auv2_wrapper(TARGET regress_auv2 CLAP_TARGET_FOR_CONFIG regress_clap
        BUNDLE_IDENTIFIER org.free-audio.wrapper-regression BUNDLE_VERSION 1.0.0
        MANUFACTURER_NAME "Free Audio" MANUFACTURER_CODE FrAD SUBTYPE_CODE WrRg INSTRUMENT_TYPE aufx)

    # the AU at run time: loaded into this process (never installed)
    add_executable(au_check au_check.cpp)
    target_link_libraries(au_check PRIVATE "-framework AudioToolbox" "-framework CoreFoundation")
    add_dependencies(au_check regress_auv2-info-plist)
endif ()
