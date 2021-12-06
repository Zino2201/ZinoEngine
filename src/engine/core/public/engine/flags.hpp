#pragma once

#include <type_traits>
#include <functional>

namespace ze
{

template<typename T>
	requires std::is_enum_v<T>
class Flags final
{
public:
	using MaskType = std::underlying_type_t<T>;

    constexpr Flags() noexcept : mask(0) {}
	constexpr ~Flags() noexcept = default;
	
    constexpr Flags(const Flags& in_other) noexcept : mask(in_other.mask) {}
    constexpr Flags(Flags&& in_other) noexcept : mask(std::move(in_other.mask)) {}
    constexpr explicit Flags(const T& in_bit) noexcept : mask(static_cast<MaskType>(in_bit)) {}
    constexpr explicit Flags(const MaskType& in_mask) noexcept : mask(in_mask) {}
	
	/** Bitwise operators */
    friend constexpr Flags<T> operator&(const Flags<T>& in_left, const Flags<T>& in_right)  noexcept
    {
        return Flags<T>(in_left.mask & in_right.mask);
    }

	friend constexpr Flags<T> operator|(const Flags<T>& in_left, const Flags<T>& in_right) noexcept
	{
		return Flags<T>(in_left.mask | in_right.mask);
	}

	friend constexpr Flags<T> operator^(const Flags<T>& in_left, const Flags<T>& in_right) noexcept
	{
		return Flags<T>(in_left.mask ^ in_right.mask);
	}

	constexpr Flags<T> operator~() const noexcept
	{
		return Flags<T>(~mask);
	}
	
	/** Compare operators */
	friend constexpr bool operator==(const Flags<T>& in_left, const Flags<T>& in_right) noexcept
	{
		return in_left.mask == in_right.mask;
	}

	friend constexpr bool operator!=(const Flags<T>& in_left, const Flags<T>& in_right) noexcept
	{
		return in_left.mask != in_right.mask;
	}

	/** Assignments operators */
	constexpr Flags<T>& operator=(const Flags<T>& in_other) noexcept
	{
		mask = in_other.mask;
		return *this;
	}

	constexpr Flags<T>& operator=(Flags<T>&& in_other) noexcept
	{
		mask = std::move(in_other.mask);
		return *this;
	}

	constexpr Flags<T>& operator|=(const Flags<T> & in_other) noexcept
	{
		mask |= in_other.mask;
		return *this;
	}

	constexpr Flags<T>& operator&=(const Flags<T> & in_other) noexcept
	{
		mask &= in_other.mask;
		return *this;
	}

	constexpr Flags<T>& operator^=(const Flags<T> & in_other) noexcept
	{
		mask ^= in_other.mask;
		return *this;
	}

	explicit constexpr operator MaskType() const noexcept
	{
		return mask;
	}

	constexpr explicit operator bool() const noexcept { return mask != 0; }
private:
	MaskType mask;
};

#define ZE_ENABLE_FLAG_ENUMS(EnumType, FlagsType) \
	constexpr auto operator&(const ze::Flags<EnumType>& in_left, EnumType in_right) noexcept \
	{ \
		return in_left & ze::Flags<EnumType>(in_right); \
	} \
	constexpr auto operator&(EnumType in_left, EnumType in_right) noexcept \
	{ \
		return ze::Flags<EnumType>(in_left) & ze::Flags<EnumType>(in_right); \
	} \
	constexpr auto operator|(EnumType in_left, EnumType in_right) noexcept \
	{ \
		return ze::Flags<EnumType>(in_left) | ze::Flags<EnumType>(in_right); \
	} \
	constexpr auto operator^(EnumType in_left, EnumType in_right) noexcept \
	{ \
		return ze::Flags<EnumType>(in_left) ^ ze::Flags<EnumType>(in_right); \
	} \
	using FlagsType = ze::Flags<EnumType>;

}

namespace std
{
	template<typename T> 
	struct hash<ze::Flags<T>>
	{
		uint64_t operator()(const ze::Flags<T>& in_flags) const
		{
			return std::hash<typename ze::Flags<T>::MaskType>()(static_cast<typename ze::Flags<T>::MaskType>(in_flags));
		}
	};
}