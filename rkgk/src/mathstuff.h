#pragma once
#include <cmath>

inline float distance(const ImVec2 p1, const ImVec2 p2)
{
	const auto dx = p2.x - p1.x;
	const auto dy = p2.y - p1.y;
	return std::sqrt(dx * dx + dy * dy);
}

inline float lerp(const float a, const float b, const float f)
{
	return a + f * (b - a);
}

class matrix3x2 {
public:
	matrix3x2() : m11(1), m12(0), m21(0), m22(1), m31(0), m32(0) {}
	matrix3x2(float a, float b, float c, float d, float e, float f) : m11(a), m12(b), m21(c), m22(d), m31(e), m32(f) {}

	void translate(float x, float y)
	{
		m31 += m11 * x + m21 * y;
		m32 += m12 * x + m22 * y;
	}

	matrix3x2 rotate(const float angle, const ImVec2 center) const
	{
		const float s = sin(angle);
		const float c = cos(angle);
		const float dx = (center.x * (1.0f - c)) + (center.y * s);
		const float dy = (center.y * (1.0f - c)) - (center.x * s);
		return { c, s,-s, c,dx, dy };
	}

	void scale(float scaleX, float scaleY) {
		m11 *= scaleX;
		m12 *= scaleY;
		m21 *= scaleX;
		m22 *= scaleY;
	}

	void scale_at(float scaleX, float scaleY, float originX, float originY) {
		m31 -= originX * (m11 * scaleX - m11);
		m32 -= originY * (m22 * scaleY - m22);
		m11 *= scaleX;
		m12 *= scaleY;
		m21 *= scaleX;
		m22 *= scaleY;
	}

	ImVec2 transform_vector(ImVec2 vec2) const {
		float tmpX = m11 * vec2.x + m21 * vec2.y + m31;
		float tmpY = m12 * vec2.x + m22 * vec2.y + m32;
		return { tmpX, tmpY };
	}

	matrix3x2 invert() const {
		float det = m11 * m22 - m12 * m21;
		if (det == 0.0f) {
			return {};
		}

		matrix3x2 inv;
		inv.m11 = m22 / det;
		inv.m12 = -m12 / det;
		inv.m21 = -m21 / det;
		inv.m22 = m11 / det;
		inv.m31 = (m21 * m32 - m31 * m22) / det;
		inv.m32 = (m31 * m12 - m11 * m32) / det;
		return inv;
	}

	matrix3x2 operator*(const matrix3x2& other) const {
		return {
			m11 * other.m11 + m21 * other.m12,
			m12 * other.m11 + m22 * other.m12,
			m11 * other.m21 + m21 * other.m22,
			m12 * other.m21 + m22 * other.m22,
			m11 * other.m31 + m21 * other.m32 + m31,
			m12 * other.m31 + m22 * other.m32 + m32
		};
	}


	float m11, m12, m21, m22, m31, m32;
};
