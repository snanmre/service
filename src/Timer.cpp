#include "Timer.h"

#include <sys/timeb.h>

Timer::Timer()
{
	set(period);
}

Timer::Timer(const uint64_t timeout)
{
	set(timeout);
}

Timer::~Timer()
{
}

void Timer::set(const uint64_t timeout)
{
	period = timeout;

	this->timeout = getCurrentClock();
}

bool Timer::isTimeout() const
{
	return (elapsed() > period);
}

uint64_t Timer::elapsed() const
{
	return getCurrentClock() - timeout;
}

void Timer::reset()
{
	set(period);
}

uint64_t Timer::getCurrentClock()
{
	struct timeb currentTime;

	ftime(&currentTime);

	return (uint64_t) ((currentTime.time * 1000) + (currentTime.millitm));
}

