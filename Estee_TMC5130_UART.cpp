/*
MIT License

Copyright (c) 2017 Tom Magnier

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "Estee_TMC5130.h"

Estee_TMC5130_UART::Estee_TMC5130_UART(Stream& serial, uint8_t slaveAddress, uint32_t fclk)
: Estee_TMC5130(fclk), _serial(serial), _slaveAddress(slaveAddress)
{

}

uint32_t Estee_TMC5130_UART::readRegister(uint8_t address)
{
	uint8_t outBuffer[4], inBuffer[8];

	outBuffer[0] = SYNC_BYTE;
	outBuffer[1] = _slaveAddress;
	outBuffer[2] = address;

	computeCrc(outBuffer, 4);

	beginTransmission();
	_serial.write(outBuffer, 4);
	endTransmission();

	if (_serial.readBytes(inBuffer, 8) != 8) //Stream.setTimeout has to be set to a decent value to avoid blocking
		return 0xFFFFFFFF; //TODO return error code

	uint8_t receivedCrc = inBuffer[7];
	computeCrc(inBuffer, 8);

	if (receivedCrc != inBuffer[7])
		return 0xFFFFFFFF; //TODO return error code

	uint32_t data = 0;
	for (int i = 0; i < 4; i++)
		data += (inBuffer[3+i] << ((3-i)*8));

	return data;
}

uint8_t Estee_TMC5130_UART::writeRegister(uint8_t address, uint32_t data)
{
	uint8_t buffer[8];
	buffer[0] = SYNC_BYTE;
	buffer[1] = _slaveAddress;
	buffer[2] = address | WRITE_ACCESS;
	for (int i = 0; i < 4; i++)
		buffer[3+i] = (data & (0xFF << ((3-i)*8))) >> ((3-i)*8);

	computeCrc(buffer, 8);

	beginTransmission();
	_serial.write(buffer, 8);
	endTransmission();

	return 0;
}

void Estee_TMC5130_UART::setSlaveAddress(uint8_t slaveAddress)
{
	_slaveAddress = slaveAddress;
}

/* From Trinamic TMC5130A datasheet Rev. 1.14 / 2017-MAY-15 §5.2 */
void Estee_TMC5130_UART::computeCrc(uint8_t *datagram, uint8_t datagramLength)
{
	int i,j;
	uint8_t* crc = datagram + (datagramLength-1); // CRC located in last byte of message
	uint8_t currentByte;

	*crc = 0;
	for (i = 0; i < (datagramLength-1); i++)
	{
		currentByte = datagram[i];
		for (j = 0; j < 8; j++)
		{
			if ((*crc >> 7) ^ (currentByte & 0x01))
				*crc = (*crc << 1) ^ 0x07;
			else
				*crc = (*crc << 1);

			currentByte = currentByte >> 1;
		} // for CRC bit
	} // for message byte
}
