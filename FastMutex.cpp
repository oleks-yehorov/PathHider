#include "FastMutex.h"
    
void KUtils::FastMutex::Init() 
{ 
    ExInitializeFastMutex(&_mutex); 
}

void KUtils::FastMutex::Lock() 
{ 
    ExAcquireFastMutex(&_mutex); 
}

void KUtils::FastMutex::Unlock() 
{ 
    ExReleaseFastMutex(&_mutex); 
}
