// Copyright (c) 2020-2023, The Monero Project
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <functional>
#include <utility>

#include "wire/filters.h"
#include "wire/traits.h"

//! A required field with the same key name and C/C++ name
#define WIRE_FIELD_ID(id, name)                       \
  ::wire::field< id >( #name , std::ref( self . name ))

//! A required field has the same key name and C/C++ name
#define WIRE_FIELD(name) \
  WIRE_FIELD_ID(0, name)

//! A required field has the same key name and C/C++ name AND is cheap to copy (faster output).
#define WIRE_FIELD_COPY(name)                   \
  ::wire::field( #name , self . name )

//! The optional field has the same key name and C/C++ name
#define WIRE_OPTIONAL_FIELD_ID(id, name)                         \
  ::wire::optional_field< id >( #name , std::ref( self . name ))

//! The optional field has the same key name and C/C++ name
#define WIRE_OPTIONAL_FIELD(name) \
  WIRE_OPTIONAL_FIELD_ID(0, name)

namespace wire
{
  /*! Links `name` to a `value` and index `I` for object serialization.

  `value_type` is `T` with optional `std::reference_wrapper` removed.
  `value_type` needs a `read_bytes` function when parsing with a
  `wire::reader` - see `read.h` for more info. `value_type` needs a
  `write_bytes` function when parsing with a `wire::writer` - see `write.h`
  for more info.

  Any `value_type` where `is_optional_on_empty<value_type> == true`, will
  automatically be converted to an optional field iff `value_type` has an
  `empty()` method that returns `true`. The old output engine omitted fields
  when an array was empty, and the standard input macro would ignore the
  `false` return for the missing field. For compability reasons, the
  input/output engine here matches that behavior. See `wrapper/array.h` to
  enforce a required field even when the array is empty or specialize the
  `is_optional_on_empty` trait. Only new fields should use this behavior.

  Additional concept requirements for `value_type` when `Required == false`:
    * must have an `operator*()` function.
    * must have a conversion to bool function that returns true when
      `operator*()` is safe to call (and implicitly when the associated field
      should be written as opposed to skipped/omitted).
  Additional concept requirements for `value_type` when `Required == false`
  when reading:
    * must have an `emplace()` method that ensures `operator*()` is safe to call.
    * must have a `reset()` method to indicate a field was skipped/omitted.

  If a standard type needs custom serialization, one "trick":
  ```
  struct custom_tag{};
  void read_bytes(wire::reader&, boost::fusion::pair<custom_tag, std::string&>)
  { ... }
  void write_bytes(wire::writer&, boost::fusion::pair<custom_tag, const std::string&>)
  { ... }

  template<typename F, typename T>
  void object_map(F& format, T& self)
  {
    wire::object(format,
      wire::field("foo", boost::fusion::make_pair<custom_tag>(std::ref(self.foo)))
    );
  }
  ```

  Basically each input/output format needs a unique type so that the compiler
  knows how to "dispatch" the read/write calls. */
  template<typename T, bool Required, unsigned I = 0>
  struct field_
  {
    using value_type = unwrap_reference_t<T>;

    //! \return True if field is forced optional when `get_value().empty()`.
    static constexpr bool optional_on_empty() noexcept
    { return is_optional_on_empty<value_type>::value; }

    static constexpr bool is_required() noexcept { return Required && !optional_on_empty(); }
    static constexpr std::size_t count() noexcept { return 1; }
    static constexpr unsigned id() noexcept { return I; }

    const char* name;
    T value;

    constexpr const value_type& get_value() const noexcept { return value; }
    value_type& get_value() noexcept { return value; }
  };

  //! Links `name` to `value`. Use `std::ref` if de-serializing.
  template<unsigned I = 0, typename T = void>
  constexpr inline field_<T, true, I> field(const char* name, T value)
  {
    return {name, std::move(value)};
  }

  //! Links `name` to `value`. Use `std::ref` if de-serializing.
  template<unsigned I = 0, typename T = void>
  constexpr inline field_<T, false, I> optional_field(const char* name, T value)
  {
    return {name, std::move(value)};
  }


  //! Indicates a field value should be written as an array
  template<typename T, typename F>
  struct as_array_
  {
    using value_type = typename unwrap_reference<T>::type;

    T value;
    F filter; //!< Each element in `value` given to this callable before `write_bytes`.

    //! \return `value` with `std::reference_wrapper` removed.
    constexpr const value_type& get_value() const noexcept
    {
      return value;
    }

    //! \return `value` with `std::reference_wrapper` removed.
    value_type& get_value() noexcept
    {
      return value;
    }
  };

  //! Callable that can filter `as_object` values or be used immediately.
  template<typename Default>
  struct as_array_filter
  {
    Default default_filter;

    template<typename T>
    constexpr as_array_<T, Default> operator()(T value) const
    {
      return {std::move(value), default_filter};
    }

    template<typename T, typename F>
    constexpr as_array_<T, F> operator()(T value, F filter) const
    {
      return {std::move(value), std::move(filter)};
    }
  };
  //! Usage: `wire::field("foo", wire::as_array(self.foo, to_string{})`. Consider `std::ref`.
  constexpr as_array_filter<identity_> as_array{};


  //! Indicates a field value should be written as an object
  template<typename T, typename F, typename G>
  struct as_object_
  {
    using map_type = typename unwrap_reference<T>::type;

    T map;
    F key_filter;   //!< Each key (`.first`) in `map` given to this callable before writing field key.
    G value_filter; //!< Each value (`.second`) in `map` given to this callable before `write_bytes`.

    //! \return `map` with `std::reference_wrapper` removed.
    constexpr const map_type& get_map() const noexcept
    {
      return map;
    }

    //! \return `map` with `std::reference_wrapper` removed.
    map_type& get_map() noexcept
    {
      return map;
    }
  };

  //! Usage: `wire::field("foo", wire::as_object(self.foo, to_string{}, wire::as_array))`. Consider `std::ref`.
  template<typename T, typename F = identity_, typename G = identity_>
  inline constexpr as_object_<T, F, G> as_object(T map, F key_filter = F{}, G value_filter = G{})
  {
    return {std::move(map), std::move(key_filter), std::move(value_filter)};
  }


  template<typename T, unsigned I>
  inline constexpr bool available(const field_<T, true, I>& elem) noexcept
  {
    /* The old output engine always skipped fields when it was an empty array,
       this follows that behavior. See comments for `field_`. */
    return elem.is_required() || (elem.optional_on_empty() && !wire::empty(elem.get_value()));
  }
  template<typename T, unsigned I>
  inline bool available(const field_<T, false, I>& elem)
  {
    return bool(elem.get_value());
  }
}

