
#ifndef __MYOS__MULTITASKING_H
#define __MYOS__MULTITASKING_H

#include <common/types.h>
#include <gdt.h>

namespace myos
{

    struct CPUState
    {
        common::uint32_t eax;
        common::uint32_t ebx;
        common::uint32_t ecx;
        common::uint32_t edx;

        common::uint32_t esi;
        common::uint32_t edi;
        common::uint32_t ebp;

        /*
        common::uint32_t gs;
        common::uint32_t fs;
        common::uint32_t es;
        common::uint32_t ds;
        */
        common::uint32_t error;

        common::uint32_t eip;
        common::uint32_t cs;
        common::uint32_t eflags;
        common::uint32_t esp;
        common::uint32_t ss;
    } __attribute__((packed));

    enum TaskState
    {
        READY,
        RUNNING,
        BLOCKED,
        FINISHED
    };
    class Task
    {
        friend class TaskManager;

    private:
        static common::uint32_t pIdCounter;
        common::uint8_t stack[4096]; // 4 KiB
        common::uint32_t pId = 0;
        common::uint32_t pPid = 0;
        TaskState taskState;
        common::uint32_t waitPid[128];
        int numChildren;
        int childIndex;
        int waitPidFor;
        CPUState *cpustate;

    public:
        Task();
        Task(GlobalDescriptorTable *gdt, void entrypoint());
        ~Task();
    };

    class TaskManager
    {
    private:
        Task tasks[256];
        int numTasks;
        int currentTask;
        GlobalDescriptorTable *gdt = nullptr;
        int getIndex(common::uint32_t pid);

    public:
        TaskManager(GlobalDescriptorTable *gdt);
        TaskManager();
        ~TaskManager();
        bool AddTask(Task *task);
        CPUState *Schedule(CPUState *cpustate);
        common::uint32_t ForkTask(CPUState *cpustate);
        common::uint32_t ExecTask(void entrypoint());
        void ExitProcess();
        void WaitProcess(common::uint32_t pid);
        void printTasks();
    };

}

#endif