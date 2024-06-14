// MathClient.cpp : Client app for MathLibrary DLL.
// #include "pch.h" Uncomment for Visual Studio 2017 and earlier
#include <iostream>
#include "MathLibrary.h"
#include "MathLibrary2.h"
#include <process.h>
#include <windows.h>
#include <profileapi.h>

#pragma comment(lib, "winmm.lib")

LARGE_INTEGER _freq;


unsigned int WINAPI Thread001(LPVOID lpParam)
{
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    QueryPerformanceCounter(&start);

    for (int i = 0; i < 10000; i++)
    {
        fibonacci_init(1, 1);
        do {

        } while (fibonacci_next());
    }

    QueryPerformanceCounter(&end);
    double cal = (end.QuadPart - start.QuadPart) / (double)(_freq.QuadPart);

    std::wcout << "[DLL] : " << cal << "sec\n";

    return 0;
}

unsigned int WINAPI Thread002(LPVOID lpParam)
{
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    QueryPerformanceCounter(&start);

    for (int i = 0; i < 10000; i++)
    {
        MathLibrary2::fibonacci_init2(1, 1);
        do {

        } while (MathLibrary2::fibonacci_next2());
    }

    QueryPerformanceCounter(&end);

    double cal = (end.QuadPart - start.QuadPart) / (double)(_freq.QuadPart);

    std::wcout << "[LIB] : " << cal << "sec\n";
    
    return 0;
}

int main()
{
    timeBeginPeriod(1);
    QueryPerformanceFrequency(&_freq);

    HANDLE handles[2];
    handles[0] = (HANDLE)_beginthreadex(nullptr, 0, Thread001, nullptr, 0, nullptr);
    handles[1] = (HANDLE)_beginthreadex(nullptr, 0, Thread002, nullptr, 0, nullptr);
    
    WaitForMultipleObjects(2, handles, true, INFINITE);
}