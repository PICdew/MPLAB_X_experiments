// for linking the definitions of the functions defined in this file with their
// declarations
#include "my_TCPIP_framework.h"

// these are used to try to connect to a network and password
// Note: These are used in WF_Config.h (in the TCPIP stack, in the "includes"
// folder"), at lines 101 and 206, respectively.
// Note: They are #defines instead of set by some API call because the default
// SSID and password used by Microchip's TCPIP stack are themselves #defines,
// and I haven't yet figured out how to use something else.  This is a crude
// solution, but it works.
// Note: MUST be defined BEFORE MHCP_TCPIP.h is included!
#define ROUTER_SSID                "Christ-2.4"
#define ROUTER_PASSWORD            "Jesus is GOD!"      // WPA2 encryption

// for our portal to the Microchip TCPIP stack
// Note: This thing is big and, in my opinion, and bit of a mess because it
// doesn't use the Microchip API to set things up in an orderly manner.
// Instead, it puts a lot of things together with an elegant, if obfuscated,
// mansion of macros.
//
// My suggestion: Don't mess with it unless you want to re-write a chunk of the
// TCPIP stack.  It appears to be heavily coupled with the rest of the stack.
#include "MCHP_TCPIP.h"

// use these sockets to communicate over the network
// Note: I am making them global because I want to split up the "send" and
// "receive" functionality, but both may need to happen on the same socket.  I
// am using C, not C++, so I can't make a member.  The next best thing is a
// static global so that it can be accessed by any function in this file, but
// not outside the file.
// Note: I am allowing for multiple sockets in case you want to make a
// multi-socket program.  I don't know what you might do with it since this
// microcontroller is not built for heavy network communication, but it's here
// if you want it.
// Note: For each socket, two pieces of information will be tracked:
// - A socket handle
// - The port used in that socket (this is just book keeping; I don't use it)
// Note: For simplicity, each socket handle will correspond to one, and only
// one, port.  Port numbers are easier for the user to track because they are
// just integers.
#define MAX_SOCKETS 5
static TCP_SOCKET	g_socket_handles[MAX_SOCKETS];
static unsigned int g_socket_port_numbers[MAX_SOCKETS];


void TCPIP_and_wifi_stack_init(void)
{
   int count = 0;

   for (count = 0; count < MAX_SOCKETS; count += 1)
   {
      g_socket_port_numbers[count] = 0;
   }

   // open the timer that the wifi module will use for it's internal tick
   TickInit();

   // initialize the basic application configuration
   InitAppConfig();

   // Initialize the core stack layers
   StackInit();

#if defined(DERIVE_KEY_FROM_PASSPHRASE_IN_HOST)
   g_WpsPassphrase.valid = FALSE;
#endif   /* defined(DERIVE_KEY_FROM_PASSPHRASE_IN_HOST) */
   WF_Connect();
}

void TCPIP_keep_stack_alive(void)
{
   // perform normal stack tasks including checking for incoming
   // packets and calling appropriate handlers
   StackTask();

   #if defined(WF_CS_TRIS)
      #if !defined(MRF24WG)
         if (gRFModuleVer1209orLater)
      #endif
      WiFiTask();
   #endif

   // this tasks invokes each of the core stack application tasks
   StackApplications();
}

void TCPIP_get_IP_address(unsigned char *ip_first, unsigned char *ip_second, unsigned char *ip_third, unsigned char *ip_fourth)
{
   // extract the sections of the IP address from MCHP_TCPIP.h's AppConfig
   // structure
   // Note: Network byte order, including IP addresses, are "big endian",
   // which means that their most significant bit is in the least
   // significant bit position.
   // Ex: IP address is 169.254.1.1.  Then "169" is the first byte, "254" is
   // 8 bits "higher", etc.
   *ip_fourth = AppConfig.MyIPAddr.byte.MB;     // highest byte (??MB??), fourth number
   *ip_third = AppConfig.MyIPAddr.byte.UB;     // middle high byte (??UB??), third number
   *ip_second = AppConfig.MyIPAddr.byte.HB;     // middle low byte (??HB??), second number
   *ip_first = AppConfig.MyIPAddr.byte.LB;     // lowest byte, first number
}

static int get_next_available_socket_index(void)
{
   int count = 0;
   int this_ret_val = 0;
   
   // check for an unused port, then use the socket of that port
   for (count = 0; count < MAX_SOCKETS; count += 1)
   {
      if (0 == g_socket_port_numbers[count])
      {
         this_ret_val = count;
         break;
      }
   }

   if (MAX_SOCKETS == count)
   {
      // no ports available
      this_ret_val = -1;
   }
   
   return this_ret_val;
}

static int find_index_of_port_number(unsigned int port_num)
{
   int this_ret_val = 0;
   int count = 0;

   for (count = 0; count < MAX_SOCKETS; count += 1)
   {
      if (port_num == g_socket_port_numbers[count])
      {
         // found it
         this_ret_val = count;
         break;
      }
   }
   
   if (MAX_SOCKETS == count)
   {
      // didn't find it
      this_ret_val = -1;
   }
   
   return this_ret_val;
}

int TCPIP_open_socket(unsigned int port_num)
{
   int this_ret_val = 0;
   int socket_index;

   // first check if this port is already in use
   socket_index = find_index_of_port_number(port_num);
   if (socket_index >= 0)
   {
      // already in use; abort
      this_ret_val = -1;
   }

   if (0 == this_ret_val)
   {
      // port number was not in use, so now get the next available socket index
      socket_index = get_next_available_socket_index();
      if (socket_index < 0)
      {
         // no index available
         this_ret_val = -2;
      }
   }

   if (0 == this_ret_val)
   {
      // this socket handle must be available, so try to open a handle to it
      g_socket_handles[socket_index] = TCPOpen(0, TCP_OPEN_SERVER, port_num, TCP_PURPOSE_GENERIC_TCP_SERVER);
      if (INVALID_SOCKET == g_socket_handles[socket_index])
      {
         // bad
         this_ret_val = -3;
      }
      else
      {
         // socket opened ok, so do some book keeping
         g_socket_port_numbers[socket_index] = port_num;
      }
   }

   return this_ret_val;
}

int TCPIP_close_socket(unsigned int port_num)
{
   int this_ret_val = 0;
   int socket_index = 0;

   // find the index of the port in use
   socket_index = find_index_of_port_number(port_num);
   if (socket_index < 0)
   {
      // couldn't find this port number, so we must not be using it
      this_ret_val = -1;
   }

   if (0 == this_ret_val)
   {
      // close the socket, and reset the port number
      // Note: We opened as a server, and only TCPClose(...) can be used to
      // destroy server sockets.  TCPDisconnect(...) only destroys socket
      // clients.
      TCPClose(g_socket_handles[socket_index]);
      g_socket_port_numbers[socket_index] = 0;
   }

   return this_ret_val;
}

int TCPIP_is_there_a_connection_on_port(unsigned int port_num)
{
   int this_ret_val = 0;
   int socket_index = 0;

   socket_index = find_index_of_port_number(port_num);
   if (socket_index < 0)
   {
      // couldn't find this port number, so we must not be using it
      this_ret_val = -1;
   }

   if (0 == this_ret_val)
   {
      if (TCPIsConnected(g_socket_handles[socket_index]))
      {
         this_ret_val = 1;
      }
      else
      {
         // leave it at 0
      }
   }

   return this_ret_val;
}

int TCPIP_bytes_in_TX_FIFO(unsigned int port_num)
{
   int this_ret_val = 0; 
   int socket_index = 0;
   
   socket_index = find_index_of_port_number(port_num);
   if (socket_index < 0)
   {
      // couldn't find this port number, so we must not be using it
      this_ret_val = -1;
   }
   
   if (0 == this_ret_val)
   {
      // check the space in the TX buffer
      // Note: The function "TCP is put ready" only uses the socket handle to 
      // perform some kind of synchronization before checking the number of 
      // bytes available in the TX FIFO.  The number of bytes available is 
      // actually independent of the socket in use, despite what the argument 
      // to the function suggests.  I think that the socket handle argument is
      // only there to ensure that a socket-port combo is active.
      this_ret_val = TCPIsPutReady(g_socket_handles[socket_index]);
   }
   
   return this_ret_val;
}

int TCPIP_bytes_in_RX_FIFO(unsigned int port_num)
{
   int this_ret_val = 0; 
   int socket_index = 0;
   
   socket_index = find_index_of_port_number(port_num);
   if (socket_index < 0)
   {
      // couldn't find this port number, so we must not be using it
      this_ret_val = -1;
   }
   
   if (0 == this_ret_val)
   {
      // check the space in the TX buffer
      // Note: See note in "TCPIP bytes in TX FIFO".
      this_ret_val = TCPIsGetReady(g_socket_handles[socket_index]);
   }
   
   return this_ret_val;
}

int TCPIP_basic_send(unsigned int port_num, unsigned char *byte_buffer, unsigned int bytes_to_send)
{
   int this_ret_val = 0;
   int socket_index = 0;
   WORD space_in_tx_buffer = 0;
   WORD bytes_sent;

   if (0 == byte_buffer)
   {
      // bad pointer
      this_ret_val = -1;
   }

   if (0 == this_ret_val)
   {
      socket_index = find_index_of_port_number(port_num);
      if (socket_index < 0)
      {
         // couldn't find this port number, so we must not be using it
         this_ret_val = -2;
      }
   }

   if (0 == this_ret_val)
   {
      if (!TCPIsConnected(g_socket_handles[socket_index]))
      {
         // there are no sockets communicating with this one, so do nothing
         this_ret_val = -3;
      }
   }

   if (0 == this_ret_val)
   {
      space_in_tx_buffer = TCPIsPutReady(g_socket_handles[socket_index]);
      if (bytes_to_send > space_in_tx_buffer)
      {
         // too much data to send at once, so do nothing
         // Note: If you're feeling clever, modify this function so that it
         // sends the data in multiple chunks.
         this_ret_val = -4;
      }
   }

   if (0 == this_ret_val)
   {
      bytes_sent = TCPPutArray(g_socket_handles[socket_index], byte_buffer, bytes_to_send);
      if (bytes_sent < bytes_to_send)
      {
         // some kind of problem; bad
         // Note: The documentation for TCPPutArray(...) says that, if the
         // number of bytes sent is less than the number that was requested,
         // then the TX buffer became full or the socket was not connected.
         // Hopefully, with the checks in this function, those should be caught
         // before reaching the call to TCPPutArray(...).
         this_ret_val = -5;
      }
      else
      {
         // all went well
         this_ret_val = bytes_sent;
      }
   }

   return this_ret_val;
}

int TCPIP_basic_receive(unsigned int port_num, unsigned char *byte_buffer, unsigned int max_buffer_size)
{
   int this_ret_val = 0;
   int socket_index = 0;
   WORD bytes_in_rx_buffer = 0;
   WORD bytes_read;

   if (0 == byte_buffer)
   {
      // bad pointer
      this_ret_val = -1;
   }

   if (0 == this_ret_val)
   {
      socket_index = find_index_of_port_number(port_num);
      if (socket_index < 0)
      {
         // couldn't find this port number, so we must not be using it
         this_ret_val = -2;
      }
   }

   if (0 == this_ret_val)
   {
      if (!TCPIsConnected(g_socket_handles[socket_index]))
      {
         // there are no sockets communicating with this one, so do nothing
         this_ret_val = -3;
      }
   }

   if (0 == this_ret_val)
   {
      bytes_in_rx_buffer = TCPIsGetReady(g_socket_handles[socket_index]);
      if (bytes_in_rx_buffer >= max_buffer_size)
      {
         // too much data to receive at once, so do nothing
         // Note: If you're feeling clever, modify this function so that it
         // receives the data in multiple chunks.
         this_ret_val = -4;
      }
   }

   if (0 == this_ret_val)
   {
      bytes_read = TCPGetArray(g_socket_handles[socket_index], byte_buffer, max_buffer_size);
      if (bytes_read >= max_buffer_size)
      {
         // this should have been caught in the "number bytes in RX buffer"
         // check prior to reading the buffer, so there was some kind of
         // problem
         this_ret_val = -5;
      }
      else
      {
         // all went well
         this_ret_val = bytes_read;
      }
   }

   return this_ret_val;
}

