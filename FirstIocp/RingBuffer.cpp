#include "RingBuffer.h"
#include <cstring>

RingBuffer::RingBuffer() : BufferSize{ RING_DEFAULT_SIZE }
{
	Buffer = new char[BufferSize + 1];
	Front = 0;
	Rear = 0;
}

RingBuffer::RingBuffer(int bufferSize) : BufferSize{ bufferSize }
{
	Buffer = new char[BufferSize + 1];
	Front = 0;
	Rear = 0;
}

RingBuffer::~RingBuffer()
{
	delete[] Buffer;
}

int RingBuffer::GetBufferSize()
{
	return BufferSize;
}

int RingBuffer::GetUseSize()
{
	int size = Rear - Front;

	if (size < 0)
	{
		size += BufferSize + 1;
	}

	return size;
}

int RingBuffer::GetFreeSize()
{
	return BufferSize - GetUseSize();
}

int RingBuffer::Enqueue(char* src, int size)
{
	// 성능상 이유로, Enqueue 실패할 때는 Enqueue 를 호출하기전 걸러냈어야 한다.
	// 필요한 조건문은 GetFreeSize 로 조사하면 된다.
	if (size > 0)
	{
		if (Rear + size > BufferSize)
		{
			int rest = BufferSize - Rear;
			memcpy(&Buffer[Rear + 1], src, rest);
			memcpy(&Buffer[0], src + rest, size - rest);
			Rear += size - BufferSize - 1;
		}
		else
		{
			memcpy(&Buffer[Rear + 1], src, size);
			Rear += size;
		}
	}
	else
	{
		size = 0;
	}


	return size;
}

int RingBuffer::Dequeue(char* dest, int size)
{
	// 성능상 이유로, Dequeue 실패할 때는 Dequeue 를 호출하기전 걸러냈어야 한다.
	if (size > 0)
	{
		if (Front + size > BufferSize)
		{
			int rest = BufferSize - Front;
			memcpy(dest, &Buffer[Front + 1], rest);
			memcpy(dest + rest, &Buffer[0], size - rest);
			Front += size - BufferSize - 1;
		}
		else
		{
			memcpy(dest, &Buffer[Front + 1], size);
			Front += size;
		}
	}
	else
	{
		size = 0;
	}

	return size;
}

int RingBuffer::Peek(char* dest, int size)
{
	if (size > 0)
	{
		if (Front + size > BufferSize)
		{
			int rest = BufferSize - Front;
			memcpy(dest, &Buffer[Front + 1], rest);
			memcpy(dest + rest, &Buffer[0], size - rest);
		}
		else
		{
			memcpy(dest, &Buffer[Front + 1], size);
		}
	}
	else
	{
		size = 0;
	}

	return size;
}

void RingBuffer::ClearBuffer()
{
	Front = Rear;
}

int RingBuffer::DirectEnqueueSize()
{
	int size = Rear - Front;

	if (size < 0)
	{
		size = Front - Rear - 1;
	}
	else
	{
		size = BufferSize - Rear;
	}

	return size;
}

int RingBuffer::DirectDequeueSize()
{
	int size = Rear - Front;

	if (size < 0)
	{
		size = BufferSize - Front;
	}

	return size;
}

int RingBuffer::MoveRear(int size)
{
	if (size > 0)
	{
		if (Rear + size > BufferSize)
		{
			Rear += (size - BufferSize - 1);
		}
		else
		{
			Rear += size;
		}
	}

	return size;
}

int RingBuffer::MoveFront(int size)
{
	if (size > 0)
	{
		if (Front + size > BufferSize)
		{
			Front += (size - BufferSize - 1);
		}
		else
		{
			Front += size;
		}
	}

	return size;
}

char* RingBuffer::GetFrontBufferPtr()
{
	int IncreaseFront = 0;

	if (Front < BufferSize)
		IncreaseFront = Front + 1;

	return &Buffer[IncreaseFront];
}

char* RingBuffer::GetRearBufferPtr()
{
	int IncreaseRear = 0;

	if (Rear < BufferSize)
		IncreaseRear = Rear + 1;

	return &Buffer[IncreaseRear];
}

char* RingBuffer::GetStartBufferPtr()
{
	return Buffer;
}