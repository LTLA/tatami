#include <gtest/gtest.h>

#include <vector>
#include <memory>
#include <tuple>

#include "tatami/base/DenseMatrix.hpp"
#include "tatami/base/DelayedSubset.hpp"
#include "tatami/utils/convert_to_sparse.hpp"

#include "../data/data.h"
#include "TestCore.h"

template<class PARAM> 
class SubsetTest : public TestCore<::testing::TestWithParam<PARAM> > {
protected:
    std::shared_ptr<tatami::NumericMatrix> dense, sparse;
protected:
    void SetUp() {
        dense = std::shared_ptr<tatami::NumericMatrix>(new tatami::DenseRowMatrix<double>(sparse_nrow, sparse_ncol, sparse_matrix));
        sparse = tatami::convert_to_sparse<false>(dense.get()); // column-major.
        return;
    }
};

/****************************************************
 ****************************************************/

class SubsetRowFullAccessTest : public SubsetTest<std::tuple<size_t, std::vector<size_t> > > {};

TEST_P(SubsetRowFullAccessTest, RowAccess) {
    size_t JUMP = std::get<0>(GetParam());
    std::vector<size_t> sub = std::get<1>(GetParam());

    auto dense_subbed = tatami::make_DelayedSubset<0>(dense, sub);
    auto sparse_subbed = tatami::make_DelayedSubset<0>(sparse, sub);

    EXPECT_EQ(sub.size(), dense_subbed->nrow());
    EXPECT_EQ(dense->ncol(), dense_subbed->ncol());
    EXPECT_EQ(dense->sparse(), dense_subbed->sparse());
    EXPECT_EQ(sparse->sparse(), sparse_subbed->sparse());
    EXPECT_TRUE(dense_subbed->prefer_rows());
    EXPECT_FALSE(sparse_subbed->prefer_rows());

    auto work_dense = dense_subbed->new_workspace(true);
    auto work_sparse = sparse_subbed->new_workspace(true);

    for (size_t i = 0; i < sub.size(); i += JUMP) {
        auto expected = extract_dense<true>(dense.get(), sub[i]);

        auto output = extract_dense<true>(dense_subbed.get(), i);
        EXPECT_EQ(output, expected);

        // Works with different backends.
        auto output2 = extract_dense<true>(sparse_subbed.get(), i);
        EXPECT_EQ(output2, expected);

        // Works in sparse mode as well.
        auto outputS = extract_sparse<true>(sparse_subbed.get(), i);
        EXPECT_EQ(outputS, expected);

        // Passes along the workspace.
        auto outputDW = extract_dense<true>(dense_subbed.get(), i, work_dense.get());
        EXPECT_EQ(outputDW, expected);

        auto outputSW = extract_sparse<true>(sparse_subbed.get(), i, work_sparse.get());
        EXPECT_EQ(outputSW, expected);
    }
}

TEST_P(SubsetRowFullAccessTest, ColumnAccess) {
    size_t JUMP = std::get<0>(GetParam());
    std::vector<size_t> sub = std::get<1>(GetParam());

    auto dense_subbed = tatami::make_DelayedSubset<0>(dense, sub);
    auto sparse_subbed = tatami::make_DelayedSubset<0>(sparse, sub);

    auto work_dense = dense_subbed->new_workspace(false);
    auto work_sparse = sparse_subbed->new_workspace(false);

    for (size_t i = 0; i < dense_subbed->ncol(); i += JUMP) {
        // Reference extraction, the simple way.
        auto raw = extract_dense<false>(dense.get(), i);
        std::vector<double> expected;
        for (auto s : sub) { expected.push_back(raw[s]); }

        auto output = extract_dense<false>(dense_subbed.get(), i);
        EXPECT_EQ(output, expected);

        // Works with different backends.
        auto output2 = extract_dense<false>(sparse_subbed.get(), i);
        EXPECT_EQ(output2, expected);

        // Works in sparse mode as well.
        auto outputS = extract_sparse<false>(sparse_subbed.get(), i);
        EXPECT_EQ(outputS, expected);

        // Passes along the workspace.
        auto outputDW = extract_dense<false>(dense_subbed.get(), i, work_dense.get());
        EXPECT_EQ(outputDW, expected);

        auto outputSW = extract_sparse<false>(sparse_subbed.get(), i, work_sparse.get());
        EXPECT_EQ(outputSW, expected);
    }
}

INSTANTIATE_TEST_CASE_P(
    DelayedSubset,
    SubsetRowFullAccessTest,
    ::testing::Combine(
        ::testing::Values(1, 3), // jump, to test the workspace memory.
        ::testing::Values(
            std::vector<size_t>({ 0, 3, 3, 13, 5, 2, 19, 4, 6, 11, 19, 8 }), // with duplicates
            std::vector<size_t>({ 1, 2, 3, 5, 9, 13, 17 }), // ordered, no duplicates
            std::vector<size_t>({ 8, 9, 10, 11}) // consecutive
        )
    )
);

/****************************************************
 ****************************************************/

class SubsetRowSlicedAccessTest : public SubsetTest<std::tuple<size_t, std::vector<size_t>, std::vector<size_t> > > {};

TEST_P(SubsetRowSlicedAccessTest, RowAccess) {
    size_t JUMP = std::get<0>(GetParam());
    std::vector<size_t> sub = std::get<1>(GetParam()); 
    std::vector<double> buffer_full(dense->nrow());

    auto dense_subbed = tatami::make_DelayedSubset<0>(dense, sub);
    auto sparse_subbed = tatami::make_DelayedSubset<0>(sparse, sub);

    auto slice = std::get<2>(GetParam());
    size_t FIRST = slice[0], LEN = slice[1], SHIFT = slice[2];

    auto work_dense = dense_subbed->new_workspace(true);
    auto work_sparse = sparse_subbed->new_workspace(true);

    for (size_t i = 0; i < sub.size(); i += JUMP, FIRST += SHIFT) {
        auto info = wrap_intervals(FIRST, FIRST + LEN, dense_subbed->ncol());
        size_t first = info.first, last = info.second;

        auto expected = extract_dense<true>(dense.get(), sub[i], first, last);

        auto output = extract_dense<true>(dense_subbed.get(), i, first, last);
        EXPECT_EQ(output, expected);

        // Same result regardless of the backend.
        auto output2 = extract_dense<true>(sparse_subbed.get(), i, first, last);
        EXPECT_EQ(output2, expected);

        // Works in sparse mode as well.
        auto outputS = extract_sparse<true>(sparse_subbed.get(), i, first, last);
        EXPECT_EQ(outputS, expected);        

        // Passes along the workspace.
        auto outputDW = extract_dense<true>(dense_subbed.get(), i, first, last, work_dense.get());
        EXPECT_EQ(outputDW, expected);

        auto outputSW = extract_sparse<true>(sparse_subbed.get(), i, first, last, work_sparse.get());
        EXPECT_EQ(outputSW, expected);
    }
}

TEST_P(SubsetRowSlicedAccessTest, ColumnAccess) {
    size_t JUMP = std::get<0>(GetParam());
    std::vector<size_t> sub = std::get<1>(GetParam()); 
    std::vector<double> buffer_full(dense->nrow());

    auto dense_subbed = tatami::make_DelayedSubset<0>(dense, sub);
    auto sparse_subbed = tatami::make_DelayedSubset<0>(sparse, sub);

    auto slice = std::get<2>(GetParam());
    size_t FIRST = slice[0], LEN = slice[1], SHIFT = slice[2];

    auto work_dense = dense_subbed->new_workspace(false);
    auto work_sparse = sparse_subbed->new_workspace(false);

    for (size_t i = 0; i < dense_subbed->ncol(); i += JUMP, FIRST += SHIFT) {
        auto info = wrap_intervals(FIRST, FIRST + LEN, dense_subbed->nrow());
        size_t first = info.first, last = info.second;

        auto raw = extract_dense<false>(dense.get(), i);
        std::vector<double> expected;
        for (size_t s = first; s < last; ++s) { expected.push_back(raw[sub[s]]); }

        auto output = extract_dense<false>(dense_subbed.get(), i, first, last);
        EXPECT_EQ(output, expected);

        // Same result regardless of the backend.
        auto output2 = extract_dense<false>(sparse_subbed.get(), i, first, last);
        EXPECT_EQ(output2, expected);

        // Works in sparse mode as well.
        auto outputS = extract_sparse<false>(sparse_subbed.get(), i, first, last);
        EXPECT_EQ(outputS, expected);        

        // Passes along the workspace.
        auto outputDW = extract_dense<false>(dense_subbed.get(), i, first, last, work_dense.get());
        EXPECT_EQ(outputDW, expected);

        auto outputSW = extract_sparse<false>(sparse_subbed.get(), i, first, last, work_sparse.get());
        EXPECT_EQ(outputSW, expected);
    }
}


INSTANTIATE_TEST_CASE_P(
    DelayedSubset,
    SubsetRowSlicedAccessTest,
    ::testing::Combine(
        ::testing::Values(1, 3), // jump, to check the workspace memory
        ::testing::Values(
            std::vector<size_t>({ 17, 18, 11, 18, 15, 17, 13, 18, 11, 9, 6, 3, 6, 18, 1 }), // with duplicates
            std::vector<size_t>({ 2, 3, 5, 7, 9, 12, 13 }), // ordered, no duplicates
            std::vector<size_t>({ 4, 5, 6, 7, 8, 9, 10 }) // consecutive
        ),
        ::testing::Values(
            std::vector<size_t>({ 0, 6, 13 }), // start, length, shift
            std::vector<size_t>({ 1, 7, 3 }),
            std::vector<size_t>({ 3, 18, 0 })
        )
    )
);

/****************************************************
 ****************************************************/

class SubsetColumnFullAccessTest : public SubsetTest<std::tuple<size_t, std::vector<size_t> > > {};

TEST_P(SubsetColumnFullAccessTest, RowAccess) {
    size_t JUMP = std::get<0>(GetParam());
    std::vector<size_t> sub = std::get<1>(GetParam());

    auto dense_subbed = tatami::make_DelayedSubset<1>(dense, sub);
    auto sparse_subbed = tatami::make_DelayedSubset<1>(sparse, sub);

    EXPECT_EQ(dense->nrow(), dense_subbed->nrow());
    EXPECT_EQ(sub.size(), dense_subbed->ncol());
    EXPECT_EQ(dense->sparse(), dense_subbed->sparse());
    EXPECT_EQ(sparse->sparse(), sparse_subbed->sparse());
    EXPECT_TRUE(dense_subbed->prefer_rows());
    EXPECT_FALSE(sparse_subbed->prefer_rows());

    auto work_dense = dense_subbed->new_workspace(true);
    auto work_sparse = sparse_subbed->new_workspace(true);

    for (size_t i = 0; i < dense_subbed->nrow(); i += JUMP) {
        // Reference extraction, the simple way.
        auto raw = extract_dense<true>(dense.get(), i);
        std::vector<double> expected;
        for (auto s : sub) { expected.push_back(raw[s]); }

        auto output = extract_dense<true>(dense_subbed.get(), i);
        EXPECT_EQ(output, expected);

        // Works with different backends.
        auto output2 = extract_dense<true>(sparse_subbed.get(), i);
        EXPECT_EQ(output2, expected);

        // Works in sparse mode as well.
        auto outputS = extract_sparse<true>(sparse_subbed.get(), i);
        EXPECT_EQ(outputS, expected);

        // Passes along the workspace.
        auto outputDW = extract_dense<true>(dense_subbed.get(), i, work_dense.get());
        EXPECT_EQ(outputDW, expected);

        auto outputSW = extract_sparse<true>(sparse_subbed.get(), i, work_sparse.get());
        EXPECT_EQ(outputSW, expected);
    }
}

TEST_P(SubsetColumnFullAccessTest, ColumnAccess) {
    size_t JUMP = std::get<0>(GetParam());
    std::vector<size_t> sub = std::get<1>(GetParam());

    auto dense_subbed = tatami::make_DelayedSubset<1>(dense, sub);
    auto sparse_subbed = tatami::make_DelayedSubset<1>(sparse, sub);

    auto work_dense = dense_subbed->new_workspace(false);
    auto work_sparse = sparse_subbed->new_workspace(false);

    for (size_t i = 0; i < dense_subbed->ncol(); i += JUMP) {
        auto expected = extract_dense<false>(dense.get(), sub[i]);

        auto output = extract_dense<false>(dense_subbed.get(), i);
        EXPECT_EQ(output, expected);

        // Works with different backends.
        auto output2 = extract_dense<false>(sparse_subbed.get(), i);
        EXPECT_EQ(output2, expected);

        // Works in sparse mode as well.
        auto outputS = extract_sparse<false>(sparse_subbed.get(), i);
        EXPECT_EQ(outputS, expected);

        // Passes along the workspace.
        auto outputDW = extract_dense<false>(dense_subbed.get(), i, work_dense.get());
        EXPECT_EQ(outputDW, expected);

        auto outputSW = extract_sparse<false>(sparse_subbed.get(), i, work_sparse.get());
        EXPECT_EQ(outputSW, expected);
    }
}

INSTANTIATE_TEST_CASE_P(
    DelayedSubset,
    SubsetColumnFullAccessTest,
    ::testing::Combine(
        ::testing::Values(1, 3), // jump, to test the workspace memory.
        ::testing::Values(
            std::vector<size_t>({ 3, 9, 1, 0, 9, 5, 8, 3, 1, 8, 7 }), // with duplicates
            std::vector<size_t>({ 0, 1, 2, 3, 5, 8 }), // ordered, no duplicates
            std::vector<size_t>({ 2, 3, 4, 5 }) // consecutive
        )
    )
);

/****************************************************
 ****************************************************/

class SubsetColumnSlicedAccessTest : public SubsetTest<std::tuple<size_t, std::vector<size_t>, std::vector<size_t> > > {};

TEST_P(SubsetColumnSlicedAccessTest, RowAccess) {
    size_t JUMP = std::get<0>(GetParam());
    std::vector<size_t> sub = std::get<1>(GetParam()); 
    std::vector<double> buffer_full(dense->nrow());

    auto dense_subbed = tatami::make_DelayedSubset<1>(dense, sub);
    auto sparse_subbed = tatami::make_DelayedSubset<1>(sparse, sub);

    auto slice = std::get<2>(GetParam());
    size_t FIRST = slice[0], LEN = slice[1], SHIFT = slice[2];

    auto work_dense = dense_subbed->new_workspace(true);
    auto work_sparse = sparse_subbed->new_workspace(true);

    for (size_t i = 0; i < dense_subbed->nrow(); i += JUMP, FIRST += SHIFT) {
        auto info = wrap_intervals(FIRST, FIRST + LEN, dense_subbed->ncol());
        size_t first = info.first, last = info.second;

        // Reference extraction, the simple way.
        auto raw = extract_dense<true>(dense.get(), i);
        std::vector<double> expected;
        for (size_t s = first; s < last; ++s) { expected.push_back(raw[sub[s]]); }

        auto output = extract_dense<true>(dense_subbed.get(), i, first, last);
        EXPECT_EQ(output, expected);

        // Same result regardless of the backend.
        auto output2 = extract_dense<true>(sparse_subbed.get(), i, first, last);
        EXPECT_EQ(output2, expected);

        // Works in sparse mode as well.
        auto outputS = extract_sparse<true>(sparse_subbed.get(), i, first, last);
        EXPECT_EQ(outputS, expected);        

        // Passes along the workspace.
        auto outputDW = extract_dense<true>(dense_subbed.get(), i, first, last, work_dense.get());
        EXPECT_EQ(outputDW, expected);

        auto outputSW = extract_sparse<true>(sparse_subbed.get(), i, first, last, work_sparse.get());
        EXPECT_EQ(outputSW, expected);
    }
}

TEST_P(SubsetColumnSlicedAccessTest, ColumnAccess) {
    size_t JUMP = std::get<0>(GetParam());
    std::vector<size_t> sub = std::get<1>(GetParam()); 
    std::vector<double> buffer_full(dense->nrow());

    auto dense_subbed = tatami::make_DelayedSubset<1>(dense, sub);
    auto sparse_subbed = tatami::make_DelayedSubset<1>(sparse, sub);

    auto slice = std::get<2>(GetParam());
    size_t FIRST = slice[0], LEN = slice[1], SHIFT = slice[2];

    auto work_dense = dense_subbed->new_workspace(false);
    auto work_sparse = sparse_subbed->new_workspace(false);

    for (size_t i = 0; i < dense_subbed->ncol(); i += JUMP, FIRST += SHIFT) {
        auto info = wrap_intervals(FIRST, FIRST + LEN, dense_subbed->nrow());
        size_t first = info.first, last = info.second;

        auto expected = extract_dense<false>(dense.get(), sub[i], first, last);

        auto output = extract_dense<false>(dense_subbed.get(), i, first, last);
        EXPECT_EQ(output, expected);

        // Same result regardless of the backend.
        auto output2 = extract_dense<false>(sparse_subbed.get(), i, first, last);
        EXPECT_EQ(output2, expected);

        // Works in sparse mode as well.
        auto outputS = extract_sparse<false>(sparse_subbed.get(), i, first, last);
        EXPECT_EQ(outputS, expected);        

        // Passes along the workspace.
        auto outputDW = extract_dense<false>(dense_subbed.get(), i, first, last, work_dense.get());
        EXPECT_EQ(outputDW, expected);

        auto outputSW = extract_sparse<false>(sparse_subbed.get(), i, first, last, work_sparse.get());
        EXPECT_EQ(outputSW, expected);
    }
}


INSTANTIATE_TEST_CASE_P(
    DelayedSubset,
    SubsetColumnSlicedAccessTest,
    ::testing::Combine(
        ::testing::Values(1, 3), // jump, to check the workspace memory
        ::testing::Values(
            std::vector<size_t>({ 2, 2, 4, 8, 0, 7, 3, 1, 1, 2, 7, 8, 9, 9, 4, 5, 8, 5, 6, 2, 0 }),
            std::vector<size_t>({ 2, 3, 5, 7, 9 }), // ordered, no duplicates
            std::vector<size_t>({ 3, 4, 5, 6, 7, 8, 9 }) // consecutive
        ),
        ::testing::Values(
            std::vector<size_t>({ 0, 6, 1 }), // start, length, shift
            std::vector<size_t>({ 5, 5, 2 }),
            std::vector<size_t>({ 3, 7, 0 })
        )
    )
);

