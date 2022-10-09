//
//  EventScheduler.hpp
//  Kernel-ARM
//
//  Created by FireWolf on 2/5/21.
//

#ifndef EventScheduler_hpp
#define EventScheduler_hpp

#include <Scheduler/Scheduler.hpp>
#include "EventControlBlock.hpp"
#include "EventController.hpp"

struct EventScheduler;

namespace Scheduler::Traits
{
    template <>
    struct SchedulerTraits<EventScheduler>
    {
        using Task = EventControlBlock;
    };
}

struct EventScheduler: public Scheduler::Assembler<
    Scheduler::Policies::FIFO::Normal::LinkedListImp<EventControlBlock>,
    Scheduler::EventHandlers::TaskCreation::Cooperative::KeepRunningCurrentWithIdleTaskSupport<EventScheduler>,
    Scheduler::EventHandlers::TaskTermination::Common::RunNextWithIdleTaskSupport<EventScheduler>>
{
public:
    // MARK: Idle Task Support Component IMP
    
    ///
    /// Get the idle task
    ///
    /// @return The non-null idle task.
    ///
    [[nodiscard]]
    EventControlBlock* getIdleTask() const
    {
        return KernelServiceRoutines::GetTaskController<EventController>().getRegisteredEvent(0);
    }
};

#endif /* EventScheduler_hpp */
