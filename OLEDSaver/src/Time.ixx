module;
#include <chrono>
export module Time;

using Clock = std::chrono::high_resolution_clock;

export
template<class Ratio>
class Duration
{
	using RepType = long long;
	using DurType = std::chrono::duration<RepType, Ratio>;
	DurType duration;

public:
	constexpr Duration(const DurType& duration) : duration(duration) {
	}

	constexpr Duration(RepType rep) : duration(DurType(rep)) {
	}

	inline DurType Get() const noexcept {
		return duration;
	}

	template<class T>
	inline constexpr T GetCount() const noexcept {
		return static_cast<T>(duration.count());
	}

	template<class Ra>
	inline constexpr Duration<Ra> Cast() const noexcept {
		return Duration<Ra>(std::chrono::duration_cast<std::chrono::duration<RepType, Ra>, RepType, Ratio>(duration));
	}

	template<class A>
	inline constexpr void operator-=(const Duration<A>& other) noexcept {
		duration -= other.Get();
	}

	template<class A>
	inline constexpr void operator+=(const Duration<A>& other) noexcept {
		duration += other.Get();
	}

	template<class A>
	inline constexpr Duration<Ratio> operator-(const Duration<A>& other) const noexcept {
		return Duration<Ratio>(duration - other.Get());
	}

	template<class T, class A>
	inline constexpr Duration<Ratio> operator+(const Duration<A>& other) const noexcept {
		return Duration<Ratio>(duration + other.Get());
	}

	template<class A>
	inline constexpr bool operator<(const Duration<A>& other) const noexcept {
		return duration < other.Get();
	}

	template<class A>
	inline constexpr bool operator>(const Duration<A>& other) const noexcept {
		return duration > other.Get();
	}

	template<class A>
	inline constexpr void operator=(const Duration<A>& other) noexcept {
		duration = other.Get();
	}
};

export class Timepoint
{
	const Clock::time_point timePoint;

public:
	constexpr Timepoint(const Clock::time_point& timepoint = Clock::now()) : timePoint(timepoint) {
	}

	inline constexpr Clock::time_point Get() const noexcept {
		return timePoint;
	}

	template<class T>
	inline constexpr Duration<std::nano> GetDurationSinceStart() const noexcept {
		return timePoint.time_since_epoch();
	}

	inline constexpr Duration<std::nano> operator-(const Timepoint& other) const noexcept {
		return timePoint - other.timePoint;
	}

	inline constexpr Duration<std::nano> operator+(const Timepoint& other) const noexcept {
		return timePoint - other.timePoint;
	}

	inline constexpr bool operator<(const Timepoint& other) const noexcept {
		return timePoint < other.timePoint;
	}

	inline constexpr bool operator>(const Timepoint& other) const noexcept {
		return timePoint > other.timePoint;
	}
};