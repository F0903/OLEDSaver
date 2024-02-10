module;
#include <chrono>
export module Time; 

export namespace Time
{
inline std::chrono::time_point<std::chrono::high_resolution_clock> GetAppStartTimepointMillis() {
	static auto appStart = std::chrono::high_resolution_clock::now();
	return std::chrono::time_point_cast<std::chrono::milliseconds>(appStart);
}

inline std::chrono::time_point<std::chrono::high_resolution_clock> GetCurrentTimepointMillis() {
	const auto now = std::chrono::high_resolution_clock::now();
	return std::chrono::time_point_cast<std::chrono::milliseconds>(now);
}

template<class T>
inline T GetMillisSinceStart() {
	const auto startMillis = GetAppStartTimepointMillis();
	const auto nowMillis = GetCurrentTimepointMillis();
	const auto duration = nowMillis - startMillis;
	const auto durationMillis = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
	return static_cast<T>(durationMillis.count());
}
}