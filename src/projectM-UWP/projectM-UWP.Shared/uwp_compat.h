// Force-included into every libprojectM source file by projectM.Library.props.
//
// UWP apps have no process environment block, so the CRT leaves getenv undeclared.
// projectM only uses it to read optional debug flags. Reporting every variable as
// unset gives the same result as running with none of them defined.
#pragma once

#ifdef __cplusplus
#include <cstdlib>

namespace std {
inline char* getenv(const char*) noexcept
{
    return nullptr;
}
} // namespace std
#endif
