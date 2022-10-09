//
//  Main.cpp
//  Kernel
//
//  Created by FireWolf on 1/27/21.
//

#include <Debug.hpp>
#include "ARM/v7-M/InterruptVectorTable.hpp"
#include <MemoryAllocator/FreeListAllocator.hpp>
#include <MemoryAllocator/MemoryAligners.hpp>
#include "EventController.hpp"
#include "EventDispatcher.hpp"
#include "EventScheduler.hpp"
#include "EventHandlerTrampolineContextBuilder.hpp"
#include "CMSIS/ARMCM3.h"
#include "UART/PL011.hpp"
#include "User.hpp"

//
// Deployment: User stack shared by all event handlers
//
OSDeclareSharedTaskStackPointer(gUserStackPointer);

//
// Deployment: Event scheduler
//
OSDeclareTaskSchedulerWithKernelServiceRoutine(EventScheduler, scheduler);

//
// Deployment: Event Controller
//
OSDeclareTaskControllerWithKernelServiceRoutine(EventController, controller);

//
// Deployment: Kernel Memory Allocator
//
OSDeclareMemoryAllocatorWithKernelServiceRoutines(FreeListAllocator<ConstantAligner<8>>, kMemoryAllocator);

//
// Deployment: Startup Routines
//
static void initMemoryManager()
{
    pinfo("Initializing the kernel memory allocator...");

    extern UInt8 sram, eram;

    pinfo("Free memory starts at 0x%p and ends at 0x%x (%d bytes).", &sram, &eram, &eram - &sram);

    passert(kMemoryAllocator.init(&sram, &eram - &sram), "Failed to configure the kernel memory allocator.");
}

static void initInterruptTable()
{
    pinfo("Initializing the kernel interrupt vector table...");

    InterruptVectorTable::setup();

    InterruptVectorTable::registerHandler(11, InterruptVectorTable::AssemblyHandler(KernelEntryPoint));
}

static void initUART1()
{
    pinfo("Configuring UART1...");

    // Both QEMU and Fast Model have UART0,1,2 enabled by default.
    // It is possible to disable all UART ports when the Fast Model platform starts.
    // If these ports are disabled, we need to initialize them manually as follows.
    // ```
    //    PL011::disableUART(PL011::kUART1);
    //    {
    //        PL011::disableAllInterrupts(PL011::kUART1);
    //        PL011::enableUARTRx(PL011::kUART1);
    //        PL011::enableUARTTx(PL011::kUART1);
    //        PL011::enableFIFO(PL011::kUART1);
    //        PL011::enableRxInterrupt(PL011::kUART1);
    //    }
    //    PL011::enableUART(PL011::kUART1);
    // ```

    // Turn on the RX interrupt on UART1
    // IRQ number is 22 (See LM3S811 Manual)
    PL011::enableRxInterrupt(PL011::kUART1);

    NVIC_EnableIRQ(Interrupt6_IRQn);

    NVIC_SetPriority(Interrupt6_IRQn, 255);

    InterruptVectorTable::registerHandler(22, InterruptVectorTable::AssemblyHandler(KernelEntryPoint));

    // By default, the hardware generates an interrupt once a byte is received
    // (i.e. The FIFO buffer can only hold a single byte)
    // We enable the FIFO buffer for both TX and RX channels
    // so that the device generates an interrupt once 8 bytes are received
    // FIFO buffers are 16 bytes long
    // We could also configure the watermark to choose a different level instead, such as 4 bytes.
    // Reference: Section 11.3.4 FIFO Operation in LM3S811 Manual

    // Enable the FIFO operations
    PL011::enableFIFO(PL011::kUART1);
}

static void initUserStack()
{
    // Allocate the shared user stack
    pinfo("Allocating the shared user stack.");

    auto ustack = new UInt8[1024];

    passert(ustack != nullptr, "Failed to allocate the shared user stack.");

    PL011::send(PL011::kUART1, Message::actuatorUserStack(reinterpret_cast<UInt32>(ustack)));

    gUserStackPointer = ustack + 1024;

    pinfo("Shared user stack at 0x%p.", ustack);

    pinfo("Initial user stack pointer at 0x%p.", gUserStackPointer);
}

static void initEvents()
{
    // Preconfigure event handlers
    pinfo("Preconfigure event handlers.");

    controller.registerEvent(kIdleEvent, idleHandler);

    controller.registerEvent(kStartWaterPlant, startWaterPlant);

    controller.registerEvent(kRunOutOfWaterEvent, runOutOfWaterHandler);

    controller.registerEvent(kStopWaterPlant, stopWaterPlant);
}

//
// Deployment: Kernel Main Routine
//
extern "C" void kmain(void)
{
    pinfo("Kernel main function started.");

    kprintf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    kprintf("Hello, Tinkertoy~Kernel-ARM~Actuator!\n");
    kprintf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    // Configure the kernel memory allocator
    initMemoryManager();

    // Configure the kernel interrupt table
    initInterruptTable();

    // Configure UART1 and RX interrupts
    initUART1();

    // Allocate shared stack for event handlers
    initUserStack();

    // Setup events and handlers
    initEvents();

    // Dispatcher
    // We assume that the idle handler was running before we first enter the dispatcher
    // We need to set up the execution context for the idle handler
    pinfo("Initialize the idle event handler.");

    EventHandlerTrampolineContextBuilder_ARM{}(nullptr, controller.getRegisteredEvent(0));

    pinfo("Enter the dispatcher.");

    EventDispatcher dispatcher(controller.getRegisteredEvent(0), controller.getRegisteredEvent(0));

    dispatcher.dispatch();
}
