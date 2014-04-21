/* 
 * File:   my_button_handler.h
 * Author: John
 *
 * Created on April 21, 2014, 8:53 AM
 */

#ifndef MY_BUTTON_HANDLER_H
#define	MY_BUTTON_HANDLER_H

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


#endif	/* MY_BUTTON_HANDLER_H */

