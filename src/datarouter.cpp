/**
 * @file datarouter.cpp
 * @author Simon Udsen
 * @date 2025-03-27
 */

/**************************************************************/
/*                          Includes                          */
/**************************************************************/
#include "datarouter.h"

/**************************************************************/
/*                     Defines and macros                     */
/**************************************************************/
#define SERVER_IP_ADRESS 192, 168, 136, 250 // IP address of TCP server
#define SERVER_PORT 7123                    // Port of TCP server
#define TCP_DELAY_MS 100                    // Delay between TCP connection attempts
#define QUEUE_TIMEOUT_MS 500                // Timeout for queue operations

/**************************************************************/
/*                     Typedefs and enums                     */
/**************************************************************/

/**************************************************************/
/*                          Private                           */
/**************************************************************/
/**************************************************************/
// Forward data from queue to TCP server
int datarouter::forwardData()
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
        data = String::format( "%lu,%d,%d,%d,%d\n",
                               sample.timestamp,
                               sample.acceleration[AXIS_X],
                               sample.acceleration[AXIS_Y],
                               sample.acceleration[AXIS_Z],
                               sample.step );

        status = client.write( ( ( const uint8_t* )data.c_str() ), data.length() );
    }

    return status;
}

/**************************************************************/
/*                           Public                           */
/**************************************************************/
/**************************************************************/
void dataRouterFunc( void* owner )
{
    datarouter* self = ( datarouter* )owner;

    while ( true )
    {
        switch ( self->state )
        {
        case DATAROUTER_STATE_BEGIN:
        {
            // The system wants to begin, connect to TCP server and set state to
            // running
            if ( self->client.connect( self->serverAddr, self->serverPort ) )
            {
                self->state = DATAROUTER_STATE_RUNNING;
                Log.info( "Datarouter: connected to server" );
            }
            else
            {
                Log.error( "Datarouter: failed to connect to server" );
                self->state = DATAROUTER_STATE_IDLE;
            }
            break;
        }
        case DATAROUTER_STATE_RUNNING:
        {
            int status = self->forwardData();
            if ( status < 0 )
            {
                Log.error( "Datarouter: failed to send data, error %d", status );
            }

            break;
        }
        case DATAROUTER_STATE_FINISH:
        {
            // Forward data until the queue is empty
            while ( self->forwardData() == 0 )
            {
            }

            // Disconnect from TCP server and set state to idle
            self->client.stop();
            self->state = DATAROUTER_STATE_IDLE;
            Log.info( "Datarouter: disconnected from server" );
            break;
        }
        case DATAROUTER_STATE_IDLE:
        default:
        {
            // If we are not running, go to sleep
            if ( os_semaphore_take( self->stateUpdateSemaphore, CONCURRENT_WAIT_FOREVER, 0 ) != 0 )
            {
                Log.error( "Datarouter thread: error in semaphore" );
            }
            break;
        }
        }
    }
}

/**************************************************************/
datarouter::datarouter( os_queue_t* dataQueue )
{
    this->dataQueue = dataQueue;         // Queue to read accelerometer data from
    this->state = DATAROUTER_STATE_IDLE; // State of datarouter

    // Save server information
    serverAddr = IPAddress( SERVER_IP_ADRESS );
    serverPort = SERVER_PORT;
}

/**************************************************************/
int datarouter::init()
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

    // Initialize thread
    if ( result == 0 )
    {
        thread = new Thread( "", dataRouterFunc, this, OS_THREAD_PRIORITY_DEFAULT, OS_THREAD_STACK_SIZE_DEFAULT );
        if ( thread == NULL )
        {
            Log.error( "Failed to create datarouter thread" );
            result = -1;
        }
    }

    return result;
}

/**************************************************************/
datarouter::~datarouter()
{
    // Stop thread, if still running
    if ( thread != NULL )
    {
        delete thread;
    }
}

/**************************************************************/
int datarouter::start()
{
    // Set state to running
    state = DATAROUTER_STATE_BEGIN;

    // Signal thread to wake up
    if ( os_semaphore_give( stateUpdateSemaphore, 0 ) != 0 )
    {
        Log.error( "Datarouter: error in semaphore" );
    }

    return 0;
}

/**************************************************************/
int datarouter::stop()
{
    // Set state to finish
    state = DATAROUTER_STATE_FINISH;

    return 0;
}
