/*

Copyright (C) 2017  by authors
Author: Jan Boon <support@no-break.space>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

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

#ifndef SEV_MAIN_EVENT_LOOP_H
#define SEV_MAIN_EVENT_LOOP_H

#include "config.h"
#ifdef SEV_MODULE_EVENT_LOOP
#ifdef SEV_MODULE_SINGLETON

#include "event_loop.h"
#include "shared_singleton.h"

namespace sev {

typedef std::function<void(EventLoop *el, const int argc, const char *argv[])> MainFunction;

class SEV_LIB MainEventLoop : public EventLoop, public SharedSingleton<MainEventLoop>
{
public:
	MainEventLoop();
	virtual ~MainEventLoop();
	
	static void main(MainFunction &&f, const int argc, const char *argv[]);
	inline static void main(MainFunction &f, const int argc, const char *argv[]) { main(MainFunction(f), argc, argv); }
	
private:
	MainEventLoop(MainEventLoop const&) = delete;
	MainEventLoop& operator=(MainEventLoop const&) = delete;
	
}; /* class MainEventLoop */

} /* namespace sev */

#endif /* #ifdef SEV_MODULE_SINGLETON */
#endif /* #ifdef SEV_MODULE_EVENT_LOOP */

#endif /* #ifndef SEV_MAIN_EVENT_LOOP_H */

/* end of file */
