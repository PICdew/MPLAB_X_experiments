/* 
 * File:   my_switch_reader.h
 * Author: John
 *
 * Created on April 21, 2014, 12:50 PM
 */

#ifndef MY_SWITCH_READER_H
#define	MY_SWITCH_READER_H

class my_switch_reader
{
public:
   static my_switch_reader& get_instance(void);
   void do_the_switch_thing(void);

private:
   my_switch_reader();
};


#endif	/* MY_SWITCH_READER_H */

