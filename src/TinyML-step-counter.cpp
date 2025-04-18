/**
 * @file TinyML-step-counter.cpp
 * @author Simon Udsen
 * @date 2025-03-26
 * @brief Main file for the TinyML step counter project
 * @details This project implements ************ by machine learning. The system
 * uses 3 threads, one for writing accelerometer data to a queue, one for
 * putting this data into 2 overlapping buffers, and a third for running the
 * machine learning algorithm. If PREDICTION_ENABLED is false and
 * DATA_COLLECTION_ENABLED is true, the second of these threads will write the
 * data to a TCP server, and the third thread is never enabled
 */

/**************************************************************/
/*                          Includes                          */
/**************************************************************/
// Include Particle Device OS APIs, and set up
#include "Particle.h"
// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE( SEMI_AUTOMATIC );
// Run the application and system concurrently in separate threads
SYSTEM_THREAD( ENABLED );
// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler( LOG_LEVEL_INFO );

#include "accelerometer.h" // Accelerometer data collection
#include "adxl343.h"       // ADXL343 accelerometer sensor
#include "config.h"        // Project configuration

#if DATA_COLLECTION_ENABLED
#include "datarouter.h" // Data router to TCP server
#endif

#if PREDICTION_ENABLED
#include "stepcounter.h" // Step counter
#endif

/**************************************************************/
/*                     Defines and macros                     */
/**************************************************************/

/**************************************************************/
/*                     Typedefs and enums                     */
/**************************************************************/
// State of system for state machine
typedef enum STATE_
{
    MEASURING_STATE_IDLE,
    MEASURING_STATE_BEGIN,
    MEASURING_STATE_RUNNING,
    MEASURING_STATE_FINISH
} MEASURING_STATE_T;

/**************************************************************/
/*                          Private                           */
/**************************************************************/

// State of measuring state machine
static MEASURING_STATE_T measuringState = MEASURING_STATE_IDLE;

// Semaphore for waking up state machine from idle
static os_semaphore_t stateUpdateSemaphore;

// Queue for tunneling data between modules
static os_queue_t dataQueue;

// Accelerometer object
static accelerometer accel( &dataQueue );

#if DATA_COLLECTION_ENABLED
// Data router object
static datarouter router( &dataQueue );
#endif

#if PREDICTION_ENABLED
// Step counter object
static stepcounter stepCounter( &dataQueue );
#endif

// Button handler predefine
static void buttonHandler( system_event_t event, int data );

/**************************************************************/
/*                           Public                           */
/**************************************************************/

/**************************************************************/
// setup() runs once, when the device is first turned on
void setup()
{
    // Set wifi credentials
    WiFi.clearCredentials(); // Clears all previous networks
    WiFi.setCredentials( "Drop it like it's hotspot", "244466666" );
    // WiFi.setCredentials( "Elahas WiFi", "NotElnazRouter" );

    int status = 0;

#if PARTICLE_CONNECTION
    // Connect to particle cloud
    Particle.connect();

    // Wait for the device to connect to the cloud
    waitFor( Particle.connected, 10 * 1000 );
    Log.info( "Starting setup" );
    Log.info( "Photons IP: %s", WiFi.localIP().toString().c_str() );
#else
    WiFi.connect();
    Serial.begin( SERIAL_BAUD_RATE );
#endif

    // Get the ability to see the reset reason
    System.enableFeature( FEATURE_RESET_INFO );
    // Print reset reason for debugging purposes
    int resetReason = System.resetReason();
    Log.info( "Reset reason: %d", resetReason );
    if ( resetReason == RESET_REASON_PANIC )
    {
        int panicCode = System.resetReasonData();
        Log.info( "Panic code: %u", panicCode );
    }

    // Set button to control the measuring state
    System.on( button_click, buttonHandler );

    // Set up pin to show status
    pinMode( LED_PIN, OUTPUT );
    digitalWrite( LED_PIN, LOW );

    // Initialize state machine semaphore
    if ( os_semaphore_create( &stateUpdateSemaphore, SEMAPHORE_MAX_COUNT, 0 ) != 0 )
    {
        Log.error( "Failed to initialize state update semaphore" );
        System.reset();
    }

    // Initialize queue for accelerometer data
    status = os_queue_create( &dataQueue, sizeof( acceleration_sample_t ), DATA_BUFFER_SIZE, NULL );
    if ( ( status != 0 ) || ( dataQueue == NULL ) )
    {
        Log.error( "Failed to create queue" );
        System.reset();
    }

    // Initialize accelerometer
#if DATA_COLLECTION_ENABLED
    status = accel.init( true );
#else
    status = accel.init();
#endif
    if ( status != 0 )
    {
        Log.error( "Failed to initialize accelerometer" );
        System.reset();
    }

    // Initialize datarouter
#if DATA_COLLECTION_ENABLED
    Log.info( "Initializing datarouter" );
    status = router.init();
    if ( status != 0 )
    {
        Log.error( "Failed to initialize datarouter" );
        System.reset();
    }
#endif

    // Initialize step counter
#if PREDICTION_ENABLED
    Log.info( "Starting step counter" );
    status = stepCounter.init();
    if ( status != 0 )
    {
        Log.error( "Failed to initialize step counter" );
        System.reset();
    }
#endif

    Log.info( "Completed setup" );
}

/**************************************************************/
// loop() runs on repeat after setup
void loop()
{

    switch ( measuringState )
    {
    case MEASURING_STATE_BEGIN:
    {
        Log.info( "Beginning..." );

        // Light up LED to show that we are measuring
        digitalWrite( LED_PIN, HIGH );
        // Set state to running
        measuringState = MEASURING_STATE_RUNNING;

        // Start accelerometer
        if ( ( accel.start() ) != 0 )
        {
            Log.error( "Failed to start accelerometer" );
            System.reset();
        }

#if DATA_COLLECTION_ENABLED

        // Start data router
        if ( ( router.start() ) != 0 )
        {
            Log.error( "Failed to start datarouter" );
            System.reset();
        }
#endif

#if PREDICTION_ENABLED
        // Start step counter
        if ( ( stepCounter.start() ) != 0 )
        {
            Log.error( "Failed to start step counter" );
            System.reset();
        }
#endif

        break;
    }
    case MEASURING_STATE_FINISH:
    {
        Log.info( "Finishing..." );
        // Stop accelerometer
        if ( ( accel.stop() ) != 0 )
        {
            Log.error( "Failed to stop accelerometer" );
            System.reset();
        }

#if DATA_COLLECTION_ENABLED
        // Stop data router
        if ( ( router.stop() ) != 0 )
        {
            Log.error( "Failed to stop datarouter" );
            System.reset();
        }
#endif

#if PREDICTION_ENABLED
        // Stop step counter
        if ( ( stepCounter.stop() ) != 0 )
        {
            Log.error( "Failed to stop step counter" );
            System.reset();
        }

        // Print the number of steps detected
        Log.info( "Current step count: %ld", stepCounter.stepCount );

#if PARTICLE_CONNECTION
        // Publish step count to Particle Cloud
        Particle.publish( "stepCount", String( stepCounter.stepCount ) );
#endif
#endif

        // Turn off LED
        digitalWrite( LED_PIN, LOW );
        // Set state to idle
        measuringState = MEASURING_STATE_IDLE;

        break;
    }
    case MEASURING_STATE_IDLE:
    case MEASURING_STATE_RUNNING:
    default:
    {
        // If we are not running, go to sleep
        if ( os_semaphore_take( stateUpdateSemaphore, CONCURRENT_WAIT_FOREVER, 0 ) != 0 )
        {
            Log.error( "Loop: error in semaphore" );
        }
        break;
    }
    }
}

void buttonHandler( system_event_t event, int data )
{
    Log.info( "Button pressed" );
    switch ( measuringState )
    {
    case MEASURING_STATE_IDLE:
    {
        measuringState = MEASURING_STATE_BEGIN;

        // Signal to passive state that state has been updated
        if ( os_semaphore_give( stateUpdateSemaphore, 0 ) != 0 )
        {
            Log.error( "Error giving to semaphore in IDLE state" );
        }

        Log.info( "Starting..." );

        break;
    }
    case MEASURING_STATE_RUNNING:
    {
        measuringState = MEASURING_STATE_FINISH;

        // Signal to passive state that state has been updated
        if ( os_semaphore_give( stateUpdateSemaphore, 0 ) != 0 )
        {
            Log.error( "Error giving to semaphore in RUNNING state" );
        }

        Log.info( "Stopping..." );

        break;
    }
    case MEASURING_STATE_BEGIN:
    case MEASURING_STATE_FINISH:
    default:
    {
        break;
    }
    }
}