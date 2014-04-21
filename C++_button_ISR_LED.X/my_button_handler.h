/* 
 * File:   button_handlers.h
 * Author: John
 *
 * Created on April 21, 2014, 8:53 AM
 */

#ifndef BUTTON_HANDLERS_H
#define	BUTTON_HANDLERS_H

class my_button_handler
{
public:
   static my_button_handler& get_instance(void);
   void do_the_button_thing(void);

private:
   my_button_handler();
   void handle_button_1(void);
   void handle_button_2(void);
};


#endif	/* BUTTON_HANDLERS_H */

