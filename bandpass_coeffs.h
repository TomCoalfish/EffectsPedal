/*
 * bandpass_coeffs.h
 *
 *  Created on: Nov 8, 2021
 *      Author: Kieran Bako & Matthew Peeters
 *
 * This file acts as a look-up table for the bandpass FIR coefficients necessary for
 * the "WAH" effect. The intention is for the program to be able to change the center
 * frequency of the bandpass in real-time by simply changing the pointer of the array
 * the FIR calculation uses. Using this look-up table is method is intented to be
 * much less computationally intensive than generating the window coefficients in real-time
 * for the desired center frequency, then performing the dot product.
 */

#ifndef BANDPASS_COEFFS_H_
#define BANDPASS_COEFFS_H_

#include <xdc/std.h>

// Define the window size used for each array
#define N 56

// Instantiate FIR coefficient arrays, each with
// a different bandpass center frequency
extern Float h_1k[];
extern Float h_2k[];
extern Float h_3k[];
extern Float h_4k[];
extern Float h_5k[];
extern Float h_6k[];
extern Float h_7k[];
extern Float h_8k[];
extern Float h_9k[];
extern Float h_10k[];

// Instantiate pointer used to point to the
// current FIR array
extern Float *h;

// Instantiate array of pointers to allow for
// easy addressing of each array
extern Float *h_arrays[];

#endif /* BANDPASS_COEFFS_H_ */
