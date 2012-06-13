//
//  Copyright (c) 2012 Pansenti, LLC.
//	
//  This file is part of Syntro
//
//  Syntro is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Syntro is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Syntro.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <syslog.h>
#include <time.h>

// our placeholder for real code
#include "virtual-registers.h"

void runCommandLoop(int fd);
int openPort(const char *port, int speed);
void closePort(int fd);
int setSigHandler();
void sigHandler(int sig);
unsigned char calculateChecksum(unsigned char *buff, int len);
void processCommand(int fd, unsigned char *rxBuff, int len);
void handleRead(int fd, unsigned char *rxBuff, int start, int dataLen);
void writePacket(int fd, unsigned char *txBuff, int len);
void handleWrite(unsigned char *rxBuff, int start, int dataLen); 
void debugDump(const char *prompt, unsigned char *buff, int len);
void msleep(int ms);
unsigned int getU32(unsigned char *buff);
void putU32(unsigned char *buff, unsigned int val);


// some globals
struct termios oldtio;
int shutdownTime;
int verbose;

void usage(const char *argv_0)
{
	printf("Usage: %s [-p <serial-port>] [-s <speed>]\n", argv_0);
	printf("Options:\n");
	printf("  -p <serial-port>  default is /dev/ttyUSB0\n");
	printf("  -s <speed>        default is 115200\n");
	printf("  -v                verbose debug output\n");
	printf("  -h                show this help\n\n");
	printf("  Example: %s -p /dev/ttyO1 -s 9600\n", argv_0);
	exit(1);
}

int main(int argc, char **argv)
{
	int opt, fd;
	char port[64];
	int speed = 115200;

	strcpy(port, "/dev/ttyUSB0");

	while ((opt = getopt(argc, argv, "p:s:vh")) != -1) {
		switch (opt) {
		case 'p':
			if (strlen(optarg) > sizeof(port) - 1) {
				printf("Port argument too long [%u] : %s\n", 
					(unsigned int)strlen(optarg), optarg);

				exit(1);
			}
				
			strcpy(port, optarg);
			break;

		case 's':
			speed = atoi(optarg);
			break;

		case 'v':
			verbose = 1;
			break;

		case 'h':
		default:
			usage(argv[0]);
			break;
		}
	}

	if (initRegisters() < 0)
		exit(1);

	if (setSigHandler() < 0)
		exit(1);

	fd = openPort(port, speed);

	if (fd < 0)
		exit(1);

	runCommandLoop(fd);

	closePort(fd);

	cleanupRegisters();

	return 0;					
}

int setSigHandler()
{
	struct sigaction sia;

	bzero(&sia, sizeof(sia));
	sia.sa_handler = sigHandler;

	if (sigaction(SIGINT, &sia, NULL) < 0) {
		perror("sigaction(SIGINT)");
		return -1;
	}
	else if (sigaction(SIGTERM, &sia, NULL) < 0) {
		perror("sigaction(SIGTERM)");
		return -1;
	}
	else if (sigaction(SIGHUP, &sia, NULL) < 0) {
		perror("sigaction(SIGHUP)");
		return -1;
	}

	return 0;
}

void sigHandler(int sig)
{
	if (sig == SIGINT || sig == SIGTERM || sig == SIGHUP)
		shutdownTime = 1;
}

int openPort(const char *port, int speed)
{
	struct termios tio;
	int baud, fd;

	switch (speed) {
	case 9600:
		baud = B9600;
		break;

	case 19200:
		baud = B19200;
		break;

	case 38400:
		baud = B38400;
		break;

	case 57600:
		baud = B57600;
		break;

	case 115200:
		baud = B115200;
		break;
	
	default:
		printf("Invalid port speed %d\n", speed);
		return -1;
	}
		
	if ((fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY)) < 0) {
		perror("open");
		return -1;
	}

	if (fcntl(fd, F_SETFL, 0) == -1) {
		perror("fnctl(FSETFL)");
		close(fd);
		return -1;
	}

	tcflush(fd, TCIFLUSH);
	tcgetattr(fd, &oldtio);
	bzero(&tio, sizeof(tio));
	tio.c_iflag = IGNPAR | IGNBRK;
	tio.c_cflag = baud | CS8 | CLOCAL | CREAD;
	tcsetattr(fd, TCSANOW, &tio);

	if (verbose)
		printf("Opened %s at %d\n", port, speed);

	return fd;
}

void closePort(int fd)
{
	// restore the port to original settings
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &oldtio);
	close(fd);
}

// arbitrary max length
#define MAX_RXCHARS 128
#define MAX_FRAMELEN 120

#define STATE_GET_START_DELIMITER      0
#define STATE_GET_LENGTH               1
#define STATE_GET_FRAME_TYPE           2
#define STATE_GET_DATA                 3
#define STATE_GET_CHECKSUM             4

#define ZIGBEE_START_DELIM             0x7E
#define ZIGBEE_FT_TRANSMIT_REQUEST     0x10
#define ZIGBEE_FT_RECEIVE_PACKET       0x90
#define ZIGBEE_FT_EXPLICIT_RX_IND      0x91


void runCommandLoop(int fd)
{
	unsigned char buff[MAX_RXCHARS];
	unsigned char c;
	int count, state, framelen;
	ssize_t readlen;

	bzero(buff, MAX_RXCHARS);	

	state = STATE_GET_START_DELIMITER;
	framelen = 0;
	count = 0;

	if (verbose)
		printf("Starting run loop\n");

	while (!shutdownTime) {
		readlen = read(fd, &c, 1);

		if (readlen < 0)
			break;
		
		if (readlen == 0) {
			msleep(50);
			continue;
		}

		//printf("--- %02X  count = %d  state = %d\n", c, count, state);

		switch (state) {
		case STATE_GET_START_DELIMITER:
			if (c == ZIGBEE_START_DELIM) {
				buff[0] = c;
				count = 1;
				state = STATE_GET_LENGTH;
			}

			break;
		
		case STATE_GET_LENGTH:
			buff[count++] = c;
			
			if (count == 3) {
				framelen = (buff[1] << 8) + buff[2];

				if (framelen <= MAX_RXCHARS)
					state = STATE_GET_FRAME_TYPE;
				else
					state = STATE_GET_START_DELIMITER;
			}

			break;

		case STATE_GET_FRAME_TYPE:
			buff[count++] = c;

			if (c == ZIGBEE_FT_RECEIVE_PACKET || c == ZIGBEE_FT_EXPLICIT_RX_IND)
				state = STATE_GET_DATA;
			else
				state = STATE_GET_START_DELIMITER;

			break;

		case STATE_GET_DATA:
			buff[count++] = c;

			if (count == framelen + 3)
				state = STATE_GET_CHECKSUM;

			break;

		case STATE_GET_CHECKSUM:
			buff[count++] = c;

			if (verbose)
				debugDump("rx:", buff, framelen + 4);

			c = calculateChecksum(buff + 3, framelen);

			if (c != buff[count - 1])
				printf("Invalid checksum\n");
			else
				processCommand(fd, buff, count);

			state = STATE_GET_START_DELIMITER;
			break;
		}
	}

	if (verbose)
		printf("Received shutdown signal\n");
}

unsigned char calculateChecksum(unsigned char *buff, int len)
{
	int i;
	unsigned char sum = buff[0];

	for (i = 1; i < len; i++)
		sum += buff[i];

	return 0xff - sum;
}

void processCommand(int fd, unsigned char *rxBuff, int framelen)
{
	int start, datalen;

	if (rxBuff[3] == ZIGBEE_FT_RECEIVE_PACKET)
		start = 15;
	else
		start = 21;

	datalen = framelen - (start + 1);

	if (datalen < 4) {
		printf("Received frame data only %d bytes\n", datalen);
		return;
	}

	//printf("-- framelen = %d  datalen = %d\n", framelen, datalen);

	// consider this a read command, the data is a mask of 'registers' to read
	if (datalen == 4) {
		handleRead(fd, rxBuff, start, datalen);
	}
	else {
		handleWrite(rxBuff, start, datalen);
	}	
}

void handleRead(int fd, unsigned char *rxBuff, int start, int dataLen)
{
	int i, j, count;
	unsigned int packetlen, mask, val;
	unsigned char *txBuff;

	unsigned int regs = getU32(rxBuff + start);
	
	for (i = 0, mask = 1, count = 0; i < 32; i++) {	
		if (mask & regs)
			count++;

		mask <<= 1;
	}

	if (count == 0)
		return;

	// 14 is zigbee transmit request packet overhead
	// 4 is the 32-bit mask for the records we are returning
	// 4 * count is the 32 bits for each data record
	packetlen = 14 + 4 + (count * 4);

	// add 4 for zigbee start delim, framelen and checksum
	txBuff = malloc(packetlen + 4);

	if (!txBuff)
		return;

	txBuff[0] = ZIGBEE_START_DELIM;
	txBuff[1] = 0xff & (packetlen >> 8);
	txBuff[2] = 0xff & packetlen;
	txBuff[3] = ZIGBEE_FT_TRANSMIT_REQUEST;
	txBuff[4] = 0; // we don't want a response
	
	// copy the src 64-bit address and src net address from rxBuff
	for (i = 5; i < 15; i++)
		txBuff[i] = rxBuff[i-1];

	txBuff[15] = 0; // broadcast radius, default
	txBuff[16] = 0; // no tx options

	// the bit mask of registers
	putU32(txBuff + 17, regs);

	for (i = 0, j = 21, mask = 1; i < 32; i++) {
		if (mask & regs) {
			if (verbose)
				printf("readRegister(%d)\n", i);

			val = readRegister(i);
			putU32(txBuff + j, val);
			j += 4;
		}

		mask <<= 1;
	}

	txBuff[packetlen + 3] = calculateChecksum(txBuff + 3, packetlen);

	if (verbose)
		debugDump("tx:", txBuff, packetlen + 4);

	writePacket(fd, txBuff, packetlen + 4);

	free(txBuff);
}

void writePacket(int fd, unsigned char *txBuff, int len)
{
	int count;
	int written = 0;

	while (written < len) {
		count = write(fd, txBuff + written, len - written);

		if (count < 0) {
			perror("write");
			break;
		}

		written += count;
	}
}

void handleWrite(unsigned char *rxBuff, int start, int dataLen)
{
	int i, j, count;
	unsigned int mask, val;

	unsigned int regs = getU32(rxBuff + start);

	for (i = 0, mask = 1, count = 0; i < 32; i++) {	
		if (mask & regs)
			count++;

		mask <<= 1;
	}

	if (count == 0)
		return;

	if (dataLen < 4 + (count * 4)) {
		printf("Invalid num bytes for write, expected = %d, got = %d\n",
			4 + (count * 4), dataLen);
		return;
	}

	for (i = 0, j = start + 4, mask = 1, count = 0; i < 32; i++) {	
		if (mask & regs) {
			val = getU32(rxBuff + j);
			j += 4;

			if (verbose)
				printf("writeRegister(%d, 0x%08X)\n", i, val); 

			writeRegister(i, val);
		}

		mask <<= 1;
	}
}

void debugDump(const char *prompt, unsigned char *buff, int len)
{
	int i;

	if (prompt)
		printf("%s ", prompt);

	for (i = 0; i < len; i++)
		printf("%02X ", buff[i]);

	printf("\n");
}

void msleep(int ms)
{
	struct timespec ts;

	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;

	nanosleep(&ts, NULL);
}

unsigned int getU32(unsigned char *buff)
{
	unsigned int val = buff[0];
	val <<= 8;
	val += buff[1];
	val <<= 8;
	val += buff[2];
	val <<= 8;
	val += buff[3];

	return val;
}

void putU32(unsigned char *buff, unsigned int val)
{
	buff[0] = 0xff & (val >> 24);
	buff[1] = 0xff & (val >> 16);
	buff[2] = 0xff & (val >> 8);
	buff[3] = 0xff & val;
}
