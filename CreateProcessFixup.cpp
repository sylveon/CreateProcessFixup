
#include <psf_framework.h>
#include "FunctionImplementations.h"

template <typename CharT>
using startup_info_t = std::conditional_t<psf::is_ansi<CharT>, STARTUPINFOA, STARTUPINFOW>;

template <typename CharT>
using startup_info_ex_t = std::conditional_t<psf::is_ansi<CharT>, STARTUPINFOEXA, STARTUPINFOEXW>;

struct attribute_list_deleter : private std::default_delete<char[]>
{
    void operator()(LPPROC_THREAD_ATTRIBUTE_LIST ptr) const
    {
        DeleteProcThreadAttributeList(ptr);
        default_delete::operator()(reinterpret_cast<char*>(ptr));
    }
};

using unique_attribute_list = std::unique_ptr<std::remove_pointer_t<LPPROC_THREAD_ATTRIBUTE_LIST>, attribute_list_deleter>;

template <typename CharT>
BOOL WINAPI CreateProcessFixup(
    _In_opt_ const CharT* applicationName,
    _Inout_opt_ CharT* commandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES processAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES threadAttributes,
    _In_ BOOL inheritHandles,
    _In_ DWORD creationFlags,
    _In_opt_ LPVOID environment,
    _In_opt_ const CharT* currentDirectory,
    _In_ startup_info_t<CharT>* startupInfo,
    _Out_ LPPROCESS_INFORMATION processInformation) noexcept
{
    startup_info_ex_t<CharT> info{};
    unique_attribute_list attributeList;

    if ((creationFlags & EXTENDED_STARTUPINFO_PRESENT) == EXTENDED_STARTUPINFO_PRESENT && startupInfo->cb >= sizeof(startup_info_ex_t<CharT>))
    {
        info = *reinterpret_cast<startup_info_ex_t<CharT>*>(startupInfo);
    }
    else
    {
        info.StartupInfo = *startupInfo;
        info.StartupInfo.cb = sizeof(info);
        creationFlags |= EXTENDED_STARTUPINFO_PRESENT;
    }

    if (!info.lpAttributeList)
    {
        // disable our other fixup
        auto guard = g_reentrancyGuard.enter();

        SIZE_T size = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
        if (size != 0)
        {
            auto listStorage = std::make_unique<char[]>(size);
            if (InitializeProcThreadAttributeList(reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(listStorage.get()), 1, 0, &size))
            {
                attributeList.reset(reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(listStorage.release()));
                DWORD attribute = PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_OVERRIDE;
                if (UpdateProcThreadAttribute(attributeList.get(), 0, PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY, &attribute, sizeof(attribute), nullptr, nullptr))
                {
                    info.lpAttributeList = attributeList.get();
                }
            }
        }
    }
    else
    {
        // this *should* work because we also hooked InitializeProcThreadAttributeList to reserve some space for this.
        DWORD attribute = PROCESS_CREATION_DESKTOP_APP_BREAKAWAY_OVERRIDE;
        UpdateProcThreadAttribute(info.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY, &attribute, sizeof(attribute), nullptr, nullptr);
    }

    return impl::CreateProcess(applicationName, commandLine, processAttributes, threadAttributes, inheritHandles, creationFlags, environment, currentDirectory, &info.StartupInfo, processInformation);
}
DECLARE_STRING_FIXUP(impl::CreateProcess, CreateProcessFixup);
