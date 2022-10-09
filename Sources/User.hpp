//
//  User.hpp
//  Kernel-ARM
//
//  Created by FireWolf on 2/23/21.
//

#ifndef User_hpp
#define User_hpp

#include "Message.hpp"

//
// Deployment: Event Handlers
//
// Event Identifiers: (Prioritized)
// Event 0: Idle (Reserved)
// Event 1: Start Watering Plant
// Event 2: Run out of water
// Event 3: Stop Watering Plant
//

enum UserEvent
{
    kIdleEvent = 0,
    kStartWaterPlant = 1,
    kRunOutOfWaterEvent= 2,
    kStopWaterPlant = 3
};

__attribute__((noreturn))
void idleHandler();

void startWaterPlant();

void runOutOfWaterHandler();

void stopWaterPlant();

#endif /* User_hpp */
