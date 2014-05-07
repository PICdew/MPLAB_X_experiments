// for linking the definitions of the functions defined in this file with their
// declarations
#include "my_TCPIP_framework.h"


/*
* This macro uniquely defines this file as the main entry point.
* There should only be one such definition in the entire project,
* and this file must define the AppConfig variable as described below.
*/
//#define THIS_IS_STACK_APPLICATION

// Include all headers for any enabled TCPIP Stack functions
// Note: Do not try including anything else. The Microchip TCPIP stack is a
// heavily coupled chunk of header and source files, with preprocessor checks
// and macros scattered and organized just so.  The only safe thing to include
// (as far as I know) and the only one you should include because of how it is
// organized, is TCPIP.h.
#include "TCPIP Stack/includes/TCPIP.h"


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



// Used for Wi-Fi assertions
// Note: Just keep this around for now
//#define WF_MODULE_NUMBER   WF_MODULE_MAIN_DEMO

// create an APP_CONFIG structure for the TCPIP stack to keep track of various
// network detail
// Note: The APP_CONFIG structure comes from the TCPIP stack's "includes"
// folder, StackTsk.h, line 136.
// Note: Appconfig is referred to, by name, in multiple places around the TCPIP
// stack.  Therefore, we cannot decalare it static.  It must be global.  I
// don't like it, but unless the TCPIP stack is re-written to remove all this
// coupling, I have to live with it being global.
APP_CONFIG AppConfig;



//??trying defining this yourself lateR??
#if defined(DERIVE_KEY_FROM_PASSPHRASE_IN_HOST)
tPassphraseReady g_WpsPassphrase;
#endif    /* defined(DERIVE_KEY_FROM_PASSPHRASE_IN_HOST) */



// create a handle for a connection profile
//static UINT8 ConnectionProfileID;


/*****************************************************************************
* FUNCTION: my_WF_connect
*
* RETURNS:  None
*
* PARAMS:   None
*
*  NOTES:   Connects to an 802.11 network.  Customize this function as needed
*           for your application.
*****************************************************************************/
static void my_WF_connect(void)
{
   UINT8 ConnectionProfileID;
   UINT8 channelList[] = MY_DEFAULT_CHANNEL_LIST;

   /* create a Connection Profile */
   WF_CPCreate(&ConnectionProfileID);

   WF_SetRegionalDomain(MY_DEFAULT_DOMAIN);

   WF_CPSetSsid(ConnectionProfileID,
      AppConfig.MySSID,
      AppConfig.SsidLength);

   WF_CPSetNetworkType(ConnectionProfileID, MY_DEFAULT_NETWORK_TYPE);

   WF_CASetScanType(MY_DEFAULT_SCAN_TYPE);

   WF_CASetChannelList(channelList, sizeof(channelList));

   // The Retry Count parameter tells the WiFi Connection manager how many attempts to make when trying
   // to connect to an existing network.  In the Infrastructure case, the default is to retry forever so that
   // if the AP is turned off or out of range, the radio will continue to attempt a connection until the
   // AP is eventually back on or in range.  In the Adhoc case, the default is to retry 3 times since the
   // purpose of attempting to establish a network in the Adhoc case is only to verify that one does not
   // initially exist.  If the retry count was set to WF_RETRY_FOREVER in the AdHoc mode, an AdHoc network
   // would never be established.
   WF_CASetListRetryCount(MY_DEFAULT_LIST_RETRY_COUNT);

   WF_CASetEventNotificationAction(MY_DEFAULT_EVENT_NOTIFICATION_LIST);

   WF_CASetBeaconTimeout(MY_DEFAULT_BEACON_TIMEOUT);

   // we are using WPA auto (see "my init app config" function for more
   // explanation on security options), so convert the specified pass phrase to
   // a WPA or WPA2 (whichever is more secure AND that the access point
   // supports) hex key, then change the security mode from "with pass phrase"
   // to "with pass key"
   // Note: These hex keys are always 32 bytes long.
   WF_ConvPassphrase2Key(
           AppConfig.SecurityKeyLength,
           AppConfig.SecurityKey,
           AppConfig.SsidLength,
           AppConfig.MySSID);
   AppConfig.SecurityMode--;
   AppConfig.SecurityKeyLength = 32;

   // set the TCPIP connection profile to use this new security setting
   WF_CPSetSecurity(ConnectionProfileID,
      AppConfig.SecurityMode,
      AppConfig.WepKeyIndex,   /* only used if WEP enabled */
      AppConfig.SecurityKey,
      AppConfig.SecurityKeyLength);

   // disable power saving
   WF_PsPollDisable();

   // direct the wifi connection manager to start scanning for and connect to
   // a network that matches the TCPIP connection profile.
   WF_CMConnect(ConnectionProfileID);
}


/*********************************************************************
* Function:        void my_init_app_config(void)
*
* PreCondition:    MPFSInit() is already called.
*
* Input:           None
*
* Output:          Write/Read non-volatile config variables.
*
* Side Effects:    None
*
* Overview:        None
*
* Note:            None
********************************************************************/
static int my_init_app_config(const char *wifi_SSID, const char *wifi_pass_phrase)
{
   int this_ret_val = 0;

   // the mac address cannot fit into a single value, like an unsigned long, so
   // it is jammed into a byte array
   // Note: In the original code in MHCP_TCPIP.h, the array type was a global
   // static (that is, global to this file) ROM BYTE array, but those were just
   // typedefs of "const" and "unsigned char", and it was a static global to
   // take advantage of a Microchip programming feature called
   // "Serialized Quick Turn Programming" (SQTP).  We are not using SQTP, so it
   // didn't need to be static, and therefore I changed it to being local to
   // this setup function.  I also removed the ROM (const) from the declaration
   // because it is only alive for the duration of this function.
   BYTE SerializedMACAddress[6] =
   {
      MY_DEFAULT_MAC_BYTE1,
      MY_DEFAULT_MAC_BYTE2,
      MY_DEFAULT_MAC_BYTE3,
      MY_DEFAULT_MAC_BYTE4,
      MY_DEFAULT_MAC_BYTE5,
      MY_DEFAULT_MAC_BYTE6
   };
   
   // start by checking for input shenanigans
   if (0 == wifi_SSID)
   {
      this_ret_val = -1;
   }
   else if (0 == wifi_pass_phrase)
   {
      this_ret_val = -2;
   }

   if (0 == this_ret_val)
   {
      // inputs are ok, but now check if they are too big for their destination
      // containers
      if (strlen(wifi_SSID) >= sizeof(AppConfig.MySSID))
      {
         // SSID too big
         this_ret_val = -3;
      }
      else if (strlen(wifi_pass_phrase) >= sizeof(AppConfig.SecurityKey))
      {
         // pass phrase too big
         this_ret_val = -4;
      }
   }

   if (0 == this_ret_val)
   {
      // everything seems to be in order, so begin configuring the TCPIP
      // application structure by zeroing it out
      memset((void*)&AppConfig, 0x00, sizeof(AppConfig));

      // set it to use DHCP
      AppConfig.Flags.bIsDHCPEnabled = TRUE;

      // put it in config mode
      // Note: I looked around the TCPIP stack code and Microchip's TCPIP stack
      // PDF a bit, and I can't find an explanation for what this means.  It
      // was set to TRUE in MHCP_TCPIP.h, from whence this code was derived, so
      // I'll leave it true.
      AppConfig.Flags.bInConfigMode = TRUE;

      // copy in the MAC address
      // Note: This comes from the TCPIP stack's "includes" directory,
      // TCPIPConfig.h, starting at line 153.
      memcpy((void*)&AppConfig.MyMACAddr, (ROM void*)SerializedMACAddress, sizeof(AppConfig.MyMACAddr));

      // copy in the default IP address as both the current and the default
      // Note: This comes from the TCPIP stack's "includes" directory,
      // TCPIPConfig.h, starting at line 160.
      AppConfig.MyIPAddr.Val = MY_DEFAULT_IP_ADDR_BYTE1 | MY_DEFAULT_IP_ADDR_BYTE2 << 8ul | MY_DEFAULT_IP_ADDR_BYTE3 << 16ul | MY_DEFAULT_IP_ADDR_BYTE4 << 24ul;
      AppConfig.DefaultIPAddr.Val = AppConfig.MyIPAddr.Val;

      // copy in the default subnet mask as both the current and the default
      // Note: This comes from the TCPIP stack's "includes" directory,
      // TCPIPConfig.h, starting at line 165.
      AppConfig.MyMask.Val = MY_DEFAULT_MASK_BYTE1 | MY_DEFAULT_MASK_BYTE2 << 8ul | MY_DEFAULT_MASK_BYTE3 << 16ul | MY_DEFAULT_MASK_BYTE4 << 24ul;
      AppConfig.DefaultMask.Val = AppConfig.MyMask.Val;

      // copy in the default gateway
      // Note: This comes from the TCPIP stack's "includes" directory,
      // TCPIPConfig.h, starting at line 170.
      // Note: Are we starting to see a pattern?
      AppConfig.MyGateway.Val = MY_DEFAULT_GATE_BYTE1 | MY_DEFAULT_GATE_BYTE2 << 8ul | MY_DEFAULT_GATE_BYTE3 << 16ul | MY_DEFAULT_GATE_BYTE4 << 24ul;

      // copy in the default primary and secondary DNS
      // Note: This comes from the TCPIP stack's "includes" directory,
      // TCPIPConfig.h, starting at line 175.
      // Note: Whoa!  Deja vu...
      AppConfig.PrimaryDNSServer.Val = MY_DEFAULT_PRIMARY_DNS_BYTE1 | MY_DEFAULT_PRIMARY_DNS_BYTE2 << 8ul | MY_DEFAULT_PRIMARY_DNS_BYTE3 << 16ul | MY_DEFAULT_PRIMARY_DNS_BYTE4 << 24ul;
      AppConfig.SecondaryDNSServer.Val = MY_DEFAULT_SECONDARY_DNS_BYTE1 | MY_DEFAULT_SECONDARY_DNS_BYTE2 << 8ul | MY_DEFAULT_SECONDARY_DNS_BYTE3 << 16ul | MY_DEFAULT_SECONDARY_DNS_BYTE4 << 24ul;

      // set and format the default NetBIOS Host Name
      // Note: This comes from the TCPIP stack's "includes" directory,
      // TCPIPConfig.h, starting at line 175.
      // Note: I would SO not have guess that the configuration as in
      // TCPIPConfig.h!
      // Note: The formatting is some string mangling to restrict the specified
      // host name to the NetBIOS standard of exactly 16 characters, with 15
      // being characters and the last being a null terminator.
      memcpy(AppConfig.NetBIOSName, (ROM void*)MY_DEFAULT_HOST_NAME, 16);
      FormatNetBIOSName(AppConfig.NetBIOSName);


      // Note: In MHCP_TCPIP.h, from whence this code was derived, there is a
      // #define check here for WF_CS_TRIS, which is a check to see if there is
      // a clock signal (CS) pin defined.  This is defined in the TCPIP stack's
      // "includes" directory, in "HardwareProfile.h", line 139.  Don't touch
      // that #define because it is used in a number of different files, and
      // since it is so widely defined, I am going to simply go without the
      // check here.

      // load the SSID name into the wifi
      memcpy(AppConfig.MySSID, (ROM void*)wifi_SSID, strlen(wifi_SSID));
      AppConfig.SsidLength = strlen(wifi_SSID);

      // use WPA auto security
      // Note: There is an explanation of the different types of security
      // available in the TCPIP stack's "includes" directory, in WFApi.h,
      // starting at line 513.  The explanation for
      // "WPA auto with pass phrase" states that it will use WPA or WPA2.
      // It will automatically choose the higher of the two depending on
      // what the wireless access point (AP) supports.
      // Note: If you want to use another security mode, check MHCP_TCPIP.h to
      // see how they handled it.
      AppConfig.SecurityMode = WF_SECURITY_WPA_AUTO_WITH_PASS_PHRASE;
      memcpy(AppConfig.SecurityKey, (ROM void*)wifi_pass_phrase, strlen(wifi_pass_phrase));
      AppConfig.SecurityKeyLength = strlen(wifi_pass_phrase);
   }

   return this_ret_val;
}












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

   my_init_app_config("Christ-2.4", "Jesus is GOD!");
   //my_init_app_config(g_my_router_ssid, g_my_router_password);


   // Initialize the core stack layers
   StackInit();

   // flag the passphrase as invalid so that it will need to be resubmitted
   // ??correct? why??
#if defined(DERIVE_KEY_FROM_PASSPHRASE_IN_HOST)
   g_WpsPassphrase.valid = FALSE;
#endif   /* defined(DERIVE_KEY_FROM_PASSPHRASE_IN_HOST) */

   // keep the wifi connection alive
   my_WF_connect();
}

void TCPIP_keep_stack_alive(void)
{
   // perform normal stack tasks including checking for incoming
   // packets and calling appropriate handlers
   StackTask();

   // do wifi network...things
   // Note: this was originally guarded by a check for WF_CS_TRIS, MRF24WG, and
   // some kind of check for the RF module version.  This code is build
   // specifically for the Microchip MRF24WB RF module, for which all of these
   // checks were always true.  I deleted the checks to clean up code.
   WiFiTask();

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

