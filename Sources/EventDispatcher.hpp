//
//  EventDispatcher.hpp
//  Kernel-ARM
//
//  Created by FireWolf on 2/5/21.
//

#ifndef EventDispatcher_hpp
#define EventDispatcher_hpp

#include <Execution/Common/Dispatcher.hpp>
#include <Execution/Common/KernelServiceRoutines.hpp>
#include <Execution/SimpleEventDriven/KernelServiceRoutines.hpp>
#include <Execution/SimpleEventDriven/EventHandlerTrampoline.hpp>
#include "EventHandlerTrampolineContextBuilder.hpp"
#include "EventHandlerSwitcher.hpp"
#include "EventScheduler.hpp"
//#include "UART/usart.hpp"
#include "UART/PL011.hpp"
#include "Message.hpp"
#include "EventController.hpp"

extern EventControlBlock gEventTable[4];

struct EventControlBlockMapper
{
    EventControlBlock* operator()(int event)
    {
        return KernelServiceRoutines::GetTaskController<EventController>().getRegisteredEvent(event);
    }
};

//
// MARK: - Define kernel service routine functions and the mapper for the dispatcher
//
namespace KernelServiceRoutines
{
    using SyscallSendEventRoutine = KernelServiceRoutines::SyscallSendEvent<EventControlBlock, EventScheduler, EventControlBlockMapper>;
    OSDefineAndRouteKernelRoutine(kSyscallSendEventRoutine, EventControlBlock, SyscallSendEventRoutine)

    using SyscallEventHandlerReturnRoutine = KernelServiceRoutines::SyscallEventHandlerReturn<EventControlBlock, EventScheduler>;
    OSDefineAndRouteKernelRoutine(kSyscallEventHandlerReturnRoutine, EventControlBlock, SyscallEventHandlerReturnRoutine)

    using SyscallUnknownIdentifierRoutine = KernelServiceRoutines::UnknownServiceIdentifier<EventControlBlock>;
    OSDefineAndRouteKernelRoutine(kSyscallUnknownIdentifier, EventControlBlock, SyscallUnknownIdentifierRoutine)

    // True if the bottle still has water
    static bool kWaterStatus = true;

    // True if the gate is open (and dripping water to the pot)
    static bool kWaterGateStatus = false;

    static EventControlBlock* kUART1ReceiveInterruptHandler(EventControlBlock* current)
    {
        pinfo("UART1 RX Interrupt.");

        Message message = {};

        PL011::receive(PL011::kUART1, message);

        auto next = current;

        if (message.magic == 0x4657)
        {
            switch (message.type)
            {
                // Controller Message
                case Message::Type::kChangeWaterStatus:
                {
                    kWaterStatus = message.data;

                    pmesg("Water status changed to [Has Water: %s].", kWaterStatus ? "YES" : "NO");

                    break;
                }

                // Sensor device message
                case Message::Type::kSoilDryAlert:
                {
                    // Wake up the handler that starts watering plant
                    next = GetTaskScheduler<EventScheduler>().onTaskCreated(current, GetTaskController<EventController>().getRegisteredEvent(1));

                    break;
                }

                // Sensor device message
                case Message::Type::kSoilWetAlert:
                {
                    // Wake up the handler that stops watering plant
                    next = GetTaskScheduler<EventScheduler>().onTaskCreated(current, GetTaskController<EventController>().getRegisteredEvent(3));

                    break;
                }
            }
        }

        PL011::clearRxInterrupt(PL011::kUART1);

        return next;
    }

    static EventControlBlock* kReadSensorRoutine(EventControlBlock* current)
    {
        current->setSyscallKernelReturnValue(kWaterStatus);

        return current;
    }

    static EventControlBlock* kSendDataRoutine(EventControlBlock* current)
    {
        const void* data = current->getSyscallArgument<const void*>();

        auto count = current->getSyscallArgument<size_t>();

        PL011::send(PL011::kUART1, data, count);

        current->setSyscallKernelReturnValue(count);

        return current;
    }

#ifndef RUN_STACK_EXP
    static EventControlBlock* kPrintRoutine(EventControlBlock* current)
    {
        const char* format = current->getSyscallArgument<const char*>();

        auto args = current->getSyscallArgument<va_list*>();

        kvprintf(format, *args);

        return current;
    }
#endif

    static EventControlBlock* kOpenGateRoutine(EventControlBlock* current)
    {
        if (kWaterGateStatus)
        {
            // Water gate is already open
            current->setSyscallKernelReturnValue(false);
        }
        else
        {
            // Water gate is closed
            kWaterGateStatus = true;

            current->setSyscallKernelReturnValue(true);
        }

        return current;
    }

    static EventControlBlock* kCloseGateRoutine(EventControlBlock* current)
    {
        if (kWaterGateStatus)
        {
            // Water gate is open
            kWaterGateStatus = false;

            current->setSyscallKernelReturnValue(true);
        }
        else
        {
            // Water gate is already closed
            current->setSyscallKernelReturnValue(false);
        }

        return current;
    }
}

struct EventDispatcherRoutineMapper
{
    using Task = EventControlBlock;

    using Routine = Task* (*)(Task*);

    using ServiceIdentifier = int;

    Routine operator()(const ServiceIdentifier& identifier)
    {
        using namespace KernelServiceRoutines;

        switch (identifier)
        {
            case 1:
                return kSyscallSendEventRoutine;

            case 2:
                return kSyscallEventHandlerReturnRoutine;

            case 3:
                return kReadSensorRoutine;

            case 4:
                return kSendDataRoutine;

#ifndef RUN_STACK_EXP
            case 5:
                return kPrintRoutine;
#endif

            case 6:
                return kOpenGateRoutine;

            case 7:
                return kCloseGateRoutine;

            case 22:
                return &kUART1ReceiveInterruptHandler;

            default:
                return kSyscallUnknownIdentifier;
        }
    }
};

//
// MARK: - Define additional code injector for the dispatcher
//

using Injector = CooperativeEventHandlerTrampolineContextInjector<EventControlBlock, EventHandlerTrampolineContextBuilder_ARM>;

//
// MARK: - Assemble a custom dispatcher for a simple event-driven system
//

using EventDispatcher = Dispatcher<EventControlBlock, int, EventDispatcherRoutineMapper, EventHandlerSwitcher, Injector>;

#endif /* EventDispatcher_hpp */
