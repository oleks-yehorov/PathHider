#include "FastMutex.h"
    
void KUtils::FastMutex::Init() 
{ 
    ExInitializeFastMutex(&m_mutex); 
}

_IRQL_raises_(APC_LEVEL) 
_IRQL_saves_global_(OldIrql, &m_mutex) 
void KUtils::FastMutex::Lock() 
{ 
    ExAcquireFastMutex(&m_mutex); 
}

_IRQL_requires_(APC_LEVEL) 
_IRQL_restores_global_(OldIrql, &m_mutex) 
void KUtils::FastMutex::Unlock() 
{ 
    ExReleaseFastMutex(&m_mutex); 
}
