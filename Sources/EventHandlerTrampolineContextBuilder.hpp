//
//  EventHandlerTrampolineContextBuilder.hpp
//  Kernel-ARM
//
//  Created by FireWolf on 2/5/21.
//

#ifndef EventHandlerTrampolineContextBuilder_hpp
#define EventHandlerTrampolineContextBuilder_hpp

#include <Execution/SimpleEventDriven/EventHandlerTrampoline.hpp>
#include <Memory.h>
#include "EventControlBlock.hpp"
#include "EventController.hpp"

/// Architecture-dependent execution context builder
struct EventHandlerTrampolineContextBuilder_ARM
{
    void operator()(__attribute__((unused)) EventControlBlock* prev, EventControlBlock* next)
    {
        UInt8* sp = next->getStackPointer();

        pinfo("BEFORE: Shared stack pointer is %p.", sp);

        // Preserve the stack pointer to switch back to the previous event handler
        UInt8* oldStack = sp;

        // Setup the execution context for the trampoline function
        sp -= sizeof(Context);

        auto context = reinterpret_cast<Context*>(sp);

        memset(context, 0xCC, sizeof(Context));

        // Return address
        context->r14 = reinterpret_cast<UInt32>(nullptr);

        // 1st argument: Handler
        context->r0 = reinterpret_cast<UInt32>(next->getHandler());

        // 2nd argument: Old stack pointer
        context->r1 = reinterpret_cast<UInt32>(oldStack);

        // Program counter
        context->r15 = reinterpret_cast<UInt32>(EventHandlerTrampoline);

        // Program status
        // https://developer.arm.com/documentation/dui0552/a/the-cortex-m3-processor/programmers-model/core-registers?lang=en
        context->xpsr = 0x01000000;

        next->setStackPointer(sp);

        pinfo(" AFTER: Shared stack pointer is %p.", sp);
    }
};


#endif /* EventHandlerTrampolineContextBuilder_hpp */
