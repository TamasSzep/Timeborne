// Timeborne/Math/SqrtExtendedIntegerRing.hpp

#pragma once

#include <EngineBuildingBlocks/Math/GLM.h>

struct Sqrt2IntegerRingExtender
{
	static constexpr int Sqr = 2;
	static constexpr float Value = 1.4142135623730950488016887242097f;
};

template <typename Extender>
struct SqrtExtendedIntegerRing
{
	int Integer;
	int Ext;

	constexpr inline SqrtExtendedIntegerRing() : Integer(0), Ext(0) {}
	explicit SqrtExtendedIntegerRing(glm::ctor) {}
	inline constexpr explicit SqrtExtendedIntegerRing(int i) : Integer(i), Ext(0) {}
	inline constexpr SqrtExtendedIntegerRing(int i, int s) : Integer(i), Ext(s) {}

	inline explicit constexpr operator float() const
	{
		return Integer + Ext * Extender::Value;
	}

	inline constexpr bool operator==(const SqrtExtendedIntegerRing& other) const
	{
		return Integer == other.Integer && Ext == other.Ext;
	}

	inline constexpr bool operator!=(const SqrtExtendedIntegerRing& other) const
	{
		return !(*this == other);
	}

	inline constexpr bool operator<(const SqrtExtendedIntegerRing& other) const
	{
		return ((float)* this) < ((float)other);
	}

	inline constexpr bool operator<=(const SqrtExtendedIntegerRing& other) const
	{
		return ((float)* this) <= ((float)other);
	}

	inline constexpr bool operator>(const SqrtExtendedIntegerRing& other) const
	{
		return ((float)* this) > ((float)other);
	}

	inline constexpr bool operator>=(const SqrtExtendedIntegerRing& other) const
	{
		return ((float)* this) >= ((float)other);
	}

	inline constexpr SqrtExtendedIntegerRing operator+(const SqrtExtendedIntegerRing& other) const
	{
		return { Integer + other.Integer, Ext + other.Ext };
	}

	inline constexpr SqrtExtendedIntegerRing operator-(const SqrtExtendedIntegerRing& other) const
	{
		return { Integer - other.Integer, Ext - other.Ext };
	}

	inline constexpr SqrtExtendedIntegerRing operator-() const
	{
		return { -Integer, -Ext };
	}

	inline constexpr SqrtExtendedIntegerRing operator*(const int s) const
	{
		return { Integer * s, Ext * s };
	}

	inline constexpr SqrtExtendedIntegerRing operator*(const SqrtExtendedIntegerRing& other) const
	{
		return { Integer * other.Integer + Ext * other.Ext * Extender::Sqr , Integer * other.Ext + Ext * other.Integer };
	}

	inline constexpr SqrtExtendedIntegerRing& operator+=(const SqrtExtendedIntegerRing& other) const
	{
		*this = *this + other;
		return *this;
	}

	inline constexpr SqrtExtendedIntegerRing& operator-=(const SqrtExtendedIntegerRing& other) const
	{
		*this = *this - other;
		return *this;
	}

	inline constexpr SqrtExtendedIntegerRing& operator*=(const int s) const
	{
		*this = *this * s;
		return *this;
	}

	inline constexpr SqrtExtendedIntegerRing& operator*=(const SqrtExtendedIntegerRing& other) const
	{
		*this = *this * other;
		return *this;
	}

	static constexpr SqrtExtendedIntegerRing Zero() { return SqrtExtendedIntegerRing(); }
	static constexpr SqrtExtendedIntegerRing One() { return SqrtExtendedIntegerRing(1); }
};