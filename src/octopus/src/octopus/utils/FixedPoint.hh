#ifndef __FixedPoint__
#define __FixedPoint__

#include <iostream>
#include <limits>
#include <cstdint>

namespace octopus
{
	/// @brief store a real x e as an integer
	template<int64_t e>
	struct FixedPoint
	{
	public:
		static constexpr int64_t OneAsLong() { return e; }
		static constexpr FixedPoint<e> Zero() { FixedPoint<e> zero_l(0, true); return zero_l; }
		static constexpr FixedPoint<e> One() { FixedPoint<e> one_l(e, true); return one_l; }
		static constexpr FixedPoint<e> MinusOne() { FixedPoint<e> one_l(-e, true); return one_l; }

		template<class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
		FixedPoint(Number const &val, bool internal_p=false) : _data(internal_p?val:val*e) {}
		FixedPoint(FixedPoint<e> const &f) = default;
		FixedPoint(FixedPoint<e> &&f) = default;
		FixedPoint &operator=(FixedPoint<e> const &f) = default;
		FixedPoint &operator=(FixedPoint<e> &&f) = default;
		FixedPoint() : _data(0) {}

		FixedPoint<e> operator*(FixedPoint<e> const &f) const
		{
			int64_t tmp_l = _data * f._data;
			return FixedPoint<e>(tmp_l/e, true);
		}
		FixedPoint<e> operator/(FixedPoint<e> const &f) const
		{
			int64_t tmp_l = e * _data;
			return FixedPoint<e>(tmp_l/f._data, true);
		}
		FixedPoint<e> operator+(FixedPoint<e> const &f) const
		{
			return FixedPoint<e>(_data + f._data, true);
		}
		FixedPoint<e> operator-(FixedPoint<e> const &f) const
		{
			return FixedPoint<e>(_data - f._data, true);
		}
		FixedPoint<e> operator-() const
		{
			return FixedPoint<e>(-_data, true);
		}

		FixedPoint<e> &operator*=(FixedPoint<e> const &f)
		{
			_data *= f._data;
			_data /= e;
			return *this;
		}
		FixedPoint<e> &operator/=(FixedPoint<e> const &f)
		{
			_data *= e;
			_data /= f._data;
			return *this;
		}
		FixedPoint<e> &operator+=(FixedPoint<e> const &f)
		{
			_data += f._data;
			return *this;
		}
		FixedPoint<e> &operator-=(FixedPoint<e> const &f)
		{
			_data -= f._data;
			return *this;
		}

		bool operator>(FixedPoint<e> const &f) const
		{
			return _data > f._data;
		}

		bool operator<(FixedPoint<e> const &f) const
		{
			return _data < f._data;
		}

		bool operator>=(FixedPoint<e> const &f) const
		{
			return _data >= f._data;
		}

		bool operator<=(FixedPoint<e> const &f) const
		{
			return _data <= f._data;
		}

		bool operator==(FixedPoint<e> const &f) const
		{
			return _data == f._data;
		}

		bool operator!=(FixedPoint<e> const &f) const
		{
			return _data != f._data;
		}

		template <class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
		FixedPoint<e> operator*(Number const &f) const
		{
			int64_t tmp_l = _data * f;
			return FixedPoint<e>(tmp_l, true);
		}
		template <class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
		FixedPoint<e> operator/(Number const &f) const
		{
			return FixedPoint<e>(_data/f, true);
		}
		template <class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
		FixedPoint<e> operator+(Number const &f) const
		{
			return FixedPoint<e>(_data + f*e, true);
		}
		template <class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
		FixedPoint<e> operator-(Number const &f) const
		{
			return FixedPoint<e>(_data - f*e, true);
		}

		template <class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
		FixedPoint<e> &operator*=(Number const &f)
		{
			_data = static_cast<int64_t>(_data * f);
			return *this;
		}
		template <class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
		FixedPoint<e> &operator/=(Number const &f)
		{
			_data = static_cast<int64_t>(_data / f);
			return *this;
		}
		template <class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
		FixedPoint<e> &operator+=(Number const &f)
		{
			_data += static_cast<int64_t>(f*e);
			return *this;
		}
		template <class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
		FixedPoint<e> &operator-=(Number const &f)
		{
			_data -= static_cast<int64_t>(f*e);
			return *this;
		}

		template <class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
		bool operator>(Number const &f) const
		{
			return _data > static_cast<int64_t>(f*e);
		}

		template <class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
		bool operator<(Number const &f) const
		{
			return _data < static_cast<int64_t>(f*e);
		}

		template <class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
		bool operator>=(Number const &f) const
		{
			return _data >= static_cast<int64_t>(f*e);
		}

		template <class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
		bool operator<=(Number const &f) const
		{
			return _data <= static_cast<int64_t>(f*e);
		}

		template <class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
		bool operator==(Number const &f) const
		{
			return _data == static_cast<int64_t>(f*e);
		}

		template <class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
		bool operator!=(Number const &f) const
		{
			return _data != static_cast<int64_t>(f*e);
		}

		int64_t to_int() const
		{
			return _data/e;
		}

		uint64_t to_uint() const
		{
			return std::abs(_data/e);
		}

		double to_double() const
		{
			return _data/double(e);
		}

		int64_t data() const
		{
			return _data;
		}

		int64_t _data;
	};

	template<typename Fixed>
	double to_double(Fixed const &f)
	{
		return f.to_double();
	}

	template<typename Fixed>
	int64_t to_int(Fixed const &f)
	{
		return f.to_int();
	}

	namespace numeric
	{

	template <int64_t e>
	octopus::FixedPoint<e> square_root(octopus::FixedPoint<e> const &v)
	{
		// quick win for exact match
		if(v == 0)
		{
			return 0;
		}
		octopus::FixedPoint<e> res_l(e, true);
		static octopus::FixedPoint<e> epsilon_l(100, true);
		static octopus::FixedPoint<e> mepsilon_l(-100, true);

		int i = 0;
		octopus::FixedPoint<e> delta_l = res_l * res_l - v;
		while(i < 100 && (delta_l < mepsilon_l || delta_l > epsilon_l))
		{
			res_l = (res_l + v / res_l) * octopus::FixedPoint<e>(e/2, true);
			delta_l = res_l * res_l - v;
			++i;
		}
		return res_l;
	}

	template <int64_t e>
	octopus::FixedPoint<e> abs(octopus::FixedPoint<e> const &f)
	{
		return octopus::FixedPoint<e>(std::abs(f.data()), true);
	}

	template <int64_t e>
	octopus::FixedPoint<e> ceil(octopus::FixedPoint<e> const &f)
	{
		int64_t data_l = f.data() + e - 1;
		return octopus::FixedPoint<e>((data_l/e)*e, true);
	}

	template <int64_t e>
	octopus::FixedPoint<e> floor(octopus::FixedPoint<e> const &f)
	{
		return octopus::FixedPoint<e>((f.data()/e)*e, true);
	}

	template<class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
	Number infinity()
	{
		return std::numeric_limits<Number>::infinity();
	}

	template<class Fixed>
	Fixed infinity()
	{
		return Fixed(std::numeric_limits<int64_t>::max(), true);
	}

	} // namespace numeric

	using Fixed = FixedPoint<10000>;

} // octopus

template <int64_t e, class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
octopus::FixedPoint<e> operator*(Number const &f, octopus::FixedPoint<e> const &fp)
{
	return fp * f;
}
template <int64_t e, class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
octopus::FixedPoint<e> operator/(Number const &f, octopus::FixedPoint<e> const &fp)
{
	return octopus::FixedPoint<e>(f) / fp;
}
template <int64_t e, class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
octopus::FixedPoint<e> operator+(Number const &f, octopus::FixedPoint<e> const &fp)
{
	return fp + f;
}
template <int64_t e, class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
octopus::FixedPoint<e> operator-(Number const &f, octopus::FixedPoint<e> const &fp)
{
	return -fp + f;
}

template <int64_t e, class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
bool operator>(Number const &f, octopus::FixedPoint<e> const &fp)
{
	return fp < f;
}

template <int64_t e, class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
bool operator<(Number const &f, octopus::FixedPoint<e> const &fp)
{
	return fp > f;
}

template <int64_t e, class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
bool operator>=(Number const &f, octopus::FixedPoint<e> const &fp)
{
	return fp <= f;
}

template <int64_t e, class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
bool operator<=(Number const &f, octopus::FixedPoint<e> const &fp)
{
	return fp >= f;
}

template <int64_t e, class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
bool operator==(Number const &f, octopus::FixedPoint<e> const &fp)
{
	return fp == f;
}

template <int64_t e, class Number, class = typename std::enable_if<std::is_arithmetic<Number>::value>::type>
bool operator!=(Number const &f, octopus::FixedPoint<e> const &fp)
{
	return fp != f;
}

namespace std
{
template <int64_t e>
std::ostream &operator<<(std::ostream &os, octopus::FixedPoint<e> const &f) {
	os << f.to_double();
	return os;
}

template <int64_t e>
octopus::FixedPoint<e> abs(octopus::FixedPoint<e> const &f)
{
	if(f < 0)
	{
		return octopus::FixedPoint<e>(-f._data, true);
	}
	return f;
}

template <int64_t e>
octopus::FixedPoint<e> sqrt(octopus::FixedPoint<e> const &f)
{
	return octopus::numeric::square_root(f);
}

}

template <int64_t e>
octopus::FixedPoint<e> sqr(octopus::FixedPoint<e> const &f) {
	return f*f;
}

double sqr(double const &f);

template <int64_t e>
bool is_zero(octopus::FixedPoint<e> const &f) {
	return std::abs(f.data()) < 1;
}

bool is_zero(double const &f);



#endif
