module;
#include <chrono>
#include <compare>
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

	constexpr DurType Get() const noexcept {
		return duration;
	}

	template<class T>
	constexpr T GetCount() const noexcept {
		return static_cast<T>(duration.count());
	}

	template<class Ra>
	constexpr Duration<Ra> Cast() const noexcept {
		return Duration<Ra>(std::chrono::duration_cast<std::chrono::duration<RepType, Ra>, RepType, Ratio>(duration));
	}

	template<class A>
	constexpr void operator-=(const Duration<A>& other) noexcept {
		duration -= other.Get();
	}

	template<class A>
	constexpr void operator+=(const Duration<A>& other) noexcept {
		duration += other.Get();
	}

	template<class A>
	constexpr Duration<Ratio> operator-(const Duration<A>& other) const noexcept {
		return Duration<Ratio>(duration - other.Get());
	}

	template<class T, class A>
	constexpr Duration<Ratio> operator+(const Duration<A>& other) const noexcept {
		return Duration<Ratio>(duration + other.Get());
	}

	template<class A>
	constexpr auto operator<=>(const Duration<A>& other) const noexcept {
		return duration <=> other.Get();
	}

	template<class A>
	constexpr auto operator<=>(const std::chrono::duration<RepType, A>& other) const noexcept {
		return duration <=> other;
	}
};

export class Timepoint
{
	Clock::time_point timePoint;

public:
	constexpr Timepoint(const Clock::time_point& timepoint = Clock::now()) : timePoint(timepoint) {
	}

	constexpr Clock::time_point Get() const noexcept {
		return timePoint;
	}

	constexpr Duration<std::nano> GetDurationSinceStart() const noexcept {
		return timePoint.time_since_epoch();
	}

	constexpr Duration<std::nano> operator-(const Timepoint& other) const noexcept {
		return timePoint - other.Get();
	}

	constexpr Duration<std::nano> operator+(const Timepoint& other) const noexcept {
		return timePoint - other.Get();
	}

	constexpr auto operator<=>(const Timepoint& other) const noexcept {
		return timePoint <=> other.Get();
	}

	constexpr void operator=(const Timepoint& other) noexcept {
		timePoint = other.Get();
	}
};