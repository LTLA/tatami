#ifndef TATAMI_TYPED_MATRIX_H
#define TATAMI_TYPED_MATRIX_H

#include "matrix.hpp"
#include "sparse_range.hpp"
#include "workspace.hpp"

/**
 * @file typed_matrix.hpp
 *
 * Virtual class for a matrix with a defined type.
 */

namespace tatami {

/**
 * @brief Virtual class for a matrix with a defined type.
 * 
 * @tparam T Type of the matrix values.
 * @tparam IDX Type of the row/column indices.
 */
template <typename T, typename IDX = int>
class typed_matrix : public matrix {
public:
    ~typed_matrix() {}

    /**
     * See comments for `column()`, which are also applicable here.
     *
     * @param r Index of the row.
     * @param buffer Pointer with enough space for at least `last - first` values.
     * @param first First column to extract for row `r`.
     * @param last One past the last column to extract in `r`.
     * @param work Pointer to a workspace.
     *
     * @return Pointer to the values of row `r`, starting from the value in the `first` column and containing `last - first` valid entries.
     */
    virtual const T* row(size_t r, T* buffer, size_t first, size_t last, workspace* work=nullptr) const = 0;

    /**
     * `buffer` may not necessarily be filled upon extraction if a pointer can be returned to the underlying data store.
     * This be checked by comparing the returned pointer to `buffer`; if they are the same, `buffer` has been filled.
     *
     * If `work` is not a null pointer, it should have been generated by `new_workspace()` with `row = false`.
     * This is optional and should only affect the efficiency of extraction, not the contents.
     *
     * @param c Index of the column.
     * @param buffer Pointer with enough space for at least `last - first` values.
     * @param first First row to extract for column `c`.
     * @param last One past the last row to extract in `c`.
     * @param work Pointer to a workspace.
     *
     * @return Pointer to the values of column `c`, starting from the value in the `first` row and containing `last - first` valid entries.
     */
    virtual const T* column(size_t c, T* buffer, size_t first, size_t last, workspace* work=nullptr) const = 0;

    /**
     * See comments for `column()`, which are also applicable here.
     *
     * @param r Index of the row.
     * @param buffer Pointer with enough space for at least `ncol()` values.
     * @param work Pointer to a workspace.
     *
     * @return Pointer to the values of row `r`.
     */
    const T* row(size_t r, T* buffer, workspace* work=nullptr) const {
        return row(r, buffer, 0, this->ncol(), work);
    }

    /**
     * See comments for `column()`, which are also applicable here.
     *
     * @param c Index of the column.
     * @param buffer Pointer with enough space for at least `nrow()` values.
     * @param work Pointer to a workspace.
     *
     * @return Pointer to the values of column `c`.
     */
    const T* column(size_t c, T* buffer, workspace* work=nullptr) const {
        return column(c, buffer, 0, this->nrow(), work);
    }

    /**
     * See comments for `sparse_column()`, which are also applicable here.
     *
     * @param r Index of the row.
     * @param vbuffer Pointer with enough space for at least `last - first` values.
     * @param ibuffer Pointer with enough space for at least `last - first` indices.
     * @param first First column to extract for row `r`.
     * @param last One past the last column to extract in `r`.
     * @param work Pointer to a workspace.
     * @param sorted Should the non-zero elements be sorted by their indices?
     *
     * @return A `sparse_range` object containing the number of non-zero elements in `r` from column `first` up to `last`.
     * This also contains pointers to their column indices and values.
     */
    virtual sparse_range<T, IDX> sparse_row(size_t r, T* vbuffer, IDX* ibuffer, size_t first, size_t last, workspace* work=nullptr, bool sorted=true) const {
        const T* val = row(r, vbuffer, first, last, work);
        for (size_t i = first; i < last; ++i) {
            ibuffer[i - first] = i;
        }
        return sparse_range(last - first, val, ibuffer); 
    }

    /**
     * `vbuffer` may not necessarily be filled upon extraction if a pointer can be returned to the underlying data store.
     * This be checked by comparing the returned `sparse_range::value` pointer to `vbuffer`; if they are the same, `vbuffer` has been filled. 
     * The same applies for `ibuffer` and the returned `sparse_range::index` pointer.
     *
     * Values in `vbuffer` are not guaranteed to be non-zero.
     * If zeroes are explicitly initialized in the underlying representation, they will be reported here.
     * However, one can safely assume that all values _not_ in `vbuffer` are zero.
     *
     * If `work` is not a null pointer, it should have been generated by `new_workspace()` with `row = true`.
     * This is optional and should only affect the efficiency of extraction, not the contents.
     *
     * Setting `sorted = false` can reduce computational work in situations where the order of non-zero elements does not matter.
     *
     * @param c Index of the column.
     * @param vbuffer Pointer with enough space for at least `last - first` values.
     * @param ibuffer Pointer with enough space for at least `last - first` indices.
     * @param first First row to extract for column `c`.
     * @param last One past the last row to extract in `c`.
     * @param work Pointer to a workspace.
     * @param sorted Should the non-zero elements be sorted by their indices?
     *
     * @return A `sparse_range` object containing the number of non-zero elements in `c` from column `first` up to `last`.
     * This also contains pointers to their row indices and values.
     */
    virtual sparse_range<T, IDX> sparse_column(size_t c, T* vbuffer, IDX* ibuffer, size_t first, size_t last, workspace* work=nullptr, bool sorted=true) const {
        const T* val = column(c, vbuffer, first, last, work);
        for (size_t i = first; i < last; ++i) {
            ibuffer[i - first] = i;
        }
        return sparse_range(last - first, val, ibuffer); 
    }

    /**
     * See comments for `sparse_column()`, which are also applicable here.
     *
     * @param r Index of the row.
     * @param vbuffer Pointer with enough space for at least `ncol()` values.
     * @param ibuffer Pointer with enough space for at least `ncol()` indices.
     * @param work Pointer to a workspace.
     * @param sorted Should the non-zero elements be sorted by their indices?
     *
     * @return A `sparse_range` object containing the number of non-zero elements in `r`.
     * This also contains pointers to their column indices and values.
     */
    sparse_range<T, IDX> sparse_row(size_t r, T* vbuffer, IDX* ibuffer, workspace* work=nullptr, bool sorted=true) const {
        return sparse_row(r, vbuffer, ibuffer, 0, this->ncol(), work, sorted);
    }

    /**
     * See comments for `sparse_column()`, which are also applicable here.
     *
     * @param c Index of the column.
     * @param vbuffer Pointer with enough space for at least `nrow()` values.
     * @param ibuffer Pointer with enough space for at least `nrow()` indices.
     * @param work Pointer to a workspace.
     * @param sorted Should the non-zero elements be sorted by their indices?
     *
     * @return A `sparse_range` object containing the number of non-zero elements in `c`.
     * This also contains pointers to their row indices and values.
     */
    sparse_range<T, IDX> sparse_column(size_t c, T* vbuffer, IDX* ibuffer, workspace* work=nullptr, bool sorted=true) const {
        return sparse_column(c, vbuffer, ibuffer, 0, this->nrow(), work, sorted);
    }

    /**
     * @return A `content_type` specifying the type of the values in the matrix.
     * This is derived from `T` by default, see `determine_content_type()`.
     */
    content_type type() const { return determine_content_type<T>(); }

    typedef T value;
    typedef IDX index;
};

/**
 * A convenient shorthand for the most common use case of double-precision matrices.
 */
using numeric_matrix = typed_matrix<double, int>;

}

#endif
