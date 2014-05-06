/* 
 * File:   my_TCPIP_framework.h
 * Author: John
 *
 * Created on May 6, 2014, 3:36 PM
 */

#ifndef MY_TCPIP_FRAMEWORK_H
#define	MY_TCPIP_FRAMEWORK_H

#ifdef	__cplusplus
extern "C" {
#endif

   void TCPIP_stack_init(void);
   void TCPIP_keep_stack_alive(void);

   int TCPIP_open_socket(unsigned int socket_num);
   int TCPIP_close_socket(unsigned int socket_num);
   void TCPIP_basic_send(unsigned int port_num, unsigned char *byte_buffer, unsigned int bytes_to_send);
   void TCPIP_basic_receive(unsigned int port_num, unsigned char *byte_buffer, unsigned int max_buffer_size);



#ifdef	__cplusplus
}
#endif

#endif	/* MY_TCPIP_FRAMEWORK_H */

