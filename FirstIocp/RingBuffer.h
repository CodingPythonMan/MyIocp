#pragma once

#define RING_DEFAULT_SIZE 10000

class RingBuffer {
public:
	RingBuffer();
	RingBuffer(int bufferSize);

	~RingBuffer();

	int GetBufferSize();

	// 사용중인 용량 얻기
	int GetUseSize();

	// 남은 용량 얻기
	int GetFreeSize();

	// WritePos 에 데이타 넣음.
	int Enqueue(char* data, int size);

	// ReadPos 에서 데이타 가져옴. ReadPos 이동.
	int Dequeue(char* dest, int size);

	// ReadPos 에서 데이타 읽어옴. ReadPos 고정.
	int Peek(char* dest, int size);

	// 버퍼의 모든 데이터 삭제
	void ClearBuffer();

	// 버퍼 포인터로 외부에서 한방에 읽고, 쓸 수 있는 길이.(끊기지 않은 길이)
	// 원형 큐의 구조상 버퍼의 끝단에 있는 데이터는 끝 -> 처음으로 돌아가서
	// 2번에 데이터를 얻거나 넣을 수 있음. 이 부분에서 끊어지지 않은 길이를 의미
	int DirectEnqueueSize();
	int DirectDequeueSize();

	// 원하는 길이만큼 읽기위치 에서 삭제 / 쓰기 위치 이동
	int	MoveRear(int size);
	int	MoveFront(int size);

	// 버퍼의 Front 포인터 얻음.
	char* GetFrontBufferPtr();

	// 버퍼의 RearPos 포인터 얻음.
	char* GetRearBufferPtr();

	// 버퍼의 처음포인터 얻음.
	char* GetStartBufferPtr();

private:
	int Front;
	int Rear;
	int BufferSize;

	char* Buffer;
};