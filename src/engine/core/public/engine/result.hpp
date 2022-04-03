#pragma once

#include "engine/core.hpp"
#include <variant>

namespace ze
{

namespace detail
{

/**
 * This is used to help template type deduction
 */
struct MakeResultValue {};
struct MakeResultError {};
struct ErrorTag {};
	
}

inline static detail::ErrorTag error_tag;
	
/**
 * std::excepted/Rust's std::Result inspired type to manage easily errors
 */
template<typename T, typename E>
class Result final
{
	using ValueType = std::decay_t<T>;
	using ErrorType = std::decay_t<E>;
	
public:
	Result(const ValueType& in_value) : value(in_value), error_handled(false) {}
	Result(ValueType&& in_value) : value(std::move(in_value)), error_handled(false) {}
	Result(const ErrorType& in_error, detail::ErrorTag in_tag = error_tag) : value(in_error), error_handled(false) { (void)(in_tag); }
	Result(ErrorType&& in_error, detail::ErrorTag in_tag = error_tag) : value(std::move(in_error)), error_handled(false) { (void)(in_tag); }
	~Result()
	{
		ZE_ASSERTF(value.index() == 0 || value.index() == 1 && error_handled, "Unhandled Result error !");
	}
	
	/** Default move ctor */
	Result(Result<T, E>&&) noexcept
		requires (!std::is_same_v<T, detail::MakeResultValue> && !std::is_same_v<E, detail::MakeResultError>) = default;

	/** Special construct for make_result to make code less cluttered */
	Result(Result<ValueType, detail::MakeResultError>&& in_other) noexcept
		requires (!std::is_same_v<T, detail::MakeResultValue>) : value(std::move(in_other.get_value())), error_handled(false) {}

    Result(Result<detail::MakeResultValue, E>&& in_other) noexcept
		requires (!std::is_same_v<E, detail::MakeResultError>) : value(std::move(in_other.get_error())), error_handled(false) {}

	/** unique_ptr support */
	template<typename U>
    Result(Result<std::unique_ptr<U>, detail::MakeResultError>&& in_other) noexcept
		requires (!std::is_same_v<T, detail::MakeResultValue>) : value(std::move(in_other.get_value())), error_handled(false) {}
	
	Result& operator=(const Result&) = default;
	Result& operator=(Result&&) noexcept = default;

	[[nodiscard]] const ValueType& get_value() const &
	{
		ZE_CHECKF(value.index() == 0, "Value is not available!");
		return std::get<0>(value);
	}

	[[nodiscard]] ValueType& get_value() &
	{
		ZE_CHECKF(value.index() == 0, "Value is not available!");
		return std::get<0>(value);
	}

	[[nodiscard]] ValueType&& get_value() &&
	{
		ZE_CHECKF(value.index() == 0, "Value is not available!");
		return std::forward<ValueType>(std::get<0>(value));
	}

	[[nodiscard]] const ErrorType& get_error()
	{
		error_handled = true;
		return std::get<1>(value);
	}

	[[nodiscard]] bool has_value() const
	{
		return value.index() == 0;
	}

	[[nodiscard]] operator bool() const
	{
		return has_value();
	}
private:
	std::variant<ValueType, ErrorType> value;
	bool error_handled;
};
	
/**
 * Utilities to facilitate results
 */
	
template<typename T>
auto make_result(const T& in_value)
{
	return Result<T, detail::MakeResultError>(in_value);
}
	
template<typename T>
auto make_result(T&& in_value)
{
	return Result<std::decay_t<T>, detail::MakeResultError>(std::forward<T>(in_value));
}

template<typename E>
auto make_result(detail::ErrorTag, E in_error)
{
	return Result<detail::MakeResultValue, std::decay_t<E>>(in_error);
}
	
template<typename E>
auto make_error(E in_error)
{
	return make_result(error_tag, in_error);
}
	
}