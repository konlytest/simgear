
include (SimGearComponent)

set(HEADERS 
    CSSBorder.hxx
    ListDiff.hxx
    ResourceManager.hxx
    SimpleMarkdown.hxx
    SVGpreserveAspectRatio.hxx
    interpolator.hxx
    make_new.hxx
    sg_dir.hxx
    sg_path.hxx
    sgstream.hxx
    stdint.hxx
    stopwatch.hxx
    strutils.hxx
    tabbed_values.hxx
    texcoord.hxx
    zfstream.hxx
    gzcontainerfile.hxx
    )

set(SOURCES
    CSSBorder.cxx
    ResourceManager.cxx
    SimpleMarkdown.cxx
    SVGpreserveAspectRatio.cxx
    interpolator.cxx
    sg_dir.cxx
    sg_path.cxx
    sgstream.cxx
    strutils.cxx
    tabbed_values.cxx
    texcoord.cxx
    zfstream.cxx
    gzcontainerfile.cxx
    )

if (APPLE)
    list(APPEND SOURCES CocoaHelpers.mm)
endif()

simgear_component(misc misc "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)

add_executable(test_CSSBorder CSSBorder_test.cxx)
add_test(CSSBorder ${EXECUTABLE_OUTPUT_PATH}/test_CSSBorder)
target_link_libraries(test_CSSBorder ${TEST_LIBS})

add_executable(test_tabbed_values tabbed_values_test.cxx)
add_test(tabbed_values ${EXECUTABLE_OUTPUT_PATH}/test_tabbed_values)
target_link_libraries(test_tabbed_values ${TEST_LIBS})

add_executable(test_streams sgstream_test.cxx )
add_test(streams ${EXECUTABLE_OUTPUT_PATH}/test_streams)
target_link_libraries(test_streams ${TEST_LIBS})

add_executable(test_path path_test.cxx )
add_test(path ${EXECUTABLE_OUTPUT_PATH}/test_path)
target_link_libraries(test_path ${TEST_LIBS})

endif(ENABLE_TESTS)

add_boost_test(SimpleMarkdown
  SOURCES SimpleMarkdown_test.cxx
  LIBRARIES ${TEST_LIBS}
)

add_boost_test(SVGpreserveAspectRatio
  SOURCES SVGpreserveAspectRatio_test.cxx
  LIBRARIES ${TEST_LIBS}
)

add_boost_test(strutils
  SOURCES strutils_test.cxx
  LIBRARIES ${TEST_LIBS}
)

add_boost_test(utf8tolatin1
  SOURCES utf8tolatin1_test.cxx
  LIBRARIES ${TEST_LIBS}
)
