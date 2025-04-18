/**
 * @file accelerometer.cpp
 * @author Simon Udsen
 * @date 2025-03-27
 */

/**************************************************************/
/*                          Includes                          */
/**************************************************************/
#include "accelerometer.h"

/**************************************************************/
/*                     Defines and macros                     */
/**************************************************************/
#define SAMPLE_DELAY_MS 1000 / ACCELEROMETER_SAMPLE_RATE_HZ
#define RUNCHECK_DELAY_MS 100 // Delay between checks if state is "running"
#define QUEUE_TIMEOUT_MS 500  // Timeout for queue operations

/**************************************************************/
/*                     Typedefs and enums                     */
/**************************************************************/

/**************************************************************/
/*                          Private                           */
/**************************************************************/

bool stepDetected = false; // Flag to indicate step detected

/**************************************************************/
// Step interrupt handler
void stepDetectedInterrupt()
{
    stepDetected = true;
    detachInterrupt( STEP_PIN ); // Detach interrupt to pin to avoid bouncing
}

/**************************************************************/
/*                           Public                           */
/**************************************************************/

/**************************************************************/
// Accelerometer thread function
void getMeasurement( void* arg )
{
    // Get self pointer
    accelerometer* self = ( accelerometer* )arg;

    // Sample to send to queue
    acceleration_sample_t sample = { 0 };

    while ( true )
    {
        switch ( self->state )
        {
        case ACCELEROMETER_STATE_RUNNING:
        {
            // Get sample from accelerometer, built in timer, (and stepDetected)
            if ( self->detectStep )
            {
                sample.step = stepDetected;
                if ( stepDetected )
                {
                    stepDetected = false;
                    attachInterrupt( STEP_PIN, stepDetectedInterrupt,
                                     RISING ); // Reattach interrupt to pin
                }
            }

            sample.timestamp = millis();
            self->adxl343.readAcceleration(
                &( sample.acceleration[AXIS_X] ), &( sample.acceleration[AXIS_Y] ), &( sample.acceleration[AXIS_Z] ) );

            // Put data in queue
            if ( os_queue_put( *( self->dataQueue ), &sample, QUEUE_TIMEOUT_MS, NULL ) )
            {
                Log.error( "Failed to put data in queue" );
                System.reset();
            }

            // Delay for sample rate
            delay( SAMPLE_DELAY_MS );

            break;
        }
        case ACCELEROMETER_STATE_IDLE:
        default:
        {
            // If we are not running, go to sleep
            if ( os_semaphore_take( self->stateUpdateSemaphore, CONCURRENT_WAIT_FOREVER, 0 ) != 0 )
            {
                Log.error( "Acceleration thread: error in semaphore" );
            }

            break;
        }
        }
    }
}

/**************************************************************/
accelerometer::accelerometer( os_queue_t* dataQueue )
{
    this->dataQueue = dataQueue;
    state = ACCELEROMETER_STATE_IDLE;
}

/**************************************************************/
accelerometer::~accelerometer()
{
    // Stop thread, if still running
    if ( thread != NULL )
    {
        delete thread;
    }
}

/**************************************************************/
int accelerometer::init( bool captureStep )
{
    int result = 0;

    // If we want to capture the step, initialize this pin to rising edge
    // interrupt and connect it to pulldown resistor
    if ( captureStep )
    {
        // Setup constant high pin to connect to button
        pinMode( STEP_REFERENCE_PIN, OUTPUT );
        digitalWrite( STEP_REFERENCE_PIN, HIGH );

        // Setup interrupt pin
        pinMode( STEP_PIN, INPUT_PULLDOWN );
        attachInterrupt( STEP_PIN, stepDetectedInterrupt,
                         RISING ); // Attach interrupt to pin
        Log.info( "Step pin initialized" );
        detectStep = true;
    }

    // Initialize accelerometer
    if ( !adxl343.begin() )
    {
        Log.error( "Failed to initialize accelerometer" );
        result = -1;
    }

    // Initialize state machine semaphore
    if ( result == 0 )
    {
        if ( os_semaphore_create( &stateUpdateSemaphore, SEMAPHORE_MAX_COUNT, 0 ) != 0 )
        {
            Log.error( "Failed to initialize state update semaphore" );
            result = -1;
        }
    }

    // Initialize thread
    if ( result == 0 )
    {
        thread = new Thread( "", getMeasurement, this, OS_THREAD_PRIORITY_DEFAULT, OS_THREAD_STACK_SIZE_DEFAULT );
        if ( thread == NULL )
        {
            Log.error( "Failed to create accelerometer thread" );
            result = -1;
        }
    }

    return result;
}

/**************************************************************/
int accelerometer::start()
{
    // Set state to running
    state = ACCELEROMETER_STATE_RUNNING;

    // Signal thread to wake up
    if ( os_semaphore_give( stateUpdateSemaphore, 0 ) != 0 )
    {
        Log.error( "Acceleration: error in semaphore" );
    }

    return 0;
}

/**************************************************************/
int accelerometer::stop()
{
    state = ACCELEROMETER_STATE_IDLE;
    return 0;
}