module;
#include <functional>
#include <vector>
export module Event;

export
template<class FuncT>
class Event
{
	using EventHandler = std::function<FuncT>;
	std::vector<EventHandler> subscribers;

public:
	constexpr void Subscribe(const EventHandler& func) noexcept {
		subscribers.push_back(func);
	}

	constexpr void UnsubscribeAll(const EventHandler& func) noexcept {
		subscribers.clear();
	}

	void Invoke() {
		for (auto& sub : subscribers) {
			sub();
		}
	}
};