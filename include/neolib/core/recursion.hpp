// recursion.hpp
/*
 *  Copyright (c) 2019, 2020 Leigh Johnston.
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

#include <neolib/neolib.hpp>

namespace neolib
{
    template <typename Tag>
    class recursion_limiter
    {
    public:
        struct too_deep : std::runtime_error { too_deep() : std::runtime_error(std::string{ "Maximum recursion depth for '" } + typeid(Tag).name() + "' exceeded") {} };
    public:
        recursion_limiter() :
            iMaxDepth{ Tag::RecursionLimit }
        {
            if (++depth() > max_depth())
                throw too_deep();
        }
        recursion_limiter(std::size_t aMaxDepth) :
            iMaxDepth{ aMaxDepth }
        {
            if (++depth() > max_depth())
                throw too_deep();
        }
        ~recursion_limiter()
        {
            --depth();
        }
    public:
        std::size_t max_depth() const
        {
            return iMaxDepth;
        }
        static std::size_t& depth()
        {
            thread_local std::size_t tDepth;
            return tDepth;
        }
    private:
        const std::size_t iMaxDepth;
    };
}

#define _limit_recursion_(a) neolib::recursion_limiter<a> _##a##_recursion_limiter_
#define _limit_recursion_to_(a, b) neolib::recursion_limiter<a> _##a##_recursion_limiter_{##b##}
