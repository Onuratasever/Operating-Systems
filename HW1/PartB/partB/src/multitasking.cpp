
#include <multitasking.h>

using namespace myos;
using namespace myos::common;


void printDigit(common::int32_t);
void printf(char *);
void printfHex32(uint32_t key);

common::uint32_t Task::pIdCounter=1;

// This is the constructor of the Task class.
Task::Task()
{
    taskState=FINISHED;
    pPid=0;
    numChildren=0;
    childIndex = -1;
    waitPidFor = -2;
    priorityCounter = 0;
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    cpustate -> eax = 0;
    cpustate -> ebx = 0;
    cpustate -> ecx = 0;
    cpustate -> edx = 0;

    cpustate -> esi = 0;
    cpustate -> edi = 0;
    cpustate -> ebp = 0;
    cpustate -> eip = 0;
    cpustate -> cs = 0;
    cpustate -> eflags = 0x202;
    
}

// This is the constructor of the Task class.
Task::Task(GlobalDescriptorTable *gdt, void entrypoint(), int priority)
{
    numChildren=0;
    pPid=0;
    pId=pIdCounter++;
    childIndex = -1;
    waitPidFor = -2;
    this->priority=priority;
    priorityCounter=0;    

    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    
    cpustate -> eax = 0;
    cpustate -> ebx = 0;
    cpustate -> ecx = 0;
    cpustate -> edx = 0;

    cpustate -> esi = 0;
    cpustate -> edi = 0;
    cpustate -> ebp = 0;
    cpustate -> eip = (uint32_t)entrypoint;
    cpustate -> cs = gdt->CodeSegmentSelector();
    cpustate -> eflags = 0x202;
    
}

Task::~Task()
{
}

// This is the constructor of the TaskManager class.
TaskManager::TaskManager()
{
    numTasks = 0;
    currentTask = -1;
}
// This is the constructor of the TaskManager class.
TaskManager::TaskManager(GlobalDescriptorTable *gdt)
{
    gdt=gdt;
    numTasks = 0;
    currentTask = -1;
}
TaskManager::~TaskManager()
{
}

// This function is used to add a task to the task manager.
// It sets the state of the task as READY.
// It copies the register values of the task to the new task.
bool TaskManager::AddTask(Task* task)
{
    if(numTasks >= 256)
        return false;
    tasks[numTasks].taskState=READY;
    tasks[numTasks].pId=task->pId;
    //priortiy ile ilgili şeyler
    tasks[numTasks].priority=task->priority;
    tasks[numTasks].priorityCounter=tasks->priorityCounter;
    tasks[numTasks].cpustate = (CPUState*)(tasks[numTasks].stack + 4096 - sizeof(CPUState));
    
    tasks[numTasks].cpustate -> eax = task->cpustate->eax;
    tasks[numTasks].cpustate -> ebx = task->cpustate->ebx;
    tasks[numTasks].cpustate -> ecx = task->cpustate->ecx;
    tasks[numTasks].cpustate -> edx = task->cpustate->edx;

    tasks[numTasks].cpustate -> esi = task->cpustate->esi;
    tasks[numTasks].cpustate -> edi = task->cpustate->edi;
    tasks[numTasks].cpustate -> ebp = task->cpustate->ebp;
    
    tasks[numTasks].cpustate -> eip = task->cpustate->eip;
    tasks[numTasks].cpustate -> cs = task->cpustate->cs;
    
    tasks[numTasks].cpustate -> eflags = task->cpustate->eflags;
    numTasks++;
    return true;
}

//It tries to find proper process according to arguments.
int TaskManager::linearSearchForPriority(int priority, int priorityValue)
{
    int i;
    int index = -1;
    for(i=0; i< numTasks; i++)
    {
        if(tasks[i].taskState == READY && tasks[i].priority == priority && tasks[i].priorityCounter < priorityValue)
        {
            index = i;
            tasks[i].priorityCounter++;
            return index;
        }
        else if(tasks[i].taskState == BLOCKED && tasks[i].priority == priority)
        {
            //WaitpidFor -1 ise processin child'ı yoktur ve processin state'i ready yapılır.
            if(tasks[i].waitPidFor == -1)
            {
                // printf("WaitPidFor -1\n");
                if(tasks[i].numChildren == 0)
                {
                    tasks[i].taskState = READY;
                    if(tasks[i].priorityCounter < priorityValue)
                    {
                        index = i;
                        tasks[i].priorityCounter++;
                        return index;
                    }
                }
            }
            else if(tasks[i].waitPidFor > -1) // WaitPidFor'un -1'den büyük olması durumunda processin child'ı vardır ve child'ın state'i finished olmuşsa processin state'i ready yapılır.
            {
                int childIndex = getIndex(tasks[i].waitPidFor);
                if(tasks[childIndex].taskState == FINISHED)
                {
                    tasks[i].taskState = READY;
                    if(tasks[i].priorityCounter < priorityValue)
                    {
                        index = i;
                        tasks[i].priorityCounter++;
                        return index;
                    }
                    tasks[i].waitPidFor = -2;
                }
            }
        }
    }
    return index;
}

//I will use this function to reset priority counters when all tasks are blocked.
void TaskManager::setPriorityCountersZero()
{
    for(int i=0; i< numTasks; i++)
    {
        tasks[i].priorityCounter = 0;
    }
}

// This function is used to schedule the tasks in the task manager.
// It sets the state of the current task to READY.
// It tries to find next READY process
// If process is blocked, it checks if it is waiting for a child process to finish.
// If the children are exited, it sets the state of the process to READY.
// It returns the CPUState of the next task.
CPUState* TaskManager::Schedule(CPUState* cpustate)
{
    int highValue, mediumValue, lowValue = 0;
    if(numTasks <= 0)
        return cpustate;
    
    if(currentTask >= 0)
        tasks[currentTask].cpustate = cpustate;
    
    
    do
    {
        if(tasks[currentTask].taskState == RUNNING)
            tasks[currentTask].taskState = READY;

        highValue = linearSearchForPriority(High, 10);
     
        if(highValue == -1)
        {
            mediumValue =  linearSearchForPriority(Medium, 5);
          
            if(mediumValue == -1)
            {
                lowValue = linearSearchForPriority(Low, 1);
               
                if(lowValue == -1)
                {
                    // printf("All tasks are blocked. Resetting priority counterszozozozoo.\n");
                    setPriorityCountersZero();
                    lowValue = 0;
                }
                else
                    currentTask = lowValue;
            }
            else
                currentTask = mediumValue;
        }
        else
        {
            currentTask = highValue;
        }
            
    } while (lowValue == -1);
    

    tasks[currentTask].taskState = RUNNING;
    // printTasks();
    return tasks[currentTask].cpustate;
}

// This function is used to fork a task. It creates a new task with the same CPUState as the current task.
// It sets the new process id.
// It copies the stack of the current task to the new task.
// It sets the ebp of the new task.
//It sets the ecx of the current task as the process id of the new task.
// It sets the waitPid array of the parent process as the process id of the new task.
common::uint32_t TaskManager::Fork(CPUState* cpustate)
{
    if(numTasks >= 256)
        return 0;

    tasks[numTasks].taskState=READY;
    tasks[numTasks].pPid=tasks[currentTask].pId;
    tasks[numTasks].pId=Task::pIdCounter++;
    tasks[numTasks].priority=tasks[currentTask].priority;
    tasks[numTasks].priorityCounter = 0;
    for (int i = 0; i < sizeof(tasks[currentTask].stack); i++)
    {
        tasks[numTasks].stack[i]=tasks[currentTask].stack[i];
    }
    //This is for to set cpu state of the new task.
    common::uint32_t currentTaskOffset=(((common::uint32_t)cpustate - (common::uint32_t) tasks[currentTask].stack));
    tasks[numTasks].cpustate=(CPUState*)(((common::uint32_t) tasks[numTasks].stack) + currentTaskOffset);

    // This is for to set ebp of the new task.
    tasks[numTasks].cpustate->ebp =  (common::uint32_t)(cpustate->ebp) - ((common::uint32_t)(tasks[currentTask].stack) - (common::uint32_t)(tasks[numTasks].stack));

    tasks[numTasks].cpustate->ecx = 0;
    cpustate->ecx = tasks[numTasks].pId;
    
    tasks[currentTask].waitPid[tasks[currentTask].numChildren++]=tasks[numTasks].pId;
    tasks[numTasks].childIndex=tasks[currentTask].numChildren-1;
    
    numTasks++;
    // printf("ForkTask. Parent ID: \n");
    return tasks[numTasks-1].pId;
}

// This is for debugging purposes and it prints the tasks in the task manager.
void TaskManager::printTasks()
{
   int i;
    printf("\npId  pPid     State  Priority   CurrentTask\n");
    for(i=0; i< numTasks; i++)
    {
        printDigit(tasks[i].pId);  printf("     "); 
        printDigit(tasks[i].pPid); printf("      ");
        switch (tasks[i].taskState) 
        {
            case READY:      printf("Ready  "); break;
            case RUNNING:    printf("Running"); break;
            case BLOCKED:    printf("Blocked"); break;
            case FINISHED:   printf("Finished"); break;
            default:                    printf("Unknown"); break;
        }
        printf("    ");
        switch (tasks[i].priority) 
        {
            case Low:       printf("Low    "); break;
            case Medium:    printf("Medium "); break;
            case High:      printf("High   "); break;
            default:        printf("Unknown"); break;
        }
        printf("     ");
        printDigit(currentTask); 
        printf("     ");
        printDigit(tasks[i].priorityCounter); 
        printf("\n");
        
    }
    for ( i = 0; i < 1000000; i++)
    {
        /* code */
    }
    
}

// This function is used to execute a task. And it returns the address of the CPUState of the task.
// Eip is the entry point of the task. It is the address of the function that the task will execute.
common::uint32_t TaskManager::ExecTask(void entrypoint())
{
    //printf("ExecTask\n");
    if(numTasks >= 256 || currentTask < 0 || numTasks < 0) 
        return 0;
    
    tasks[numTasks].cpustate = (CPUState*)(tasks[currentTask].stack + 4096 - sizeof(CPUState));
    // tasks[numTasks].cpustate->eip = (uint32_t)entrypoint;
    // tasks[numTasks].cpustate->cs = gdt->CodeSegmentSelector();

    tasks[numTasks].cpustate -> eax = 0;
    tasks[numTasks].cpustate -> ebx = 0;
    tasks[numTasks].cpustate -> ecx = 0;
    tasks[numTasks].cpustate -> edx = 0;

    tasks[numTasks].cpustate -> esi = 0;
    tasks[numTasks].cpustate -> edi = 0;
    tasks[numTasks].cpustate -> ebp = 0;
    tasks[numTasks].cpustate -> eip = (uint32_t)entrypoint;
    tasks[numTasks].cpustate -> cs = gdt->CodeSegmentSelector();
    tasks[numTasks].cpustate -> eflags = 0x202;
    return (uint32_t)tasks[numTasks].cpustate;
}

// This function is used to exit a process. It sets the state of the current task to FINISHED.
// If the current task is the last task in the task manager, it does nothing.
// It sets the parent's waitPid array as -1 for the child process.
// It decrements the number of children of the parent process.
void TaskManager::ExitProcess()
{
    if(numTasks <= 0 || currentTask < 0) 
        return;
    tasks[currentTask].taskState = FINISHED;
    int parentIndex = getIndex(tasks[currentTask].pPid);
    tasks[parentIndex].waitPid[tasks[currentTask].childIndex]=-1;
    tasks[parentIndex].numChildren--;
    // printf("ExitProcess. Parent ID: ");
    // printDigit(tasks[currentTask].pPid);
    // printf("  numOfTasks: ");
    // printDigit(tasks[currentTask].numChildren);
    // printf("\n");
    // printTasks();
}

// This function is used to get the index of a task in the task manager by its process id.
int TaskManager::getIndex(common::uint32_t pid){
    int index=-1;
    for (int i = 0; i < numTasks; i++)
    {
        if(tasks[i].pId==pid){
            index=i;
            break;
        }
    }
    return index;
}

// This function is used to wait for a process to finish.
// Until the process with the given process id finishes, the current task is blocked.
// If pid is -1, it means parent waits for all children.
// If pid is greater than -1, it means parent waits for the child with the given pid.
void TaskManager::WaitProcess(int pid)
{
    if(numTasks <= 0 || currentTask < 0) 
        return;
    tasks[currentTask].taskState = BLOCKED;
    tasks[currentTask].waitPidFor=pid;
}