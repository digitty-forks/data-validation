/* Copyright 2018 Google LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_DATA_VALIDATION_ANOMALIES_PATH_H_
#define TENSORFLOW_DATA_VALIDATION_ANOMALIES_PATH_H_

#include <iosfwd>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "tensorflow_metadata/proto/v0/path.pb.h"

namespace tensorflow {
namespace data_validation {

using std::string;

// This represents a sequence of steps (i.e. strings) in a structured example.
// The main functionality here is the ability Serialize and Deserialize a
// list of paths in a 'human-readable' way.
// Specifically, individual steps in a path can be arbitrary strings (in the
// byte sense). As we cannot assume that the strings are valid unicode, we
// should not try to parse them as such.
// However, many steps will look like your conventional alphanumeric variable
// names in a host of languages, i.e.:
// [A-Za-z_][A-Za-z0-9]*
// Others will have the proto2 extension format, (roughly)
// \([A-Za-z0-9_.]*\)
// This is a superset, but it captures the basic idea. In both these cases, it
// is sufficient to simply use dot separators to create the path.
// More generally, for steps of the form:
// ([^.()']+)| (\([^()]\))
// Serialize will leave them untouched.
// For example:
// foo
// bar
// (foo.bar)
// (foo.'bar)
// Other steps will be encapsulated by single quotes and any internal single
// quotes will be doubled.
// For example:
// ((c) becomes '((c)'
// Marty's becomes 'Marty''s'
// Steps, once serialized, will be concatenated with dots. E.g.:
// {foo, bar, baz} becomes foo.bar.baz
// {foo, ((c), Marty's} becomes foo.'((c)'.'Marty''s'
// Importantly, note that Serialize is an injection (1-1). For any string
// generated by Serialize(), Deserialize() will invert the process.
class Path {
 public:
  Path() = default;
  explicit Path(std::vector<string> step) : step_(std::move(step)) {}
  explicit Path(const tensorflow::metadata::v0::Path& p);
  Path(const Path& p) = default;
  Path(Path&& p) = default;
  Path& operator=(const Path& p) = default;
  Path& operator=(Path&& p) = default;

  // Returns -1, 0, 1 if *this is greater than, less than or equal to p.
  int Compare(const Path& p) const;

  // Number of steps in a path.
  size_t size() const { return step_.size(); }

  // Since we store the steps with the separators, sometimes we need to remove
  // the separator.
  const string& last_step() const { return step_.back(); }

  // Serialize a path into a string that can be Deserialized.
  // Intended to be as human-readable as possible.
  // See class-level comments for the style of the string.
  string Serialize() const;

  // Serialize the path to a proto.
  tensorflow::metadata::v0::Path AsProto() const;

  // Deserializes a string created with Serialize().
  // Note: for any path p (i.e. arbitrary steps):
  // Path p2;
  // TF_CHECK_OK(Path::Deserialize(p.Serialize(), &p2));
  // EXPECT_EQ(p, p2);
  static absl::Status Deserialize(absl::string_view str, Path* result);

  // True if there are no steps.
  bool empty() const { return step_.empty(); }

  // Get the parent path.
  Path GetParent() const {
    return Path(std::vector<string>(step_.begin(), step_.end() - 1));
  }

  Path GetChild(absl::string_view last_step) const;

  // Returns ("foo", Path({"rest","of", "path"})) if this is
  // Path({"foo", "rest", "of", "path"}).
  std::pair<string, Path> PopHead() const;

 private:
  // Returns true iff this is equal to p.
  // Part of the implementation of Compare().
  bool Equals(const Path& p) const;

  // Returns true iff this is less than p.
  // Part of the implementation of Compare().
  bool Less(const Path& p) const;

  // The raw steps of a path.
  std::vector<string> step_;
};

// Lexicographical ordering on steps.
// Needed for std::less (for std::set).
bool operator<(const Path& a, const Path& b);
bool operator>(const Path& a, const Path& b);
bool operator==(const Path& a, const Path& b);
bool operator!=(const Path& a, const Path& b);
bool operator>=(const Path& a, const Path& b);
bool operator<=(const Path& a, const Path& b);

// gUnit uses this function to pretty print a Path.
void PrintTo(const Path& p, std::ostream*);

}  // namespace data_validation
}  // namespace tensorflow

#endif  // TENSORFLOW_DATA_VALIDATION_ANOMALIES_PATH_H_
