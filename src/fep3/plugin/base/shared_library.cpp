/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */


#include <fep3/fep3_errors.h>
#include <fep3/fep3_filesystem.h>
#include <a_util/strings/strings_functions.h>
#include "shared_library.h"

namespace fep3
{
namespace plugin
{
namespace arya
{

SharedLibrary::SharedLibrary(const std::string& file_path, bool prevent_unloading)
    : _file_path(file_path)
    , _prevent_unloading(prevent_unloading)
{
    std::string trimmed_file_path = file_path;
    a_util::strings::trim(trimmed_file_path);
    fs::path full_file_path{trimmed_file_path};
    auto file_name = full_file_path.filename().string();
    full_file_path.remove_filename();

    // add prefix
#ifndef WIN32
    if(0 != file_name.find("lib"))
    {
        file_name = "lib" + file_name;
    }
#endif
    full_file_path /= file_name;
    if (full_file_path.extension().empty())
    {
        // add extension
#ifdef WIN32
        full_file_path.replace_extension("dll");
#else
        full_file_path.replace_extension("so");
#endif
    }

    const auto& full_file_path_string = full_file_path.string();
#ifdef WIN32
    // remember the cwd
    const auto& original_working_dir = fs::current_path();
    // on windows we need to switch to the directory where the library is located
    // to ensure loading of dependee dlls that reside in the same directory
    fs::current_path(full_file_path.parent_path());

    _library_handle = ::LoadLibrary(full_file_path_string.c_str());
    if (!_library_handle)
    {
        throw std::runtime_error("unable to load shared library '" + file_path + "'");
    }

    // switch back to the original cwd
    auto ec = std::error_code{};
    if (fs::current_path(original_working_dir, ec); ec) {
        throw std::runtime_error("unable to switch back to original working directory: "+
            original_working_dir.string() + "; error: '" + std::to_string(ec.value()) + " - " +
            ec.message() + "'; current working directory: " + full_file_path.parent_path().string());
    }
#else
    _library_handle = ::dlopen(full_file_path_string.c_str(), RTLD_LAZY);
    if (!_library_handle)
    {
        throw std::runtime_error("unable to load shared library '" + file_path + "': " + dlerror());
    }
#endif
}

SharedLibrary::~SharedLibrary()
{
    if(!_prevent_unloading)
    {
        if(nullptr != _library_handle)
        {
#ifdef WIN32
            ::FreeLibrary(_library_handle);
#else
            ::dlclose(_library_handle);
#endif
        }
    }
}

SharedLibrary::SharedLibrary(SharedLibrary&& rhs)
    : _library_handle(std::move(rhs._library_handle))
    , _file_path(std::move(rhs._file_path))
    , _prevent_unloading(std::move(rhs._prevent_unloading))
{
    rhs._library_handle = nullptr;
}

std::string SharedLibrary::getFilePath() const
{
    return _file_path;
}

} // namespace arya
} // namespace plugin
} // namespace fep3
