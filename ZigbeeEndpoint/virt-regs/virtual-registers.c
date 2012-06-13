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

#include <strings.h>

#define NUM_REGS 32

// virtRegister is just a placeholder for this simple example
unsigned int virtRegisters[NUM_REGS];

int initRegisters()
{
	// return -1 if you want to fail here

	// nothing to do for this example

	bzero(virtRegisters, sizeof(virtRegisters));
	
	return 0;
}

void cleanupRegisters()
{
	// nothing to do
}

unsigned int readRegister(int reg) 
{
	if (reg >= 0 && reg < NUM_REGS)
		return virtRegisters[reg];

	return 0;
}

void writeRegister(int reg, unsigned int val)
{
	if (reg >= 0 && reg < NUM_REGS)
		virtRegisters[reg] = val; 
} 
