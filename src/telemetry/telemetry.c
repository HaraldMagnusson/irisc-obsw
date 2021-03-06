/* -----------------------------------------------------------------------------
 * Component Name: Telemetry
 * Author(s): Adam Smialek, William Eriksson
 * Purpose: Provide a queue for telemetry messages to be sent to ground and
 *          send them when possible.
 *
 * -----------------------------------------------------------------------------
 */

#include "global_utils.h"
#include "downlink.h"
#include "downlink_queue.h"

#define MODULE_COUNT 2

/* This list controls the order of initialisation */
static const module_init_t init_sequence[MODULE_COUNT] = {
        {"downlink_queue",       &init_downlink_queue},
        {"downlink", &init_downlink}
};

int init_telemetry(void* args){

    /* init whatever in this module */
    return init_submodules(init_sequence, MODULE_COUNT);
}

/**
 * Put data into the downlink queue.
 *
 * @param d     Data to be sent.
 * @param p     Priority of the data (lower `p` indicates higher
 *              priority).
 * @param flag  Indicate if data is a filepath(1) or string(0)
 *
 * @return      0
 */
int send_telemetry(char *filepath, int p, int flag, unsigned short packets_sent) {
    return send_telemetry_local(filepath, p, flag, packets_sent);
}

void check_downlink_list(void){
    check_downlink_list_local();

    return;
}
