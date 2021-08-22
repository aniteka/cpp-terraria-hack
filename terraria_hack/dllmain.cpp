// dllmain.cpp : Defines the entry point for the DLL application.
#include "Windows.h"
#include <iostream>
#include <vector>

HMODULE myhModule;
// __stdcall - ����� ���������� � �������
DWORD __stdcall EjectThread(LPVOID lpParametr)
{
    Sleep(100);
    FreeLibraryAndExitThread(myhModule, 0);
}

DWORD getAdressFromSignature(std::vector<int> signature, DWORD start_adress = 0, DWORD end_adress = 0)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    
    if(start_adress == 0)
        start_adress = (DWORD)si.lpMinimumApplicationAddress;
    if(end_adress == 0)
        end_adress = (DWORD)si.lpMaximumApplicationAddress;

    // ̳����� ���������� ��� ����� ��� ������� � ����������� ��������� ������� �������
    MEMORY_BASIC_INFORMATION mbi{ 0 };
    /**
    * ���� �������� ��������� :
    * BaseAdress        - �����, ����� ���� ��������� ���������� ��� ������� ���'��( = ������� ���������� ��������� � ������� VirtualQuery)
    * RegionSize        -   �������� �������� ����� ����� �������,
                            �� ����� � � �������� �������, ������ � ���, �� � ������� �� ������ BaseAdress
    * State             - �������� ���� ����(MEM_FREE, MEM_RESERVE �� MEM_COMMIT), �� ������� �� RegionSize 
    * Protect           - �������� ������� �������
    */
    
    DWORD protect_flag = (PAGE_GUARD | PAGE_NOCACHE | PAGE_NOACCESS);

    for (auto i = start_adress; i < end_adress + signature.size(); ++i)
    {

        //std::cout << "stanning:" << std::hex << i << std::endl;

        // VirtualQuery - ������ ���������� ��� ����� ��� ������� � ��������� ������� ���
        // 1 ��������   - ������, ����� �������� ����������
        // 2 ��������   - ����, ���� ������� �������� ���������� 䳿 �������
        // ret          - ������� �����, ��������� � ������������� �����
        if( VirtualQuery( (LPCVOID)i, &mbi, sizeof(mbi) ) )
        { 
            // ����������� ������ �� �����������, ���� ���� �������� �� ���������� ��
            // mbi.Protect & protect_flag - ���� �������� ������� ��������� �� ��������� �� protect_flag
            if (mbi.Protect & protect_flag || !(mbi.State & MEM_COMMIT))
            {
                i += mbi.RegionSize;    // ���� ������� ��������� ������ ��� ��������, �� �� ���������
                continue;               // ���� bad adress, ��� �� ������ ����
            }

            for(DWORD k = (DWORD)mbi.BaseAddress; k < (DWORD)mbi.BaseAddress + mbi.RegionSize - signature.size(); ++k)
                for (DWORD j = 0; j < signature.size(); ++j)
                {
                    // ���� ���� � �������� �� ������� -1 � �� ������ ����� � �������� �� ���'��, �� ���������� �� �������� ������ 
                    if (signature.at(j) != -1 && signature.at(j) != *(byte*)(k + j))
                        break;
                    // ���� �� ����� �� ����, �� ������ �� ��������� ���������� � ����� ������� �� ������
                    if (j + 1 == signature.size())
                        return k;
                }
            i = (DWORD)mbi.BaseAddress + mbi.RegionSize;

        }

    }

    return 0;
}

DWORD WINAPI Menue()
{
    AllocConsole();                         // ³������� ���� ������
    FILE* pl;
    freopen_s(&pl, "CONOUT$", "w", stdout); // ��� ������� AllocConsole, ������� ����������� std::cout, 
                                            //! ��� ����� ������� ������� stdout � ������ ����� "CONOUT$"
    std::cout << "test" << std::endl;       // ϳ��� ����� cout ���� ��������� 

    while (1)
    {
        Sleep(100);
        if (GetAsyncKeyState(VK_ESCAPE))
            break;
        if (GetAsyncKeyState('1'))
        {
            // ���������(����� �� ������� ������� � ������) �������� get_LocalPlayer
            std::vector<int> sig = { 0xA1, -1, -1, -1, -1, 0x8B, 0x15, -1, -1, -1, -1, 0x3B, 0x50, 0x04, 0x73, 0x05, 0x8B, 0x44, 0x90, 0x08 };
            DWORD entry = getAdressFromSignature(sig, 0x20000000, 0x30000000);
            if (entry == 0)
                entry = getAdressFromSignature(sig);
            std::cout << "Result: " << std::hex << entry << std::endl;
        }
    }

    fclose(pl);
    FreeConsole();                          // ������� �������
    CreateThread(0, 0, EjectThread, 0, 0, 0);
    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
        // ���� DLL ���� ��� ����������
    case DLL_PROCESS_ATTACH:
        myhModule = hModule;
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Menue, NULL, 0, NULL);    // ��������� �������� ����

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

