/**
 * @file statisticalfeatures.cpp
 * @author Simon Udsen
 * @date 2025-04-15
 * @brief Calculate statistical features from accelerometer data
 */

/**************************************************************/
/*                          Includes                          */
/**************************************************************/
#include "statisticalfeatures.h" // Header file for this module

/**************************************************************/
/*                     Defines and macros                     */
/**************************************************************/

/**************************************************************/
/*                     Typedefs and enums                     */
/**************************************************************/

/**************************************************************/
/*                          Private                           */
/**************************************************************/

/**************************************************************/
/**
 * mean
 * Calculates the mean of the given samples for the specified axis.
 * @param samples Pointer to array of samples
 * @param size Number of samples in array
 * @param axis Axis to calculate mean for (X, Y, or Z)
 * @returns Mean value of the specified axis
 */
static int16_t statisticalfeatures_mean( acceleration_sample_t* samples, uint8_t size, AXIS_T axis )
{
    int32_t sum = 0;
    for ( uint8_t i = 0; i < size; i++ )
    {
        sum += ( int32_t )samples[i].acceleration[axis];
    }
    return ( int16_t )( sum / size );
}

/**************************************************************/
/**
 * max
 * Calculates the maximum value of the given samples for the specified axis.
 * @param samples Pointer to array of samples
 * @param size Number of samples in array
 * @param axis Axis to calculate max for (X, Y, or Z)
 * @returns Maximum value of the specified axis
 */
static int16_t statisticalfeatures_max( acceleration_sample_t* samples, uint8_t size, AXIS_T axis )
{
    int16_t maxVal = samples[0].acceleration[axis];

    for ( uint8_t i = 1; i < size; i++ )
    {
        if ( ( samples[i].acceleration[axis] ) > ( maxVal ) )
        {
            maxVal = samples[i].acceleration[axis];
        }
    }
    return maxVal;
}

/**************************************************************/
/**
 * min
 * Calculates the minimum value of the given samples for the specified axis.
 * @param samples Pointer to array of samples
 * @param size Number of samples in array
 * @param axis Axis to calculate min for (X, Y, or Z)
 * @returns Minimum value of the specified axis
 */
static int16_t statisticalfeatures_min( acceleration_sample_t* samples, uint8_t size, AXIS_T axis )
{
    int16_t minVal = samples[0].acceleration[axis];

    for ( uint8_t i = 1; i < size; i++ )
    {
        if ( ( samples[i].acceleration[axis] ) < ( minVal ) )
        {
            minVal = samples[i].acceleration[axis];
        }
    }
    return minVal;
}

/**************************************************************/
/**
 * Standard deviation
 * Calculates the standard deviation of the given samples for the specified axis.
 * @param samples Pointer to array of samples
 * @param size Number of samples in array
 * @param axis Axis to calculate std for (X, Y, or Z)
 * @param mean Mean value of the specified axis
 * @returns Standard deviation of the specified axis
 */
static int16_t statisticalfeatures_std( acceleration_sample_t* samples, uint8_t size, AXIS_T axis, int16_t mean )
{
    int64_t sum = 0;
    for ( uint8_t i = 0; i < size; i++ )
    {
        sum += ( int64_t )( samples[i].acceleration[axis] - mean ) * ( samples[i].acceleration[axis] - mean );
    }
    return ( int16_t )( sum / ( size - 1 ) );
}

/**************************************************************/
/**
 * Mean absolute difference
 * Calculates the mean absolute difference of the given samples for the specified axis.
 * @param samples Pointer to array of samples
 * @param size Number of samples in array
 * @param axis Axis to calculate mean absolute difference for (X, Y, or Z)
 * @param mean Mean value of the specified axis
 * @returns Mean absolute difference of the specified axis
 */
static int16_t statisticalfeatures_mean_abs_diff( acceleration_sample_t* samples,
                                                  uint8_t size,
                                                  AXIS_T axis,
                                                  int16_t mean )
{
    int32_t sum = 0;
    for ( uint8_t i = 0; i < size; i++ )
    {
        int32_t diff =
            ( int32_t )( ( samples[i].acceleration[axis] > mean ) ? ( samples[i].acceleration[axis] - mean )
                                                                  : ( mean - samples[i].acceleration[axis] ) );
        sum += diff;
    }
    return ( int16_t )( sum / size );
}

/**************************************************************/
/**
 * Max min diff
 * Calculates the maximum - minimum difference of the given samples for the specified axis.
 * @param samples Pointer to array of samples
 * @param size Number of samples in array
 * @param axis Axis to calculate max - min diff for (X, Y, or Z)
 * @returns Maximum - minimum difference of the specified axis
 */
static int16_t statisticalfeatures_max_min_diff( acceleration_sample_t* samples, uint8_t size, AXIS_T axis )
{
    int16_t maxVal = statisticalfeatures_max( samples, size, axis );
    int16_t minVal = statisticalfeatures_min( samples, size, axis );
    return maxVal - minVal;
}

/**************************************************************/
/**
 * Above mean count
 * Counts the number of samples above the mean for the specified axis.
 * @param samples Pointer to array of samples
 * @param size Number of samples in array
 * @param axis Axis to count above mean for (X, Y, or Z)
 * @param mean Mean value of the specified axis
 * @returns Count of samples above the mean for the specified axis
 */
/* UNUSED - SAVE FOR (POSSIBLE) LATER USE
static int16_t statisticalfeatures_above_mean_count( acceleration_sample_t* samples,
                                                     uint8_t size,
                                                     AXIS_T axis,
                                                     int16_t mean )
{
    int16_t count = 0;
    for ( uint8_t i = 0; i < size; i++ )
    {
        if ( ( samples[i].acceleration[axis] ) > mean )
        {
            count++;
        }
    }
    return count;
}*/

/**************************************************************/
/*                           Public                           */
/**************************************************************/

/**
 * 'mean_abs_diff_z'
 * 'min_y'
 * 'max_x'
 * 'max_min_diff_x'
 * 'max_min_diff_y'
 * 'max_min_diff_z'
 */

/**************************************************************/
uint8_t statisticalfeatures_getFeatures( acceleration_sample_t* samples, uint16_t size, int16_t* features )
{
    int16_t mean_z_val = statisticalfeatures_mean( samples, size, AXIS_Z );

    features[0] = statisticalfeatures_std( samples, size, AXIS_Z, mean_z_val );
    features[1] = statisticalfeatures_mean_abs_diff( samples, size, AXIS_Z, mean_z_val );
    features[2] = statisticalfeatures_min( samples, size, AXIS_Y );
    features[3] = statisticalfeatures_max_min_diff( samples, size, AXIS_X );
    features[4] = statisticalfeatures_max_min_diff( samples, size, AXIS_Y );
    features[5] = statisticalfeatures_max_min_diff( samples, size, AXIS_Z );

    return 0;
}