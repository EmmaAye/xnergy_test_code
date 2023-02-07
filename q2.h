#include <stdbool.h>

extern bool enable_cmd = false; // this should be replaced with global variable across files
extern uint16_t CC_ref;
extern uint16_t CV_ref;
extern uint16_t CC_fb;
extern uint16_t CV_fb;

void CAN_write(CAN_msg_typedef *msg);
bool CAN_read(CAN_msg_typedef *msg);
void control_routine(void);
void CAN_write_handler(void);
void CAN_read_handler(void);
void network_management(void);