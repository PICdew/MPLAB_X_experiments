// for linking the definitions of the functions defined in this file with their
// declarations
#include "my_TCPIP_framework.h"

// for our portal to the Microchip TCPIP stack
// Note: This thing is big and, in my opinion, and bit of a mess because it
// doesn't use the Microchip API to set things up in an orderly manner.
// Instead, it puts a lot of things together with an elegant, if obfuscated,
// mansion of macros.
//
// My suggestion: Don't mess with it unless you want to re-write a chunk of the
// TCPIP stack.  It appears to be heavily coupled with the rest of the stack.
#include "MCHP_TCPIP.h"

// these are used to try to connect to a network and password
// Note: These are used in WF_Config.h (in the TCPIP stack, in the "includes"
// folder"), at lines 101 and 206, respectively.
// Note: They are #defines instead of set by some API call because the default
// SSID and password used by Microchip's TCPIP stack are themselves #defines,
// and I haven't yet figured out how to use something else.  This is a crude
// solution, but it works.
#define ROUTER_SSID                "Christ-2.4"
#define ROUTER_PASSWORD            "Jesus is GOD!"      // WPA2 encryption

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
// - An enum to track whether we opened or closed the socket
#define MAX_SOCKETS 5
static TCP_SOCKET	g_socket_handles[MAX_SOCKETS];
static unsigned int g_socket_port_numbers[MAX_SOCKETS];


void TCPIP_stack_init(void)
{
   int count = 0;

   for (count = 0; count < MAX_SOCKETS; count += 1)
   {
      g_socket_port_numbers[count] = 0;
   }

   TickInit();
   myI2CWriteToLine(I2C2, "tick init done", 1);

   // initialize the basic application configuration
   InitAppConfig();
   myI2CWriteToLine(I2C2, "app init done", 1);

   // Initialize the core stack layers
   StackInit();
   myI2CWriteToLine(I2C2, "stack init done", 1);

#if defined(DERIVE_KEY_FROM_PASSPHRASE_IN_HOST)
   g_WpsPassphrase.valid = FALSE;
#endif   /* defined(DERIVE_KEY_FROM_PASSPHRASE_IN_HOST) */
   WF_Connect();
   myI2CWriteToLine(I2C2, "WF connect done", 1);
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

int TCPIP_open_socket(unsigned int port_num)
{
   int this_ret_val = 0;
   int count = 0;
   int socket_index_to_use;

   // first check if this port is already in use
   for (count = 0; count < MAX_SOCKETS; count += 1)
   {
      if (port_num == g_socket_port_numbers[count])
      {
         // port is already in use, so abort
         this_ret_val = -1;
      }
   }

   if (0 == this_ret_val)
   {
      // port number was not in use, so now get the next available socket index
      socket_index_to_use = get_next_available_socket_index();
      if (socket_index_to_use < 0)
      {
         // no index available
         this_ret_val = -2;
      }
   }

   if (0 == this_ret_val)
   {
      // this socket handle must be available, so try to open a handle to it
      g_socket_handles[socket_index_to_use] = TCPOpen(0, TCP_OPEN_SERVER, port_num, TCP_PURPOSE_GENERIC_TCP_SERVER);
      if (INVALID_SOCKET = g_socket_handles[socket_index_to_use])
      {
         // bad
         this_ret_val = -3;
      }
      else
      {
         // socket opened ok, so do some book keeping
         g_socket_port_numbers[socket_index_to_use] = port_num;
      }
   }

   return this_ret_val;
}

int TCPIP_close_socket(unsigned int socket_num)
{
   int this_ret_val = 0;

   if (socket_num >= MAX_SOCKETS)
   {
      this_ret_val = -1;
   }

   if (0 == this_ret_val)
   {
      // socket number was okay, so close it if it is open (else ignore it)
      if (SOCKET_CLOSED == g_TCP_socket_states[socket_num])
      {
         // already closed, so do nothing
      }
      else
      {
         // destroy the socket handle and do some book keeping
         TCPClose(g_socket_handles[socket_num]);
         g_socket_port_numbers[socket_num] = 0;
         g_TCP_socket_states[socket_num] = SOCKET_CLOSED;
      }
   }

   return this_ret_val;
}

void TCPIP_basic_send(unsigned int port_num, unsigned char *byte_buffer, unsigned int bytes_to_send);
void TCPIP_basic_receive(unsigned int port_num, unsigned char *byte_buffer, unsigned int max_buffer_size);

