// plugin_manager.hpp - v1.0
/*
 *  Copyright (c) 2007 Leigh Johnston.
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *     * Neither the name of Leigh Johnston nor the names of any
 *       other contributors to this software may be used to endorse or
 *       promote products derived from this software without specific prior
 *       written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 *  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "neolib.hpp"
#include <map>
#include <memory>
#include <boost/filesystem.hpp>
#include "observable.hpp"
#include "vector.hpp"
#include "module.hpp"
#include "reference_counted.hpp"
#include "i_plugin_manager.hpp"
#include "i_application.hpp"

namespace neolib
{
    class plugin_manager : public reference_counted<i_plugin_manager>, private observable<i_plugin_manager::i_subscriber>
    {
        // types
    private:
        typedef vector<i_string, string> plugin_file_extensions_t;
        typedef vector<i_string, string> plugin_folders_t;
        typedef std::map<uuid, std::unique_ptr<module>> modules_t;
        typedef vector<i_auto_ref<i_plugin>, auto_ref<i_plugin>> plugins_t;
        // construction
    public:
        plugin_manager(i_application& aApplication);
        ~plugin_manager();
        // implementation
    public:
        // from i_discoverable
        bool discover(const uuid& aId, void*& aObject) override;
        // from i_plugin_manager
        const plugin_file_extensions_t& plugin_file_extensions() const override;
        plugin_file_extensions_t& plugin_file_extensions() override;
        const plugin_folders_t& plugin_folders() const override;
        plugin_folders_t& plugin_folders() override;
        bool load_plugins() override;
        bool load_plugin(const i_string& aPluginPath) override;
        void enable_plugin(i_plugin& aPlugin, bool aEnable) override;
        bool plugin_enabled(const i_plugin& aPlugin) const override;
        void unload_plugins() override;
        const plugins_t& plugins() const override;
        const i_auto_ref<i_plugin>& find_plugin(const uuid& aId) const override;
        i_auto_ref<i_plugin>& find_plugin(const uuid& aId) override;
        bool open_uri(const i_string& aUri) override;
    public:
        void subscribe(i_subscriber& aObserver) override;
        void unsubscribe(i_subscriber& aObserver) override;
        // implementation
        // from observable<i_plugin_manager::i_subscriber>
    private:
        void notify_observer(observer_type& aObserver, notify_type aType, const void* aParameter, const void* aParameter2) override;
        // own
    private:
        i_auto_ref<i_plugin>& create_plugin(const i_string& aPluginPath);
        // attributes
    private:
        i_application& iApplication;
        plugin_file_extensions_t iPluginFileExtensions;
        plugin_folders_t iPluginFolders;
        modules_t iModules;
        plugins_t iPlugins;
        bool iInitializing;
    };
}
