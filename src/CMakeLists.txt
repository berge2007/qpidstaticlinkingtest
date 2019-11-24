add_executable (x main.cpp)

include(ExternalProject)

########################################################
# apache qpid proton

ExternalProject_Add(qpid_ext
    PREFIX extern/qpid
    GIT_REPOSITORY    https://github.com/apache/qpid-proton.git
    GIT_TAG           0.29.0
    CMAKE_ARGS        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
                      -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                      -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
                      -DBUILD_WITH_CXX=ON
                      -DBUILD_CPP=ON
                      -DBUILD_CPP_03=OFF
                      -DBUILD_STATIC_LIBS=ON
                      #-DENABLE_WARNING_ERROR=OFF
                      #-DENABLE_UNDEFINED_ERROR=ON
                      #-DENABLE_LINKTIME_OPTIMIZATION=ON
                      #-DENABLE_HIDE_UNEXPORTED_SYMBOLS=OFF
                      -DENABLE_FUZZ_TESTING=OFF
                      -DFUZZ_LONG_TESTS=OFF
                      -DFUZZ_REGRESSION_TESTS=OFF
                      -DBUILD_TESTING=OFF
                      -DBUILD_BINDINGS="" # no bindings enabled
                      -DSYSINSTALL_BINDINGS=ON
    TEST_COMMAND      "" # do not run tests
    UPDATE_COMMAND    "" # do not check git repo again
)

ExternalProject_Get_Property(qpid_ext INSTALL_DIR)

set(qpid_INCLUDE_DIR ${INSTALL_DIR}/include)
file(MAKE_DIRECTORY ${qpid_INCLUDE_DIR})  # Must exists

add_library(qpid1 STATIC IMPORTED)
set_target_properties(qpid1 PROPERTIES
	IMPORTED_LOCATION ${INSTALL_DIR}/lib/libqpid-proton-core-static${CMAKE_STATIC_LIBRARY_SUFFIX}
	INTERFACE_INCLUDE_DIRECTORIES ${qpid_INCLUDE_DIR}
)
add_dependencies(qpid1 qpid_ext)

add_library(qpid2 STATIC IMPORTED)
set_target_properties(qpid2 PROPERTIES
	IMPORTED_LOCATION ${INSTALL_DIR}/lib/libqpid-proton-proactor-static${CMAKE_STATIC_LIBRARY_SUFFIX}
	INTERFACE_INCLUDE_DIRECTORIES ${qpid_INCLUDE_DIR}
)
add_dependencies(qpid2 qpid_ext)

add_library(qpid3 STATIC IMPORTED)
set_target_properties(qpid3 PROPERTIES
	IMPORTED_LOCATION ${INSTALL_DIR}/lib/libqpid-proton-static${CMAKE_STATIC_LIBRARY_SUFFIX}
	INTERFACE_INCLUDE_DIRECTORIES ${qpid_INCLUDE_DIR}
)
add_dependencies(qpid3 qpid_ext)

unset(INSTALL_DIR)

# apache qpid proton
########################################################

target_link_libraries (x qpid1 qpid2 qpid3)