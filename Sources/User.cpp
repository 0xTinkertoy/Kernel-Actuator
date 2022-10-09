//
//  User.cpp
//  Kernel-ARM
//
//  Created by FireWolf on 2/23/21.
//

#include "User.hpp"
#include "Syscall.hpp"
#include "Message.hpp"

// Remove all printing if we are running experiments to measure the stack usage
#ifdef RUN_STACK_EXP
    #define sysprintf (void)
#endif

__attribute__((noreturn))
void idleHandler()
{
    while (true)
    {
        asm("wfi");
    }
}

void startWaterPlant()
{
    sysprintf("====================================\n");

    sysprintf("SWP: Receive a soil dry event. Will water the plant.\n");

    // Check whether we have water in the bottle
    if (!sysReadSensor(0))
    {
        sysprintf("SWP: Running out of water! Aborted watering the plant.\n");

        sysprintf("SWP: Will notify users to refill the water bottle.\n");

        sysSendEvent(kRunOutOfWaterEvent);

        return;
    }

    sysprintf("SWP: Prepare to open the gate.\n");

    if (!sysOpenGate())
    {
        sysprintf("SWP: The gate is already opened.\n");
    }
    else
    {
        sysprintf("SWP: The gate is now open.\n");
    }

    sysprintf("====================================\n");
}

void runOutOfWaterHandler()
{
    sysprintf("====================================\n");

    sysprintf("ROW: Prepare to send an alert to the user.\n");

    Message message = Message::runOutOfWaterAlert();
    
    if (sysSendData(&message, sizeof(message)))
    {
        sysprintf("ROW: Alert has been sent.\n");
    }
    else
    {
        sysprintf("ROW: Failed to send the alert.\n");
    }

    sysprintf("====================================\n");
}

void stopWaterPlant()
{
    sysprintf("====================================\n");

    sysprintf("EWP: Receive a soil wet event. Will stop watering the plant.\n");

    sysprintf("EWP: Prepare to close the gate.\n");

    if (!sysCloseGate())
    {
        sysprintf("EWP: The gate has already been closed.\n");

        return;
    }

    sysprintf("EWP: The gate has been closed. Sending an ACK message.\n");

    Message message = Message::ackSoilWet();

    if (sysSendData(&message, sizeof(message)))
    {
        sysprintf("EWP: Ack message has been sent.\n");
    }
    else
    {
        sysprintf("EWP: Failed to send the ACK message.\n");
    }

    sysprintf("====================================\n");
}