/******************************************************************************/
/**
 * @file        math.h
 * @project     Pulse Protocol
 * @author      Ban Kai Wei Benjamin
 * @brief       This file manages math library instances Vector2 and Matrix3x3
 *				with serialization/deserialization featuresling, linking, validating,
 *				and managing shader programs.
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once
#include "pch/pch_temp.h"
#include "CoreEngine/ECS/system.h"

//for .NET
#include "CoreEngine/Core/ImportExport.h" // for DLL_API, so that functions using vector2 in the editor will be exported and can be used

class DLL_API Vector2 : public Systems
{
public:
	// ctr and dtr
	Vector2();
	Vector2(float x_compo, float y_compo);
	Vector2(Vector2 const &v);
	~Vector2();

	// copy assignment operator= - assign vector to another
	Vector2 &operator=(Vector2 vec);

	// subscript operator[] - access element of vector
	float &operator[](size_t i);
	float operator[](size_t i) const;

	// arithmetic operators: vector operations +-
	Vector2 &operator+=(Vector2 vec);
	Vector2 &operator-=(Vector2 vec);

	// arithmetic operators: scalar operations */
	Vector2 &operator*=(float scalar);
	Vector2 &operator/=(float scalar);

	// arithmetic operators: negation
	Vector2 operator-(void) const;

	// length of vector
	float Len() const;

	// squared length of vector
	float LenSq() const;

	// normalization
	Vector2 Norm() const; // non-mutating, returns a normalized copy
	void Normalize();	  // mutating, normalizes in place
	//

	//comparison operator
	bool operator==(Vector2 const& other) const;
	bool operator!=(Vector2 const& other) const;

	//getter/setter
	float& setX();
	float& setY();
	float  getX() const;
	float  getY() const;
	Vector2& Set(float x, float y);

	// Serialize
	void Serialize(rapidjson::Value &out, rapidjson::Document::AllocatorType &alloc) const;
	void Deserialize(rapidjson::Value const &in);

private:
	float m[2];

public:
	// delegates
	float &x; //!< Access to X component.
	float &y; //!< Access to Y component.
}; // end of vector class

DLL_API float distance(Vector2 v0, Vector2 v1);
DLL_API float distanceSq(Vector2 v0, Vector2 v1);
DLL_API float dot(Vector2 v0, Vector2 v1);
DLL_API float cross(Vector2 v0, Vector2 v1);

DLL_API Vector2 operator+(Vector2 v0, Vector2 v1);
DLL_API Vector2 operator-(Vector2 v0, Vector2 v1);
DLL_API Vector2 operator*(Vector2 v, float s); // vector * scalar
DLL_API Vector2 operator*(float s, Vector2 v); // scalar * vector
DLL_API Vector2 operator/(Vector2 v, float s); // vector / scalar
DLL_API std::ostream &operator<<(std::ostream &os, Vector2 const &vec);

class DLL_API Matrix3x3
{
private:
	float m[9]; // column-major flat array
public:
	Matrix3x3(); // default identity
	Matrix3x3(float e00, float e10, float e20,
			  float e01, float e11, float e21,
			  float e02, float e12, float e22);
	Matrix3x3(Matrix3x3 const &m);
	~Matrix3x3();

	class ProxyClass
	{
	public:
		ProxyClass(Matrix3x3 &m, size_t c);
		ProxyClass(Matrix3x3 const &m, size_t c);

		float &operator[](size_t r);
		float const &operator[](size_t r) const;

	private:
		Matrix3x3 const *outer_; // pointer to const works for both
		size_t col_;
	};
	ProxyClass operator[](size_t c);
	ProxyClass const operator[](size_t c) const;

	// = operator: assignment
	Matrix3x3 &operator=(Matrix3x3 const &copied);
	// () operators: component access
	float &operator()(size_t column, size_t row);
	const float &operator()(size_t column, size_t row) const;
	// glm conversion operator
	explicit operator glm::mat3() const;

	// iterators
	float *Begin();
	const float *Begin() const;
	float *End();
	const float *End() const;

	void SetTo(float e00, float e10, float e20,
			   float e01, float e11, float e21,
			   float e02, float e12, float e22);
	void SetToZero();	  // zero all 9 elements
	void SetToIdentity(); // set 9 elements to define an identity matrix
	void SetToTranspose();
	Matrix3x3 GetTranspose() const;

	// Serialize
	void Serialize(rapidjson::Value &out, rapidjson::Document::AllocatorType &alloc) const;
	void Deserialize(rapidjson::Value const &in);

}; // end of matrix 3 class

// * operator: matrix multiplication
DLL_API Matrix3x3 operator*(Matrix3x3 const &l, Matrix3x3 const &r);

// * operator: scalar multiplication
DLL_API Matrix3x3 operator*(float s, Matrix3x3 const &mat);

DLL_API std::ostream &operator<<(std::ostream &os, Matrix3x3 const &mat3);