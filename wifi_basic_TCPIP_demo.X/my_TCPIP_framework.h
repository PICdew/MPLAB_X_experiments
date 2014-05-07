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

   void TCPIP_and_wifi_stack_init(void);
   void TCPIP_keep_stack_alive(void);
   void TCPIP_get_IP_address(unsigned char *ip_first, unsigned char *ip_second, unsigned char *ip_third, unsigned char *ip_fourth);

   int TCPIP_open_socket(unsigned int port_num);
   int TCPIP_close_socket(unsigned int port_num);

   int TCPIP_is_there_a_connection_on_port(unsigned int port_num);

   int TCPIP_bytes_in_TX_FIFO(unsigned int port_num);
   int TCPIP_bytes_in_RX_FIFO(unsigned int port_num);

   int TCPIP_basic_send(unsigned int port_num, unsigned char *byte_buffer, unsigned int bytes_to_send);
   int TCPIP_basic_receive(unsigned int port_num, unsigned char *byte_buffer, unsigned int max_buffer_size);



#ifdef	__cplusplus
}
#endif

#endif	/* MY_TCPIP_FRAMEWORK_H */

