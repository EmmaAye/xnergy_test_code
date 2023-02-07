#include <stdbool.h>
# include "q2.h"

// define constant values
const int CC_min = 0x000A; // Assume current min as const
const int CC_max = 0xFFFF;
const int CV_min = 0x000A;
const int CV_max = 0xFFFF;

const char Kp_CC = 0xF0; // Proportional Gain for Current control with 8 bit int
const char Kp_CV = 0xF0; // Proportional Gain for Current control with 8 bit int
const char Ki_CC  = 0x0A; // Integral Gain for Current control with 8 bit int
const char Ki_CV  = 0x0A; // Integral Gain for Current control with 8 bit int


enum states{ IDLE = 0, CC_CONST, CV_CONST} ;
struct stpi_ctrl
{
    char p_gain;
    char i_gain;
    int feedback;
    int ref;
    int cumsum_err;
}CC_pi_ctrl, CV_pi_ctrl;


// define global variables, assume all variables are 16 bits
int CC_feedback = 0x0000; // declare as 16 bit  for current feedback
int CV_feedback = 0x0000; // declare as 16 bit  for voltage feedback
int CC_out = 0x0000; // declare as 16 bit  for current output
int CV_out = 0x0000; // declare as 16 bit  for voltage output

enum states current_state = 0x00;


void Initialization(void){
//initialize your variables here
    current_state = IDLE;
    CC_out = 0x00F0;
    CV_out = 0x00F0;
    // initialize pi control parameters for CC
    CC_pi_ctrl.p_gain = Kp_CC;
    CC_pi_ctrl.i_gain = Ki_CC;
    CC_pi_ctrl.cumsum_err = 0
    CC_pi_ctrl.feedback = Get_currentFB(); //Dummy function to read feedback current value
    CC_pi_ctrl.ref = CC_ref;

    // initialize pi control parameters for CV
    CV_pi_ctrl.p_gain = Kp_CV;
    CV_pi_ctrl.i_gain = Ki_CV;
    CV_pi_ctrl.cumsum_err = 0
    CV_pi_ctrl.feedback = Get_voltageFB(); // Dummy function to read feedback voltage value
    CV_pi_ctrl.ref = CV_ref;

}


int pi_control(stpi_ctrl * pst_pi_ctrl){
//run the control algorithm here
    int error_val = 0;
    int out_val = 0;
    int P_term  = 0;
    int I_term = 0;
    //Cacluate error value
    error_val = pst_pi_ctrl->ref - pst_pi_ctrl->feedback;
    // caculate p term
    P_term =(int) (pst_pi_ctrl->p_gain * error_val);
    // Calcuate I term
    pst_pi_ctrl->cumsum_err = pst_pi_ctrl->cumsum_err + error_val;
    I_term = (int)(pst_pi_ctrl->i_gain * pst_pi_ctrl->cumsum_err);
    out_val = pst_pi_ctrl->feedback - P_term - I_term;

    return out_val;  

}
void main_state_machine(void){
//run the state transition here
 switch (current_state)
 {
    case IDLE:
    /* Idle state */
    
    if(enable_cmd == true)
    {
        current_state = CC_CONST;
    }

    break;
    case CC_CONST:
    /* Constant Current*/
    CC_feedback = CC_fb; //read feedback current
    CV_feedback = CV_fb; // read feedback voltage value
    if(CV_feedback == CV_ref)
    {
        /* switch to next state if cv feedback is equal to cv current*/
        current_state = CV_CONST; // switch to next state
        CC_pi_ctrl.cumsum_err = 0; // reset cumulated sum for cc
    }
    else
    {
        CC_pi_ctrl.feedback = CC_feedback;
        // get output from pi control
        CC_out = pi_control(&CC_pi_ctrl);
        // check max and min limit
        if(CC_out > CC_max)
        {
            CC_out = CC_max;
        }
        if(CC_out < CC_min)
        {
            CC_out = CC_min;
        }
        Set_CurrentOutput(CC_out); // set current output value
    }

    break;

    case CV_CONST:
        /* Constant voltage*/
        CC_feedback = CC_fb; //read feedback current
        CV_feedback = CV_fb; // read feedback voltage value

         if(CC_feedback <= CC_min)
        {
            /* switch to Idle if cc feedback reached to CC_min*/
            current_state = IDLE;
            enable_cmd = false; // set enable command to flase
            CV_pi_ctrl.cumsum_err = 0; // reset cumulated sum for cv
        }
        else
        {
            CV_pi_ctrl.feedback = CV_feedback;
            // get output from PI control
            CV_out = pi_control(&CV_pi_ctrl);
            // check limit
            if(CV_out > CV_max)
            {
                CV_out = CV_max;
            }
            if(CV_out < CV_min)
            {
                CV_out = CV_min;
            }
            Set_VoltageOutput(CV_out); // set current output value
        }
    break;
 
    default:
    current_state = IDLE;
    break;
 }

}
void main(void){
    Initialization();
    PieVectTable.EPWM1_INT = &control_routine;
    while(true){
    main_state_machine();
    network_management();
}