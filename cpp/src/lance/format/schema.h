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

#include <arrow/type.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "lance/encodings/encoder.h"
#include "lance/format/format.pb.h"
#include "lance/format/visitors.h"

namespace lance::format {

class Field;

/// Schema is a tree representation of on-disk columns.
///
class Schema final {
 public:
  Schema() = default;

  /// Build the Schema from Arrow.
  /// TODO: Should we make it factory method so that it can return Result<>?
  Schema(std::shared_ptr<::arrow::Schema> schema);

  Schema(const google::protobuf::RepeatedPtrField<::lance::format::pb::Field>& pb_fields);

  ~Schema() = default;

  /// Convert to arrow schema.
  std::shared_ptr<::arrow::Schema> ToArrow() const;

  /// Serialize Schema into a list of protobuf Fields.
  std::vector<lance::format::pb::Field> ToProto() const;

  /// Create a Projection of this schema that only contains the specified columns.
  ///
  /// \param column_names a list of fully qualified column names.
  /// \return a view of schema that only contains the specified columns. Returns failures if such
  ///         view can not be built.
  [[nodiscard]] ::arrow::Result<std::shared_ptr<Schema>> Project(
      const std::vector<std::string>& column_names) const;

  /// Use arrow::schema to create a project of the current schema.
  ::arrow::Result<std::shared_ptr<Schema>> Project(const ::arrow::Schema& arrow_schema) const;

  /// Exclude (subtract) the fields from the given schema.
  ///
  /// \param other the schema to be excluded. It must to be a strict subset of this schema.
  /// \return The newly created schema, excluding any column in "other".
  ::arrow::Result<std::shared_ptr<Schema>> Exclude(std::shared_ptr<Schema> other) const;

  /// Add a new parent field.
  void AddField(std::shared_ptr<Field> f);

  /// Get nested field by it. It can access any field in the schema tree.
  std::shared_ptr<Field> GetField(int32_t id) const;

  /// Top level fields;
  const std::vector<std::shared_ptr<Field>> fields() const { return fields_; }

  /// Count the number of all fields, including nested fields.
  int32_t GetFieldsCount() const;

  /// Get the field by fully qualified field name.
  ///
  /// \param name the fully qualified name, i.e., "annotations.box.xmin".
  /// \return the field if found. Return nullptr if not found.
  std::shared_ptr<Field> GetField(const std::string& name) const;

  std::string ToString() const;

  bool Equals(const std::shared_ptr<Schema>& other, bool check_id = true) const;

  bool Equals(const Schema& other, bool check_id = true) const;

  /// Compare two schemas are the same.
  bool operator==(const Schema& other) const;

 private:
  /// (Re-)Assign Field IDs to all the fields.
  void AssignIds();

  /// Make a full copy of the schema.
  std::shared_ptr<Schema> Copy() const;

  bool RemoveField(int32_t id);

  std::vector<std::shared_ptr<Field>> fields_;
};

/// \brief Field is the metadata of a column on disk.
class Field final {
 public:
  /// Default constructor
  Field();

  /// Convert an arrow Field to Field.
  Field(const std::shared_ptr<::arrow::Field>& field);

  Field(const pb::Field& pb);

  /// Add a subfield.
  ::arrow::Status Add(const pb::Field& pb);

  void AddChild(std::shared_ptr<Field> child);

  std::shared_ptr<Field> Get(int32_t id);

  std::shared_ptr<::arrow::Field> ToArrow() const;

  std::vector<lance::format::pb::Field> ToProto() const;

  /// Returns Field ID.
  int32_t id() const;

  int32_t parent_id() const { return parent_; }

  std::shared_ptr<::arrow::DataType> type() const;

  std::string name() const;

  std::string logical_type() const { return logical_type_; };

  void set_encoding(lance::format::pb::Encoding encoding);

  const std::shared_ptr<::arrow::Array>& dictionary() const;

  ::arrow::Status set_dictionary(std::shared_ptr<::arrow::Array> dict_arr);

  lance::format::pb::Encoding encoding() const { return encoding_; };

  ::arrow::Result<std::shared_ptr<lance::encodings::Decoder>> GetDecoder(
      std::shared_ptr<::arrow::io::RandomAccessFile> infile);

  std::shared_ptr<lance::encodings::Encoder> GetEncoder(
      std::shared_ptr<::arrow::io::OutputStream> sink);

  /// Debug String
  std::string ToString() const;

  /// All subfields;
  const std::vector<std::shared_ptr<Field>>& fields() const { return children_; };

  const std::shared_ptr<Field>& field(int i) const { return children_[i]; };

  /// Returns the direct child with the name. Returns nullptr if such field does not exist.
  const std::shared_ptr<Field> Get(const std::string_view& name) const;

  /// Check if two fields are equal.
  ///
  /// \param other the point to the other field to check against to.
  /// \param check_id if true, check Id are equal as well. Can be set to false in unit test.
  /// \return true if they are equal.
  bool Equals(const std::shared_ptr<Field>& other, bool check_id = true) const;

  bool Equals(const Field& other, bool check_id = true) const;

  bool operator==(const Field& other) const;

 private:
  Field(const Field& field) = delete;

  const std::shared_ptr<Field> Get(const std::vector<std::string>& field_path,
                                   std::size_t start_idx = 0) const;

  /// Make a new copy of the field.
  std::shared_ptr<Field> Copy(bool include_children = false) const;

  /// Project an arrow field to this field.
  std::shared_ptr<Field> Project(const std::shared_ptr<::arrow::Field>& arrow_field) const;

  /// Load dictionary array from disk.
  ::arrow::Status LoadDictionary(std::shared_ptr<::arrow::io::RandomAccessFile> infile);

  void SetId(int32_t parent_id, int32_t* current_id);

  bool RemoveChild(int32_t id);

  int32_t GetFieldsCount() const;

  // TODO: use enum to replace protobuf enum.
  pb::Field::Type GetNodeType() const;

  int32_t id_ = -1;
  int32_t parent_ = -1;
  std::string name_;
  std::string logical_type_;
  lance::format::pb::Encoding encoding_ = lance::format::pb::Encoding::NONE;

  // Dictionary type
  int64_t dictionary_offset_ = -1;
  int64_t dictionary_page_length_ = 0;
  std::shared_ptr<::arrow::Array> dictionary_;

  friend class FieldVisitor;
  friend class ToArrowVisitor;
  friend class WriteDictionaryVisitor;
  friend class Schema;
  friend ::arrow::Status CopyField(std::shared_ptr<Field> new_field,
                                   std::shared_ptr<Field> field,
                                   std::vector<std::string> components,
                                   std::size_t comp_idx);

  std::vector<std::shared_ptr<Field>> children_;
};

}  // namespace lance::format
