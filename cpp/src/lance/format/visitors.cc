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

#include "lance/format/visitors.h"

#include <arrow/type.h>

#include <memory>

#include "lance/format/schema.h"

namespace lance::format {

::arrow::Status ToArrowVisitor::Visit(std::shared_ptr<Field> root) {
  for (auto& child : root->children_) {
    ARROW_ASSIGN_OR_RAISE(auto arrow_field, DoVisit(child));
    arrow_fields_.push_back(arrow_field);
  }
  return ::arrow::Status::OK();
}

std::shared_ptr<::arrow::Schema> ToArrowVisitor::Finish() { return ::arrow::schema(arrow_fields_); }

::arrow::Result<::std::shared_ptr<::arrow::Field>> ToArrowVisitor::DoVisit(
    std::shared_ptr<Field> node) {
  return std::make_shared<::arrow::Field>(node->name(), node->type());
}

}  // namespace lance::format
