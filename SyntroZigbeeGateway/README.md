  SyntroZigbeeGateway
--------

SyntroZigbeeGateway allows communication from applications connected to a Syntro 
cloud with devices on a Zigbee network.

SyntroZigbeeGateway is a cross-platform Qt application with two modes of operation
GUI and console.

Because it is the bridge between the Syntro cloud and a Zigbee network, it requires
a TCP network connection to Syntro and a serial connection to a Zigbee module.

It has only been tested with Zigbee Series 2 and Series 2.5 radios.

SyntroZigbeeGateway requires the radios to be running API firmware. AT mode is not
supported.

The way it works is as follows:

1. Any Zigbee traffic destined for the SyntroZigbeeGateway radio from the Zigbee
network is pushed using a single known multicast stream onto the Syntro network. 
The 64-bit address of the source radio is prepended to the actual data. Clients
can subscribe to this stream.

2. Any Syntro traffic sent to the E2E stream the SyntroZigbeeGateway provides is
forwarded out the Zigbee radio. The E2E stream must have the destination radio's 
64-bit address as the first 8 bytes of the E2E data so the gateway can properly
format the packet. This leading 64-bit address is stripped from the packet data
before sending.


  Requirements
=======

The machine and O/S requirements are the same as for any Syntro application.

Only the Syntro libraries are required to build SyntroZigbeeGateway.

If you do not already have a Syntro cloud running, you will need to build and run 
SyntroControl on at least one machine on your network to establish the cloud. It's 
no problem if it's the same machine as the SyntroZigbeeGateway.

 Detailed setup guides for different operating systems and machines can be found on the 
 Pansenti website under [Documentation](http://www.pansenti.com/wordpress/?page_id=58)


  Build
=======

Retrieve the source code from the [SyntroZigbee](https://github.com/Syntro/SyntroZigbee) 
repository on Github.

Build instructions are the same as the other Syntro applications.

There is a VS2010 solution for Win7/VS2010 developers.

There are qmake .pro files for use with QtCreator or for generating a Makefile for use 
from the command line on MacOS X and Linux machines.


  Run 
=======

Started without arguments, the SyntroZigbeeGateway machine will attempt to start in GUI
mode. If a '-c' argument is provided on the command line, a console mode version of the
program will start. The functionality is the same.

The only configuration that is required is the specification of the COM port for the 
Zigbee and the baud rate to use.

If you start the application and immediately shut it down, it will leave a SyntroZigbeeGateway.ini
file that you can modify with any text editor.

Most of the ini contents are standard Syntro fields documented elsewhere. 

The SyntroZigbeeGateway specific fields are the following

[General]
...
zigbeePort=COM12
zigbeeSpeed=115200
multicastService=zbmc
e2eService=zbe2e
promiscuousMode=true
...

[zigbeeDevices]
1\address=0x0013a2004081abe3
1\alias=MapleMini
1\readOnly=false
1\pollInterval=0
size=1

The Zigbee radio must be serially connected to the machine, but it could be a USB serial
connection or RS232 or direct TTY serial lines.

The default zigbeePort field shown above is for a Windows system. 

Linux systems will use values like /dev/ttyUSB0 or /dev/ttyS1.

MacOS systems will use values like /dev/tty.usbserial-A4013BDR at least for a USB serial connection.

zigbeeSpeed is the serial baud rate to use. It depends on how you setup your radio.

If you make no other changes the gateway should work in 'promiscuous' mode where it will
forward traffic between the Syntro cloud and the Zigbee network without restriction.






