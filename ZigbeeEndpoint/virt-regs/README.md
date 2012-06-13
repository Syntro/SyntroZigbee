  virt-regs
--------

This is a small demo project designed to be run on a Linux machine serially
connected to a Zigbee module. The machine will most likely be some small board
like a Gumstix or Beagle, but it doesn't really matter. It could be a regular
Linux workstation.

When testing it's nice to have a Zigbee endpoint device that isn't likely to 
have 'real' device issues. virt-regs simulates 32 32-bit registers that can be 
read or written remotely.

The purpose is to demonstrate communications between a Syntro application
running on a Syntro cloud (a TCP network) and an endpoint device running on
a Zigbee network using the SyntroZigbee gateway component.


  Requirements
--------

The Zigbee radios should be running API firmware, not AT firmware.

Only tested this with Zigbee Series2 and 2.5 radios.

You'll need at least two radios, one for machine running virt-regs and 
another connected to the SyntroZigbee gateway machine.

The radios can be connected with USBSerial devices or directly to the serial
lines.


  What does it do?
-------

The program opens a serial port connected to a Zigbee.

The program will listen for receive frames, either 0x90 or 0x91.

The received data will be interpreted as follows:

The first 4 bytes represents a 32-bit mask of 'registers' to take action on

Then

READ COMMAND

If there is only 4 bytes of data, then the registers that are set in the bit-mask
are returned with the data in the following format

The first 4 bytes, repeats the received bit-mask, the next 4 bytes the first 'register',
the next 4 bytes the second 'register', etc...

The byte order is MSB first for each register.

A Zigbee 'Transmit Request Packet', frame type 0x10, is used to send data back to
the radio who sent the request.


WRITE COMMAND

If there is data following the 4 byte bit-mask, then there should be 4 bytes for
every bit set in the bit-mask. If not the write is ignored.

Each 4 bytes after the bit-mask is interpreted as an unsigned 32-bit integer. The
first byte is the MSB.

There is no response to a write command.


  virt-registers ???
--------

The 'virtual registers' are implemented in virtual-registers.h and virtual-registers.c

The intent is that you could replace this implementation with something that acted
on a real device like /dev/pwm or maybe some i2c devices.

Take a look at the code. It's just a place-holder to give some ideas on how you might
approach this.

 
  build
--------

The provided Makefile should work for a native build.

        root@tide:~/Syntro/SyntroZigbee/ZigbeeEndpoint/virt-regs# make
        gcc -Wall -O2 main.c virtual-registers.c -o virt-regs


TODO: Add a cross build makefile.


  Running virt-regs 
-------

Start up a Syntro cloud with a SyntroGateway running on it.

FYI, the SyntroGateway radio in the example has the address 0x0013a2004054f0e3.

Then start up SyntroZigbeeDemo on some machine connected to the Syntro cloud.
SyntroZigbeeDemo is an simple cross-platform GUI client for testing Syntro
Zigbee networks.

Start the program virt-regs in verbose mode on your Zigbee machine, I'm using 
a Gumstix for this example with a radio address of 0x0013a2004054f0f0.

        root@tide:~/Syntro/SyntroZigbee/ZigbeeEndpoint/virt-regs# ./virt-regs -v
        Opened /dev/ttyUSB0 at 115200
        Starting run loop

Using the SyntroZigbeeDemo client, choose the radio you want to target, the radio connected
to the Gumstix in this case. 

1. Read the first two registers.

Enter the following in SyntroZigbeeDemo, the 'To' field is a drop-down list box.

        To: Gumstix(0x0013a2004054f0f0)
        TX Data: 00 00 00 03

And click the Send button.

This is what the Gumstix running virt-regs will show

        rx: 7E 00 16 91 00 13 A2 00 40 54 F0 E3 F3 62 E8 E8 00 11 C1 05 01 00 00 00 03 52
        readRegister(0)
        readRegister(1)
        tx: 7E 00 1A 10 00 00 13 A2 00 40 54 F0 E3 F3 62 00 00 00 00 00 03 00 00 00 00 00 00 00 00 7B

And you'll see the following show up in the RX fields of SytnroZigbeeDemo.

        From: Gumstix(0x0013a2004054f0f0)
		RX Data: 00 00 00 03 00 00 00 00 00 00 00 00

2. Write 0xdeadbeef to register 4 (0x00000010)

In SyntroZigbeeDemo enter the following and click Send.

        TX Data: 00 00 00 10 de ad be ef

You'll see this on the Gumstix running virt-regs 

        rx: 7E 00 1A 91 00 13 A2 00 40 54 F0 E3 F3 62 E8 E8 00 11 C1 05 01 00 00 00 10 DE AD BE EF 0D
        writeRegister(4, 0xDEADBEEF)


3. Read back register 4

Enter the following in SyntroZigbeeDemo and click Send.

        TX Data: 00 00 00 10

You'll see this output from virt-regs 

        rx: 7E 00 16 91 00 13 A2 00 40 54 F0 E3 F3 62 E8 E8 00 11 C1 05 01 00 00 00 10 45
        readRegister(4)
        tx: 7E 00 16 10 00 00 13 A2 00 40 54 F0 E3 F3 62 00 00 00 00 00 10 DE AD BE EF 36

And you'll see this in SyntroZigbeeDemo

        RX Data: 00 00 00 10 DE AD BE EF


Use Ctl-c to shutdown virt-regs.

        ^CReceived shutdown signal
        root@tide:~/ZigbeeEndpoint#
  

If you don't run with the -v verbose mode flag, virt-regs will only output errors.

Any machine on the Syntro network could be running SyntroZigbeeDemo. If it was running on 
multiple machines on the Syntro cloud, each would get the 'read' response that virt-regs 
returned via Syntro's multicasting feature.

Any of the Syntro cloud machines could also write to the virt-regs device.

There could be many devices running virt-regs or a real endpoint applications on the Zigbee 
network and each would be accessible to Syntro applications in a similar manner. 
