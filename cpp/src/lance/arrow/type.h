//  Copyright 2022 Lance Authors
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#pragma once

#include <arrow/scalar.h>
#include <arrow/type.h>
#include <arrow/type_traits.h>
#include <fmt/format.h>

#include <concepts>
#include <memory>
#include <string>
#include <vector>


template <typename T>
concept HasToString = requires(T t) {
                        { t.ToString() } -> std::same_as<std::string>;
                      };

template <HasToString T>
struct fmt::formatter<T> : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const T& v, FormatContext& ctx) -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), "{}", v.ToString());
  }
};

template <HasToString T>
struct fmt::formatter<std::shared_ptr<T>> : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const std::shared_ptr<T>& v, FormatContext& ctx)
      -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), "{}", v->ToString());
  }
};

namespace lance::arrow {

/// Returns True if the data type is a list.
inline auto is_list(const std::shared_ptr<::arrow::DataType>& dtype) {
  return dtype->id() == ::arrow::Type::LIST || dtype->id() == ::arrow::Type::LARGE_LIST;
}

/// Returns True if the data type is a struct.
inline bool is_struct(const std::shared_ptr<::arrow::DataType>& dtype) {
  return dtype->id() == ::arrow::Type::STRUCT;
}

/// Returns True if the data type si a map.
inline bool is_map(std::shared_ptr<::arrow::DataType> dtype) {
  return dtype->id() == ::arrow::Type::MAP;
}

/// Convert arrow DataType to a string representation.
::arrow::Result<std::string> ToLogicalType(std::shared_ptr<::arrow::DataType> dtype);

::arrow::Result<std::shared_ptr<::arrow::DataType>> FromLogicalType(
    ::arrow::util::string_view logical_type);

}  // namespace lance::arrow
