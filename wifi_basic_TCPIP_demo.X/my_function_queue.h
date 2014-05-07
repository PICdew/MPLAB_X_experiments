/* 
 * File:   my_function_queue.h
 * Author: John
 *
 * Created on May 6, 2014, 12:14 PM
 */

#ifndef MY_FUNCTION_QUEUE_H
#define	MY_FUNCTION_QUEUE_H

#ifdef	__cplusplus
extern "C" {
#endif

   void function_queue_init(void);
   void execute_functions_in_queue(void);
   int add_function_to_queue(void *function_ptr);


#ifdef	__cplusplus
}
#endif

#endif	/* MY_FUNCTION_QUEUE_H */

