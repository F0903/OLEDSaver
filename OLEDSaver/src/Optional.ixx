module;
#include <exception> 
export module Optional;

export
template<class T>
class OptionalValue
{
	const bool hasValue;
	const T value;

public:  
	constexpr OptionalValue(const T& value) : value(value), hasValue(true) {
	}

	constexpr OptionalValue(T&& value) : value(value), hasValue(true) {
	}

	constexpr OptionalValue() : hasValue(false) {
	}

	constexpr void AssertValue() const {
		if (!hasValue) {
			throw std::exception("Optional was none when unwrapped!");
		}
	} 

	constexpr operator OptionalValue<const T>() {
		return OptionalValue<const T>(value);
	}

public:
	[[nodiscard]] constexpr bool HasValue() const noexcept {
		return hasValue;
	}

	[[nodiscard]] constexpr T&& Unwrap() const {
		AssertValue();
		return std::move(value);
	}

	[[nodiscard]] constexpr T&& Unwrap() {
		AssertValue();
		return std::move(value);
	}
};

export namespace Optional
{

template<class T>
[[nodiscard]] constexpr auto Value(T&& value) noexcept {
	return OptionalValue<T>(std::forward<T>(value));
}

template<class T>
[[nodiscard]] constexpr auto None() noexcept {
	return OptionalValue<T>();
}

}

