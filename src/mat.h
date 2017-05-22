
#ifndef __MAT_H__
#define __MAT_H__ 1

/*
 * mat[n_rows][n_cols]:
 * mat[0][0] mat[0][1] ... mat[0][n_cols]
 * mat[1][0] ...
 * mat[n_rows-1] ...       mat[n_rows][n_cols]
 */
template<typename T>
class Mat
{
 public:
  GF<T> *gf;
  int n_rows;
  int n_cols;
  T *mem;
#define MAT_ITEM(mat, i, j) ((mat)->mem[(i) * (mat)->n_cols + (j)])
  Mat(GF<T> *gf, int n_rows, int n_cols);
  void zero_fill(void);
  void inv(void);
  void mult(Vec<T> *output, Vec<T> *v);
  void vandermonde(void);
  void vandermonde_suitable_for_ec(void);
  void cauchy(void);
  void dump(void);
 private:
  void swap_rows(int row0, int row1);
  void mul_row(int row, T factor);
  void add_rows(int src_row, int dst_row, T factor);
  void reduced_row_echelon_form(void);
  void ec_transform1(int i);
  void ec_transform2(int i, int j);
  bool row_is_identity(int row);
};

#endif