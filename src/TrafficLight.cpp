#include <iostream>
#include <random>
#include <future>
#include "TrafficLight.h"

// https://cppbyexample.com/random_int.html
static std::mt19937 mersenneEngine(std::random_device{}());

template<typename T>
T uniform_random_real(T a, T b) {
	std::uniform_real_distribution<T> dist(a, b);
	return dist(mersenneEngine);
}

/* Implementation of class "MessageQueue" */

template<typename T>
T MessageQueue<T>::receive() {
	// FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait()
	// to wait for and receive new messages and pull them from the queue using move semantics.
	// The received object should then be returned by the receive function.
	std::unique_lock<std::mutex> unique_lock(_mutex);
	_condition.wait(unique_lock, [this] { return !_queue.empty(); });

	T msg = std::move(_queue.back());
	_queue.pop_back();
	return msg;
}

template<typename T>
void MessageQueue<T>::send(T &&msg) {
	// FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex>
	// as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
	std::lock_guard<std::mutex> unique_lock(_mutex);
	// clearing queue to get better performance
	// see https://knowledge.udacity.com/questions/586056
	_queue.clear();
	_queue.emplace_back(std::move(msg));
	_condition.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight() {
	_currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen() {
	// FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop
	// runs and repeatedly calls the receive function on the message queue.
	// Once it receives TrafficLightPhase::green, the method returns.
	while (true) {
		TrafficLightPhase phase = _queue.receive();
		if (phase == green) return;
	}
}

TrafficLightPhase TrafficLight::getCurrentPhase() {
	return _currentPhase;
}

void TrafficLight::simulate() {
	// FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the
	// public method „simulate“ is called. To do this, use the thread queue in the base class.
	threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases() {
	// FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles
	// and toggles the current phase of the traffic light between red and green and sends an update method
	// to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds.
	// Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.

	// Declare vars outside any loop for good practice, In some cases, there will be a consideration such as
	// performance that justifies pulling the variable out of the loop.
	std::chrono::time_point<std::chrono::system_clock> t_start, t_end;
	long duration;

	t_start = std::chrono::system_clock::now();

	// Making the random numbers different after every execution
	// https://www.bitdegree.org/learn/random-number-generator-cpp
	// TODO: replace with std::mt19937
	//  https://cppbyexample.com/random_int.html
	//  double cycle_duration = uniform_random_real<double>(4, 6);
	srand((unsigned)time(0));
	int cycle_duration = (rand() % 3) + 4;

	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		// check if time gap reached cycle duration
		t_end = std::chrono::system_clock::now();
		// convert t_time to double --> https://stackoverflow.com/a/50495821/5983691
		duration =
				std::chrono::system_clock::to_time_t(std::chrono::system_clock::time_point(std::chrono::duration_cast<std::chrono::seconds>(
						std::chrono::duration<double>(t_end - t_start))));
		if (duration >= cycle_duration) {
			this->_currentPhase = (this->_currentPhase == red) ? green : red;
			// toggle & send updates
			// TODO: we can send directly here --> _trafficMessages.send(std::move(_currentPhase));
			auto futures =
					std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, &_queue, std::move(_currentPhase));
			futures.wait();
			// reset start time and duration length
			t_start = std::chrono::system_clock::now();
			cycle_duration = (rand() % 3) + 4;
		}
	}
}
