/*
 * Copyright (c) 2015 PLUMgrid, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdint>
#include <string>

namespace ebpf {

struct TableDesc {
  int fd;
  size_t key_size;  // sizes are in bytes
  size_t leaf_size;
  size_t max_entries;
  std::string key_desc;
  std::string leaf_desc;
  std::string key_reader;
  std::string leaf_reader;
};

}  // namespace ebpf
