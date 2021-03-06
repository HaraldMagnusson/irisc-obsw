/* -----------------------------------------------------------------------------
 * Component Name: Downlink
 * Parent Component: Telemetry
 * Author(s): William Eriksson
 * Purpose: Read telemetry messages from the queue and send them over the
 *          downlink whenever possible.
 * -----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <pthread.h>
#include <string.h> // strlen()
#include <unistd.h> //sleep
#include <stdlib.h>
#include <limits.h>

#include "e_link.h"
#include "downlink_queue.h"
#include "global_utils.h"

/* prototypes declaration */
static void* thread_func(void*);
static unsigned short send_file(char *filepath, unsigned short packets_sent, int priority);

int init_downlink(void* args) {

    return create_thread("downlink", thread_func, 15);
    
}


static void* thread_func(void* param){

    struct node temp;
    unsigned short ret;

    while(1){
        char msg[1400];
        char *data;
        int len;

        memset(msg, 0, sizeof(msg));
        temp = read_downlink_queue();
        if(temp.flag==0){
            /* ID for string */
            msg[0]=0;
            msg[1]=0;
            data = temp.filepath;
            len = strlen(data);

            char* bytes = (char*)&len;
            msg[2] = bytes[0];
            msg[3] = bytes[1];

            for(int ii=0; ii<len+1; ++ii){
                msg[ii+4] = data[ii];
            }

            write_elink(msg, strlen(data)+4);

        } else {

            ret = send_file(temp.filepath, temp.packets_sent, temp.priority);
            if(ret != 0){
                msg[0]=0;
                msg[1]=0;
                msg[2]=0;
                msg[3]=0;
                msg[4]=0;
                msg[5]=0;
                write_elink(msg, 6); /* Telling GS a file transfer is aborted */
                send_telemetry_local(temp.filepath, (temp.priority)-1, temp.flag, ret);
            }
        }
    }

    return SUCCESS;
}


static unsigned short send_file(char *filepath, unsigned short packets_sent, int priority){

    int max_packet_size = 1400-6;
    char buffer[max_packet_size];
    char msg[max_packet_size];
    unsigned short n, packets, current_packet;
    long buff_size, sent_bytes;

    current_packet = 0;

    FILE *fp;
    #ifdef DOWNLINK_DEBUG
    logging(DEBUG, "downlink", "Starting to send file: %s", filepath);
    #endif
    fp=fopen(filepath, "rb");
    if(fp == NULL){
        return FAILURE;
    }
    if(fseek(fp, 0L, SEEK_END) != 0){
        return FAILURE;
    }

    buff_size = ftell(fp);
    if(buff_size == -1){
        logging(ERROR, "downlink", "ftell");
    }
    #ifdef DOWNLINK_DEBUG
    logging(DEBUG, "downlink", "Filesize is: %ld", buff_size);
    #endif

    if(fseek(fp, 0L, SEEK_SET) != 0){
        logging(ERROR, "downlink", "fseek");
    }

    sent_bytes = max_packet_size*packets_sent;
    buff_size=buff_size-sent_bytes;

    n = buff_size/max_packet_size;

    if(buff_size%max_packet_size>0){
        packets=++n;
    } else {
        packets=n;
    }
    
    /* ID for file info */
    msg[0]=1;
    msg[1]=0;
    
    /* How many packets that have already been sent */
    char* current_packet_num = (char*)&current_packet;
    msg[2] = current_packet_num[0];
    msg[3] = current_packet_num[1];

    /* Total packets for file */
    char* packet_num = (char*)&packets;
    msg[4] = packet_num[0];
    msg[5] = packet_num[1];

    /* filepath */
    char* fn = &filepath[strlen(filepath)];
    while(*--fn!='/'){}
    fn++;
    int len = strlen(fn)+1;
    for(int ii=0; ii<len; ++ii){
        msg[ii+8] = fn[ii];
    }

    /* Length of filepath */
    char* bytes_num = (char*)&len;
    msg[6] = bytes_num[0];
    msg[7] = bytes_num[1];
    
    write_elink(msg, len+8);

    char temp[6];

    char* total = malloc(1400);

    size_t read_bytes;

    if(packets_sent>0){
           fseek(fp, packets_sent*max_packet_size, SEEK_SET); 
    }

    for(int i = 0; i<n;i++){
        
        /* ID for file data */
        temp[0]=1;
        temp[1]=1;

        /* What packet is being sent */
        char* current_packet_num = (char*)&current_packet;
        temp[2] = current_packet_num[0];
        temp[3] = current_packet_num[1];

        read_bytes = fread(buffer, sizeof(char), max_packet_size, fp);
        
        /* Bytes in packet */
        char* bytes_num = (char*)&read_bytes;
        temp[4] = bytes_num[0];
        temp[5] = bytes_num[1];

        memcpy(total, temp,6*sizeof(char));
        memcpy(total+6, buffer, (read_bytes)*sizeof(char));

        write_elink(total, read_bytes+6);
        current_packet++;


        /*
         *  Check if there is another item in downlink queue with higher priority
         */

        if(i%10==0){
            if(priority>queue_priority()){
                fclose(fp);
                free(total);
                return current_packet;
            }            
        }
    }
    #ifdef DOWNLINK_DEBUG
    logging(DEBUG, "downlink", "Done sending file: %s", filepath);
    #endif

    fclose(fp);
    free(total);

    return 0;
}
