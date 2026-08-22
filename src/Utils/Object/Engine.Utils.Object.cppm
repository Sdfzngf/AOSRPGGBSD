/**
 * @brief Object
 *
 */
module;

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

export module Engine.Utils.Object;

export namespace Engine::Utils::Object {
template <typename T, size_t Rows, size_t Cols>
struct Matrix;

template <typename T, size_t Rows, size_t Cols, size_t RowIdx, size_t ColIdx>
struct MatrixFiller {
    static constexpr void fill(Matrix<T, Rows, Cols>& mat, const T& val)
    {
        mat.data[RowIdx][ColIdx] = val;
        // 更新索引：先增加列，列满则换行
        constexpr size_t nextRow = (ColIdx + 1 == Cols) ? RowIdx + 1 : RowIdx;
        constexpr size_t nextCol = (ColIdx + 1 == Cols) ? 0 : ColIdx + 1;
        MatrixFiller<T, Rows, Cols, nextRow, nextCol>::fill(mat, val);
    }
};

template <typename T, size_t Rows, size_t Cols>
struct MatrixFiller<T, Rows, Cols, Rows, 0> {
    static constexpr void fill(Matrix<T, Rows, Cols>&, const T&) { }
};

template <typename T, size_t N, size_t RowIdx>
struct IdentityBuilder {
    static constexpr void build(Matrix<T, N, N>& mat)
    {
        mat.data[RowIdx][RowIdx] = T(1);
        IdentityBuilder<T, N, RowIdx + 1>::build(mat);
    }
};

template <typename T, size_t N>
struct IdentityBuilder<T, N, N> {
    static constexpr void build(Matrix<T, N, N>&) { }
};

template <typename T, size_t Rows, size_t Cols>
struct Matrix {
    std::array<std::array<T, Cols>, Rows> data { };

    constexpr auto operator()(size_t row, size_t col) -> T&
    {
        return data[row][col];
    }
    constexpr auto operator()(size_t row, size_t col) const -> const T&
    {
        return data[row][col];
    }

    static constexpr auto filled(const T& val) -> Matrix
    {
        Matrix mat { };
        MatrixFiller<T, Rows, Cols, 0, 0>::fill(mat, val);
        return mat;
    }

    static constexpr auto identity() -> Matrix
    {
        static_assert(Rows == Cols, "单位矩阵仅适用于方阵");
        Matrix mat { };
        IdentityBuilder<T, Rows, 0>::build(mat);
        return mat;
    }

    [[nodiscard]] constexpr auto operator[](size_t row) -> std::array<T, Cols>&
    {
        return data.at(row);
    }

    [[nodiscard]] constexpr auto operator[](size_t row) const -> const std::array<T, Cols>&
    {
        return data.at(row);
    }

    [[nodiscard]] constexpr auto at(size_t row) -> std::array<T, Cols>&
    {
        return data.at(row);
    }

    [[nodiscard]] constexpr auto at(size_t row) const -> const std::array<T, Cols>&
    {
        return data.at(row);
    }
};

template <typename T, size_t Rows, size_t Cols, size_t Index>
struct MatrixAdder {
    static constexpr void apply(const Matrix<T, Rows, Cols>& a,
                                const Matrix<T, Rows, Cols>& b,
                                Matrix<T, Rows, Cols>& res)
    {
        constexpr size_t row = Index / Cols;
        constexpr size_t col = Index % Cols;
        res.data[row][col] = a.data[row][col] + b.data[row][col];
        MatrixAdder<T, Rows, Cols, Index + 1>::apply(a, b, res);
    }
};

template <typename T, size_t Rows, size_t Cols>
struct MatrixAdder<T, Rows, Cols, Rows * Cols> {
    static constexpr void apply(const Matrix<T, Rows, Cols>&,
                                const Matrix<T, Rows, Cols>&,
                                Matrix<T, Rows, Cols>&) { }
};

template <typename T, size_t Rows, size_t Cols>
constexpr auto operator+(const Matrix<T, Rows, Cols>& a,
                         const Matrix<T, Rows, Cols>& b) -> Matrix<T, Rows, Cols>
{
    Matrix<T, Rows, Cols> result;
    MatrixAdder<T, Rows, Cols, 0>::apply(a, b, result);
    return result;
}

class ObjectExt {
};

// 右手坐标系下
class Object_Base {
private:
    using mt_type = double;
    // X,Y
    long double X { 0 }, Y { 0 };
    // Z，深度，在二维模式下Z轴方向垂直屏幕向外
    long double Z { 0 };
    // 绕三个轴的旋转,2D下只有R_Z有用，其余虽然不会用到但依然会被更改
    double R_X { 0 }, R_Y { 0 }, R_Z { 0 };
    // 当前的旋转矩阵
    Matrix<mt_type, 3, 3> mat { { { { 0, 0, 0 },
                                    { 0, 0, 0 },
                                    { 0, 0, 0 } } } };

    auto RotationX(double angle) -> Matrix<mt_type, 3, 3>
    {
        double c = cos(angle), s = sin(angle);
        return { { { { 1, 0, 0 },
                     { 0, c, -s },
                     { 0, s, c } } } };
    }

    auto RotationY(double angle) -> Matrix<mt_type, 3, 3>
    {
        double c = cos(angle), s = sin(angle);
        return { { { { c, 0, s },
                     { 0, 1, 0 },
                     { -s, 0, c } } } };
    }

    auto RotationZ(double angle) -> Matrix<mt_type, 3, 3>
    {
        double c = cos(angle), s = sin(angle);
        return { { { { c, -s, 0 },
                     { s, c, 0 },
                     { 0, 0, 1 } } } };
    }

    auto multiply(const Matrix<mt_type, 3, 3>& a, const Matrix<mt_type, 3, 3>& b) -> Matrix<mt_type, 3, 3>
    {
        Matrix<mt_type, 3, 3> result = { };
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                for (int k = 0; k < 3; ++k)
                    result.at(i).at(j) += b.at(k).at(j) * a.at(i).at(k);
        return result;
    }

    auto UpdateMat() -> void
    {
        Matrix<mt_type, 3, 3> R = RotationZ(R_Z);
        R = multiply(R, RotationY(R_Y));
        mat = multiply(R, RotationX(R_X));
    }

public:
    Object_Base(const Object_Base&) = default;
    Object_Base(Object_Base&&) = default;
    auto operator=(const Object_Base&) -> Object_Base& = default;
    auto operator=(Object_Base&&) -> Object_Base& = default;
    ~Object_Base() = default;
    // 绕X轴旋转
    auto RX(double a) -> void
    {
        R_X = a;
        UpdateMat();
    }
    // 绕Y轴旋转
    auto RY(double a) -> void
    {
        R_Y = a;
        UpdateMat();
    }
    // 绕Z轴旋转
    auto RZ(double a) -> void
    {
        R_Z = a;
        UpdateMat();
    }
    // 绕XY轴旋转
    auto RXY(double xa, double ya) -> void
    {
        R_X = xa;
        R_Y = ya;
        UpdateMat();
    }
    // 绕XZ轴旋转
    auto RXZ(double xa, double za) -> void
    {
        R_X = xa;
        R_Z = za;
        UpdateMat();
    }
    // 绕YZ轴旋转
    auto RYZ(double ya, double za) -> void
    {
        R_Y = ya;
        R_Z = za;
        UpdateMat();
    }
    // 绕XZ轴旋转
    auto RXYZ(double xa, double ya, double za) -> void
    {
        R_X = xa;
        R_Y = ya;
        R_Z = za;
        UpdateMat();
    }
};
}
