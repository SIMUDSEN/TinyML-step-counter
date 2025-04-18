/**
 * @file accelerometer.h
 * @author Simon Udsen
 * @date 2025-03-27
 * @brief Accelerometer data collection
 */
#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

/**************************************************************/
/*                          Includes                          */
/**************************************************************/
#include "Particle.h" // Particle Device OS APIs
#include "adxl343.h"  // ADXL343 accelerometer sensor
#include "config.h"   // Project configuration

/**************************************************************/
/*                     Defines and macros                     */
/**************************************************************/

/**************************************************************/
/*                     Typedefs and enums                     */
/**************************************************************/

// Accelerometer state machine state
typedef enum accelerometer_state_
{
    ACCELEROMETER_STATE_IDLE,
    ACCELEROMETER_STATE_RUNNING,
} accelerometer_state_t;

class accelerometer
{
  public:
    /**************************************************************/
    /*                           Public                           */
    /**************************************************************/
    /**
     * Object to read accelerometer data
     * @param[in] dataQueue Queue to write accelerometer data to
     */
    accelerometer( os_queue_t* dataQueue );

    /**
     * Deletes thread, if still initialized
     */
    ~accelerometer();

    /**
     * Initializes member variables
     * @returns Status
     * @retval 0: Success
     */
    int init( bool captureStep = false );

    /**
     * Starts reading accelerometer data asynchronously to the queue
     * @returns Status
     * @retval 0: Success
     */
    int start();

    /**
     * Stops data reading
     * @returns Status
     * @retval 0: Success
     */
    int stop();

    // Thread functions
    friend void getMeasurement( void* owner );

  private:
    /**************************************************************/
    /*                          Private                           */
    /**************************************************************/
    Thread* thread; // Thread for reading accelerometer data asynchronously
    os_queue_t* dataQueue;       // Queue to write accelerometer data to
    ADXL343 adxl343;             // Accelerometer object
    accelerometer_state_t state; // State of accelerometer state machine
    os_semaphore_t
        stateUpdateSemaphore; // Semaphore to wake up state machine thread
    bool detectStep;          // Flag to indicate if step detection is enabled
};

#endif // ACCELEROMETER_H
