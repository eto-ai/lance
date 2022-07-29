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

#include "lance/arrow/scanner.h"

#include <arrow/dataset/dataset.h>
#include <arrow/dataset/scanner.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "lance/arrow/file_lance.h"
#include "lance/format/schema.h"

namespace lance::arrow {

ScannerBuilder::ScannerBuilder(std::shared_ptr<::arrow::dataset::Dataset> dataset)
    : dataset_(dataset) {}

void ScannerBuilder::Project(const std::vector<std::string>& columns) { columns_ = columns; }

void ScannerBuilder::Filter(const ::arrow::compute::Expression& filter) { filter_ = filter; }

void ScannerBuilder::Limit(int64_t limit, int64_t offset) {
  limit_ = limit;
  offset_ = offset;
}

::arrow::Result<std::shared_ptr<::arrow::dataset::Scanner>> ScannerBuilder::Finish() const {
  if (offset_ < 0) {
    return ::arrow::Status::Invalid("Offset is negative");
  }

  auto builder = ::arrow::dataset::ScannerBuilder(dataset_);
  ARROW_RETURN_NOT_OK(builder.Filter(filter_));
  ARROW_ASSIGN_OR_RAISE(auto scanner, builder.Finish());

  /// We do the schema projection manuallly here to support nested structs.
  /// Esp. for `list<struct>`, supports Spark-like access, for example,
  ///
  /// for schema: `objects: list<struct<id:int, value:float>>`,
  /// We can access subfields via column name `objects.value`.
  if (columns_.has_value()) {
    auto schema = lance::format::Schema(scanner->options()->dataset_schema);
    fmt::print("Lance schema: {}\n\n", schema.ToString());
    ARROW_ASSIGN_OR_RAISE(auto projected_schema, schema.Project(columns_.value()));
    fmt::print("Lance projected: columns={}\nschema=\n{}\n",
               columns_.value(),
               projected_schema->ToString());
    scanner->options()->projected_schema = projected_schema->ToArrow();
  }
  return scanner;
}

}  // namespace lance::arrow
