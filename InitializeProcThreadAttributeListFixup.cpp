
#include <psf_framework.h>
#include "FunctionImplementations.h"

BOOL WINAPI InitializeProcThreadAttributeListFixup(
    LPPROC_THREAD_ATTRIBUTE_LIST attributeList,
    DWORD attributeCount,
    DWORD flags,
    PSIZE_T size
)
{
    auto guard = g_reentrancyGuard.enter();
    if (guard)
    {
        attributeCount += 1;
    }

    return impl::InitializeProcThreadAttributeList(attributeList, attributeCount, flags, size);
}
DECLARE_FIXUP(impl::InitializeProcThreadAttributeList, InitializeProcThreadAttributeListFixup);
