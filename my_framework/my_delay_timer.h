/* 
 * File:   my_delay_timer.h
 * Author: John
 *
 * Created on April 21, 2014, 9:04 AM
 */

#ifndef MY_DELAY_TIMER_H
#define	MY_DELAY_TIMER_H

class my_delay_timer
{
public:
   static my_delay_timer& get_instance(void);
   void init(unsigned int pb_clock);
   void delay_ms(unsigned int milliseconds);
   unsigned int get_elapsed_time(void);

private:
   my_delay_timer(void);
   bool m_has_been_initialized;
};


#endif	/* MY_DELAY_TIMER_H */

