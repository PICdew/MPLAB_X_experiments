// for linking the definitions of the functions defined in this file with their
// declarations
#include "my_TCPIP_framework.h"


/*
* This macro uniquely defines this file as the main entry point.
* There should only be one such definition in the entire project,
* and this file must define the AppConfig variable as described below.
*/
#define THIS_IS_STACK_APPLICATION

//// for the definition of the APP_CONFIG structure, which the TCPIP stack uses
//// throughout the stack's lifetime
//#include "TCPIP Stack/includes/StackTsk.h"
//
#define BAUD_RATE       (19200)		// bps


// Include all headers for any enabled TCPIP Stack functions
#include "TCPIP Stack/includes/TCPIP.h"

#if defined(STACK_USE_ZEROCONF_LINK_LOCAL)
#include "TCPIP Stack/ZeroconfLinkLocal.h"
#endif
#if defined(STACK_USE_ZEROCONF_MDNS_SD)
#include "TCPIP Stack/ZeroconfMulticastDNS.h"
#endif





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










#if defined(EEPROM_CS_TRIS) || defined(SPIFLASH_CS_TRIS)
void SaveAppConfig(const APP_CONFIG *AppConfig);
#else
#define SaveAppConfig(a)
#endif

// Define a header structure for validating the AppConfig data structure in EEPROM/Flash
typedef struct
{
   unsigned short wConfigurationLength;	// Number of bytes saved in EEPROM/Flash (sizeof(APP_CONFIG))
   unsigned short wOriginalChecksum;		// Checksum of the original AppConfig defaults as loaded from ROM (to detect when to wipe the EEPROM/Flash record of AppConfig due to a stack change, such as when switching from Ethernet to Wi-Fi)
   unsigned short wCurrentChecksum;		// Checksum of the current EEPROM/Flash data.  This protects against using corrupt values if power failure occurs while writing them and helps detect coding errors in which some other task writes to the EEPROM in the AppConfig area.
} NVM_VALIDATION_STRUCT;


// Used for Wi-Fi assertions
// Note: Just keep this around for now
#define WF_MODULE_NUMBER   WF_MODULE_MAIN_DEMO

// create an APP_CONFIG structure for the TCPIP stack to keep track of various
// network detail
// Note: The APP_CONFIG structure comes from the TCPIP stack's "includes"
// folder, StackTsk.h, line 136.
// Note: Appconfig is referred to, by name, in multiple places around the TCPIP
// stack.  Therefore, we cannot decalare it static.  It must be global.  I
// don't like it, but unless the TCPIP stack is re-written to remove all this
// coupling, I have to live with it being global.
APP_CONFIG AppConfig;


// Private helper functions.
// These may or may not be present in all applications.
#if defined(WF_CS_TRIS)
void WF_Connect(void);
#if !defined(MRF24WG)
extern BOOL gRFModuleVer1209orLater;
#endif

//??trying defining this yourself lateR??
#if defined(DERIVE_KEY_FROM_PASSPHRASE_IN_HOST)
tPassphraseReady g_WpsPassphrase;
#endif    /* defined(DERIVE_KEY_FROM_PASSPHRASE_IN_HOST) */
#endif


#if defined(WF_CS_TRIS)
// Global variables
UINT8 ConnectionProfileID;
#endif

#if defined(WF_CS_TRIS)

/*****************************************************************************
* FUNCTION: WF_Connect
*
* RETURNS:  None
*
* PARAMS:   None
*
*  NOTES:   Connects to an 802.11 network.  Customize this function as needed
*           for your application.
*****************************************************************************/
void WF_Connect(void)
{
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

#if !defined(MRF24WG)
   if (gRFModuleVer1209orLater)
#else
   {
      // If WEP security is used, set WEP Key Type.  The default WEP Key Type is Shared Key.
      if (AppConfig.SecurityMode == WF_SECURITY_WEP_40 || AppConfig.SecurityMode == WF_SECURITY_WEP_104)
      {
         WF_CPSetWepKeyType(ConnectionProfileID, MY_DEFAULT_WIFI_SECURITY_WEP_KEYTYPE);
      }
   }
#endif

#if defined(MRF24WG)
   // Error check items specific to WPS Push Button mode
#if (MY_DEFAULT_WIFI_SECURITY_MODE==WF_SECURITY_WPS_PUSH_BUTTON)
#if !defined(WF_P2P)
   WF_ASSERT(strlen(AppConfig.MySSID) == 0);  // SSID must be empty when using WPS
   WF_ASSERT(sizeof(channelList) == 11);        // must scan all channels for WPS
#endif

#if (MY_DEFAULT_NETWORK_TYPE == WF_P2P)
   WF_ASSERT(strcmp((char *)AppConfig.MySSID, "DIRECT-") == 0);
   WF_ASSERT(sizeof(channelList) == 3);
   WF_ASSERT(channelList[0] == 1);
   WF_ASSERT(channelList[1] == 6);
   WF_ASSERT(channelList[2] == 11);
#endif
#endif

#endif /* MRF24WG */

#if defined(DERIVE_KEY_FROM_PASSPHRASE_IN_HOST)
   if (AppConfig.SecurityMode == WF_SECURITY_WPA_WITH_PASS_PHRASE
      || AppConfig.SecurityMode == WF_SECURITY_WPA2_WITH_PASS_PHRASE
      || AppConfig.SecurityMode == WF_SECURITY_WPA_AUTO_WITH_PASS_PHRASE) {
      WF_ConvPassphrase2Key(AppConfig.SecurityKeyLength, AppConfig.SecurityKey,
         AppConfig.SsidLength, AppConfig.MySSID);
      AppConfig.SecurityMode--;
      AppConfig.SecurityKeyLength = 32;
   }
#if defined (MRF24WG)
   else if (AppConfig.SecurityMode == WF_SECURITY_WPS_PUSH_BUTTON
      || AppConfig.SecurityMode == WF_SECURITY_WPS_PIN) {
      WF_YieldPassphrase2Host();
   }
#endif    /* defined (MRF24WG) */
#endif    /* defined(DERIVE_KEY_FROM_PASSPHRASE_IN_HOST) */

   WF_CPSetSecurity(ConnectionProfileID,
      AppConfig.SecurityMode,
      AppConfig.WepKeyIndex,   /* only used if WEP enabled */
      AppConfig.SecurityKey,
      AppConfig.SecurityKeyLength);

#if MY_DEFAULT_PS_POLL == WF_ENABLED
   WF_PsPollEnable(TRUE);
#if !defined(MRF24WG)
   if (gRFModuleVer1209orLater)
      WFEnableDeferredPowerSave();
#endif    /* !defined(MRF24WG) */
#else     /* MY_DEFAULT_PS_POLL != WF_ENABLED */
   WF_PsPollDisable();
#endif    /* MY_DEFAULT_PS_POLL == WF_ENABLED */

#ifdef WF_AGGRESSIVE_PS
#if !defined(MRF24WG)
   if (gRFModuleVer1209orLater)
      WFEnableAggressivePowerSave();
#endif
#endif

#if defined(STACK_USE_UART)
   WF_OutputConnectionInfo(&AppConfig);
#endif

#if defined(DISABLE_MODULE_FW_CONNECT_MANAGER_IN_INFRASTRUCTURE)
   WF_DisableModuleConnectionManager();
#endif

#if defined(MRF24WG)
   WFEnableDebugPrint(ENABLE_WPS_PRINTS | ENABLE_P2P_PRINTS);
#endif
   WF_CMConnect(ConnectionProfileID);
}
#endif /* WF_CS_TRIS */


/*********************************************************************
* Function:        void InitAppConfig(void)
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
// MAC Address Serialization using a MPLAB PM3 Programmer and
// Serialized Quick Turn Programming (SQTP).
// The advantage of using SQTP for programming the MAC Address is it
// allows you to auto-increment the MAC address without recompiling
// the code for each unit.  To use SQTP, the MAC address must be fixed
// at a specific location in program memory.  Uncomment these two pragmas
// that locate the MAC address at 0x1FFF0.  Syntax below is for MPLAB C
// Compiler for PIC18 MCUs. Syntax will vary for other compilers.
//#pragma romdata MACROM=0x1FFF0
static ROM BYTE SerializedMACAddress[6] = { MY_DEFAULT_MAC_BYTE1, MY_DEFAULT_MAC_BYTE2, MY_DEFAULT_MAC_BYTE3, MY_DEFAULT_MAC_BYTE4, MY_DEFAULT_MAC_BYTE5, MY_DEFAULT_MAC_BYTE6 };
//#pragma romdata

static int InitAppConfig(const char *wifi_SSID, const char *wifi_password)
{
   int this_ret_val = 0;

   // start by checking for input shenanigans
   if (0 == wifi_SSID)
   {
      this_ret_val = -1;
   }
   else if (0 == wifi_password)
   {
      this_ret_val = -1;
   }

   // Start out zeroing all AppConfig bytes to ensure all fields are
   // deterministic for checksum generation
   memset((void*)&AppConfig, 0x00, sizeof(AppConfig));

   AppConfig.Flags.bIsDHCPEnabled = TRUE;
   AppConfig.Flags.bInConfigMode = TRUE;
   memcpypgm2ram((void*)&AppConfig.MyMACAddr, (ROM void*)SerializedMACAddress, sizeof(AppConfig.MyMACAddr));

   AppConfig.MyIPAddr.Val = MY_DEFAULT_IP_ADDR_BYTE1 | MY_DEFAULT_IP_ADDR_BYTE2 << 8ul | MY_DEFAULT_IP_ADDR_BYTE3 << 16ul | MY_DEFAULT_IP_ADDR_BYTE4 << 24ul;
   AppConfig.DefaultIPAddr.Val = AppConfig.MyIPAddr.Val;
   AppConfig.MyMask.Val = MY_DEFAULT_MASK_BYTE1 | MY_DEFAULT_MASK_BYTE2 << 8ul | MY_DEFAULT_MASK_BYTE3 << 16ul | MY_DEFAULT_MASK_BYTE4 << 24ul;
   AppConfig.DefaultMask.Val = AppConfig.MyMask.Val;
   AppConfig.MyGateway.Val = MY_DEFAULT_GATE_BYTE1 | MY_DEFAULT_GATE_BYTE2 << 8ul | MY_DEFAULT_GATE_BYTE3 << 16ul | MY_DEFAULT_GATE_BYTE4 << 24ul;
   AppConfig.PrimaryDNSServer.Val = MY_DEFAULT_PRIMARY_DNS_BYTE1 | MY_DEFAULT_PRIMARY_DNS_BYTE2 << 8ul | MY_DEFAULT_PRIMARY_DNS_BYTE3 << 16ul | MY_DEFAULT_PRIMARY_DNS_BYTE4 << 24ul;
   AppConfig.SecondaryDNSServer.Val = MY_DEFAULT_SECONDARY_DNS_BYTE1 | MY_DEFAULT_SECONDARY_DNS_BYTE2 << 8ul | MY_DEFAULT_SECONDARY_DNS_BYTE3 << 16ul | MY_DEFAULT_SECONDARY_DNS_BYTE4 << 24ul;

   // Load the default NetBIOS Host Name
   memcpypgm2ram(AppConfig.NetBIOSName, (ROM void*)MY_DEFAULT_HOST_NAME, 16);
   FormatNetBIOSName(AppConfig.NetBIOSName);


   // Note: In MHCP_TCPIP.h, from whence this code was derived, there is a
   // #define check here for WF_CS_TRIS, which is a check to see if there is
   // a clock signal (CS) pin defined.  This is defined in the TCPIP stack's
   // "includes" directory, in "HardwareProfile.h", line 139.  Don't touch
   // that #define because it is used in a number of different files, and
   // since it is so widely defined, I am going to simply go without the
   // check here.

   // load the SSID name into the wifi
   WF_ASSERT(strlen(wifi_SSID) <= sizeof(AppConfig.MySSID));
   memcpy(AppConfig.MySSID, (ROM void*)wifi_SSID, strlen(wifi_SSID));
   AppConfig.SsidLength = strlen(wifi_SSID);

   // use WPA auto security
   // Note: There is an explanation of the different types of security
   // available in the TCPIP stack's "includes" directory, in WFApi.h,
   // starting at line 513.  The explanation for
   // "WPA auto with pass phrase" states that it will use WPA or WPA2.
   // It will automatically choose the higher of the two depending on
   // what the wireless access point (AP) supports.
   AppConfig.SecurityMode = WF_SECURITY_WPA_AUTO_WITH_PASS_PHRASE;
   memcpy(AppConfig.SecurityKey, (ROM void*)wifi_password, strlen(wifi_password));
   AppConfig.SecurityKeyLength = strlen(wifi_password);
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

   InitAppConfig("Christ-2.4", "Jesus is GOD!");
   //InitAppConfig(g_my_router_ssid, g_my_router_password);


   // Initialize the core stack layers
   StackInit();

   // we
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

