// Resource-interface reflection for the ahead-of-time compiled shaders.
//
// The native renderer has to create pipeline layouts for shaders it did not
// translate itself, so it needs to know exactly which descriptor sets and
// bindings each module declares, and of what type. Rather than hand-copying
// the emulated backend's layout code -- which would silently drift the moment
// either side changed -- this reads the interface back out of the SPIR-V that
// was actually produced.
//
// Reads the manifest written by aot_shaders and adds a "descriptors" array to
// every shader entry, plus a "descriptor_sets" summary of the union across all
// shaders, which is what the pipeline layout has to be built from.

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {

// SPIR-V opcodes and constants we care about. Kept local so this tool does not
// have to agree with anyone else's headers about the spec revision.
constexpr uint32_t kMagic = 0x07230203u;

constexpr uint16_t kOpName = 5;
constexpr uint16_t kOpTypeImage = 25;
constexpr uint16_t kOpTypeSampler = 26;
constexpr uint16_t kOpTypeSampledImage = 27;
constexpr uint16_t kOpTypeArray = 28;
constexpr uint16_t kOpTypeRuntimeArray = 29;
constexpr uint16_t kOpTypeStruct = 30;
constexpr uint16_t kOpTypePointer = 32;
constexpr uint16_t kOpVariable = 59;
constexpr uint16_t kOpDecorate = 71;

constexpr uint32_t kDecorationBlock = 2;
constexpr uint32_t kDecorationBufferBlock = 3;
constexpr uint32_t kDecorationBinding = 33;
constexpr uint32_t kDecorationDescriptorSet = 34;

constexpr uint32_t kStorageClassUniformConstant = 0;
constexpr uint32_t kStorageClassUniform = 2;
constexpr uint32_t kStorageClassPushConstant = 9;
constexpr uint32_t kStorageClassStorageBuffer = 12;

struct TypeInfo {
  uint16_t opcode = 0;
  uint32_t element_type = 0;  // array element / pointer pointee / sampled image
  uint32_t array_length_id = 0;
  uint32_t image_sampled = 0;  // OpTypeImage: 1 = sampled, 2 = storage
  uint32_t pointer_storage_class = 0;
  bool is_block = false;
  bool is_buffer_block = false;
};

struct Variable {
  uint32_t type_id = 0;
  uint32_t storage_class = 0;
};

struct Descriptor {
  uint32_t set = 0;
  uint32_t binding = 0;
  std::string type;
  uint32_t count = 1;  // 0 means a runtime (unsized) array
  std::string name;
};

std::vector<uint32_t> ReadWords(const fs::path& p) {
  std::ifstream f(p, std::ios::binary | std::ios::ate);
  if (!f) {
    return {};
  }
  std::vector<uint32_t> words(size_t(f.tellg()) / sizeof(uint32_t));
  f.seekg(0);
  f.read(reinterpret_cast<char*>(words.data()), std::streamsize(words.size() * 4));
  return words;
}

// Resolve a variable's pointee through arrays down to the thing that decides
// the descriptor type, accumulating the array length on the way.
const char* ClassifyDescriptor(uint32_t type_id, uint32_t storage_class,
                               const std::map<uint32_t, TypeInfo>& types, uint32_t* count_out) {
  *count_out = 1;
  const TypeInfo* t = nullptr;
  auto it = types.find(type_id);
  if (it == types.end()) {
    return nullptr;
  }
  t = &it->second;

  // A variable's type is a pointer; step through to what it points at.
  if (t->opcode == kOpTypePointer) {
    it = types.find(t->element_type);
    if (it == types.end()) {
      return nullptr;
    }
    t = &it->second;
  }

  // Descriptor arrays: the binding's count is the array length. Unsized arrays
  // report 0, which the native renderer must treat as a bindless-style binding.
  while (t->opcode == kOpTypeArray || t->opcode == kOpTypeRuntimeArray) {
    *count_out = t->opcode == kOpTypeRuntimeArray ? 0 : UINT32_MAX;
    it = types.find(t->element_type);
    if (it == types.end()) {
      return nullptr;
    }
    t = &it->second;
  }

  switch (t->opcode) {
    case kOpTypeSampledImage:
      return "COMBINED_IMAGE_SAMPLER";
    case kOpTypeSampler:
      return "SAMPLER";
    case kOpTypeImage:
      return t->image_sampled == 2 ? "STORAGE_IMAGE" : "SAMPLED_IMAGE";
    case kOpTypeStruct:
      if (storage_class == kStorageClassStorageBuffer || t->is_buffer_block) {
        return "STORAGE_BUFFER";
      }
      if (storage_class == kStorageClassUniform && t->is_block) {
        return "UNIFORM_BUFFER";
      }
      return "UNIFORM_BUFFER";
    default:
      return nullptr;
  }
}

bool Reflect(const std::vector<uint32_t>& w, std::vector<Descriptor>* out, bool* uses_push_constants) {
  if (w.size() < 5 || w[0] != kMagic) {
    return false;
  }

  std::map<uint32_t, TypeInfo> types;
  std::map<uint32_t, Variable> variables;
  std::map<uint32_t, uint32_t> decor_set, decor_binding;
  std::map<uint32_t, std::string> names;
  std::set<uint32_t> blocks, buffer_blocks;

  for (size_t i = 5; i < w.size();) {
    uint32_t word_count = w[i] >> 16;
    uint16_t opcode = uint16_t(w[i] & 0xFFFFu);
    if (word_count == 0 || i + word_count > w.size()) {
      return false;
    }
    const uint32_t* op = &w[i];

    switch (opcode) {
      case kOpName:
        if (word_count >= 3) {
          names[op[1]] = reinterpret_cast<const char*>(&op[2]);
        }
        break;
      case kOpDecorate:
        if (word_count >= 3) {
          uint32_t target = op[1], decoration = op[2];
          if (decoration == kDecorationDescriptorSet && word_count >= 4) {
            decor_set[target] = op[3];
          } else if (decoration == kDecorationBinding && word_count >= 4) {
            decor_binding[target] = op[3];
          } else if (decoration == kDecorationBlock) {
            blocks.insert(target);
          } else if (decoration == kDecorationBufferBlock) {
            buffer_blocks.insert(target);
          }
        }
        break;
      case kOpTypeImage:
        if (word_count >= 8) {
          TypeInfo t;
          t.opcode = opcode;
          t.image_sampled = op[7];
          types[op[1]] = t;
        }
        break;
      case kOpTypeSampler:
      case kOpTypeStruct: {
        TypeInfo t;
        t.opcode = opcode;
        types[op[1]] = t;
      } break;
      case kOpTypeSampledImage:
        if (word_count >= 3) {
          TypeInfo t;
          t.opcode = opcode;
          t.element_type = op[2];
          types[op[1]] = t;
        }
        break;
      case kOpTypeArray:
        if (word_count >= 4) {
          TypeInfo t;
          t.opcode = opcode;
          t.element_type = op[2];
          t.array_length_id = op[3];
          types[op[1]] = t;
        }
        break;
      case kOpTypeRuntimeArray:
        if (word_count >= 3) {
          TypeInfo t;
          t.opcode = opcode;
          t.element_type = op[2];
          types[op[1]] = t;
        }
        break;
      case kOpTypePointer:
        if (word_count >= 4) {
          TypeInfo t;
          t.opcode = opcode;
          t.pointer_storage_class = op[2];
          t.element_type = op[3];
          types[op[1]] = t;
        }
        break;
      case kOpVariable:
        if (word_count >= 4) {
          Variable v;
          v.type_id = op[1];
          v.storage_class = op[3];
          variables[op[2]] = v;
        }
        break;
      default:
        break;
    }
    i += word_count;
  }

  // Apply Block/BufferBlock decorations onto the struct types they name.
  for (uint32_t id : blocks) {
    auto it = types.find(id);
    if (it != types.end()) {
      it->second.is_block = true;
    }
  }
  for (uint32_t id : buffer_blocks) {
    auto it = types.find(id);
    if (it != types.end()) {
      it->second.is_buffer_block = true;
    }
  }

  *uses_push_constants = false;
  for (const auto& [id, v] : variables) {
    if (v.storage_class == kStorageClassPushConstant) {
      *uses_push_constants = true;
      continue;
    }
    if (v.storage_class != kStorageClassUniformConstant &&
        v.storage_class != kStorageClassUniform && v.storage_class != kStorageClassStorageBuffer) {
      continue;
    }
    auto set_it = decor_set.find(id);
    auto binding_it = decor_binding.find(id);
    if (set_it == decor_set.end() || binding_it == decor_binding.end()) {
      continue;
    }
    uint32_t count = 1;
    const char* type = ClassifyDescriptor(v.type_id, v.storage_class, types, &count);
    if (!type) {
      continue;
    }
    Descriptor d;
    d.set = set_it->second;
    d.binding = binding_it->second;
    d.type = type;
    d.count = count;
    auto name_it = names.find(id);
    d.name = name_it == names.end() ? "" : name_it->second;
    out->push_back(d);
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: spirv_reflect <spv-dir>\n");
    return 2;
  }
  fs::path dir = argv[1];

  std::ifstream mf(dir / "manifest.json");
  if (!mf) {
    std::fprintf(stderr, "no manifest.json in %s\n", dir.string().c_str());
    return 1;
  }
  json manifest;
  mf >> manifest;

  // Sets below kMutableSetsStart hold the shared memory and the constant
  // buffers: every shader agrees on those, so they get one layout for the whole
  // renderer. The texture sets above it are packed densely per shader -- binding
  // 2 is a sampled image in one shader and a sampler in the next -- so each
  // shader needs its own layout there, which is why the emulated backend splits
  // the pipeline layout at the same place.
  constexpr uint32_t kMutableSetsStart = 2;

  // set -> binding -> (type, count), for the immutable sets only.
  std::map<uint32_t, std::map<uint32_t, std::pair<std::string, uint32_t>>> union_sets;
  std::vector<std::string> conflicts;
  // Distinct texture-set layouts, so we know how many the renderer must create.
  std::map<std::string, size_t> mutable_layouts;
  size_t reflected = 0, failed = 0;

  for (auto& entry : manifest["shaders"]) {
    fs::path spv = dir / entry["spirv"].get<std::string>();
    std::vector<uint32_t> words = ReadWords(spv);
    std::vector<Descriptor> descriptors;
    bool push_constants = false;
    if (words.empty() || !Reflect(words, &descriptors, &push_constants)) {
      ++failed;
      std::fprintf(stderr, "reflect failed: %s\n", spv.filename().string().c_str());
      continue;
    }
    ++reflected;

    json arr = json::array();
    for (const Descriptor& d : descriptors) {
      arr.push_back({{"set", d.set},
                     {"binding", d.binding},
                     {"type", d.type},
                     {"count", d.count},
                     {"name", d.name}});

      if (d.set >= kMutableSetsStart) {
        continue;
      }
      auto& slot = union_sets[d.set][d.binding];
      if (slot.first.empty()) {
        slot = {d.type, d.count};
      } else if (slot.first != d.type) {
        conflicts.push_back("set " + std::to_string(d.set) + " binding " +
                            std::to_string(d.binding) + ": " + slot.first + " vs " + d.type);
      } else {
        slot.second = std::max(slot.second, d.count);
      }
    }

    // Signature of this shader's texture sets, so identical ones share a layout.
    std::string signature;
    for (const Descriptor& d : descriptors) {
      if (d.set >= kMutableSetsStart) {
        signature += std::to_string(d.set) + ":" + std::to_string(d.binding) + ":" + d.type + ";";
      }
    }
    ++mutable_layouts[signature];
    entry["texture_set_signature"] = signature;

    entry["descriptors"] = std::move(arr);
    entry["push_constants"] = push_constants;
  }

  json sets = json::array();
  for (const auto& [set, bindings] : union_sets) {
    json b = json::array();
    for (const auto& [binding, tc] : bindings) {
      b.push_back({{"binding", binding}, {"type", tc.first}, {"count", tc.second}});
    }
    sets.push_back({{"set", set}, {"bindings", std::move(b)}});
  }
  manifest["descriptor_sets"] = sets;

  std::ofstream out(dir / "manifest.json");
  out << manifest.dump(1) << "\n";

  std::printf("reflected %zu shaders (%zu failed)\n", reflected, failed);
  std::printf("shared descriptor sets (one layout for the whole renderer):\n");
  for (const auto& [set, bindings] : union_sets) {
    std::printf("  set %u:\n", set);
    for (const auto& [binding, tc] : bindings) {
      std::printf("    binding %2u  %-24s count=%s\n", binding, tc.first.c_str(),
                  tc.second == 0 ? "runtime" : std::to_string(tc.second).c_str());
    }
  }
  if (!conflicts.empty()) {
    std::printf("CONFLICTS (%zu) in the shared sets -- these must agree:\n", conflicts.size());
    for (const std::string& c : conflicts) {
      std::printf("  %s\n", c.c_str());
    }
    return 1;
  }
  std::printf("shared sets are consistent across all %zu shaders\n", reflected);

  size_t empty_layouts = 0;
  for (const auto& [signature, count] : mutable_layouts) {
    if (signature.empty()) {
      empty_layouts = count;
    }
  }
  std::printf("texture set layouts: %zu distinct (%zu shaders need no textures at all)\n",
              mutable_layouts.size(), empty_layouts);
  return failed ? 1 : 0;
}
