//===================== Copyright (c) Valve Corporation. All Rights Reserved. ======================
#include "threadtools.h"

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
CThread::CThread()
	: m_pThread( NULL )
{}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
CThread::~CThread()
{
	Join();
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void CThread::Start()
{
	if ( Init() )
	{
		m_pThread = new std::thread( &CThread::Run, this );
	}
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void CThread::Join()
{
	if ( m_pThread )
	{
		m_pThread->join();
		delete m_pThread;
		m_pThread = NULL;
	}
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
CThreadEvent::CThreadEvent( bool bManualReset )
{
	m_hSyncObject = CreateEvent( NULL, bManualReset, FALSE, NULL );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
CThreadEvent::~CThreadEvent()
{
	if ( m_hSyncObject )
	{
		CloseHandle( m_hSyncObject );
	}
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
bool CThreadEvent::Wait( uint32_t nTimeoutMs )
{
	return WaitForSingleObject( m_hSyncObject, nTimeoutMs ) == WAIT_OBJECT_0;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
bool CThreadEvent::Set()
{
	return SetEvent( m_hSyncObject ) != 0;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
bool CThreadEvent::Reset()
{
	return ResetEvent( m_hSyncObject ) != 0;
}

