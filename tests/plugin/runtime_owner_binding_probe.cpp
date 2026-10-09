#include "../../src/plugin/runtime_internal.hpp"

plugin::detail::RuntimeState* RuntimeOwnerFromSeparateTranslationUnit() noexcept
{
    return &plugin::detail::GetRuntimeState();
}
