#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    std::unique_lock<std::mutex> lck(_mutex);
    _condition.wait(lck, [this] { return !_queue.empty(); });
    T message = std::move(_queue.back());
    _queue.pop_back();
    return message;
}

template <typename T>
void MessageQueue<T>::send(T &&message)
{
    std::lock_guard<std::mutex> lck(_mutex);
    _queue.emplace_back(std::move(message));
    _condition.notify_one();
}


/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    while(true)
    {
        //sleep for 1 ms to reduce load on the processor.
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (_lightPhaseMsgQ.receive() == green)
        {
            return;
        }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this)); 
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // setting cycle duration to a random number between 4-6 secs.
    std::random_device randomDevice;
    std::mt19937 eng(randomDevice());
    std::uniform_int_distribution<int> dist(4000,6000);
    int cycleTime = dist(eng);

    //init stopwatch
    std::chrono::time_point<std::chrono::system_clock>  prevUpdate = std::chrono::system_clock::now();

    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); //sleep for 1 ms to reduce load on the processor.

        long currentUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - prevUpdate).count();
        
        if (currentUpdate >= cycleTime)
        {
            // toggling the current phase.
            if(_currentPhase == red) 
            { 
                _currentPhase = green;
            } 
            else 
            { 
                _currentPhase = red;
            }
            
            _lightPhaseMsgQ.send(std::move(_currentPhase));  //Send an update Message to Queue

            prevUpdate = std::chrono::system_clock::now(); // reset stop watch for next cycle

            cycleTime = dist(eng); // re-generating random 4-6 seconds time.
        }
    }
}
