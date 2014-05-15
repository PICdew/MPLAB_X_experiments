Author: John Cox, former student of Andrew O'Fallon's CptS 466 class in Spring
2013 at Washington State University.
Email: john.cox@email.wsu.edu

Most of the non-TCPIP-stack and wifi code was written by me.  The TCPIP stack 
and wifi code belongs to Microchip.  I think that this stack was last updated 
in 2009.  It still works, so I'll keep using it for now.

Additional credit goes to a classmate of mine, Josh Sackos, for discovering how
to tame the TCPIP stack for use with FreeRTOS.  
I haven't asked him if I can divulge his email, so I won't do so.


What is this project?
   This is my demo of how to use the Microchip MRF24WB0MA pmod with the 
   Digilent MX4cK.  My board is using a PIC32MX46F512L processor.

Why did I make it?
   My former classmate Josh Sackos tamed the stack, but it wasn't easy to 
   understand because it was sitll using the bulk of an old Microchip demo that 
   seemed to be trying to run some kind of wifi demo that did too many things 
   for too many different pieces of hardware.  I wanted something that was 
   easier to understand, that worked only with my hardware so that the basic 
   configuration was comprehendable, that worked without FreeRTOS (an RTOS adds 
   additional complications when you are trying to figure out how code works), 
   and that was more customizable.

Misc notes you should know beforehand:
   - Timer tick
      Bottom line: 
      Do not try to use timer 5 in this code.
     
      Justification:
      The Microchip TCPIP stack's internal timer "tick" requires the use of a 
      timer.  Originally, the tick was operated by timer 1, but because the 
      rest of us tend to use timer 1 as a first choice, and because FreeRTOS 
      uses timer 1 as well, Josh lightly modified Tick.c in the stack source 
      code to use timer 5 instead.  This code does not use an RTOS, but I do 
      make use of timer 1 myself, so it is nice to have the tick operations 
      moved to the little-used timer 5.
      
   - Where to plug in the wifi module:
      Bottom line:
      Plug the wifi module into Digilent port JB.
      Set the jumper JP3, which is right next to JB, to short the middle pin 
      and INT3.
      Do not use port JF.
      Do not use the I2C bus #1.

      Justification:
      In the TCPIP stack's HardwareProfile.h, the program is configured 
      meticulously with #defines and some macros to use the MX4cK's port JB.  I 
      think that, technically, this hardware profile header file was intended 
      for an older Digilent board, but JB on the MX4cK seems to work 
      identically.
      
      The wifi communicates with the processor using SPI (??I think it is SPI 3?) 
      and external interrupt 3.
      All that configuration appears to take place in HardwareProfile.h with 
      #defines and macros and not the XC32 port and interrupt API.  I don't 
      know how to change these things.
      
      Neither I nor Josh Sackos wrote HardwareProfile.h.  It came in the TCPIP 
      stack along with the wifi demo that Josh tamed, and the #defines are used 
      by name throughout several parts of the rest of the stack.  Best to just 
      leave it be and use port JB for the wifi module, don't use the same SPI 
      line as the wifi module (??SPI 3??), and don't use external interrupt 3.

      To use the wifi module, you must plug it into Digilent port JB, as 
      previously noted, and change jumper JP3, which is right next to the port 
      on the MX4cK, to short the middle pin and the INT3 pin.  This will allow 
      the wifi module to trigger an interrupt when data comes over the network.
      Note: INT3 is also connected to Digilent port JF and the I2C1 bus, so 
      while the INT3 jumper is shorted for port JB, do not use port JF or the 
      I2C1 bus.
      It will mess you up.
      
   - Setting up the CLS pmod:
      Bottom line:
      Plug the CLS pmod into I2C bus #2.  I use the I2C #2 port on the top of 
      the MX4cK.
      Set the CLS' jumper JP1 to short between the middle pin and RST.
      Short the CLS' pins for MD0 (MD "zero").
      Short the CLS' pins for MD1.
      Do NOT short MD2.
      
      Justification:
      This is hardware setup for the Digilent CLS pmod, revision E.
      
   - About the sockets:
      Bottom line:
      We only use one socket for one port.
      
      Although it may be technically possible to open multiple sockets on the
      same port, I don't know how to do that with the Microchip TCPIP API.
      To make it simple, I offer 5 possible sockets, but only allow one socket
      per port.
      
      Note: The demo only uses a single socket-port combo, and I have not 
      tested the program will multiple socket-port combinations.
      

What is included?
   I have included header and source file pairs for the following:
   - a function queue 
      This is like a crude implementation of an RTOS scheduler.
      
      This is a queue for void functions that take no arguments.  It is used by 
      a timer in main.c to indirectly the TCPIP stack servicing function every 
      50 milliseconds.  The stack servicing function will block all 
      non-interrupt operations for ~30 seconds when it first connects because 
      of some kind of encryption thing that takes a bit for this little 80MHz 
      professor to get through, but part of that stack servicing requires the 
      use of the SPI interrupt.  The SPI interrupt cannot happen while the 
      timer interrupt handler is being serviced, so the TCPIP stack servicing 
      therefore cannot be done from inside an interrupt.
      
      The TCPIP stack serving still needs to happen approximately every 50 
      milliseconds though to keep the connection alive, so the timer interrupt 
      handler will use the function queue to queue up the TCPIP stack servicing
      function, and the while(1) loop in main() will execute the functions in 
      the queue.
      
      If you call "add function to queue", but the queue is full (for example, 
      when the program first connects to a network, there is ~30 seconds of 
      blocked activity in which the while(1) loop in main() is not executing 
      the functions in the queue), then the function will simply not be added.
      It's a crude measure, but it works.

   - I2C handling
      This is mostly my code, but it was heavily inspired by some old demo code
      that belonged to Microchip.
      
   - TCPIP framework
      This is mostly my code.  It was inspired by Josh Sackos' code and by an 
      old Microchip TCPIP demo.  I adapted and simplified the TCPIP and wifi 
      setup from Microchip's MHCP_TCPIP.h, but I left that file as a relic for 
      the user to examine if they wanted to look at more configuration options.
      
   - TCPIP stack and wifi source code
      This code all belongs to Microchip.
      

What does it do?
   This demo will attempt to connect to the network specified in the 
   "TCPIP and wifi stack init" function's wifi_SSID and wifi_password 
   arguments.
   
   If it can connect, it will display the IP address on the CLS module, and 
   and then open a socket as a server (??is this important??) on port #5 (you 
   can change this; it is a simple #define in main.c) and listen on it.
   At this stage, the primary while(1) loop (as opposed to the 
   "wait for connection" while(1) loop) will blink the MX4cK's onboard LED #3,
   which is port B, pin 12.

   If the user opens a socket using port #5 (or whatever they set it to) to the 
   IP address that the MX4cK obtained, then the primary while(1) loop will 
   clear the MX4cK's onboard LED #3 (port B, pin 12) and blink LED #4 (port B,
   pin 13). 
   
   At this stage, the user can send data over the network using the sockets.
   - In my program, receiving is simply done by calling 
      TCPIP_basic_receive(...) and specifying a port to receive on, a buffer to 
      store the data, and the max number of bytes to receive.
   - In my program, sending is done by calling TCPIP_basic_send(...) and 
      specifying a port to send data on, a buffer with data to send, and the
      number of bytes to send.
   
   It is the responsibility of the user to check how many bytes are available 
   in the TX first-in first-out (FIFO )buffer before trying to send anything to 
   make sure that there is enough space available to send and that the buffer 
   is not backed up.
   I haven't had trouble with this, but you might if you are pushing a lot of 
   network traffic for this little 80MHz processor.
   Ditto for checking the RX FIFO before receiving.
   Check these two things with calls to TCPIP_bytes_in_TX_FIFO(port number) and
   TCPIP_bytes_in_RX_FIFO (port number).  Read the comments in those two 
   functions for more details.
   
   The socket will never close in this demo, but the user can choose to close 
   it with my TCPIP_close_socket(port number) function.
   
   If the socket closes on the other device (likely a PC), then the MX4cK 
   onboard LED #4 will be cleared and LED #3 will start blinking again.
   
   Note: I have the TX and RX buffer size (the buffers are defined as 
   unsigned char arrays in main()) defined just above main as 
   MESSAGE_BUFFER_SIZE_BYTES, and it is defined as 64.  Change according to 
   your needs.  I chose 64 because it was a nice number that was bigger than
   most things that I would type, and it is a nice round number.
   

How does it...um...how does it work?
I know not, my liege.
Consult the book of firmware!
   - Once everything is plugged in and the MX4cK is programmed with this demo 
      and powered, it will run through the setup and begin connecting to the 
      network.
      
   - Throughout this demo, the following LEDs will blink:
      - Onboard LED #1 will blink as long as the while(1) loops in main() are 
         executing (one for setup while waiting for the wifi to connect, and 
         one for the primary program logic after the connection is up and 
         running).
      - Onboard LED #2 will blink every time the TCPIP servicing function is 
         added to the function queue from the interrupt handler for timer #1.  
         This is approximately every 50 milliseconds.
      - Onboard LED #3 will blink once a socket is opened on the MX4cK's side,
         but no one is connected to it.  
      - Onboard LED #4 will blink once there is at least one connection on the
         opened socket.
      - Onboard LEDs #3 and #4 will not blink at the same time.
      
   - The default IP address is 169.254.1.1, which indicates that it has an IP
      known only to itself.  The first two numbers, 169 and 254, indicate,
      according to some network RFC that I don't recall right now, that the 
      device only has "local linking".  No router anywhere should ever give an 
      IP address that starts with 169.254.  Therefore, this demo waits for the 
      most significant (it is big endian, so the most significant bytes are 
      the lowest, address-wise) two bytes of the IP address to change from 169 
      and 254, respectively, and then it will display the new IP address on the 
      CLS pmod.
      
   - Following startup, it takes ~30 seconds to get an IP address if the SSID
      and pass phrase are correct.  If I am reading the Microchip TCPIP stack
      code comments correctly, these 30 seconds are mostly taken up in intial
      encryption and handshaking.

   - After it gets a new IP address, the program will try to open a socket on 
      that IP address using the port number supplied to the "TCPIP open socket" 
      connection.  If it succeeds in opening a socket, onboard LED #3 will 
      start blinking.  If it does not succeed, it will go into a while(1) loop
      and sit there because I don't know how to handle this problem, and LED #3
      will not blink.
      
   - I use Tera Term in Windows to open a socket to communicate with it once 
      LED #3 starts blinking.
      
      If you haven't set up Tera Term before, I suggest the following:
      - Setup -> Terminal setup: Check the "Local echo" box so that you can see
         what you type.  If you do not, no text will be printed to the Tera 
         Term terminal as you type.
         Note: The "return" key, in Tera Term, only puts out a 
         "return carriage" key.  It does NOT put out a new line character.  
         Check out the bottom of the primary while(1) loop in main() to see how
         I formatted the return string.  You will notice both a 
         "return carriage" and a new line character at the beginning of the 
         string and at the end of the string.  I had some trouble earlier in 
         the build when I though that the program wasn't spitting out data and 
         I couldn't figure out why.  It turned out that it WAS spitting back 
         out the exact same text string without leading return carriage and new
         line characters.  Tera Term only put out a return carriage character 
         when I hit return, so the returned text was printing right on top of 
         the text that Tera Term had echoed.  After that, I decided to format 
         the return text so that it would appear on a new line and appear 
         visually different.
      
      - Setup -> TCP/IP: Uncheck the "Auto window close" box.  When checked, 
         Tera Term will close when it disconnects from or fails to connect to 
         whatever you were trying to connect to.  This is annoying behavior, so 
         I turned it off.
         
      Using Tera Term to connect, communicate with, and disconnect from the 
      MX4cK's open socket.:
      - Alt-N (or File -> New Connection) to start a new connection.
      - Select the radio button labeled "TCP/IP".
      - Type the IP address displayed on the CLS pmod into the text box labeled
         "Host".  In my case, it was "10.10.10.126".
      - Select the "Service" radio button labeled "Telnet".  This is a network
         protocol that I don't understand, but it works.
      - Type the port number into the text box labeled "TCP port#:".  In my 
         case, I am using the demo's default port number of "5".  
      - Hit return key or click the "OK" button.
      - This should connect within a fraction of a second.
      - Type something and hit return.
      - The message should be displayed on the CLS pmod, line 2, or at least up
         to 17 characters, which is the maximum that can be displayed on a 
         single CLS line.
      - The message should also be spit back to Tera Term's terminal, encased 
         in a "--You said: '%s'" statement, in which "%s" is the string you 
         typed.
      - Alt-I (or (File -> Disconnect) to close downt he socket on the PC side.
         The socket will remain open on the MX4cK's side.
      
   Possible problems:
   - Turn it on, but the CLS stays blank.
      Possible explanation: The I2C line is hooked up incorrectly.  My I2C 
      framework, while easy to use, is entirely blocking and will happily 
      wait forever until the CLS responds.
   
   - Onboard LED #1 started blinking at startup, but quickly stopped.  
      LED #3 is still blinking.
      Possible explanation: This is normal.  LED #1 toggles once per loop 
      iteration in main()'s while(1) loops.  Part of those loops is 
      executing the functions in the function queue.  The TCPIP stack 
      servicing function is the only thing added to that queue in the demo,
      and once it starts to connect, it will take ~30 seconds for the 
      connection to complete.  During that time, LED #2 will still blink 
      because it toggles once every 50 milliseconds in timer 1's interrupt 
      handler, but the while(1) loop will not get around to another 
      iteration until the function queue, and therefore the TCPIP stack 
      servicing function, completes, so LED #1 will not blink again until the
      loop finishes that iteration and comes around for another pass.
   
   - Onboard LED #2 is the only one blinking, and it has been much longer 
      than 30 seconds.
      Possible explanation: Check you SSID and pass phrase.  If those are 
      incorrect, then it will never connect, and therefore the program will
      not proceed to open a socket.  
      
      If they are correct, check your network's security settings.  This 
      demo only uses WPA or WPA 2 (it automatically detects if the wireless
      access point (AP) supports them, and it automatically chooses WPA2 if it
      is supported).  If additional security settings are in place, such as MAC 
      address registration, then this demo will be unable to get past them.
      
      This demo only works on simple networks with WPA or WPA2 encryption.
   
   - Oh noes!  The bytes have not yet begun to receive!  I must succeed!
      Possible explanation:  What?!  The Engrish has taken the fort!  To
      Boston, mine brethren!


What if I want to use an RTOS like FreeRTOS?
   In that case, remove timer 1 (FreeRTOS uses timer 1 for its scheduler), and
   instead of using the function queue, call the TCPIP stack servicing function
   to a task that runs every 50 milliseconds.
   
   You can also move a lot of the code out of main() and into various setup 
   tasks, but I strongly advise you to treat the I2C bus as a resource locked 
   by a mutex, because if an I2C communication is attempted while another one 
   was already in progress, then my I2C functions will simply hang until the 
   I2C line goes idle, which of course it won't because it is busy.