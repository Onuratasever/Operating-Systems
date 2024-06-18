
#include <syscalls.h>
 
using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;
 
SyscallHandler::SyscallHandler(InterruptManager* interruptManager, uint8_t InterruptNumber)
:    InterruptHandler(interruptManager, InterruptNumber  + interruptManager->HardwareInterruptOffset())
{
}

SyscallHandler::~SyscallHandler()
{
}


void printf(char*);

uint32_t SyscallHandler::HandleInterrupt(uint32_t esp)
{
    CPUState* cpu = (CPUState*)esp;
    

    switch(cpu->eax)
    {
        case SYSCALLS::PRINTF_SYS:
            printf((char*)cpu->ebx);
            break;
        case SYSCALLS::FORK:
            InterruptHandler::sys_fork(cpu);
            break;
        case SYSCALLS::EXEC:
            esp = InterruptHandler::sys_exec(cpu->ebx);
            break;
        case SYSCALLS::EXIT:
            // printf("Exiting process\n");
            InterruptHandler::sys_exit();
            esp = InterruptHandler::ScheduleAgain(esp);
            break;
        case SYSCALLS::WAITPID:
            InterruptHandler::sys_wait(cpu->ebx);
            esp = InterruptHandler::ScheduleAgain(esp);
            break;
        default:
            break;
    }

    
    return esp;
}

int myos::fork()
{
    int pid;
    asm("int $0x80" :"=c" (pid): "a" (SYSCALLS::FORK));
    return pid;
}

int myos::execve(void entrypoint())
{
    int result;
    asm("int $0x80" : "=c" (result) : "a" (SYSCALLS::EXEC), "b" ((uint32_t)entrypoint));
    return result;
}

void myos::exit_process()
{
    asm("int $0x80" : : "a" (SYSCALLS::EXIT));
}

int myos::wait_process(common::uint8_t pid)
{
    asm("int $0x80" : : "a" (SYSCALLS::WAITPID),"b" (pid));
    
    return 0;
}

