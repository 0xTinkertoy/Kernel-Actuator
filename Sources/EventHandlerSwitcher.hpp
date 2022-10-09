//
//  EventHandlerSwitcher.hpp
//  Kernel-ARM
//
//  Created by FireWolf on 2/5/21.
//

#ifndef EventHandlerSwitcher_hpp
#define EventHandlerSwitcher_hpp

#include "EventControlBlock.hpp"
#include "EventController.hpp"
#include <Debug.hpp>

extern "C" void KernelEntryPoint();

volatile UInt8* gUserStack;

struct EventHandlerSwitcher
{
    using ServiceIdentifier = int;

    using Task = EventControlBlock;

    static int switchTask([[maybe_unused]] EventControlBlock* prev, EventControlBlock* next)
    {
        [[maybe_unused]] auto& controller = KernelServiceRoutines::GetTaskController<EventController>();

        //pinfo("Will switch from handler %d to %d.", controller.getEventID(prev), controller.getEventID(next));

        gUserStack = next->getStackPointer();

        pinfo("Shared user stack pointer at %p.", gUserStack);

        pinfo("==> User");

        asm volatile(// Push all general-purpose registers, the return address and flags on the kernel stack
                     "push {r0-r12} \n"
                     "push {LR} \n"
                     "mrs r0, PSR \n"
                     "push {r0} \n"
                     
                     // Load the user stack pointer
                     "ldr r0, =gUserStack \n"
                     "ldr r0, [r0] \n"
                     
                     // Restore all callee-saved registers from the process stack
                     "ldmfd r0!, {r4-r11} \n"
                     
                     // Restore the process stack pointer
                     "msr PSP, r0 \n"

                     // Enable interrupts
                     "cpsie i \n"

                     // Exit the kernel and switch back to thread mode with SP_proc
                     // The processor will restore all caller-saved registers from the process stack
                     // i.e. Like a iret on x86.
                     "ldr pc, =0xFFFFFFFD \n"
                     // ---> User Process
                 "KernelEntryPoint:\n"
                     // <--- Kernel
                     // System call entry point
                     // Disable interrupts
                     "cpsid i \n"
                     // !!! Assume that the kernel uses SP_main, while user processes use `SP_proc`. !!!
                     // The processor has stored all caller-saved registers on the process stack
                     // Load the current process stack pointer
                     // - MRS: Read from a special-purpose register
                     // - MSR: Write to a special-purpose register
                     "mrs r0, PSP \n"
                     
                     // Save all callee-saved registers on the process stack
                     // Store multiple registers in decreasing order
                     // i.e. r11 is stored at the highest address, while r4 is stored at the lowest address
                     // i.e. Equivalent to pushing r11 first on the stack.
                     // The exclamation mark is the writeback suffix, so r0 will store the address of the stack top
                     "stmdb r0!, {r4-r11} \n"
                     
                     // Save the user stack pointer
                     "ldr r1, =gUserStack \n"
                     "str r0, [r1] \n"
                     
                     // Restore all flags, the return address and all general-purpose registers from the kernel stack
                     "pop {r0} \n"
                     "msr PSR, r0 \n"
                     "pop {LR} \n"
                     "pop {r0-r12} \n"
                     :
                     :
                     : "memory"
                     );

        pinfo("Kern <==");

        pinfo("Shared user stack pointer at %p.", gUserStack);

        next->setStackPointer((UInt8*) gUserStack);

        // The low 8 bits in the ICSR register stores the current IRQ handler number
        UInt32 irq = *reinterpret_cast<volatile UInt32*>(0xE000ED04) & 0xFF;

        pinfo("IRQ number is %d.", irq);

        // Check whether users have invoked a system call
        if (irq == 11)
        {
            irq = reinterpret_cast<Context*>(next->getStackPointer())->r0;
        }

        return irq;
    }
};

#endif /* EventHandlerSwitcher_hpp */
