// dllmain.cpp : Defines the entry point for the DLL application.
#include "Windows.h"
#include <iostream>
#include <vector>

HMODULE myhModule;
// __stdcall - особо соглашение о виклиці
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

    // Містить інформацію про цілий ряд сторінок у віртуальному адресному просторі процесу
    MEMORY_BASIC_INFORMATION mbi{ 0 };
    /**
    * Опис елементів структкри :
    * BaseAdress        - адрес, звітки було запрошено інформацію про участок пам'яті( = першому переданому параметру в функцію VirtualQuery)
    * RegionSize        -   говорить сумарний розмір групи сторінок,
                            які мають ті ж атрибути захисту, состіяніє і тип, що й сторінка по адресу BaseAdress
    * State             - говорить прос стан(MEM_FREE, MEM_RESERVE чи MEM_COMMIT), які входять до RegionSize 
    * Protect           - атрибути захисту сторінок
    */
    
    DWORD protect_flag = (PAGE_GUARD | PAGE_NOCACHE | PAGE_NOACCESS);

    for (auto i = start_adress; i < end_adress + signature.size(); ++i)
    {

        //std::cout << "stanning:" << std::hex << i << std::endl;

        // VirtualQuery - отримує інформацію про цілий ряд сторінок у адресному просторі гри
        // 1 переметр   - адреса, звідки починати сканування
        // 2 параметр   - місце, куди петрібно записати результати дії функції
        // ret          - кількість байтів, повернена в інформаційний буфер
        if( VirtualQuery( (LPCVOID)i, &mbi, sizeof(mbi) ) )
        { 
            // Переверяємо адреси на захищенність, якщо вони захищенні то пропускаємо їх
            // mbi.Protect & protect_flag - якщо значення захисту сходиться із значенням із protect_flag
            if (mbi.Protect & protect_flag || !(mbi.State & MEM_COMMIT))
            {
                i += mbi.RegionSize;    // Якщо декілька наступних адресів теж захищенні, ми їх пропускаєм
                continue;               // Якщо bad adress, тоді не читаємо його
            }

            for(DWORD k = (DWORD)mbi.BaseAddress; k < (DWORD)mbi.BaseAddress + mbi.RegionSize - signature.size(); ++k)
                for (DWORD j = 0; j < signature.size(); ++j)
                {
                    // Якщо байт в сигнатурі не доріюнює -1 і не довнює байту в сигнатурі із пам'яті, то переходимо до наступної адреси 
                    if (signature.at(j) != -1 && signature.at(j) != *(byte*)(k + j))
                        break;
                    // Якщо ми дійшли до кінця, це означає що сигнатури співпадають і можна вертати її адресу
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
    AllocConsole();                         // Відкриття вікна консолі
    FILE* pl;
    freopen_s(&pl, "CONOUT$", "w", stdout); // При виклику AllocConsole, потрібно перевідкрити std::cout, 
                                            //! для цього потрібно відкрити stdout з назвою файла "CONOUT$"
    std::cout << "test" << std::endl;       // Після цього cout буде пряцювати 

    while (1)
    {
        Sleep(100);
        if (GetAsyncKeyState(VK_ESCAPE))
            break;
        if (GetAsyncKeyState('1'))
        {
            // Сигнатура(тобто як виклядає функція в байтах) функціїї get_LocalPlayer
            std::vector<int> sig = { 0xA1, -1, -1, -1, -1, 0x8B, 0x15, -1, -1, -1, -1, 0x3B, 0x50, 0x04, 0x73, 0x05, 0x8B, 0x44, 0x90, 0x08 };
            DWORD entry = getAdressFromSignature(sig, 0x20000000, 0x30000000);
            if (entry == 0)
                entry = getAdressFromSignature(sig);
            std::cout << "Result: " << std::hex << entry << std::endl;
        }
    }

    fclose(pl);
    FreeConsole();                          // Закриває консоль
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
        // Якщо DLL файл був загружений
    case DLL_PROCESS_ATTACH:
        myhModule = hModule;
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Menue, NULL, 0, NULL);    // Створюємо віддільний потік

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

