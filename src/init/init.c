/* -----------------------------------------------------------------------------
 * Component Name: Init
 * Author(s): Harald Magnusson
 * Purpose: Called by OS. Initialise the entire system and start threads.
 *
 * -----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#include "camera.h"
#include "command.h"
#include "data_storage.h"
#include "e_link.h"
#include "global_utils.h"
#include "gpio.h"
#include "img_processing.h"
#include "mode.h"
#include "sensors.h"
#include "telemetry.h"
#include "thermal.h"
#include "tracking.h"
#include "watchdog.h"

/* not including init */
#define MODULE_COUNT 13

static int ret;
static struct sigaction sa;

/* This list controls the order of initialisation */
static const module_init_t init_sequence[MODULE_COUNT] = {
    {"watchdog", &init_watchdog},
    {"gpio", &init_gpio},
    {"camera", &init_camera},
    {"command", &init_command},
    {"data_storage", &init_data_storage},
    {"e_link", &init_elink},
    {"global_utils", &init_global_utils},
    {"img_processing", &init_img_processing},
    {"mode", &init_mode},
    {"sensors", &init_sensors},
    {"telemetry", &init_telemetry},
    {"thermal", &init_thermal},
    {"tracking", &init_tracking}
};

static void sigint_handler(int signum){
    write(STDOUT_FILENO, "\nSIGINT caught\n", 15);

    stop_watchdog();
    write(STDOUT_FILENO, "watchdog stopped\n", 17);

    #ifdef __arm__
        gpio_unexport(GYRO_TRIG_PIN);
    #elif defined(__aarch64__)
        gpio_unexport(GYRO_TRIG_PIN);
    #endif
    write(STDOUT_FILENO, "gpio pin unexported\n", 20);

    pid_t st_pid = get_star_tracker_pid();
    kill(st_pid, SIGTERM);
    write(STDOUT_FILENO, "SIGTERM sent to star tracker\n", 29);
    waitpid(st_pid, NULL, 0);
    write(STDOUT_FILENO, "exiting\n\n", 9);

    _exit(EXIT_SUCCESS);
}

int main(int argc, char* const argv[]){

    sa.sa_handler = sigint_handler;
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    /* redirect stderr to a log file */
    /* freopen("../test.log", "w", stderr); */

    ret = mlockall(MCL_CURRENT|MCL_FUTURE);
    if( ret != 0 ){
        logging(ERROR, "INIT",
            "init: Failed mlockall. Return value: %d, %s", errno, strerror(errno));
        //return FAILURE;
    }

    int count = 0;
    for(int i=0; i<MODULE_COUNT; ++i){
        if(strcmp(init_sequence[i].name, "global_utils") == 0){
            ret = init_sequence[i].init(argv[0]);
        }
        else{
            ret = init_sequence[i].init(NULL);
        }

        if( ret == SUCCESS ){
            logging(INFO, "INIT", "Module \"%s\" initialised successfully.",
                init_sequence[i].name);
        } else if( ret == FAILURE ){
            logging(ERROR, "INIT", "Module \"%s\" FAILED TO INITIALISE, return value: %d",
                init_sequence[i].name, ret);
            ++count;
        } else {
            logging(ERROR, "INIT", "Module \"%s\" FAILED TO INITIALISE, return value: %d, %s",
                init_sequence[i].name, ret, strerror(ret));
            ++count;
        }
    }

    logging(INFO, "INIT",
        "A total of %d modules initialised successfully and %d failed.",
        MODULE_COUNT-count, count);

    if(count != 0){
        return FAILURE;
    }

    while(1){
        sleep(1000);
    }

    return SUCCESS;
}
