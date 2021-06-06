#include <gtest/gtest.h>

#include <vector>
#include <deque>
#include <numeric>

#include "tatami/base/DenseMatrix.hpp"

TEST(DenseMatrix, Construction) {
    std::vector<double> contents(200);
    double counter = -105;
    for (auto& i : contents) { i = counter++; }
    tatami::DenseColumnMatrix<double> mat(10, 20, contents);
    EXPECT_EQ(mat.nrow(), 10);
    EXPECT_EQ(mat.ncol(), 20);
    EXPECT_EQ(mat.type(), tatami::_double);

    // Checks run properly.
    contents.clear();
    EXPECT_ANY_THROW({
        tatami::DenseColumnMatrix<double> mat(10, 20, contents);
    });
    EXPECT_ANY_THROW({
        tatami::DenseColumnMatrix<double> mat(10, 20, std::move(contents));
    });

    std::deque<double> more_contents(200);
    std::iota(more_contents.begin(), more_contents.end(), 1);
    tatami::DenseColumnMatrix<double, std::deque<double> > mat2(10, 20, more_contents);
    EXPECT_EQ(more_contents.size(), 200);

    tatami::DenseColumnMatrix<double, std::deque<double> > mat3(20, 10, std::move(more_contents)); // can't run many tests for state of more_contents here.
}
