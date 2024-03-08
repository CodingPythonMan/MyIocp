#include "Parser.h"
#include <stdio.h>
#include <string>

Parser::Parser() : buffer{ nullptr }
{
}

Parser::~Parser()
{
	free(buffer);
}

Parser* Parser::GetInstance()
{
	static Parser parser;
	return &parser;
}

void Parser::LoadFile(const char* fileName)
{
	FILE* file;
	fopen_s(&file, fileName, "r+");

	if (file == nullptr)
		return;

	fseek(file, 0, SEEK_END);
	int length = ftell(file);
	fseek(file, 0, SEEK_SET);

	buffer = (char*)malloc(length + 1);
	if (buffer == nullptr)
		return;

	fread_s(buffer, length + 1, 1, length + 1, file);
	buffer[length] = '\0';

	fclose(file);
}

bool Parser::GetValueInt(const char* valueName, int* column)
{
	int length = 0;
	char word[256];
	char* chrBuffer = buffer;

	while (1)
	{
		if (GetNextWord(&chrBuffer, &length))
		{
			// Word 버퍼에 찾은 단어 저장
			memset(word, 0, 256);
			memcpy(word, chrBuffer, length);
			word[255] = '\0';

			chrBuffer += length;
			// 인자로 입력받은 단어와 같은지 검사한다.
			if (0 == strcmp(valueName, word))
			{
				// 바로 뒤 = 을 찾는다.
				if (GetNextWord(&chrBuffer, &length))
				{
					memset(word, 0, 256);
					memcpy(word, chrBuffer, length);

					chrBuffer += length;
					if (0 == strcmp(word, "="))
					{
						// 바로 뒤 값을 찾는다.
						if (GetNextWord(&chrBuffer, &length))
						{
							memset(word, 0, 256);
							memcpy(word, chrBuffer, length);
							*column = atoi(word);
							return true;
						}
						return false;
					}
				}
				return false;
			}
		}
	}

	return false;
}

bool Parser::GetValueStr(const char* valueName, char** column)
{
	int length = 0;
	char word[256];
	char* chrBuffer = buffer;

	while (1)
	{
		if (GetNextWord(&chrBuffer, &length))
		{
			// Word 버퍼에 찾은 단어 저장
			memset(word, 0, 256);
			memcpy(word, chrBuffer, length);
			word[255] = '\0';

			chrBuffer += length;
			// 인자로 입력받은 단어와 같은지 검사한다.
			if (0 == strcmp(valueName, word))
			{
				// 바로 뒤 = 을 찾는다.
				if (GetNextWord(&chrBuffer, &length))
				{
					memset(word, 0, 256);
					memcpy(word, chrBuffer, length);

					chrBuffer += length;
					if (0 == strcmp(word, "="))
					{
						// 바로 뒤 값을 찾는다.
						if (GetNextWord(&chrBuffer, &length))
						{
							*column = new char[length + 1];
							memcpy(*column, chrBuffer, length);
							(*column)[length] = '\0';
							return true;
						}
						return false;
					}
				}
				return false;
			}
		}
	}

	return false;
}

bool Parser::GetValueWStr(const char* valueName, WCHAR** column)
{
	char* tempColumn;
	GetValueStr(valueName, &tempColumn);
	size_t size = strlen(tempColumn) + 1;
	*column = new WCHAR[size];

	size_t outSize;
	mbstowcs_s(&outSize, *column, size, tempColumn, size - 1);

	return false;
}

bool Parser::GetNextWord(char** chrBufferPtr, int* lengthPtr)
{
	// length 는 처음에 초기화
	*lengthPtr = 0;

	// 다음 유효 글자 첫번째까지 이동
	if (SkipToNextWord(chrBufferPtr))
	{
		return false;
	}

	char* chrBuffer = *chrBufferPtr;
	while (1)
	{
		if (*chrBuffer == ',' || *chrBuffer == '"' || *chrBuffer == 0x20 || *chrBuffer == 0x08
			|| *chrBuffer == 0x09 || *chrBuffer == 0x0a || *chrBuffer == 0x0d)
		{
			break;
		}

		chrBuffer++;
		(*lengthPtr)++;
	}

	if (*chrBuffer == '\0')
		return false;

	return true;
}

bool Parser::SkipToNextWord(char** chrBufferPtr)
{
	while (1)
	{
		char* chrBuffer = *chrBufferPtr;

		// 주석처리 스킵 
		if (*chrBuffer == '/')
		{
			if (*(chrBuffer + 1) == '\0')
			{
				break;
			}

			if (*(chrBuffer + 1) == '/')
			{
				// New Line 처리까지 스킵
				while (1)
				{
					if (*chrBuffer == 0x0d && *(chrBuffer + 1) == 0x0a)
						break;

					if (*chrBuffer == '\0')
						break;

					(*chrBufferPtr)++;
				}
			}
		}

		// 스페이스, 탭, 엔터코드
		// 0x20 스페이스
		// 0x08 백스페이스
		// 0x09 탭
		// 0x0a 라인 피드
		// 0x0d 캐리지 리턴
		if (*chrBuffer == 0x20 || *chrBuffer == 0x08 || *chrBuffer == 0x09 ||
			*chrBuffer == 0x0a || *chrBuffer == 0x0d || *chrBuffer == '{' || *chrBuffer == '"')
		{
			(*chrBufferPtr)++;
		}
		else
		{
			break;
		}
	}

	if (**chrBufferPtr == '\0')
		return true;

	return false;
}