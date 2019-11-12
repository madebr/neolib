// i_simple_variant.hpp - v1.0
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

#include <neolib/neolib.hpp>
#include <string>
#include <boost/lexical_cast.hpp>
#include <neolib/i_reference_counted.hpp>
#include <neolib/i_string.hpp>
#include <neolib/string.hpp>
#include <neolib/i_enum.hpp>
#include <neolib/i_custom_type.hpp>

namespace neolib
{
    enum class simple_variant_type
    {
        Empty,
        Boolean,
        Integer,
        Real,
        String,
        Enum,
        CustomType,
        COUNT
    };

    class i_simple_variant : public i_reference_counted
    {
    public:
        struct unknown_type : std::logic_error { unknown_type() : std::logic_error("neolib::i_variant::unknown_type") {} };
        struct type_mismatch : std::logic_error { type_mismatch() : std::logic_error("neolib::i_variant::type_mismatch") {} };
        struct unsupported_operation : std::logic_error { unsupported_operation(const std::string& aReason) : std::logic_error("neolib::simple_variant::unsupported_operation (" + aReason + ")") {} };
    public:
        virtual i_simple_variant& operator=(const i_simple_variant& aOther) = 0;
    public:
        virtual simple_variant_type type() const = 0;
        bool is(simple_variant_type aType) const { return type() == aType; }
        bool empty() const { return is(simple_variant_type::Empty); }
    public:
        virtual const bool& value_as_boolean() const = 0;
        virtual bool& value_as_boolean() = 0;
        virtual const int64_t& value_as_integer() const = 0;
        virtual int64_t& value_as_integer() = 0;
        virtual const double& value_as_real() const = 0;
        virtual double& value_as_real() = 0;
        virtual const i_string& value_as_string() const = 0;
        virtual i_string& value_as_string() = 0;
        virtual const i_enum& value_as_enum() const = 0;
        virtual i_enum& value_as_enum() = 0;
        virtual const i_custom_type& value_as_custom_type() const = 0;
        virtual i_custom_type& value_as_custom_type() = 0;
    };

    namespace detail
    {
        template <typename T>
        struct variant_getter;
        template <>
        struct variant_getter<bool> 
        {
            const bool& operator()(const i_simple_variant& aVariant) { return aVariant.value_as_boolean(); }
            bool& operator()(i_simple_variant& aVariant) { return aVariant.value_as_boolean(); }
        };
        template <>
        struct variant_getter<int64_t>
        {
            const int64_t& operator()(const i_simple_variant& aVariant) { return aVariant.value_as_integer(); }
            int64_t& operator()(i_simple_variant& aVariant) { return aVariant.value_as_integer(); }
        };
        template <>
        struct variant_getter<double>
        {
            const double& operator()(const i_simple_variant& aVariant) { return aVariant.value_as_real(); }
            double& operator()(i_simple_variant& aVariant) { return aVariant.value_as_real(); }
        };
        template <>
        struct variant_getter<i_string>
        {
            const i_string& operator()(const i_simple_variant& aVariant) { return aVariant.value_as_string(); }
            i_string& operator()(i_simple_variant& aVariant) { return aVariant.value_as_string(); }
        };
        template <>
        struct variant_getter<i_enum>
        {
            const i_enum& operator()(const i_simple_variant& aVariant) { return aVariant.value_as_enum(); }
            i_enum& operator()(i_simple_variant& aVariant) { return aVariant.value_as_enum(); }
        };
        template <>
        struct variant_getter<i_custom_type>
        {
            const i_custom_type& operator()(const i_simple_variant& aVariant) { return aVariant.value_as_custom_type(); }
            i_custom_type& operator()(i_simple_variant& aVariant) { return aVariant.value_as_custom_type(); }
        };
    }

    template <typename T>
    inline const typename std::remove_reference<T>::type& get(const i_simple_variant& aVariant)
    {
        return detail::variant_getter<typename std::remove_reference<T>::type>()(aVariant);
    }
    template <typename T>
    inline typename std::remove_reference<T>::type& get(i_simple_variant& aVariant)
    {
        return detail::variant_getter<typename std::remove_reference<T>::type>()(aVariant);
    }

    inline bool operator==(const i_simple_variant& lhs, const i_simple_variant& rhs)
    {
        if (lhs.type() != rhs.type())
            return false;
        switch (lhs.type())
        {
        case simple_variant_type::Empty:
            return true;
        case simple_variant_type::Boolean:
            return get<bool>(lhs) == get<bool>(rhs);
        case simple_variant_type::Integer:
            return get<int64_t>(lhs) == get<int64_t>(rhs);
        case simple_variant_type::Real:
            return get<double>(lhs) == get<double>(rhs);
        case simple_variant_type::String:
            return get<i_string>(lhs) == get<i_string>(rhs);
        case simple_variant_type::Enum:
            return get<i_enum>(lhs) == get<i_enum>(rhs);
        case simple_variant_type::CustomType:
            return get<i_custom_type>(lhs) == get<i_custom_type>(rhs);
        default:
            throw i_simple_variant::unknown_type();
        }
    }

    inline bool operator!=(const i_simple_variant& lhs, const i_simple_variant& rhs)
    {
        if (lhs.type() != rhs.type())
            return true;
        switch (lhs.type())
        {
        case simple_variant_type::Empty:
            return false;
        case simple_variant_type::Boolean:
            return get<bool>(lhs) != get<bool>(rhs);
        case simple_variant_type::Integer:
            return get<int64_t>(lhs) != get<int64_t>(rhs);
        case simple_variant_type::Real:
            return get<double>(lhs) != get<double>(rhs);
        case simple_variant_type::String:
            return get<i_string>(lhs) != get<i_string>(rhs);
        case simple_variant_type::CustomType:
            return get<i_custom_type>(lhs) != get<i_custom_type>(rhs);
        default:
            throw i_simple_variant::unknown_type();
        }
    }

    inline bool operator<(const i_simple_variant& lhs, const i_simple_variant& rhs)
    {
        if (lhs.type() != rhs.type())
            return lhs.type() < rhs.type();
        switch (lhs.type())
        {
        case simple_variant_type::Empty:
            return false;
        case simple_variant_type::Boolean:
            return get<bool>(lhs) < get<bool>(rhs);
        case simple_variant_type::Integer:
            return get<int64_t>(lhs) < get<int64_t>(rhs);
        case simple_variant_type::Real:
            return get<double>(lhs) < get<double>(rhs);
        case simple_variant_type::String:
            return get<i_string>(lhs) < get<i_string>(rhs);
        case simple_variant_type::CustomType:
            return get<i_custom_type>(lhs) < get<i_custom_type>(rhs);
        default:
            throw i_simple_variant::unknown_type();
        }
    }

    inline string to_string(const i_simple_variant& value)
    {
        switch (value.type())
        {
        case simple_variant_type::Empty:
            return "";
        case simple_variant_type::Boolean:
            return boost::lexical_cast<std::string>(get<bool>(value));
        case simple_variant_type::Integer:
            return boost::lexical_cast<std::string>(get<int64_t>(value));
        case simple_variant_type::Real:
            return boost::lexical_cast<std::string>(get<double>(value));
        case simple_variant_type::String:
            return get<i_string>(value).to_std_string();
        case simple_variant_type::Enum:
            return get<i_enum>(value).to_string();
        case simple_variant_type::CustomType:
            return get<i_custom_type>(value).to_string();
        default:
            throw i_simple_variant::unknown_type();
        }
    }
}
