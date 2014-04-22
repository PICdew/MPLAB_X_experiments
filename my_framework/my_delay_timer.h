/* 
 * File:   my_delay_timer.h
 * Author: John
 *
 * Created on April 21, 2014, 9:04 AM
 */

#ifndef MY_DELAY_TIMER_H
#define	MY_DELAY_TIMER_H

void my_delay_timer_init(unsigned int pb_clock);
void delay_ms(unsigned int milliseconds);
unsigned int get_elapsed_time(void);


#endif	/* MY_DELAY_TIMER_H */

