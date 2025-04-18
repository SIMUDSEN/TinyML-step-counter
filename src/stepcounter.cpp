/**
 * @file stepcounter.cpp
 * @author Simon Udsen
 * @date 2025-03-27
 */

/**************************************************************/
/*                          Includes                          */
/**************************************************************/
#include "stepcounter.h"         // Header file for this module
#include "statisticalfeatures.h" // Statistical features
#include "step_counter_model.h"  // Step counter model

/**************************************************************/
/*                     Defines and macros                     */
/**************************************************************/
#define QUEUE_TIMEOUT_MS 500 // Timeout for queue operations

/**************************************************************/
/*                     Typedefs and enums                     */
/**************************************************************/

/**************************************************************/
/*                          Private                           */
/**************************************************************/
/**************************************************************/

/**************************************************************/
// Use ML algorithm to detect how many steps a buffer of size DATA_BUFFER_SIZE contains

int countSteps( acceleration_sample_t* buffer )
{
    // Calculate statistical features
    int16_t features[STATISTICALFEATURES_NUM_FEATURES] = { 0 };
    statisticalfeatures_getFeatures( buffer, DATA_BUFFER_SIZE, features );

    Log.info(
        "Features: %d %d %d %d %d %d", features[0], features[1], features[2], features[3], features[4], features[5] );

    // Predict number of steps
    int steps = ( int )step_counter_model_predict( features, STATISTICALFEATURES_NUM_FEATURES );

    Log.info( "Predicted steps: %d", steps );
    return steps;
}

/**************************************************************/
// Forward data from queue to double buffer
int stepcounter::forwardData()
{
    // Status of operation
    int status = 0;

    // Data to get from accelerometer
    acceleration_sample_t sample = { 0 };

    // Data to send to TCP server
    String data;

    status = os_queue_take( *dataQueue, &sample, QUEUE_TIMEOUT_MS, NULL );

    if ( status == 0 )
    {
        // Write data to dual buffer
        buffer[bufferWriteIndex] = sample;

        // Increment write index
        bufferWriteIndex = ( bufferWriteIndex + 1 ) % DATA_BUFFER_SIZE;

        // If buffer is full, signal to process data
        if ( bufferWriteIndex == 0 )
        {
            // Signal to process data, and wait until it is done
            if ( os_semaphore_give( bufferReadySemaphore, 0 ) != 0 )
            {
                Log.error( "Stepcounter: error in semaphore" );
            }
            if ( os_semaphore_take( bufferProcessedSemaphore, CONCURRENT_WAIT_FOREVER, 0 ) != 0 )
            {
                Log.error( "Stepcounter: error in semaphore" );
            }
        }
    }

    return status;
}

/**************************************************************/
/*                           Public                           */
/**************************************************************/
/**************************************************************/
void bufferPiping( void* owner )
{
    stepcounter* self = ( stepcounter* )owner;

    while ( true )
    {
        switch ( self->state )
        {
        case STEPCOUNTER_STATE_BEGIN:
        {
            self->firstBufferFilled = false;
            self->state = STEPCOUNTER_STATE_RUNNING;
            break;
        }
        case STEPCOUNTER_STATE_RUNNING:
        {
            int status = self->forwardData();
            if ( status < 0 )
            {
                Log.error( "Stepcounter: failed to pipe data to buffer, error %d", status );
            }

            break;
        }
        case STEPCOUNTER_STATE_FINISH:
        {
            // Forward data until the queue is empty
            while ( self->forwardData() == 0 )
            {
            }

            // Disconnect from TCP server and set state to idle
            self->state = STEPCOUNTER_STATE_IDLE;
            Log.info( "Stepcounter: stopped" );
            break;
        }
        case STEPCOUNTER_STATE_IDLE:
        default:
        {
            // If we are not running, go to sleep
            if ( os_semaphore_take( self->stateUpdateSemaphore, CONCURRENT_WAIT_FOREVER, 0 ) != 0 )
            {
                Log.error( "Stepcounter: error in semaphore" );
            }
            break;
        }
        }
    }
}

/**************************************************************/
void predictSteps( void* owner )
{
    stepcounter* self = ( stepcounter* )owner;

    while ( true )
    {
        // Wait for signal to process data
        if ( os_semaphore_take( self->bufferReadySemaphore, CONCURRENT_WAIT_FOREVER, 0 ) != 0 )
        {
            Log.error( "Stepcounter: error in semaphore" );
        }

        if ( self->firstBufferFilled )
        {
            self->stepCount += countSteps( self->buffer );
        }
        else
        {
            // Ignore first buffer, as it may contain garbage data
            self->firstBufferFilled = true;
        }

        // Signal that data processing is done
        if ( os_semaphore_give( self->bufferProcessedSemaphore, 0 ) != 0 )
        {
            Log.error( "Stepcounter: error in semaphore" );
        }
    }
}

/**************************************************************/
stepcounter::stepcounter( os_queue_t* dataQueue )
{
    // Queue to read accelerometer data from
    this->dataQueue = dataQueue;

    // State of stepcounter
    this->state = STEPCOUNTER_STATE_IDLE;

    // Make buffer is zeroes
    memset( &( this->buffer ), 0x00, sizeof( this->buffer ) );
}

/**************************************************************/
int stepcounter::init()
{
    int result = 0;

    // Initialize state machine semaphore
    if ( result == 0 )
    {
        if ( os_semaphore_create( &stateUpdateSemaphore, SEMAPHORE_MAX_COUNT, 0 ) != 0 )
        {
            Log.error( "Failed to initialize state update semaphore" );
            result = -1;
        }
    }

    // Initialize buffer semaphoreS
    if ( result == 0 )
    {
        if ( os_semaphore_create( &bufferReadySemaphore, SEMAPHORE_MAX_COUNT, 0 ) != 0 )
        {
            Log.error( "Failed to initialize buffer semaphore" );
            result = -1;
        }
    }
    if ( result == 0 )
    {
        if ( os_semaphore_create( &bufferProcessedSemaphore, SEMAPHORE_MAX_COUNT, 0 ) != 0 )
        {
            Log.error( "Failed to initialize buffer semaphore" );
            result = -1;
        }
    }

    // Initialize bufferThread
    if ( result == 0 )
    {
        bufferThread = new Thread( "", bufferPiping, this, OS_THREAD_PRIORITY_DEFAULT, OS_THREAD_STACK_SIZE_DEFAULT );
        if ( bufferThread == NULL )
        {
            Log.error( "Failed to create stepcounter buffer thread" );
            result = -1;
        }
    }

    // Initialize predictorThread
    if ( result == 0 )
    {
        predictorThread =
            new Thread( "", predictSteps, this, OS_THREAD_PRIORITY_DEFAULT, OS_THREAD_STACK_SIZE_DEFAULT );
        if ( predictorThread == NULL )
        {
            Log.error( "Failed to create stepcounter predictor thread" );
            result = -1;
        }
    }

    return result;
}

/**************************************************************/
stepcounter::~stepcounter()
{
    // Stop threads, if initialized
    if ( bufferThread != NULL )
    {
        delete bufferThread;
    }
    if ( predictorThread != NULL )
    {
        delete predictorThread;
    }
}

/**************************************************************/
int stepcounter::start()
{
    // Set state to running
    state = STEPCOUNTER_STATE_BEGIN;

    // Signal thread to wake up
    if ( os_semaphore_give( stateUpdateSemaphore, 0 ) != 0 )
    {
        Log.error( "Stepcounter: error in semaphore" );
    }

    return 0;
}

/**************************************************************/
int stepcounter::stop()
{
    // Set state to finish
    state = STEPCOUNTER_STATE_FINISH;

    return 0;
}
