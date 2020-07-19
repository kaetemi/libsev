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

#include "main_event_loop.h"
#ifdef SEV_MODULE_EVENT_LOOP
#ifdef SEV_MODULE_SINGLETON

namespace sev {

MainEventLoop::MainEventLoop()
{
	
}

MainEventLoop::~MainEventLoop()
{
	
}

void MainEventLoop::main(MainFunction &&f, int argc, const char *argv[])
{
	MainEventLoop::Instance instance = MainEventLoop::instance();
	EventLoop *const el = instance.get();
	el->immediate([f = std::move(f), el, argc, argv]() -> void {
		f(el, argc, argv);
	});
	el->runSync();
}

} /* namespace sev */

namespace sev {

namespace /* anonymous */ {

void f(sev::EventLoop *el, int argc, const char *argv[])
{
	// ...
}

void main(int argc, const char *argv[])
{
	MainEventLoop::main(f, argc, argv);
}

} /* anonymous namespace */

} /* namespace sev */

#endif /* #ifdef SEV_MODULE_SINGLETON */
#endif /* #ifdef SEV_MODULE_EVENT_LOOP */

/* end of file */
