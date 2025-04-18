/**
 * @file statisticalFeatures.h
 * @author Simon Udsen
 * @date 2025-04-11
 * @brief Statistical features to be used in machine learning algorithm
 * @details This file contains the function to calculate statistical features.
 * The features calculated are the following:
 * - Standard deviation of z-axis
 * - Mean absolute difference of z-axis
 * - Minimum of y-axis
 * - Maximum - minimum difference of x-axis
 * - Maximum - minimum difference of y-axis
 * - Maximum - minimum difference of z-axis
 */
#ifndef STATISTICALFEATURES_H
#define STATISTICALFEATURES_H
/**************************************************************/
/*                          Includes                          */
/**************************************************************/
#include "config.h" // Project configuration
/**************************************************************/
/*                     Defines and macros                     */
/**************************************************************/

#ifndef STATISTICALFEATURES_NUM_FEATURES
#define STATISTICALFEATURES_NUM_FEATURES 6
#endif

/**************************************************************/
/*                     Typedefs and enums                     */
/**************************************************************/

/**************************************************************/
/*                          Private                           */
/**************************************************************/

/**************************************************************/
/*                           Public                           */
/**************************************************************/

/**************************************************************/
/**
 * Calculates statistical features
 * @param[in] samples Pointer to array of samples
 * @param[in] size Number of samples in array
 * @param[out] features Pointer to array of features. Should be
 * STATISTICALFEATURES_NUM_FEATURES features
 * @returns Status
 * @retval 0: Success
 */
uint8_t statisticalfeatures_getFeatures( acceleration_sample_t* samples, uint16_t size, int16_t* features );

#endif // STATISTICALFEATURES_H
