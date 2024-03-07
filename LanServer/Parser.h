#pragma once

class Parser
{
private:
	Parser();
	~Parser();

	bool GetNextWord(char** chrBuffer, int* length);
	bool SkipToNextWord(char** chrBufferPtr);

	char* buffer;

public:
	static Parser* GetInstance();

	void LoadFile(const char* fileName);

	// Get 하는 함수들
	bool GetValueInt(const char* valueName, int* column);
	bool GetValueStr(const char* valueName, char** column);

	int Version;
	int ServerID;
	char* Path;
	char* FileName;
};