//
//  EventController.hpp
//  Kernel-ARM~Moisture
//
//  Created by FireWolf on 2/23/21.
//

#ifndef EventController_hpp
#define EventController_hpp

#include "EventControlBlock.hpp"
#include <Execution/SimpleEventDriven/KernelServiceRoutines.hpp>

using EventController = TableBasedEventController<EventControlBlock, Event, 4>;

#endif /* EventController_hpp */
