#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <chrono>

// A simple stopwatch. After starting, `elapsed<time_unit>` returns time elapsed measured in <time_unit>.
class stopwatch {
public:
	template <typename time_unit>
	size_t elapsed();
	void start();

	using seconds = std::chrono::seconds;
	using milliseconds = std::chrono::milliseconds;
	using microseconds = std::chrono::microseconds;
private:
	std::chrono::steady_clock::time_point _start;
};

template <typename T>
size_t stopwatch::elapsed() { return std::chrono::duration_cast<T>(std::chrono::steady_clock::now() - _start).count(); }
void stopwatch::start() { _start = std::chrono::steady_clock::now(); }

#endif //STOPWATCH_H
