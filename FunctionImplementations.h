#pragma once

#include "reentrancy_guard.h"
#include <psf_framework.h>

// A much bigger hammer to avoid reentrancy. Still, the impl::* functions are good to have around to prevent the
// unnecessary invocation of the fixup
inline thread_local psf::reentrancy_guard g_reentrancyGuard;

namespace impl
{
    inline auto CreateProcess = psf::detoured_string_function(&::CreateProcessA, &::CreateProcessW);
    inline auto InitializeProcThreadAttributeList = &::InitializeProcThreadAttributeList;
}
