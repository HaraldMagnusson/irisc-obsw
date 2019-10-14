/* -----------------------------------------------------------------------------
 * Component Name: Encoder Poller
 * Parent Component: Sensor Poller
 * Author(s): Harald Magnusson
 * Purpose: Poll the encoders for the current attitude relative to the gondola.
 * -----------------------------------------------------------------------------
 */

#pragma once

int init_encoder_poller(void* args);


/* set offsets for the azimuth and altitude angle encoders */
void set_offsets(double az, double alt);

/* fetch a single sample from the encoder */
int enc_single_samp_ll(encoder_t* enc);
