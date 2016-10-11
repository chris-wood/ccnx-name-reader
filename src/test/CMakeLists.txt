cmake_minimum_required(VERSION 2.8)

project (ccnx_name_reader)
set(CMAKE_BUILD_TYPE Release)

include_directories(
        $ENV{CCNX_DEPENDENCIES}/include
        $ENV{CCNX_HOME}/include
    )

link_directories(
    $ENV{CCNX_HOME}/lib
    )

set(CCNX_NAME_READER_LIBRARIES
       longbow
       longbow-ansiterm
       parc
       ccnx_common
       ccnx_transport_rta
       )

macro(AddTest testFile)
    add_executable(${ARGV0} ${ARGV0}.c)
    target_link_libraries(${ARGV0} ${CCNX_NAME_READER_LIBRARIES})
    add_test(${ARGV0} ${ARGV0})
endmacro(AddTest)

AddTest(test_ccnx_NameReader)


