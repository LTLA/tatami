#include <gtest/gtest.h>

#include "write_matrix_market.h"
#include "tatami/ext/MatrixMarket.hpp"
#include "tatami/ext/MatrixMarket_layered.hpp"

#include <limits>
#include <string>
#include <vector>

#include "mock_layered_sparse_data.h"

const unsigned char* to_pointer(const std::string& x) {
    return reinterpret_cast<const unsigned char*>(x.c_str());
}

class MatrixMarketBufferTest : public ::testing::TestWithParam<std::tuple<int, int, IntVec, IntVec, IntVec> > {
protected:
    size_t NR, NC;
    std::vector<int> rows, cols, vals;

    template<class PARAM>
    auto dump(PARAM param) {
        if (rows.size() != cols.size() || vals.size() != cols.size()) {
            throw std::runtime_error("inconsistent lengths in the sparse vectors");
        }

        NR = std::get<0>(param);
        NC = std::get<1>(param);
        rows = std::get<2>(param);
        cols = std::get<3>(param);
        vals = std::get<4>(param);

        return;
    }
};

TEST_P(MatrixMarketBufferTest, Simple) {
    dump(GetParam());
    std::stringstream stream;
    write_matrix_market(stream, NR, NC, vals, rows, cols);
    auto stuff = stream.str();

    auto out = tatami::MatrixMarket::load_sparse_matrix_from_buffer(to_pointer(stuff), stuff.size());

    // Checking against a reference.
    auto indptrs = tatami::compress_sparse_triplets<false>(NR, NC, vals, rows, cols);
    typedef tatami::CompressedSparseColumnMatrix<double, int, decltype(vals), decltype(rows), decltype(indptrs)> SparseMat; 
    auto ref = std::shared_ptr<tatami::NumericMatrix>(new SparseMat(NR, NC, std::move(vals), std::move(rows), std::move(indptrs))); 

    for (size_t i = 0; i < NC; ++i) {
        auto stuff = out->column(i);
        EXPECT_EQ(stuff, ref->column(i));
    }
}

TEST_P(MatrixMarketBufferTest, Layered) {
    dump(GetParam());
    std::stringstream stream;
    write_matrix_market(stream, NR, NC, vals, rows, cols);
    auto stuff = stream.str();

    tatami::MatrixMarket::LineAssignments ass;
    {
        tatami::MatrixMarket::BaseMMParser parser;
        byteme::RawBufferReader reader(to_pointer(stuff), stuff.size());
        parser(reader, ass);
        ass.finish();
    }

    size_t nonzeros = 0;
    for (auto& i : vals) { 
        nonzeros += (i != 0);
    }
    EXPECT_EQ(std::accumulate(ass.lines_per_category.begin(), ass.lines_per_category.end(), 0), nonzeros);
    EXPECT_EQ(ass.lines_per_category.size(), 3);

    EXPECT_EQ(std::accumulate(ass.rows_per_category.begin(), ass.rows_per_category.end(), 0), NR);
    EXPECT_EQ(ass.rows_per_category.size(), 3);

    auto copy = ass.permutation;
    EXPECT_EQ(copy.size(), NR);
    std::sort(copy.begin(), copy.end()); // 0->(NR-1);
    for (size_t i = 0; i < copy.size(); ++i) {
        EXPECT_EQ(i, copy[i]);
    }

    // Checking against a reference.
    auto loaded = tatami::MatrixMarket::load_layered_sparse_matrix_from_buffer(to_pointer(stuff), stuff.size());
    const auto& out = loaded.matrix;

    auto indptrs = tatami::compress_sparse_triplets<false>(NR, NC, vals, rows, cols);
    typedef tatami::CompressedSparseColumnMatrix<double, int, decltype(vals), decltype(rows), decltype(indptrs)> SparseMat; 
    auto ref = std::shared_ptr<tatami::NumericMatrix>(new SparseMat(NR, NC, std::move(vals), std::move(rows), std::move(indptrs))); 

    EXPECT_EQ(out->nrow(), ref->nrow());
    EXPECT_EQ(out->ncol(), ref->ncol());
    EXPECT_TRUE(out->sparse());
    EXPECT_FALSE(out->prefer_rows());

    for (size_t i = 0; i < NR; ++i) {
        int adjusted = loaded.permutation[i];
        auto stuff = out->row(adjusted);
        EXPECT_EQ(stuff, ref->row(i));

        auto maxed = *std::max_element(stuff.begin(), stuff.end());
        if (ass.category[i] == 2) {
            EXPECT_TRUE(maxed > std::numeric_limits<uint16_t>::max());
        } else if (ass.category[i] == 0) {
            EXPECT_TRUE(maxed <= std::numeric_limits<uint8_t>::max());
        } else if (ass.category[i] == 1) {
            EXPECT_TRUE(maxed <= std::numeric_limits<uint16_t>::max());
            EXPECT_TRUE(maxed > std::numeric_limits<uint8_t>::max());
        }
    }
}

TEST_P(MatrixMarketBufferTest, LayeredByRow) {
    dump(GetParam());

    // Sorting by row, 
    auto indptrs = tatami::compress_sparse_triplets<true>(NR, NC, vals, rows, cols);
    
    std::stringstream stream;
    write_matrix_market(stream, NR, NC, vals, rows, cols);
    auto stuff = stream.str();

    auto loaded = tatami::MatrixMarket::load_layered_sparse_matrix_from_buffer(to_pointer(stuff), stuff.size());
    const auto& out = loaded.matrix;

    // Creating a reference.
    typedef tatami::CompressedSparseRowMatrix<double, int, decltype(vals), decltype(cols), decltype(indptrs)> SparseMat; 
    auto ref = std::shared_ptr<tatami::NumericMatrix>(new SparseMat(NR, NC, vals, cols, indptrs)); 

    for (size_t i = 0; i < NR; ++i) {
        int adjusted = loaded.permutation[i];
        auto stuff = out->row(adjusted);
        EXPECT_EQ(stuff, ref->row(i));
    }
}

TEST_P(MatrixMarketBufferTest, LayeredByColumn) {
    dump(GetParam());

    // Sorting by column.
    auto indptrs = tatami::compress_sparse_triplets<false>(NR, NC, vals, rows, cols);

    std::stringstream stream;
    write_matrix_market(stream, NR, NC, vals, rows, cols);
    auto stuff = stream.str();

    auto loaded = tatami::MatrixMarket::load_layered_sparse_matrix_from_buffer(to_pointer(stuff), stuff.size());
    const auto& out = loaded.matrix;

    // Creating a reference.
    typedef tatami::CompressedSparseColumnMatrix<double, int, decltype(vals), decltype(rows), decltype(indptrs)> SparseMat; 
    auto ref = std::shared_ptr<tatami::NumericMatrix>(new SparseMat(NR, NC, vals, rows, indptrs)); 

    for (size_t i = 0; i < NR; ++i) {
        int adjusted = loaded.permutation[i];
        auto stuff = out->row(adjusted);
        EXPECT_EQ(stuff, ref->row(i));
    }
}

INSTANTIATE_TEST_CASE_P(
    MatrixMarket,
    MatrixMarketBufferTest,
    ::testing::Values(
        std::make_tuple(
            // this example guarantees a few rows in each chunk.
            // (the zero is deliberate, we want to check that it gets properly removed).
            10,
            5,
            IntVec{ 1, 5, 8, 2, 9, 0, 4 }, 
            IntVec{ 2, 3, 1, 0, 2, 2, 4 },
            IntVec{ 0, 1, 10, 100, 1000, 10000, 100000 }
        ),
        std::make_tuple(
            // this example checks that the maximum category is used for each row.
            5,
            8,
            IntVec{ 1, 1, 2, 2, 3, 3, 4, 4 },
            IntVec{ 1, 7, 2, 5, 4, 3, 4, 0 },
            IntVec{ 10, 1, 10, 1000, 10000, 100000, 1, 100000 }
        ),
        std::make_tuple(
            // this example checks that we handle missing categories; in this case, only shorts.
            10,
            9,
            IntVec{ 1, 9, 7, 5, 3, 1, 3, 3, 7, 9 },
            IntVec{ 2, 4, 8, 8, 4, 6, 8, 6, 0, 0 },
            IntVec{ 1, 3, 2, 1, 7, 8, 9, 1, 1, 3 }
        ),
        std::make_tuple(
            // this example checks that we handle missing categories; in this case, only uint16.
            20,
            15,
            IntVec{ 15, 0, 4, 14, 0, 19, 19, 8, 11, 18, 2,  3, 6,  4, 9,  3, 16,  4, 13, 12 },
            IntVec{  3, 3, 4,  2, 8, 12,  3, 6,  2,  3, 2, 11, 1, 11, 5, 12,  7, 12,  5,  0 },
            IntVec{ 1000, 3000, 2000, 1000, 7000, 10000, 9000, 1000, 600, 500, 382, 826, 992, 244, 138, 852, 400, 542, 980, 116 }
        ),
        std::make_tuple(
            // this example checks that we handle missing categories; in this case, only uint32.
            100,
            20,
            IntVec{ 27, 83, 85, 60, 17, 45, 62, 30, 98, 47 },
            IntVec{ 0, 12, 17, 3, 17, 0, 8, 3, 8, 8 },
            IntVec{ 130875, 673886, 405953, 989598, 981526, 794394, 680144, 553105, 277529, 540959 }
        ),
        std::make_tuple(
            // this example checks that we handle empties.
            10,
            9,
            IntVec{},
            IntVec{},
            IntVec{}
        )
    )
);

void quickMMErrorCheck(std::string contents, std::string msg) {
    EXPECT_ANY_THROW({
        try {
            tatami::MatrixMarket::load_sparse_matrix_from_buffer(to_pointer(contents), contents.size());
        } catch (std::exception& e) {
            EXPECT_TRUE(std::string(e.what()).find(msg) != std::string::npos);
            throw;
        }
    });
}

TEST(MatrixMarketTest, Errors) {
    quickMMErrorCheck("%% asdasdad\n1 2 -1", "non-negative");
    quickMMErrorCheck("%% asdasdad\n1 2 1a", "non-negative");
    quickMMErrorCheck("%% asdasdad\n1 2 1 5", "three values");

    quickMMErrorCheck("%% asdasdad\n1 2 3\n\n\n", "three values");
    quickMMErrorCheck("%% asdasdad\n1 2\n", "three values");

    quickMMErrorCheck("%% asdasdad\n", "no header line");

    quickMMErrorCheck("%% asdasdad\n1 2 1\n0 2 3\n", "must be positive"); 
    quickMMErrorCheck("%% asdasdad\n1 2 1\n2 2 3\n", "out of range"); 

    quickMMErrorCheck("%% asdasdad\n1 2 3\n1 2 3\n", "but 3 lines specified in the header"); 
    quickMMErrorCheck("%% asdasdad\n1 2 1\n1 2 3\n1 1 3\n", "more lines present"); 
}

TEST(MatrixMarketTest, EdgeCases) {
    auto check = [](const auto& mat) {
        EXPECT_TRUE(mat->sparse());
        EXPECT_EQ(mat->nrow(), 5);
        EXPECT_EQ(mat->ncol(), 6);
        
        for (size_t i = 0; i < 3; ++i) {
            auto col = mat->sparse_column(i);
            EXPECT_EQ(col.index.size(), 1);
            EXPECT_EQ(col.index[0], i);
            EXPECT_EQ(col.value[0], i + 1);
        }

        for (size_t i = 3; i < 6; ++i) {
            auto col = mat->sparse_column(i);
            EXPECT_EQ(col.index.size(), 0);
        }
    };

    // Handles arbitrary number of spaces.
    {
        std::string buffer = "5   6 3\n1 \t1 1 \n2 2 2   \n3 3 3 \n";
        auto out = tatami::MatrixMarket::load_sparse_matrix_from_buffer(to_pointer(buffer), buffer.size());
        check(out);
    }

    // Handles absence of a termianting newline.
    {
        std::string buffer = "5 6 3\n1 1 1\n2 2 2\n3 3 3";
        auto out = tatami::MatrixMarket::load_sparse_matrix_from_buffer(to_pointer(buffer), buffer.size());
        check(out);
    }
}

TEST(MatrixMarketTest, ComplexLayered) {
    size_t NR = 1000;
    size_t NC = 10;

    {
        std::vector<size_t> rows, cols;
        std::vector<int> vals;
        mock_layered_sparse_data<false>(NR, NC, rows, cols, vals);

        std::stringstream stream;
        write_matrix_market(stream, NR, NC, vals, rows, cols);
        auto stuff = stream.str();

        auto out = tatami::MatrixMarket::load_layered_sparse_matrix_from_buffer(to_pointer(stuff), stuff.size());

        // Checking against a reference.
        auto indptrs = tatami::compress_sparse_triplets<false>(NR, NC, vals, rows, cols);
        typedef tatami::CompressedSparseColumnMatrix<double, int, decltype(vals), decltype(rows), decltype(indptrs)> SparseMat; 
        auto ref = std::shared_ptr<tatami::NumericMatrix>(new SparseMat(NR, NC, std::move(vals), std::move(rows), std::move(indptrs))); 

        for (size_t i = 0; i < NR; ++i) {
            auto stuff = out.matrix->row(out.permutation[i]);
            EXPECT_EQ(stuff, ref->row(i));
        }
    }

    // Checking in the other orientation. 
    {
        std::vector<size_t> rows, cols;
        std::vector<int> vals;
        mock_layered_sparse_data<true>(NR, NC, rows, cols, vals);

        std::stringstream stream;
        write_matrix_market(stream, NR, NC, vals, rows, cols);
        auto stuff = stream.str();

        auto out = tatami::MatrixMarket::load_layered_sparse_matrix_from_buffer(to_pointer(stuff), stuff.size());

        // Checking against a reference.
        auto indptrs = tatami::compress_sparse_triplets<true>(NR, NC, vals, rows, cols);
        typedef tatami::CompressedSparseRowMatrix<double, int, decltype(vals), decltype(cols), decltype(indptrs)> SparseMat; 
        auto ref = std::shared_ptr<tatami::NumericMatrix>(new SparseMat(NR, NC, std::move(vals), std::move(cols), std::move(indptrs))); 

        for (size_t i = 0; i < NR; ++i) {
            auto stuff = out.matrix->row(out.permutation[i]);
            EXPECT_EQ(stuff, ref->row(i));
        }
    }
}

TEST(MatrixMarketTest, ManyRows) {
    size_t NR = 100000; // past the 16-bit limit; making sure that the dispatch to a larger int type works correctly.
    size_t NC = 10;

    std::vector<size_t> rows, cols;
    std::vector<int> vals;
    mock_layered_sparse_data<false>(NR, NC, rows, cols, vals);

    std::stringstream stream;
    write_matrix_market(stream, NR, NC, vals, rows, cols);
    auto stuff = stream.str();

    auto out = tatami::MatrixMarket::load_layered_sparse_matrix_from_buffer(to_pointer(stuff), stuff.size());

    // Checking against a reference.
    auto indptrs = tatami::compress_sparse_triplets<false>(NR, NC, vals, rows, cols);
    typedef tatami::CompressedSparseColumnMatrix<double, int, decltype(vals), decltype(rows), decltype(indptrs)> SparseMat; 
    auto ref = std::shared_ptr<tatami::NumericMatrix>(new SparseMat(NR, NC, std::move(vals), std::move(rows), std::move(indptrs))); 

    for (size_t i = 0; i < NR; ++i) {
        auto stuff = out.matrix->row(out.permutation[i]);
        EXPECT_EQ(stuff, ref->row(i));
    }
}

TEST(MatrixMarketTest, EmptyLayered) {
    {
        // Get some coverage on the cases where there are no columns.
        std::string stuff = "%%\n1000 0 0";
        auto out = tatami::MatrixMarket::load_layered_sparse_matrix_from_buffer(to_pointer(stuff), stuff.size());

        EXPECT_EQ(out.matrix->nrow(), 1000);
        EXPECT_EQ(out.matrix->ncol(), 0);

        for (size_t i = 0; i < 1000; ++i) {
            EXPECT_EQ(out.permutation[i], i);
        }
    }

    {
        // Get some coverage on the cases where there are no values.
        std::string stuff = "%%\n1000 10 0";
        auto out = tatami::MatrixMarket::load_layered_sparse_matrix_from_buffer(to_pointer(stuff), stuff.size());

        EXPECT_EQ(out.matrix->nrow(), 1000);
        EXPECT_EQ(out.matrix->ncol(), 10);

        for (size_t i = 0; i < 1000; ++i) {
            EXPECT_EQ(out.permutation[i], i);
            auto rdata = out.matrix->sparse_row(i);
            EXPECT_EQ(rdata.value.size(), 0);
        }
    }
}

TEST(MatrixMarketTest, Inspection) {
    std::string buffer = "5 6 3\n1 1 1\n2 2 2\n3 3 3";
    auto details = tatami::MatrixMarket::extract_header_from_buffer(to_pointer(buffer), buffer.size());
    EXPECT_EQ(details.nrow, 5);
    EXPECT_EQ(details.ncol, 6);
    EXPECT_EQ(details.nlines, 3);

    // Check that the SFINAE works to enable early abort of the parsing.
    bool val = tatami::MatrixMarket::BaseMMParser::preamble_only<tatami::MatrixMarket::SimpleBuilder<double, int>::Core>::value;
    EXPECT_FALSE(val);
    EXPECT_TRUE(tatami::MatrixMarket::BaseMMParser::preamble_only<tatami::MatrixMarket::Inspector::Core>::value);
}
