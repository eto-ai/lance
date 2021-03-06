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

#include "lance/arrow/reader.h"

#include <arrow/builder.h>
#include <arrow/io/api.h>
#include <arrow/table.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <catch2/catch_test_macros.hpp>

#include "lance/arrow/stl.h"
#include "lance/arrow/type.h"
#include "lance/arrow/writer.h"

TEST_CASE("Test List Array With Nulls") {
  auto int_builder = std::make_shared<::arrow::Int32Builder>();
  auto list_builder = ::arrow::ListBuilder(::arrow::default_memory_pool(), int_builder);

  CHECK(list_builder.Append().ok());
  CHECK(int_builder->AppendValues({1, 1, 1}).ok());
  CHECK(list_builder.Append().ok());
  CHECK(int_builder->Append(2).ok());
  CHECK(list_builder.AppendNulls(3).ok());

  auto values = std::static_pointer_cast<::arrow::ListArray>(list_builder.Finish().ValueOrDie());
  auto schema = ::arrow::schema({::arrow::field("values", ::arrow::list(::arrow::int32()))});
  auto table = ::arrow::Table::Make(schema, {values});

  auto sink = arrow::io::BufferOutputStream::Create().ValueOrDie();
  CHECK(lance::arrow::WriteTable(*table, sink, "").ok());

  auto infile = make_shared<arrow::io::BufferReader>(sink->Finish().ValueOrDie());
  auto reader_result = lance::arrow::FileReader::Make(infile);
  INFO("Open file: " << reader_result.status());
  auto reader = std::move(reader_result.ValueOrDie());
  auto table_result = reader->ReadTable();
  INFO("Table read result: " << table_result.status());
  CHECK(table_result.ok());

  INFO("Expected table: " << table->ToString()
                          << "\n Actual table: " << table_result.ValueOrDie()->ToString());
  CHECK(table->Equals(*table_result.ValueOrDie()));

  auto row = reader->Get(0).ValueOrDie();
  auto scalar = row[0];
  CHECK(scalar->Equals(::arrow::ListScalar(::lance::arrow::ToArray({1, 1, 1}).ValueOrDie())));

  for (int i = 2; i < 5; i++) {
    scalar = reader->Get(i).ValueOrDie()[0];
    CHECK(scalar->Equals(::arrow::NullScalar()));
  }
}
