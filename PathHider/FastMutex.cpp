#include "FastMutex.h"
    
void KUtils::FastMutex::Init() 
{ 
    ExInitializeFastMutex(&m_mutex); 
}

void KUtils::FastMutex::Lock() 
{ 
    ExAcquireFastMutex(&m_mutex); 
}

void KUtils::FastMutex::Unlock() 
{ 
    ExReleaseFastMutex(&m_mutex); 
}
