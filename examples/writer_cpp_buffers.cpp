// SPDX-License-Identifier: MIT
//
// Writer example: JsonBuilder can borrow contiguous mutable C++ buffers.
// The builder never owns or resizes them. For std::vector / std::string the
// usable capacity is size(), so resize before constructing the builder.
#include "jsonlib/json.hpp"

#include <array>
#include <cstdio>
#include <vector>

int main()
{
  std::array<char, 96> array_buf{};
  {
    jsonlib::JsonBuilder b(array_buf);
    auto root = b.Object();
    root["Kind"] = "array";
    root["Value"] = 42;
  }
  std::printf("array: %s\n", array_buf.data());

  std::vector<char> vector_buf(96u);
  size_t json_len = 0u;
  {
    jsonlib::JsonBuilder b(vector_buf);
    auto root = b.Object();
    root["Kind"] = "vector";
    root["Value"] = 43;
    json_len = b.len();
  }
  std::printf("vector: %s (json_len=%zu, vector_size=%zu)\n",
              vector_buf.data(), json_len, vector_buf.size());

  char c_buf[96];
  {
    jsonlib::JsonBuilder b(c_buf);
    auto root = b.Object();
    root["Kind"] = "c-array";
    root["Value"] = 44;
  }
  std::printf("c-array: %s\n", c_buf);

  return 0;
}
