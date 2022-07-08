#include "lance/arrow/file_lance.h"

#include <arrow/dataset/file_base.h>
#include <arrow/util/future.h>
#include <fmt/format.h>

#include <memory>

#include "lance/arrow/reader.h"
#include "lance/format/schema.h"
#include "lance/io/reader.h"
#include "lance/io/scanner.h"
#include "lance/io/writer.h"

const char kLanceFormatTypeName[] = "lance";

namespace lance::arrow {

std::shared_ptr<LanceFileFormat> LanceFileFormat::Make() {
  return std::make_shared<LanceFileFormat>();
}

std::string LanceFileFormat::type_name() const { return kLanceFormatTypeName; }

bool LanceFileFormat::Equals(const FileFormat& other) const {
  return type_name() == other.type_name();
}

::arrow::Result<bool> LanceFileFormat::IsSupported(
    [[maybe_unused]] const ::arrow::dataset::FileSource& source) const {
  return true;
}

::arrow::Result<std::shared_ptr<::arrow::Schema>> LanceFileFormat::Inspect(
    const ::arrow::dataset::FileSource& source) const {
  ARROW_ASSIGN_OR_RAISE(auto infile, source.Open());
  ARROW_ASSIGN_OR_RAISE(auto reader, lance::arrow::FileReader::Make(infile));
  return reader->GetSchema();
}

::arrow::Result<::arrow::RecordBatchGenerator> LanceFileFormat::ScanBatchesAsync(
    const std::shared_ptr<::arrow::dataset::ScanOptions>& options,
    const std::shared_ptr<::arrow::dataset::FileFragment>& file) const {
  ARROW_ASSIGN_OR_RAISE(auto infile, file->source().Open());

  auto reader = std::make_shared<lance::io::FileReader>(infile);
  ARROW_RETURN_NOT_OK(reader->Open());
  auto scanner = lance::io::Scanner(reader, options);
  ARROW_RETURN_NOT_OK(scanner.Open());
  auto generator = ::arrow::RecordBatchGenerator(std::move(scanner));
  return generator;
}

::arrow::Result<std::shared_ptr<::arrow::dataset::FileWriter>> LanceFileFormat::MakeWriter(
    std::shared_ptr<::arrow::io::OutputStream> destination,
    std::shared_ptr<::arrow::Schema> schema,
    std::shared_ptr<::arrow::dataset::FileWriteOptions> options,
    ::arrow::fs::FileLocator destination_locator) const {
  return std::shared_ptr<::arrow::dataset::FileWriter>(
      new io::FileWriter(schema, options, destination, destination_locator));
}

std::shared_ptr<::arrow::dataset::FileWriteOptions> LanceFileFormat::DefaultWriteOptions() {
  return std::make_shared<FileWriteOptions>();
}

FileWriteOptions::FileWriteOptions()
    : ::arrow::dataset::FileWriteOptions(std::make_shared<LanceFileFormat>()) {}

}  // namespace lance::arrow