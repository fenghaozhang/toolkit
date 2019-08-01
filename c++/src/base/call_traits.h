//  (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/utility for most recent version including documentation.

// call_traits: defines typedefs for function usage
// (see libs/utility/call_traits.htm)

/* Release notes:
   23rd July 2000:
      Fixed array specialization. (JM)
      Added Borland specific fixes for reference types
      (issue raised by Steve Cleary).
*/

// This file is taken from boost-1.58 with modifications:
//
// 1. Replace boosts::is_{pointer, is_arithmetic, is_enum} with
//    functions in str::tr1.
// 2. Remove code for Borland compiler compatibility.
//
// GLOBAL_NOLINT

#ifndef _SRC_BASE_CALL_TRAITS_H
#define _SRC_BASE_CALL_TRAITS_H

#include <cstddef>
#include <tr1/type_traits>

namespace detail
{

template <typename T, bool small_>
struct ct_imp2
{
   typedef const T& param_type;
};

template <typename T>
struct ct_imp2<T, true>
{
   typedef const T param_type;
};

template <typename T, bool isp, bool b1, bool b2>
struct ct_imp
{
   typedef const T& param_type;
};

template <typename T, bool isp, bool b2>
struct ct_imp<T, isp, true, b2>
{
   typedef typename ct_imp2<T, sizeof(T) <= sizeof(void*)>::param_type param_type;
};

template <typename T, bool isp, bool b1>
struct ct_imp<T, isp, b1, true>
{
   typedef typename ct_imp2<T, sizeof(T) <= sizeof(void*)>::param_type param_type;
};

template <typename T, bool b1, bool b2>
struct ct_imp<T, true, b1, b2>
{
   typedef const T param_type;
};

}

template <typename T>
struct call_traits
{
public:
   typedef T value_type;
   typedef T& reference;
   typedef const T& const_reference;
   //
   // C++ Builder workaround: we should be able to define a compile time
   // constant and pass that as a single template parameter to ct_imp<T,bool>,
   // however compiler bugs prevent this - instead pass three bool's to
   // ct_imp<T,bool,bool,bool> and add an extra partial specialisation
   // of ct_imp to handle the logic. (JM)
   typedef typename detail::ct_imp<
      T,
      ::std::tr1::is_pointer<T>::value,
      ::std::tr1::is_arithmetic<T>::value,
      ::std::tr1::is_enum<T>::value
   >::param_type param_type;
};

template <typename T>
struct call_traits<T&>
{
   typedef T& value_type;
   typedef T& reference;
   typedef const T& const_reference;
   typedef T& param_type;  // hh removed const
};

template <typename T, std::size_t N>
struct call_traits<T [N]>
{
private:
   typedef T array_type[N];
public:
   // Degrades array to pointer:
   typedef const T* value_type;
   typedef array_type& reference;
   typedef const array_type& const_reference;
   typedef const T* const param_type;
};

template <typename T, std::size_t N>
struct call_traits<const T [N]>
{
private:
   typedef const T array_type[N];
public:
   // Degrades array to pointer:
   typedef const T* value_type;
   typedef array_type& reference;
   typedef const array_type& const_reference;
   typedef const T* const param_type;
};

#endif  // _SRC_BASE_CALL_TRAITS_H
