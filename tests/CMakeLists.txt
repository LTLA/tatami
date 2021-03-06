include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Main test executable.
add_executable(
    libtest 
    src/base/DenseMatrix.cpp
    src/base/CompressedSparseMatrix.cpp
    src/base/DelayedBind.cpp
    src/base/DelayedSubset.cpp
    src/base/DelayedSubsetBlock.cpp
    src/base/DelayedTranspose.cpp
    src/base/arith_vector_helpers.cpp
    src/base/arith_scalar_helpers.cpp
    src/base/math_helpers.cpp
    src/base/compress_sparse_triplets.cpp
    src/stats/sums.cpp
    src/stats/variances.cpp
    src/stats/medians.cpp
    src/stats/ranges.cpp
    src/stats/apply.cpp
    src/utils/wrap_shared_ptr.cpp
    src/utils/NakedArray.cpp
    src/utils/convert_to_dense.cpp
    src/utils/convert_to_sparse.cpp
    src/utils/bind_intersection.cpp
    src/data/data_sparse.cpp
    src/data/data_triangular.cpp
)

target_link_libraries(
    libtest
    gtest_main
    tatami
)

# Test custom parallelization during apply.
add_executable(
    cuspartest 
    src/stats/sums.cpp
    src/stats/variances.cpp
    src/stats/medians.cpp
    src/stats/ranges.cpp
    src/stats/apply.cpp
    src/data/data_sparse.cpp
    src/data/data_triangular.cpp
)

target_link_libraries(
    cuspartest 
    gtest_main
    tatami
)

# Test extensions
add_executable(
    exttest 
    src/ext/MatrixMarketBuffer.cpp
    src/ext/MatrixMarketText.cpp
    src/ext/MatrixMarketGzip.cpp
    src/ext/convert_to_layered_sparse.cpp
    src/ext/SomeNumericArray.cpp
    src/ext/ArrayView.cpp
)

target_link_libraries(
    exttest
    gtest_main
    tatami
)

# Checking whether to add OpenMP support. This is turned off
# by default to make it easier to debug test failures.
set(USE_OPENMP OFF CACHE BOOL "Compile with OpenMP support")
if (USE_OPENMP)
    find_package(OpenMP)
    target_link_libraries(libtest OpenMP::OpenMP_CXX)
    target_link_libraries(exttest OpenMP::OpenMP_CXX)
    target_link_libraries(cuspartest OpenMP::OpenMP_CXX)
endif()

target_compile_definitions(cuspartest PRIVATE CUSTOM_PARALLEL_TEST=1)

find_package(ZLIB)
target_link_libraries(exttest ZLIB::ZLIB)
target_compile_definitions(exttest PRIVATE TATAMI_USE_ZLIB=1)

include(CheckIncludeFiles)
check_include_files(filesystem HAVE_CXX_FS)
if (NOT HAVE_CXX_FS) 
    target_link_libraries(exttest stdc++fs) 
endif()

set(CODE_COVERAGE OFF CACHE BOOL "Enable coverage testing")
if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(libtest PRIVATE -O0 -g --coverage)
    target_compile_options(exttest PRIVATE -O0 -g --coverage)
    target_link_options(libtest PRIVATE --coverage)
    target_link_options(exttest PRIVATE --coverage)
endif()

# Making the tests discoverable.
include(GoogleTest)
gtest_discover_tests(libtest)
gtest_discover_tests(exttest)
gtest_discover_tests(cuspartest)

# Test HDF5 capabilities.
set(TEST_HDF5 ON CACHE BOOL "Test HDF5 support")
if (TEST_HDF5) 
    find_package(HDF5 COMPONENTS C CXX)
    if (HDF5_FOUND) 
        add_executable(
            h5test 
            src/ext/HDF5DenseMatrix.cpp
            src/ext/HDF5CompressedSparseMatrix.cpp
        )

        # Also checking the custom parallelization in the HDF5 libraries.
        add_executable(
            cusparh5test
            src/ext/HDF5DenseMatrix.cpp
            src/ext/HDF5CompressedSparseMatrix.cpp
        )
        target_compile_definitions(cusparh5test PRIVATE TEST_CUSTOM_PARALLEL=1)

        target_link_libraries(h5test gtest_main tatami hdf5::hdf5 hdf5::hdf5_cpp)
        target_link_libraries(cusparh5test gtest_main tatami hdf5::hdf5 hdf5::hdf5_cpp)

        if (NOT HAVE_CXX_FS) 
            target_link_libraries(h5test stdc++fs) 
            target_link_libraries(cusparh5test stdc++fs) 
        endif()

        if (USE_OPENMP)
            # Assume that the package has already been found.
            target_link_libraries(h5test OpenMP::OpenMP_CXX)
        endif()

        gtest_discover_tests(h5test)
        gtest_discover_tests(cusparh5test)
    endif()
endif()

