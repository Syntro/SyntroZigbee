  SyntroZigbee
--------

SyntroZigbee is a collection of programs to allow communication between
Syntro cloud applications and devices running on Zigbee networks.

Here is a list of the current applications in this repository and a brief
description.

Refer to the Pansenti website for more documentation: http://www.pansenti.com


  SyntroZigbeeGateway
========

The core SyntroZigbee component is SyntroZigbeeGateway. This provides the
link between the two networks and so requires both a TCP connection and a 
serially connected Zigbee radio. 

The Zigbee connection can be a USB serial device, RS232 or direct TTL serial
lines as long as the Zigbee appears as a serial device to the O/S.

SyntroZigbeeGateway can run on any machine that can run Syntro apps. It has
been tested on Mac, Win32, Linux including Gumstix, BeagleBone, BeagleBoard-XM,
and Pandaboards.

SyntroZigbeeGateway can be run with a GUI or in console-only mode.


  SyntroZigbeeDemo
========

SyntroZigbeeDemo can be used to communicate with Zigbee devices over Syntro.
 
This is a useful application for testing the SyntroZigbeeGateway. 

SyntroZigbeeDemo is cross-platform, but provides only a GUI interface. There
is no console mode.

SyntroZigbeeDemo does not require a Zigbee radio, it communicates only with
the Syntro cloud.

The interface provides GUI fields for sending and receiving data from Zigbee
devices using the Syntro streams provided by the SyntroZigbeeGateway.


  ZigbeeTestNode
========

This is a Zigbee only GUI application that can be used to read/write to other
Zigbee radios including the SyntroZigbeeGateway device. It is a cross-platform
Qt GUI application, but does not require Syntro.

This application was written for testing radio communications external to Syntro.


  ZigbeeEndpoint
========

This directory contains demo projects designed to be Zigbee endpoint applications. 

Apps under Maple or Arduino are specific to those architectures.

All others can be assumed to be Linux applications unless otherwise specified.


  ZigbeeEndpoint/virt-regs
========

This is a Linux only console application written in plain C. It requires a 
serial port connected Zigbee radio. When running it simulates a device with 
32 32-bit virtual registers that can be read or written remotely over the Zigbee
interface.

virt-regs is tested mostly on small boards like the Gumstix or BeagleBone.

It can be used as a starting point for a real Zigbee endpoint application.


  ZigbeeEndpoint/Maple/zb_ledtoggle
========

A small demo using a LeafLabs Maple device as a Zigbee endpoint. It allows Syntro
cloud applications to control the Maple board LED via the SyntroZigbeeGateway.

It runs on any of the Maple boards: MapleMini, Maple and MapleNative


