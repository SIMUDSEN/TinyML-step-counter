/**
 * @file stepcounter.cpp
 * @author Simon Udsen
 * @date 2025-03-27
 * @brief Predict step count from accelerometer data
 */

#ifndef STEPCOUNTER_H
#define STEPCOUNTER_H

/**************************************************************/
/*                          Includes                          */
/**************************************************************/
#include "Particle.h"
#include "config.h"

/**************************************************************/
/*                     Defines and macros                     */
/**************************************************************/

/**************************************************************/
/*                     Typedefs and enums                     */
/**************************************************************/

// stepcounter state machine states
typedef enum stepcounter_state_
{
    STEPCOUNTER_STATE_IDLE,
    STEPCOUNTER_STATE_BEGIN,
    STEPCOUNTER_STATE_RUNNING,
    STEPCOUNTER_STATE_FINISH,
} stepcounter_state_t;

class stepcounter
{
  public:
    /**************************************************************/
    /*                           Public                           */
    /**************************************************************/
    uint32_t stepCount; // Number of steps counted
    /**
     * Object to predict step count from accelerometer data
     * @param[in] dataQueue Queue to read accelerometer data from
     */
    stepcounter( os_queue_t* dataQueue ); // Constructor

    /**
     * Deletes thread, if initialized
     */
    ~stepcounter(); // Destructor

    /**
     * Initializes member variables
     * @returns Status
     * @retval 0: Success
     */
    int init();

    /**
     * Starts predicting step count asynchronously
     * @returns Status
     * @retval 0: Success
     */
    int start();

    /**
     * Stops predicting step count
     * @returns Status
     * @retval 0: Success
     */
    int stop();

    // Thread functions
    friend void bufferPiping( void* owner );
    friend void predictSteps( void* owner );

  private:
    /**************************************************************/
    /*                          Private                           */
    /**************************************************************/
    os_queue_t* dataQueue; // Queue to read accelerometer data from

    Thread* predictorThread;             // Thread for predicting step count asynchronously
    Thread* bufferThread;                // Thread for piping data from queue to dual buffer
    os_semaphore_t stateUpdateSemaphore; // Semaphore to wake up state machine thread

    acceleration_sample_t buffer[DATA_BUFFER_SIZE]; // Buffer for storing acceleration samples
    uint16_t bufferWriteIndex;                      // Write index for buffer
    os_semaphore_t bufferReadySemaphore;            // Signal that a buffer is full
    os_semaphore_t bufferProcessedSemaphore;        // Signal that a buffer is processed

    stepcounter_state_t state; // State of step counter
    bool firstBufferFilled; // Flag to indicate if buffer has been filled once before. This avoids processing the first
                            // buffer, which may contain garbage data

    // Helper function to forward data from queue to double buffer
    int forwardData();
};

#endif // STEPCOUNTER_H
