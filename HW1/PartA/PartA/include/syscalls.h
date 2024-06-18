 
#ifndef __MYOS__SYSCALLS_H
#define __MYOS__SYSCALLS_H

#include <common/types.h>
#include <hardwarecommunication/interrupts.h>
#include <multitasking.h>

namespace myos
{
    enum SYSCALLS{EXIT, GETPID, WAITPID, FORK, EXEC, PRINTF, ADDTASK, PRINTF_SYS};
    
    class SyscallHandler : public hardwarecommunication::InterruptHandler
    {
        
    public:
        SyscallHandler(hardwarecommunication::InterruptManager* interruptManager, myos::common::uint8_t InterruptNumber);
        ~SyscallHandler();
        
        virtual myos::common::uint32_t HandleInterrupt(myos::common::uint32_t esp);

    };
    int fork();
    int execve(void entrypoint());
    void exit_process();
    int wait_process(common::uint8_t pid);
}


#endif