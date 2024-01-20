module;
#include <exception>
#include <type_traits>
export module Optional;

export
template<class T>
class Optional
{
	const bool hasValue;
	const T value;

public:
	[[nodiscard]] static constexpr Optional<T> Value(T&& value) noexcept {
		return Optional<T>(std::forward<T>(value));
	}

	[[nodiscard]] static constexpr Optional<T> None() noexcept {
		return Optional<T>();
	}

private:
	constexpr Optional(T&& value) : value(value), hasValue(true) {
	}

	constexpr Optional() : value(nullptr), hasValue(false) {
	}

	inline constexpr void assertValue() const {
		if (!hasValue) {
			throw std::exception("Optional was none when unwrapped!");
		}
	}

public:
	[[nodiscard]] constexpr bool has_value() const noexcept {
		return hasValue;
	}

	[[nodiscard]] constexpr const T&& unwrap_value() const {
		assertValue();
		return std::move(value);
	}

	[[nodiscard]] constexpr T&& unwrap_value() {
		assertValue();
		return std::move(value);
	}
};