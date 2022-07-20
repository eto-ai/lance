//  Copyright 2022 Lance Authors
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#include "lance/io/project.h"

#include <arrow/api.h>
#include <arrow/result.h>

#include "lance/arrow/utils.h"
#include "lance/io/filter.h"
#include "lance/io/reader.h"

namespace lance::io {

Project::Project(std::shared_ptr<format::Schema> dataset_schema,
                 std::shared_ptr<format::Schema> projected_schema,
                 std::shared_ptr<format::Schema> scan_schema,
                 std::unique_ptr<Filter> filter)
    : dataset_schema_(dataset_schema),
      projected_schema_(projected_schema),
      scan_schema_(scan_schema),
      filter_(std::move(filter)) {}

::arrow::Result<std::unique_ptr<Project>> Project::Make(
    std::shared_ptr<format::Schema> schema,
    std::shared_ptr<::arrow::dataset::ScanOptions> scan_options) {
  ARROW_ASSIGN_OR_RAISE(auto filter, Filter::Make(*schema, scan_options->filter));
  ARROW_ASSIGN_OR_RAISE(auto projected_schema, schema->Project(*scan_options->projected_schema));
  auto scan_schema = projected_schema;
  if (filter) {
    // Remove the columns in filter from the project schema, to avoid duplicated scan
    ARROW_ASSIGN_OR_RAISE(scan_schema, projected_schema->Exclude(filter->schema()));
  }
  return std::unique_ptr<Project>(
      new Project(schema, projected_schema, scan_schema, std::move(filter)));
}

::arrow::Result<std::shared_ptr<::arrow::RecordBatch>> Project::Execute(
    std::shared_ptr<FileReader> reader, int32_t chunk_idx) {
  if (filter_) {
    auto result = filter_->Execute(reader, chunk_idx);
    if (!result.ok()) {
      return result.status();
    }
    auto [indices, values] = result.ValueUnsafe();
    ARROW_ASSIGN_OR_RAISE(auto batch, reader->ReadChunk(*scan_schema_, chunk_idx, indices));
    assert(values->num_rows() == batch->num_rows());
    ARROW_ASSIGN_OR_RAISE(auto merged, lance::arrow::MergeRecordBatches(values, batch));
    return merged;
  } else {
    // Read without filter.
    return reader->ReadChunk(*scan_schema_, chunk_idx);
  }
}

}  // namespace lance::io