#include "pch.h"
#include "Lock.h"
#include "CoreMacro.h"
#include "CoreTLS.h"


void Lock::WriteLock()
{
	// ������ �����尡 �����ϰ� �ִٸ� ������ ����.
	const uint32 lockThreadId = (_lockFlag.load() & WRITE_THREAD_MASK) >> 16;
	if (LThreadId == lockThreadId)
	{
		//���� writelock�� ��� �ִ»��¿��� �ѹ��� ȣ���ϸ� ī��Ʈ�� �÷���
		_writeCount++;
		return;
	}

	// �ƹ��� ���� �� �����ϰ� ���� ���� ��, �����ؼ� �������� ��´�.
	const int64 beginTick = ::GetTickCount64();

	const uint32 desired = ((LThreadId << 16) & WRITE_THREAD_MASK);
	while (true)
	{
		for (uint32 spinCount = 0; spinCount < MAX_SPIN_COUNT; spinCount++)
		{
			uint32 expected = EMPTY_FLAG;
			//compare_exchange_strong
			//�� �Լ�, _lockFlag �� expected �� ���� ���ٸ�, _lockFlag�� desired�� �ٲ�
	        //���� �ʴٸ� expected�� _lockFlag�� �ٲ�
			if (_lockFlag.compare_exchange_strong(OUT expected, desired))
			{
				_writeCount++;  
				return;
			}
		}

		//������ ƽ���� ������� �ǵ������� ũ���ø� ȣ����
		if (::GetTickCount64() - beginTick >= ACQUIRE_TIMEOUT_TICK) 
			CRASH("LOCK_TIMEOUT");

		//�������� ��� �������� ���ý�Ʈ ����Ī
		this_thread::yield(); 
	}
}

void Lock::WriteUnlock()
{
	// ReadLock �� Ǯ�� ������ WriteUnlock �Ұ���.
	if ((_lockFlag.load() & READ_COUNT_MASK) != 0)
		CRASH("INVALID_UNLOCK_ORDER");

	const int32 lockCount = --_writeCount;
	if (lockCount == 0)
		_lockFlag.store(EMPTY_FLAG);
}

void Lock::ReadLock()
{
	// ������ �����尡 �����ϰ� �ִٸ� ������ ����.
	const uint32 lockThreadId = (_lockFlag.load() & WRITE_THREAD_MASK) >> 16;
	if (LThreadId == lockThreadId)
	{
		_lockFlag.fetch_add(1);
		return;
	}

	// �ƹ��� �����ϰ� ���� ���� �� �����ؼ� ���� ī��Ʈ�� �ø���.
	const int64 beginTick = ::GetTickCount64();
	while (true)
	{
		for (uint32 spinCount = 0; spinCount < MAX_SPIN_COUNT; spinCount++)
		{
			// �� �ε� : ��� �Լ� load
			// _lockFlag �� �޾ƿ�
			uint32 expected = (_lockFlag.load() & READ_COUNT_MASK);
			
			//�̰� �����ϸ� �ƹ��� write���� ������ ���¶�°�
			if (_lockFlag.compare_exchange_strong(OUT expected, expected + 1))
				return;
		}

		if (::GetTickCount64() - beginTick >= ACQUIRE_TIMEOUT_TICK)
			CRASH("LOCK_TIMEOUT");

		this_thread::yield();
	}
}

void Lock::ReadUnlock()
{
	if ((_lockFlag.fetch_sub(1) & READ_COUNT_MASK) == 0)
		CRASH("MULTIPLE_UNLOCK");
}