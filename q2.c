#include "q2.h"

typedef struct {
uint8_t Data[8];
uint16_t Length;
uint32_t ID;
} CAN_msg_typedef;

const uint8_t bmsmsg_timeout = 25; // after 5 sec ( 200ms x 25)

uint32_t time_ms;

uint8_t charge_status = 0x00;
enum network_states{ INIT_STATE = 0, PRE_OPR, OPR_STATE} ;
enum network_states network_state = 0x00;

uint8_t bms_timeout_cntr = 0;


void Initialization(void){
    network_state = INIT_STATE;
}
void control_routine(void){
    //run the control algorithm here
    time_ms++; //assume INT frequency is 1kHz, for timing purpose
    if((network_state > INIT_STATE) && ((time_ms %200) == 0))
    {
        CAN_read_handler();
    }
    if((network_state > PRE_OPR) && ((time_ms %200) == 0))
    {
        // send out feedback message every 200ms after operation state
        CAN_write_handler(0x181);
    }
    if((time_ms % 1000) == 0)
    {
        // send out heartbeat message every 1000ms after operation state
        CAN_write_handler(0x701);
    }
    // roll back timer for max value
    if(time_ms == 0xFFFFFFFF)
    {
        // reset timer to 0
        time_ms = 0;
    }


}

void CAN_write_handler(uint32_t msg_id){
//CAN tx
    CAN_msg_typedef Can_tx = {[0,0,0,0,0,0,0,0],0,0};;
    switch (msg_id)
    {
    case 0x701:
        /* Heart beat message from charger*/
        Can_tx.ID = 0x701;
        Can_tx.Length = 0x01;
        Can_tx.Data[0] = (uint8_t)network_state;
        break;

    case 0x181:
        /* feedback status message from charger*/
        Can_tx.ID = 0x181;
        Can_tx.Length = 0x04; // this should be  5 instead??? but it was given as 4 in the question
        Can_tx.Data[0] = (uint8_t)((CV_fb & 0xFF00)>>8); // MSB of CV feedback
        Can_tx.Data[1] = (uint8_t)(CV_fb & 0x00FF); // LSB of CV feedback
        Can_tx.Data[2] = (uint8_t)((CC_fb & 0xFF00)>>8); // MSB of CC feedback
        Can_tx.Data[3] = (uint8_t)(CC_fb & 0x00FF); // LSB of CV feedback
        Can_tx.Data[4] = charge_status;

        break;
    
    default:
        break;
    }

    CAN_write(&Can_tx);

}

void CAN_read_handler(void){
//CAN tx
    CAN_msg_typedef Can_rx = {[0,0,0,0,0,0,0,0],0,0};
  
    CAN_read(&Can_rx);
    switch (Can_rx.ID)
    {

        case 0x201:
            /* voltage request & current request from BMS*/
            CV_ref = 0x0000 | ((uint16_t)(Can_rx.Data[0]) << 8);
            CV_ref = CV_ref | (uint16_t)(Can_rx.Data[1])
            CC_ref = 0x0000 | ((uint16_t)(Can_rx.Data[2]) << 8);
            CC_ref = CC_ref | (uint16_t)(Can_rx.Data[3])
            enable_cmd = (bool)(Can_rx.Data[4])
            // reset bms message timeout counter
            bms_timeout_cntr = 0;
            break;
        
        default:
        // if no message was received, increased bms message timeout counter
            bms_timeout_cntr++;

            break;
    }
  

}

void network_management(void){
//run the network management here
    control_routine();
    switch (network_state)
    {
        case INIT_STATE:
        /* initialization*/
            charge_status = 0x00; // initialize to not charging
            CC_ref = 0x0000;
            CV_ref = 0x0000;
            CC_fb = 0x0000;
            CV_fb = 0x0000;
            enable_cmd = false;
            time_ms = 0; // reset timer
            // switch network state to next state after initialization
            network_state = PRE_OPR;

            break;

        case PRE_OPR:
        /* Pre Operation */
            if(enable_cmd == true)
            {
                network_state = OPR_STATE
            }
            charge_status = 0x00;
        break;

        case OPR_STATE:
        /* Operation State */  
            charge_status = 0x01;
            // switch to idle if stop charging command received or bms message timeout
            if((enable_cmd == false) ||(bms_timeout_cntr == bmsmsg_timeout))
            {
                network_state = PRE_OPR;
                bms_timeout_cntr = 0; // reset bms message timeout counter
            }
 
        break;
        
        default:
            break;
    }

}
