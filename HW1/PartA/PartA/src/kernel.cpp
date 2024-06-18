
#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecommunication/interrupts.h>
#include <syscalls.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <drivers/ata.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>

#include <drivers/amd_am79c973.h>
#include <net/etherframe.h>
#include <net/arp.h>
#include <net/ipv4.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/tcp.h>

// #define GRAPHICSMODE

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;
using namespace myos::net;

void printf(char *str)
{
    static uint16_t *VideoMemory = (uint16_t *)0xb8000;

    static uint8_t x = 0, y = 0;

    for (int i = 0; str[i] != '\0'; ++i)
    {
        switch (str[i])
        {
        case '\n':
            x = 0;
            y++;
            break;
        default:
            VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | str[i];
            x++;
            break;
        }

        if (x >= 80)
        {
            x = 0;
            y++;
        }

        if (y >= 25)
        {
            for (y = 0; y < 25; y++)
                for (x = 0; x < 80; x++)
                    VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | ' ';
            x = 0;
            y = 0;
        }
    }
}

void printfHex(uint8_t key)
{
    char *foo = "00";
    char *hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}
void printfHex16(uint16_t key)
{
    printfHex((key >> 8) & 0xFF);
    printfHex(key & 0xFF);
}
void printfHex32(uint32_t key)
{
    printfHex((key >> 24) & 0xFF);
    printfHex((key >> 16) & 0xFF);
    printfHex((key >> 8) & 0xFF);
    printfHex(key & 0xFF);
}

void printDigit(int digit)
{
    char buff[256];
    int n;
    int i;

    // check if the digit is positive or negative
    if (digit < 0)
    {
        digit *= -1;
        buff[0] = '-';
        i = n = 1;
    }
    else
    {
        i = n = 0;
    }

    do
    {
        buff[n] = '0' + (digit % 10);
        digit /= 10;
        ++n;
    } while (digit > 0);

    buff[n] = '\0';

    while (i < n / 2)
    {
        int temp = buff[i];
        buff[i] = buff[n - i - 1];
        buff[n - i - 1] = temp;
        ++i;
    }
    printf((char *)buff);
}

class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
public:
    void OnKeyDown(char c)
    {
        char *foo = " ";
        foo[0] = c;
        printf(foo);
    }
};

class MouseToConsole : public MouseEventHandler
{
    int8_t x, y;

public:
    MouseToConsole()
    {
        uint16_t *VideoMemory = (uint16_t *)0xb8000;
        x = 40;
        y = 12;
        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4 | (VideoMemory[80 * y + x] & 0xF000) >> 4 | (VideoMemory[80 * y + x] & 0x00FF);
    }

    virtual void OnMouseMove(int xoffset, int yoffset)
    {
        static uint16_t *VideoMemory = (uint16_t *)0xb8000;
        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4 | (VideoMemory[80 * y + x] & 0xF000) >> 4 | (VideoMemory[80 * y + x] & 0x00FF);

        x += xoffset;
        if (x >= 80)
            x = 79;
        if (x < 0)
            x = 0;
        y += yoffset;
        if (y >= 25)
            y = 24;
        if (y < 0)
            y = 0;

        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4 | (VideoMemory[80 * y + x] & 0xF000) >> 4 | (VideoMemory[80 * y + x] & 0x00FF);
    }
};

class PrintfUDPHandler : public UserDatagramProtocolHandler
{
public:
    void HandleUserDatagramProtocolMessage(UserDatagramProtocolSocket *socket, common::uint8_t *data, common::uint16_t size)
    {
        char *foo = " ";
        for (int i = 0; i < size; i++)
        {
            foo[0] = data[i];
            printf(foo);
        }
    }
};

class PrintfTCPHandler : public TransmissionControlProtocolHandler
{
public:
    bool HandleTransmissionControlProtocolMessage(TransmissionControlProtocolSocket *socket, common::uint8_t *data, common::uint16_t size)
    {
        char *foo = " ";
        for (int i = 0; i < size; i++)
        {
            foo[0] = data[i];
            printf(foo);
        }

        if (size > 9 && data[0] == 'G' && data[1] == 'E' && data[2] == 'T' && data[3] == ' ' && data[4] == '/' && data[5] == ' ' && data[6] == 'H' && data[7] == 'T' && data[8] == 'T' && data[9] == 'P')
        {
            socket->Send((uint8_t *)"HTTP/1.1 200 OK\r\nServer: MyOS\r\nContent-Type: text/html\r\n\r\n<html><head><title>My Operating System</title></head><body><b>My Operating System</b> http://www.AlgorithMan.de</body></html>\r\n", 184);
            socket->Disconnect();
        }

        return true;
    }
};

void sysprintf(char *str)
{
    asm("int $0x80" : : "a"(SYSCALLS::PRINTF_SYS), "b"(str));
}

uint32_t time = 10000000;

void taskA()
{
    int i = 0;
    while (true)
    {
        if (i % time == 0)
        {
            printf("A ");
            exit_process();
        }
        i++;
    }
}

void taskB()
{
    int i = 0;
    while (true)
    {
        if (i % time == 0)
            printf("B ");
        i++;
    }
}

void taskE()
{
    int i = 0;
    while (true)
    {
        if (i % time == 0)
            printf("E ");
        i++;
    }
}

void taskC()
{
    int i = 0;
    while (true)
    {
        if (i % time == 0)
            // printf("C ");
            i++;
    }
}

void execTestExamle1()
{
    printf("Buradan devam edecek\n");
    printf("execTestExamle1 ");
    printf(" finished.\n");
    int i = 0;
    while (true)
    {
        if (i % time == 0)
            printf("Exec2 ");
        i++;
    }
}
void execTestExamle()
{
    printf("execTestExamle ");
    printf(" start\n");
    int exec1 = execve(execTestExamle1);
    int i = 0;
    while (true)
    {
        if (i % time == 0)
            printf("Exec1 ");
        i++;
    }
}

int myos::fork();
void taskD()
{
    int i = 0;
    int counter = 0;
    int child = 1;
    printf("Before fork\n");
    child = myos::fork();
    printf("Child: ");
    printDigit(child);
    printf("\n");
    if (child == 0)
    {
        // sysprintf("Child");
        while (true)
        {
            if (i % time == 0)
            {
                printf("CHILD ");
                counter++;
            }
            if (counter == 10)
            {
                exit_process();
            }
            i++;
        }
    }
    else
    {
        wait_process(child);
        while (true)
        {
            // sysprintf("Parent");
            if (i % time == 0)
                printf("Parent ");
            i++;
        }
    }
}

void collatz_sequence(int n)
{
    printDigit(n);
    printf(" : ");
    while (n != 1)
    {
        if (n % 2 == 0)
        {
            n = n / 2;
        }
        else
        {
            n = 3 * n + 1;
        }
        printDigit(n);
        printf(" ");
    }
    printf("\n");
}

void long_running_program()
{
    int n = 1000;
    uint64_t result = 0;
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            result += i * j;
        }
    }
    printf("Result: ");
    printDigit(result);
    printf("\n");
    exit_process();
}

void TaskForkCollatz()
{
    int child = -1;
    int i = 0;
    child = myos::fork();
    // printf("Fork value: ");
    // printDigit(child);
    // printf("\n");

    if (child == 0)
    {
        collatz_sequence(3);
        exit_process();
    }
    else
    {
        wait_process(child);
        collatz_sequence(7);
        exit_process();
    }
}

void TaskForkLongRunningProgram()
{
    int child = -1;
    int i = 0;
    child = myos::fork();
    // printf("Fork value: ");
    // printDigit(child);
    // printf("\n");

    if (child == 0)
    {
        int exec1 = execve(long_running_program);
        exit_process();
    }
    else
    {
        wait_process(child);
        exit_process();
    }
}

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for (constructor *i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}

extern "C" void kernelMain(const void *multiboot_structure, uint32_t /*multiboot_magic*/)
{
    printf("Hello World! --- http://www.AlgorithMan.de\n");

    GlobalDescriptorTable gdt;

    TaskManager taskManager(&gdt);

    // Task task1(&gdt, taskA);
    // Task task2(&gdt, taskB);
    // Task task3(&gdt, taskC);
    // Task task4(&gdt, taskD);
    // Task task5(&gdt,execTestExamle);
    Task task6(&gdt, TaskForkCollatz);
    Task task7(&gdt, TaskForkCollatz);
    Task task8(&gdt, TaskForkCollatz);
    Task task9(&gdt, TaskForkLongRunningProgram);
    Task task10(&gdt, TaskForkLongRunningProgram);
    Task task11(&gdt, TaskForkLongRunningProgram);

    // taskManager.AddTask(&task1);
    // taskManager.AddTask(&task2);
    // taskManager.AddTask(&task3);
    // taskManager.AddTask(&task4);
    // taskManager.AddTask(&task5);
    taskManager.AddTask(&task6);
    taskManager.AddTask(&task7);
    taskManager.AddTask(&task8);
    taskManager.AddTask(&task9);
    taskManager.AddTask(&task10);
    taskManager.AddTask(&task11);

    InterruptManager interrupts(0x20, &gdt, &taskManager);
    SyscallHandler syscalls(&interrupts, 0x80);

    interrupts.Activate();

    while (1)
    {
#ifdef GRAPHICSMODE
        desktop.Draw(&vga);
#endif
    }
}
