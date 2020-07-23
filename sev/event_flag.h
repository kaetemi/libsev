/*

Copyright (C) 2020  Jan BOON (Kaetemi) <jan.boon@kaetemi.be>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/*

Event flag. Thread waits if flag is false, one thread continues when the flag is set.
Setting the flag multiple times has no effect, only one thread will continue.
Only one thread is supposed to wait for this flag.

*/

#pragma once
#ifndef SEV_EVENT_FLAG_H
#define SEV_EVENT_FLAG_H

#include "platform.h"

#include <atomic>
#include <condition_variable>

namespace sev {

class EventFlag
{
public:
	EventFlag(bool manualReset = false) : m_ResetValue(manualReset)
	{

	}

	~EventFlag()
	{
		// ...
	}

	void wait()
	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		bool flag = true;
		while (!m_Flag.compare_exchange_weak(flag, m_ResetValue))
		{
			flag = true;
			m_CondVar.wait(lock);
		}
	}

	void notify()
	{
		m_Flag = true;
	}

	void reset()
	{
		m_Flag = false;
	}

private:
	std::atomic_bool m_Flag;
	std::condition_variable m_CondVar;
	std::mutex m_Mutex;
	bool m_ResetValue;

};

}

#endif /* #ifndef SEV_EVENT_FLAG_H */

/* end of file */
