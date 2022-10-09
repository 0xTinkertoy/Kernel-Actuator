//
//  Syscall.cpp
//  Kernel-ARM
//
//  Created by FireWolf on 2/5/21.
//

#include "Syscall.hpp"
#include <ARM/Syscall.hpp>
#include <cstdarg>

void sysSendEvent(int event)
{
    syscall(SyscallIdentifiers::SendEvent, event);
}

void sysEventHandlerReturn(uint8_t* oldStack)
{
    syscall(SyscallIdentifiers::EventHandlerReturn, oldStack);
}

int sysReadSensor(int id)
{
    return syscall(SyscallIdentifiers::ReadSensor, id);
}

size_t sysSendData(const void* bytes, size_t count)
{
    return syscall(SyscallIdentifiers::SendData, bytes, count);
}

void sysprintf(const char* format, ...)
{
    va_list args;

    va_start(args, format);

    syscall(SyscallIdentifiers::Print, format, &args);

    va_end(args);
}

int sysOpenGate()
{
    return syscall(SyscallIdentifiers::OpenGate);
}

int sysCloseGate()
{
    return syscall(SyscallIdentifiers::CloseGate);
}