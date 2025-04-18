/**
 * @file config.h
 * @author Simon Udsen
 * @date 2025-03-26
 * @brief Defines for debugging project
 */
#ifndef CONFIG_H
#define CONFIG_H

/**************************************************************/
/*                          Includes                          */
/**************************************************************/
#include <stdint.h> // Standard integer types

/**************************************************************/
/*                     Defines and macros                     */
/**************************************************************/
#define PARTICLE_CONNECTION true
#define SERIAL_BAUD_RATE 115200
#define DEBUG_START_WAIT_TIME_MS 10 * 1000

#define SEMAPHORE_MAX_COUNT 10

#define DATA_COLLECTION_ENABLED false
#define PREDICTION_ENABLED true

#if !( PREDICTION_ENABLED ^ DATA_COLLECTION_ENABLED )
#error "Either prediction or data collection must be enabled, but not both"
#endif

#define ACCELEROMETER_SAMPLE_RATE_HZ 100 // Sample rate of accelerometer in Hz

#define DATA_BUFFER_SIZE_MS 1000 // Size of buffer for ML algorithm in milliseconds

#define DATA_BUFFER_SIZE                                                                                               \
    ( ( DATA_BUFFER_SIZE_MS * ACCELEROMETER_SAMPLE_RATE_HZ ) / 1000 ) // Size of buffer for ML algorithm in samples

#define STEP_PIN D2           // Pin to detect step
#define STEP_REFERENCE_PIN D3 // Constant high to attach a button to STEP_PIN
#define LED_PIN D7            // Pin to show status

/**************************************************************/
/*                     Typedefs and enums                     */
/**************************************************************/
// Structure for storing acceleration samples
typedef struct acceleration_sample_
{
    uint32_t timestamp;      // Timestamp in microseconds
    int16_t acceleration[3]; // X, Y, Z-acceleration
    bool step;               // Step in sample
} acceleration_sample_t;
// Axis of accelerometer
typedef enum axis_
{
    AXIS_X = 0,
    AXIS_Y = 1,
    AXIS_Z = 2
} AXIS_T;

#endif // CONFIG_H
