// Copyright 2010 Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
// * Neither the name of Google Inc. nor the names of its contributors
//   may be used to endorse or promote products derived from this software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

extern "C" {
#include <signal.h>
#include <unistd.h>
}

#include <cstdlib>

#include <atf-c++.hpp>

#include "utils/defs.hpp"
#include "utils/fs/path.hpp"
#include "utils/process/children.ipp"
#include "utils/signals/exceptions.hpp"
#include "utils/signals/misc.hpp"

namespace fs = utils::fs;
namespace process = utils::process;
namespace signals = utils::signals;


namespace {


static void program_reset_raise(void) UTILS_NORETURN;

static void
program_reset_raise(void)
{
    struct ::sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (::sigaction(SIGUSR1, &sa, NULL) == -1)
        std::exit(EXIT_FAILURE);

    signals::reset(SIGUSR1);
    ::kill(::getpid(), SIGUSR1);

    // Should not be reached, but we do not assert this condition because we
    // want to exit cleanly if the signal does not abort our execution to let
    // the parent easily know what happened.
    std::exit(EXIT_SUCCESS);
}


}  // anonymous namespace


ATF_TEST_CASE_WITHOUT_HEAD(reset__ok);
ATF_TEST_CASE_BODY(reset__ok)
{
    // TODO(jmmv): We should have a child type that inherits both stdout and
    // stderr so that we do not have to specify files.
    std::auto_ptr< process::child_with_files > child =
        process::child_with_files::fork(program_reset_raise,
                                        fs::path("stdout.txt"),
                                        fs::path("stderr.txt"));
    process::status status = child->wait();
    ATF_REQUIRE(status.signaled());
    ATF_REQUIRE_EQ(SIGUSR1, status.termsig());
}


ATF_TEST_CASE_WITHOUT_HEAD(reset__immutable);
ATF_TEST_CASE_BODY(reset__immutable)
{
    ATF_REQUIRE_THROW(signals::system_error, signals::reset(SIGKILL));
    ATF_REQUIRE_THROW(signals::system_error, signals::reset(SIGSTOP));
}


ATF_INIT_TEST_CASES(tcs)
{
    ATF_ADD_TEST_CASE(tcs, reset__ok);
    ATF_ADD_TEST_CASE(tcs, reset__immutable);
}