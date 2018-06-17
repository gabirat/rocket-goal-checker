#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include "SerialPort.h"

using namespace std;

void blinkArduino(SerialPort* arduino) {
    char data = 'g';
    arduino->writeSerialPort(&data, sizeof(data));
}

void somebodyScored(uint32_t totalGoals, SerialPort* serialDevice = NULL) {
    if(serialDevice != NULL) {
        blinkArduino(serialDevice);
    }
    cout << "GOAL'S BEEN SCORED!" << endl;
    cout << "Total goals: " << dec << totalGoals << endl;
}

DWORD_PTR dwGetModuleBaseAddress(DWORD dwProcID, TCHAR *szModuleName)
{
    DWORD_PTR dwModuleBaseAddress = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcID);
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 ModuleEntry32;
        ModuleEntry32.dwSize = sizeof(MODULEENTRY32);
        if (Module32First(hSnapshot, &ModuleEntry32))
        {
            do
            {
                if (_tcsicmp(ModuleEntry32.szModule, szModuleName) == 0)
                {
                    dwModuleBaseAddress = (DWORD_PTR)ModuleEntry32.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnapshot, &ModuleEntry32));
        }
        CloseHandle(hSnapshot);
    }
    return dwModuleBaseAddress;
}

int main() {
    SerialPort arduino("\\\\.\\COM3");
    if(!arduino.isConnected())
        cout << "Arduino is not connected!\n";

    HWND game = FindWindowA(NULL, "Rocket League (32-bit, DX9, Cooked)");
    //DWORD baseAddr = 0x29D7F00 + (0x1100000 - 1070000); //RocketLeague.exe+1967F00 or 29D7F00 (?) RocketLeague.exe = 1070000 (???) NOPE, now its 1100000, ok it just wasnt fixed address, now function finds correct adress of the module!
    DWORD gameModuleAddr = 0;
    DWORD baseAddr = 0;
    DWORD firstOffset = 0x34;
    DWORD seconOffset = 0x11C;
    DWORD processId = 0;
    DWORD tempAddr = 0;
    DWORD maybeScorebordClassAddr = 0;
    DWORD scoreAddr = 0;

    uint32_t lastSumOfScores = 0;
    uint32_t sumOfScores = 0;


    if(game == NULL) {
        cout << "Couldn't find the game. (Did you start it first?)\n";
        return -1;
    }

    cout << "Game found!\n";

    GetWindowThreadProcessId(game, &processId);

    gameModuleAddr = dwGetModuleBaseAddress(processId, _T("RocketLeague.exe"));
    baseAddr = gameModuleAddr + 0x1967F00;

    HANDLE openedGameProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, false, processId);

    if(processId == NULL) {
        cout << "Cannot obtain process :/\n";
        return -1;
    }

    ReadProcessMemory(openedGameProcessHandle, (PBYTE*)baseAddr, &tempAddr, sizeof(DWORD), 0);
    //cout << hex << scoreBaseAddr << endl;
    tempAddr += firstOffset;
    ReadProcessMemory(openedGameProcessHandle, (PBYTE*)tempAddr, &maybeScorebordClassAddr, sizeof(DWORD), 0);
    scoreAddr = maybeScorebordClassAddr + seconOffset;
    ReadProcessMemory(openedGameProcessHandle, (PBYTE*)(scoreAddr), &lastSumOfScores, sizeof(DWORD), 0);

    while(ReadProcessMemory(openedGameProcessHandle, (PBYTE*)(scoreAddr), &sumOfScores, sizeof(DWORD), 0)) {
        if(sumOfScores != lastSumOfScores) {
            somebodyScored((uint32_t)sumOfScores, &arduino);
        }
        lastSumOfScores = sumOfScores;
        Sleep(10);
    }

    cout << sumOfScores << endl;

    return 0;
}
