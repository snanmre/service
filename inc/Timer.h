#ifndef TIMER_H_
#define TIMER_H_

#include <cstdint>

class Timer
{
public:

	Timer();

	Timer(const uint64_t timeout);

	virtual ~Timer();

	void set(const uint64_t timeout);

	bool isTimeout() const;

	uint64_t elapsed() const;

	void reset();

	static uint64_t getCurrentClock();

	inline uint64_t getPeriod() const;

private:
	uint64_t timeout;
	uint64_t period;
};

inline uint64_t Timer::getPeriod() const
{
	return period;
}

#endif /* TIMER_H_ */
