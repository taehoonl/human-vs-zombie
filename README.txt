ECE 3140 example radio application for eZ430-RF2500
===================================================

We have provided an example CCS project with applications that demonstrate
how to transmit and receive packets over the radio on the eZ430-RF2500 board.

The code uses Minimal RF Interface (MRFI) drivers, which are part of TI's
SimpliciTI networking protocol. (http://www.ti.com/tool/simpliciti).


Installing the example project
------------------------------

 1. Unzip this demo wherever you want it to live, preferably somewhere without
    strange characters (e.g. spaces) in the path.

 2. Open CCS (Tested with CD version 4.0.2.01003, code generation tools 3.2.1)

 3. Click "Project -> Import Existing CCS/CCE Eclipse Project"

 4. "Select root directory:" to be the demo folder. "radio_example" should show
    up in the list of projects with a check mark. DO NOT select "Copy projects
    into workspace" (this will mess up the demo hierarchy, so the driver
    includes will not work).

 5. Click "Finish". You should have a new project called "radio_example".


Running the radio example
-------------------------

1. The demo includes the MRFI radio drivers and two application files: 
   "simple_send_app.c" and "simple_receive_app.c". Only one of these may be
   in your build at a given time; the other should be excluded.

2. Build the "send" application and program it onto one of your boards. Remove
   the board after it is programmed and plug it into your battery board. The 
   "send" device will toggle its red LED every time it sends a packet.

3. Program the "receive" application into your other board and start running 
   it. The "receive" device will blink its green LED continuously, and will
   toggle its red LED every time it receives a packet from the "send" device.

   The two devices will appear to blink their red LEDs in a synchronized
   fashion. If you remove the power from the "send" device or move it out of
   range, the red LED on the "receive" device will stop blinking as it stops
   getting packets.

   You can also watch the receiver in the CCS debugger. Try setting a
   breakpoint in the MRFI_RxCompleteISR, and you will see packets being
   received. You can use the CCS watch utilities to see the contents of the
   packet, both size/address header and data payload.

