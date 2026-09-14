/******************************************************************************/
/**
 * @file        math.cpp
 * @project     Pulse Protocol
 * @author      Ban Kai Wei Benjamin
 * @brief       This file manages math library instances Vector2 and Matrix3x3
 *              with serialization/deserialization featuresling, linking, validating,
 *              and managing shader programs.
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#include "math.h"

/************************************************* */
/********************ctor/dtor******************** */
/************************************************* */
// default ctor
Vector2::Vector2() : m{0.0f, 0.0f},
                     x(m[0]), y(m[1])
{ /*empty by design*/
}

// 2 float param ctor
Vector2::Vector2(float x_, float y_) : m{x_, y_},
                                       x(m[0]), y(m[1])
{ /*empty by design*/
}

// copy ctor
Vector2::Vector2(Vector2 const &v) : m{v.m[0], v.m[1]},
                                     x(m[0]), y(m[1])
{ /*empty by design*/
}

// default dtor
Vector2::~Vector2() = default;

/************************************************* */
/*************member functions******************** */
/************************************************* */

// copy assignment
Vector2 &Vector2::operator=(Vector2 v)
{
    if (this != &v)
    {
        m[0] = v.m[0];
        m[1] = v.m[1];
    }
    return *this;
}

// element access
float &Vector2::operator[](size_t i)
{
    assert(i < 2);
    return m[i];
}
float Vector2::operator[](size_t i) const
{
    assert(i < 2);
    return m[i];
}

Vector2 &Vector2::operator+=(Vector2 v)
{
    m[0] += v.m[0];
    m[1] += v.m[1];
    return *this;
}
Vector2 &Vector2::operator-=(Vector2 v)
{
    m[0] -= v.m[0];
    m[1] -= v.m[1];
    return *this;
}

Vector2 &Vector2::operator*=(float s)
{
    m[0] *= s;
    m[1] *= s;
    return *this;
}
Vector2 &Vector2::operator/=(float s)
{
    assert(s != 0.0f); // divison by zero
    m[0] /= s;
    m[1] /= s;
    return *this;
}

// unary minus
Vector2 Vector2::operator-() const
{
    return Vector2(-m[0], -m[1]);
}

// length / normalization
float Vector2::Len() const
{
    return std::sqrt(m[0] * m[0] + m[1] * m[1]);
}
float Vector2::LenSq() const
{
    return m[0] * m[0] + m[1] * m[1];
}

Vector2 Vector2::Norm() const
{
    float L = Len();
    return (L > 0.0f) ? Vector2(m[0] / L, m[1] / L) : *this;
}

void Vector2::Normalize()
{
    float L = Len();
    if (L > 0.0f)
    {
        *this /= L;
    }
}

bool Vector2::operator==(Vector2 const& other) const {
    return x == other.x && y == other.y;
}
bool Vector2::operator!=(Vector2 const& other) const {
    return !(*this == other);
}

float& Vector2::setX() { 
    return m[0]; 
}
float& Vector2::setY() { 
    return m[1]; 
}
float  Vector2::getX() const { 
    return m[0]; 
}
float  Vector2::getY() const { 
    return m[1]; 
}
Vector2& Vector2::Set(float x_, float y_)
{
    m[0] = x_;
    m[1] = y_;
    return *this;
}
//non-member functions
float distance(Vector2 v0, Vector2 v1)
{
    float dx = v1.x - v0.x;
    float dy = v1.y - v0.y;
    return std::sqrt((dx * dx) + (dy * dy));
}

float distanceSq(Vector2 v0, Vector2 v1)
{
    float dx = v1.x - v0.x;
    float dy = v1.y - v0.y;
    return (dx * dx) + (dy * dy);
}

float dot(Vector2 v0, Vector2 v1)
{
    return (v0.x * v1.x) + (v0.y * v1.y);
}

float cross(Vector2 v0, Vector2 v1)
{
    return (v0.x * v1.y) - (v0.y * v1.x);
}

Vector2 operator+(Vector2 v0, Vector2 v1)
{
    Vector2 tmp{v0};
    return tmp += v1;
}
Vector2 operator-(Vector2 v0, Vector2 v1)
{
    Vector2 tmp{v0};
    return tmp -= v1;
}
Vector2 operator*(Vector2 v, float s)
{ // vector * scalar
    Vector2 tmp{v};
    return tmp *= s;
}
Vector2 operator*(float s, Vector2 v)
{ // scalar * vector
    Vector2 tmp{v};
    return tmp *= s;
}
Vector2 operator/(Vector2 v, float s)
{
    Vector2 tmp{v};
    return tmp /= s;
}



std::ostream &operator<<(std::ostream &os, Vector2 const &vec)
{
    os << std::fixed << std::setprecision(3);
    return os << "Vector2(" << vec[0] << ", " << vec[1] << ")";
}

/* ==============================MAT3 =======================*/
Matrix3x3::Matrix3x3() { SetToIdentity(); }

Matrix3x3::Matrix3x3(float e00, float e10, float e20,
                     float e01, float e11, float e21,
                     float e02, float e12, float e22)
{
    m[0] = e00;
    m[1] = e10;
    m[2] = e20;
    m[3] = e01;
    m[4] = e11;
    m[5] = e21;
    m[6] = e02;
    m[7] = e12;
    m[8] = e22;
}
Matrix3x3::Matrix3x3(Matrix3x3 const &copied)
{
    std::memcpy(m, copied.Begin(), sizeof(m));
}

Matrix3x3::~Matrix3x3() = default;

Matrix3x3 &Matrix3x3::operator=(Matrix3x3 const &other)
{
    if (this != &other)
    {
        std::memcpy(m, other.Begin(), sizeof(m));
    }
    return *this;
}

// () operators: component access
float &Matrix3x3::operator()(size_t column, size_t row)
{
    assert(column < 3 && row < 3);
    return m[column * 3 + row];
}
const float &Matrix3x3::operator()(size_t column, size_t row) const
{
    assert(column < 3 && row < 3);
    return m[column * 3 + row];
}

// iterators
float *Matrix3x3::Begin() { return &m[0]; }
float const *Matrix3x3::Begin() const { return &m[0]; }
float *Matrix3x3::End() { return &m[9]; }
float const *Matrix3x3::End() const { return &m[9]; }

void Matrix3x3::SetTo(float e00, float e10, float e20,
                      float e01, float e11, float e21,
                      float e02, float e12, float e22)
{
    m[0] = e00;
    m[1] = e10;
    m[2] = e20;
    m[3] = e01;
    m[4] = e11;
    m[5] = e21;
    m[6] = e02;
    m[7] = e12;
    m[8] = e22;
}

void Matrix3x3::SetToIdentity()
{
    m[0] = 1.0f;
    m[1] = 0.0f;
    m[2] = 0.0f; // col 0
    m[3] = 0.0f;
    m[4] = 1.0f;
    m[5] = 0.0f; // col 1
    m[6] = 0.0f;
    m[7] = 0.0f;
    m[8] = 1.0f; // col 2
}

void Matrix3x3::SetToZero()
{
    m[0] = 0.0f;
    m[1] = 0.0f;
    m[2] = 0.0f; // col 0
    m[3] = 0.0f;
    m[4] = 0.0f;
    m[5] = 0.0f; // col 1
    m[6] = 0.0f;
    m[7] = 0.0f;
    m[8] = 0.0f; // col 2
}

void Matrix3x3::SetToTranspose()
{
    for (unsigned int i = 0; i < 3; ++i)
    {
        for (unsigned int j = i + 1; j < 3; ++j)
        {
            std::swap((*this)(i, j), (*this)(j, i));
        }
    }
}

Matrix3x3 Matrix3x3::GetTranspose() const
{
    Matrix3x3 result{};
    for (unsigned int i = 0; i < 3; ++i)
    {
        for (unsigned int j = 0; j < 3; ++j)
        {

            result(i, j) = (*this)(j, i);
        }
    }
    return result;
}

// * operator: matrix multiplication
Matrix3x3 operator*(Matrix3x3 const &L, Matrix3x3 const &R)
{

    Matrix3x3 tmp{};

    for (size_t col = 0; col < 3; ++col)
    {
        for (size_t row = 0; row < 3; ++row)
        {
            float sum = 0.0f;
            for (size_t k = 0; k < 3; ++k)
            {
                sum += L[k][row] * R[col][k];
            }
            tmp[col][row] = sum;
        }
    }

    return tmp;
}

// * operator: scalar multiplication
Matrix3x3 operator*(float s, Matrix3x3 const &mat)
{
    Matrix3x3 tmp{mat};
    for (float *p = tmp.Begin(); p != tmp.End(); ++p)
    {
        *p *= s;
    }
    return tmp;
}

Matrix3x3::ProxyClass::ProxyClass(Matrix3x3 &m, size_t c) : outer_(&m), col_(c) {}
Matrix3x3::ProxyClass::ProxyClass(Matrix3x3 const &m, size_t c) : outer_(&m), col_(c) {}

float &Matrix3x3::ProxyClass::operator[](size_t r)
{
    // access element at (col = c, row = inner_mat_row)
    // return outer_mat.m[inner_mat_col * 3 + r];
    return const_cast<float &>((*outer_)(col_, r));
}

float const &Matrix3x3::ProxyClass::operator[](size_t r) const
{
    return (*outer_)(col_, r);
}

typename Matrix3x3::ProxyClass Matrix3x3::operator[](size_t c)
{
    return ProxyClass(*this, c); // explicit *this for readability
}

typename Matrix3x3::ProxyClass const Matrix3x3::operator[](size_t c) const
{
    return ProxyClass(static_cast<Matrix3x3 const &>(*this), c); // allows const element access
}

Matrix3x3::operator glm::mat3() const
{
    return glm::mat3(
        m[0], m[3], m[6],
        m[1], m[4], m[7],
        m[2], m[5], m[8]);
}

std::ostream &operator<<(std::ostream &os, Matrix3x3 const & /*M*/)
{
    // Disabled matrix printing for performance
    /*
    os << "Printing Matrix\n";
    for (unsigned int row = 0; row < 3; ++row)
    {
        os << "[ ";
        for (unsigned int col = 0; col < 3; ++col)
        {
            os << M(col, row) << ' ';
        }
        os << "]\n";
    }
    */
    return os;
}

/*Serialization && Deserialization */
// vector2
void Vector2::Serialize(rapidjson::Value &out, rapidjson::Document::AllocatorType &alloc) const
{
    // using namespace rapidjson;
    out.SetObject();
    out.AddMember("x", x, alloc);
    out.AddMember("y", y, alloc);
}

void Vector2::Deserialize(rapidjson::Value const &in)
{
    // not an obj and doesnt have the members x y
    if (!in.IsObject() || !in.HasMember("x") || !in.HasMember("y"))
    {
        throw std::runtime_error("Invalid JSON for Vector2");
    }
    x = in["x"].GetFloat();
    y = in["y"].GetFloat();
}

// matrix3
void Matrix3x3::Serialize(rapidjson::Value &out, rapidjson::Document::AllocatorType &alloc) const
{
    out.SetArray();
    for (int i = 0; i < 9; ++i)
    {
        out.PushBack(m[i], alloc);
    }
}

void Matrix3x3::Deserialize(rapidjson::Value const &in)
{
    if (!in.IsArray() || in.Size() != 9)
    {
        throw std::runtime_error("Invalid JSON for Matrix3x3");
    }
    for (int i = 0; i < 9; ++i)
    {
        m[i] = in[i].GetFloat();
    }
}