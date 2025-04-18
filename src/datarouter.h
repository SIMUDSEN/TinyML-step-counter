/**
 * @file datarouter.h
 * @author Simon Udsen
 * @date 2025-03-27
 * @brief Sends data to TCP server
 */
#ifndef DATAROUTER_H
#define DATAROUTER_H

/**************************************************************/
/*                          Includes                          */
/**************************************************************/
#include "Particle.h" // Particle Device OS APIs
#include "config.h"   // Project configuration

/**************************************************************/
/*                     Defines and macros                     */
/**************************************************************/

/**************************************************************/
/*                     Typedefs and enums                     */
/**************************************************************/

// Datarouter state machine states
typedef enum datarouter_state_
{
    DATAROUTER_STATE_IDLE,
    DATAROUTER_STATE_BEGIN,
    DATAROUTER_STATE_RUNNING,
    DATAROUTER_STATE_FINISH,
} datarouter_state_t;

// Datarouter class
class datarouter
{
  public:
    /**************************************************************/
    /*                           Public                           */
    /**************************************************************/
    /**
     * Object to send data to TCP server
     * @param[in] dataQueue Queue to read accelerometer data from
     */
    datarouter( os_queue_t* dataQueue );

    /**
     * Deletes thread, if initialized
     */
    ~datarouter();

    /**
     * Initializes member variables
     * @returns Status
     * @retval 0: Success
     */
    int init();

    /**
     * Connects to TCP server and starts sending data
     * @returns Status
     * @retval 0: Success
     */
    int start();

    /**
     * Finishes sending data in the queue and disconnects from TCP server
     * @returns Status
     * @retval 0: Success
     */
    int stop();

    // Thread functions
    friend void dataRouterFunc( void* owner );

  private:
    /**************************************************************/
    /*                          Private                           */
    /**************************************************************/
    Thread* thread; // Thread for sending data to TCP server asynchronously
    os_queue_t* dataQueue; // Queue to read accelerometer data from
    os_semaphore_t
        stateUpdateSemaphore; // Semaphore to wake up state machine thread
    datarouter_state_t state; // State of datarouter

    // TCP client
    TCPClient client;     // TCP client
    IPAddress serverAddr; // Server address
    uint16_t serverPort;  // Server port

    // Helper function to forward data from queue to TCP server
    int forwardData();
};

#endif // DATAROUTER_H
