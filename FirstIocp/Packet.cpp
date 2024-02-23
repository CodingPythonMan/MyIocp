#include "Packet.h"
#include <cstring>

Packet::Packet()
{
	Buffer = new char[MAX_PACKET_SIZE];
	BufferSize = MAX_PACKET_SIZE;
	ReadPos = 0;
	WritePos = 0;
}

Packet::Packet(int bufferSize)
{
	Buffer = new char[bufferSize];
	BufferSize = bufferSize;
	ReadPos = 0;
	WritePos = 0;
}

Packet::~Packet()
{
	delete[] Buffer;
}

void Packet::Clear(void)
{
	ReadPos = 0;
	WritePos = 0;
}

int Packet::MoveWritePos(int size)
{
	if (size > 0)
	{
		WritePos += size;

		if (WritePos + size > BufferSize)
		{
			__debugbreak();
		}
	}
	else
	{
		size = 0;
	}

	return size;
}

int Packet::MoveReadPos(int size)
{
	if (size > 0)
	{
		ReadPos += size;

		if (ReadPos + size > BufferSize)
		{
			__debugbreak();
		}
	}
	else
	{
		size = 0;
	}

	return size;
}

Packet& Packet::operator=(Packet& srcPacket)
{
	ReadPos = srcPacket.ReadPos;
	WritePos = srcPacket.WritePos;

	if (srcPacket.GetBufferSize() != BufferSize)
	{
		delete[] Buffer;
		BufferSize = srcPacket.BufferSize;
		Buffer = new char[BufferSize];
	}

	memcpy(Buffer + ReadPos, srcPacket.GetBufferPtr(), WritePos - ReadPos);

	return *this;
}

Packet& Packet::operator<<(unsigned char value)
{
	Buffer[WritePos] = value;
	WritePos++;

	return *this;
}

Packet& Packet::operator<<(char value)
{
	Buffer[WritePos] = value;
	WritePos++;

	return *this;
}

Packet& Packet::operator<<(short value)
{
	memcpy(&Buffer[WritePos], &value, sizeof(short));
	WritePos += sizeof(short);

	return *this;
}

Packet& Packet::operator<<(unsigned short value)
{
	memcpy(&Buffer[WritePos], &value, sizeof(unsigned short));
	WritePos += sizeof(unsigned short);

	return *this;
}

Packet& Packet::operator<<(int value)
{
	memcpy(&Buffer[WritePos], &value, sizeof(int));
	WritePos += sizeof(int);

	return *this;
}

Packet& Packet::operator<<(unsigned int value)
{
	memcpy(&Buffer[WritePos], &value, sizeof(unsigned int));
	WritePos += sizeof(unsigned int);

	return *this;
}

Packet& Packet::operator<<(long value)
{
	memcpy(&Buffer[WritePos], &value, sizeof(long));
	WritePos += sizeof(long);

	return *this;
}

Packet& Packet::operator<<(float value)
{
	memcpy(&Buffer[WritePos], &value, sizeof(float));
	WritePos += sizeof(float);

	return *this;
}

Packet& Packet::operator<<(__int64 value)
{
	memcpy(&Buffer[WritePos], &value, sizeof(__int64));
	WritePos += sizeof(__int64);

	return *this;
}

Packet& Packet::operator<<(double value)
{
	memcpy(&Buffer[WritePos], &value, sizeof(double));
	WritePos += sizeof(double);

	return *this;
}

Packet& Packet::operator>>(unsigned char& value)
{
	value = Buffer[ReadPos];
	ReadPos++;

	return *this;
}

Packet& Packet::operator>>(char& value)
{
	value = Buffer[ReadPos];
	ReadPos++;

	return *this;
}

Packet& Packet::operator>>(short& value)
{
	value = *((short*)(Buffer + ReadPos));
	ReadPos += sizeof(short);

	return *this;
}

Packet& Packet::operator>>(unsigned short& value)
{
	value = *((unsigned short*)(Buffer + ReadPos));
	ReadPos += sizeof(unsigned short);

	return *this;
}

Packet& Packet::operator>>(int& value)
{
	value = *((int*)(Buffer + ReadPos));
	ReadPos += sizeof(int);

	return *this;
}

Packet& Packet::operator>>(unsigned int& value)
{
	value = *((unsigned int*)(Buffer + ReadPos));
	ReadPos += sizeof(unsigned int);

	return *this;
}

Packet& Packet::operator>>(float& value)
{
	value = *((float*)(Buffer + ReadPos));
	ReadPos += sizeof(float);

	return *this;
}

Packet& Packet::operator>>(__int64& value)
{
	value = *((__int64*)(Buffer + ReadPos));
	ReadPos += sizeof(__int64);

	return *this;
}

Packet& Packet::operator>>(double& value)
{
	value = *((double*)(Buffer + ReadPos));
	ReadPos += sizeof(double);

	return *this;
}

int Packet::GetData(char* chpDest, int size)
{
	if (size > 0)
	{
		memcpy(chpDest, &Buffer[ReadPos], size);
		ReadPos += size;
	}
	else
	{
		size = 0;
	}

	return size;
}

int Packet::PutData(char* chpSrc, int size)
{
	if (size > 0)
	{
		memcpy(&Buffer[WritePos], chpSrc, size);
		WritePos += size;
	}
	else
	{
		size = 0;
	}

	return size;
}
