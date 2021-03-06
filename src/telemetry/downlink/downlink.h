/* -----------------------------------------------------------------------------
 * Component Name: Downlink
 * Parent Component: Telemetry
 * Author(s): Adam Smialek, William Eriksson
 * Purpose: Read telemetry messages from the queue and send them over the
 *          downlink whenever possible.
 * -----------------------------------------------------------------------------
 */

#pragma once

/* initialise the downlink component */
int init_downlink(void* args);
