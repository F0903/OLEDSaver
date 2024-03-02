module;
#include <functional>
#include <vector>
#include <unordered_map>
#include <typeinfo>
#include <utility>
export module Event;

struct Hashcode
{
	const std::size_t hashcode;

	Hashcode(const std::size_t hashcode) : hashcode(hashcode)
	{ }

	Hashcode(const std::type_info& typeInfo) : hashcode(typeInfo.hash_code())
	{ }

	constexpr operator std::size_t() const {
		return hashcode;
	}
};

template<class... Args>
struct EventHandler
{
	using FuncT = void(Args...);
	using WrappedFuncT = std::function<FuncT>;
	WrappedFuncT func;

	template<class T>
	inline EventHandler(const WrappedFuncT& func) : func(func) {
	}

	// Works for lambdas somehow
	template<class T>
	inline EventHandler(T&& func) : func(std::forward<T>(func)) {
	}

	inline void Call(Args... args) const {
		func(args...);
	}

	inline bool operator==(const EventHandler& other) const noexcept {
		return func.target_type() == other.func.target_type();
	}
};

namespace std
{
template <class... Args>
struct hash<EventHandler<Args...>>
{
	std::size_t operator()(const EventHandler<Args...>& k) const {
		return k.associatedHashCode;
	}
};
}

export
template<class... Args>
class Event
{
	std::unordered_map<std::size_t, EventHandler<Args...>> subscribers;

public:
	inline void Subscribe(Hashcode hashcode, const EventHandler<Args...>& handler) noexcept {
		subscribers.insert({hashcode, handler});
	}

	inline void Unsubscribe(Hashcode hashcode) noexcept {
		subscribers.erase(hashcode);
	}

	inline void Clear() noexcept {
		subscribers.clear();
	}

	inline void Invoke(Args... args) {
		for (auto& sub : subscribers) {
			sub.second.Call(args...);
		}
	}
};