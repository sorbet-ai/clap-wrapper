# The wrapper-regressions check (tests/CMakeLists.txt adds it as a ctest: ctest -R wrapper-regressions):
#   - target_add_vst3_wrapper puts the VST3 in ASSET_OUTPUT_DIRECTORY (macOS bundle, Windows single
#     file);
#   - target_add_auv2_wrapper with CLAP_TARGET_FOR_CONFIG names the component after the CLAP
#     target's OUTPUT_NAME, here one with spaces;
#   - the AU's Info.plist still has its AudioComponents entry after a re-configure and a build
#     that links nothing;
#   - at run time (au_check), no Cocoa view for a plugin without a gui, and the class data
#     carries the present preset's name.
# cmake -DCLAP_WRAPPER_ROOT=... -DCLAP_SDK_ROOT=... -DVST3_SDK_ROOT=... [-DAUDIOUNIT_SDK_ROOT=...]
#       -DBUILD_DIR=... [-DGENERATOR=...] -P run.cmake
set(src "${CMAKE_CURRENT_LIST_DIR}")
set(b "${BUILD_DIR}")
file(REMOVE_RECURSE "${b}")
set(gen "")
if (GENERATOR)
    set(gen -G "${GENERATOR}")
endif ()
set(args -DCLAP_WRAPPER_ROOT=${CLAP_WRAPPER_ROOT} -DCLAP_SDK_ROOT=${CLAP_SDK_ROOT} -DVST3_SDK_ROOT=${VST3_SDK_ROOT}
    -DCMAKE_BUILD_TYPE=Release)
if (AUDIOUNIT_SDK_ROOT)
    list(APPEND args -DAUDIOUNIT_SDK_ROOT=${AUDIOUNIT_SDK_ROOT})
endif ()

function(run)
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if (NOT rc EQUAL 0)
        message(FATAL_ERROR "failed (${rc}): ${ARGN}\n${out}${err}")
    endif ()
    set(out "${out}" PARENT_SCOPE)
endfunction()
function(expect_exists path what)
    if (EXISTS "${path}")
        message(STATUS "ok   ${what}")
    else ()
        message(FATAL_ERROR "FAIL ${what}: no ${path}")
    endif ()
endfunction()
function(check_au when)
    file(READ "${b}/Wrapper Regression.component/Contents/Info.plist" plist)
    string(FIND "${plist}" "AudioComponents" at)
    if (at EQUAL -1)
        message(FATAL_ERROR "FAIL the component's Info.plist has no AudioComponents entry ${when}")
    endif ()
    message(STATUS "ok   the component's Info.plist has its AudioComponents entry ${when}")
endfunction()

run("${CMAKE_COMMAND}" -S "${src}" -B "${b}" ${gen} ${args})
run("${CMAKE_COMMAND}" --build "${b}" --config Release)
if (CMAKE_HOST_APPLE)
    set(v3 "${b}/assets/Wrapper Regression.vst3/Contents/MacOS/Wrapper Regression")
else ()
    set(v3 "${b}/assets/Wrapper Regression.vst3")
endif ()
if (NOT EXISTS "${v3}") # (a multi-config generator: the configuration's directory in it, as CMake does)
    string(REPLACE "/assets/" "/assets/Release/" v3 "${v3}")
endif ()
expect_exists("${v3}" "the VST3 is in ASSET_OUTPUT_DIRECTORY")
if (CMAKE_HOST_APPLE)
    expect_exists("${b}/Wrapper Regression.component" "the component is named after the CLAP target's OUTPUT_NAME")
    check_au("after the first build")
    # a re-configure (CMake writes the bundle's Info.plist again) and a build that links nothing
    run("${CMAKE_COMMAND}" -S "${src}" -B "${b}" ${args} -DREGRESSION_RECONFIGURE=1)
    run("${CMAKE_COMMAND}" --build "${b}" --config Release)
    check_au("after a re-configure and a build")
    # (Ninja) after the bundle is signed, a build has nothing to do: the plist's copy doesn't depend
    # on the binary, which signing rewrites
    if ("${GENERATOR}" STREQUAL "Ninja")
        execute_process(COMMAND codesign --force --deep --sign - "${b}/Wrapper Regression.component" OUTPUT_QUIET ERROR_QUIET)
        run("${CMAKE_COMMAND}" --build "${b}" --config Release -- -n)
        if (NOT out MATCHES "no work to do")
            message(FATAL_ERROR "FAIL a build after signing the bundle has work to do: ${out}")
        endif ()
        message(STATUS "ok   a build after signing the bundle has nothing to do")
    endif ()
    run("${b}/au_check" "${b}/Wrapper Regression.component")
    message(STATUS "${out}")
endif ()
