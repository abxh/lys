#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>

#include "shared.h"
#include "utils/tiny_obj_loader/tiny_obj_loader.h"

#ifdef PROGHEADER
#include PROGHEADER
#else
#error "undefined PROGHEADER"
struct futhark_context_config;
int futhark_entry_text_content(struct futhark_context *, const float,
                               const struct futhark_opaque_state *);
#endif

namespace detail {

using printf_out_t = union {
  std::int64_t val;
  char *sum_name;
};

template <std::size_t... i, std::size_t N = sizeof...(i)>
static void build_text(struct futhark_context *futctx,
                       struct futhark_opaque_state *futstate, char *dest,
                       size_t dest_len, const char *format, float fps,
                       char ***sum_names, std::array<printf_out_t, N> out,
                       const std::index_sequence<i...> &) {
  FUT_CHECK(futctx,
            futhark_entry_text_content(futctx, &out[i].val..., fps, futstate));

  ((sum_names[i] != NULL ? out[i].sum_name = sum_names[i][(int32_t)out[i].val]
                         : (char *)0),
   ...);

  std::snprintf(dest, dest_len, format, out[i].val...);
}

template <typename T> struct function_arity;
template <typename Ret, typename... Args>
struct function_arity<Ret (*)(Args...)>
    : std::integral_constant<std::size_t, sizeof...(Args)> {};
static constexpr size_t n_printf_arguments() {
  return function_arity<decltype(&futhark_entry_text_content)>::value - 3;
}

}; // namespace detail

extern "C" int64_t lys_wall_time() {
  using namespace std::chrono;
  return duration_cast<microseconds>(system_clock::now().time_since_epoch())
      .count();
}

extern "C" int64_t lys_time_us() {
  using namespace std::chrono;
  return duration_cast<microseconds>(steady_clock::now().time_since_epoch())
      .count();
}

extern "C" size_t n_printf_arguments() { return detail::n_printf_arguments(); }

extern "C" void build_text(struct futhark_context *futctx,
                           struct futhark_opaque_state *futstate, char *dest,
                           size_t dest_len, const char *format, float fps,
                           char ***sum_names) {
  constexpr auto N = detail::n_printf_arguments();
  std::array<detail::printf_out_t, N> out;
  detail::build_text(futctx, futstate, dest, dest_len, format, fps, sum_names,
                     out, std::make_index_sequence<N>{});
}

extern "C" const char *get_cache_path(const char *progname) {
  namespace fs = std::filesystem;

  if (!progname) {
    return nullptr;
  }

  static char cache_path_buffer[4096];

  std::string base_path;
  const char *xdg_cache = std::getenv("XDG_CACHE_HOME");

  if (xdg_cache && xdg_cache[0] != '\0') {
    base_path = xdg_cache;
  } else {
    const char *home = std::getenv("HOME");
    if (!home)
      return nullptr;
    base_path = std::string(home) + "/.cache";
  }

  try {
    fs::path full_path = fs::path(base_path) / "lys";

    // create directories if they don't exist
    if (!fs::exists(full_path)) {
      if (!fs::create_directories(full_path)) {
        return nullptr;
      }
    }

    full_path /= fs::path(progname).filename().replace_extension("lyscache");

    const std::string path_str = full_path.string();
    if (path_str.length() >= sizeof(cache_path_buffer)) {
      return nullptr;
    }
    std::snprintf(cache_path_buffer, sizeof(cache_path_buffer), "%s",
                  path_str.c_str());

    std::cout << "Using cache at: " << cache_path_buffer << "\n";

    return cache_path_buffer;

  } catch (const fs::filesystem_error &e) {
    return nullptr;
  }
}

#ifndef PROGHEADER
int futhark_entry_input_file_names(struct futhark_context *,
                                   struct futhark_u8_1d **);
struct futhark_u8_1d;
const int64_t *futhark_shape_u8_1d(struct futhark_context *,
                                   struct futhark_u8_1d *);
int futhark_values_u8_1d(struct futhark_context *, struct futhark_u8_1d *,
                         uint8_t *);
int futhark_context_sync(struct futhark_context *);
int futhark_free_u8_1d(struct futhark_context *, struct futhark_u8_1d *);
struct futhark_u8_1d *futhark_new_u8_1d(struct futhark_context *,
                                        const uint8_t *, int64_t);
int futhark_free_u8_1d(struct futhark_context *, struct futhark_u8_1d *);
int futhark_entry_load_bin(struct futhark_context *,
                           struct futhark_opaque_state **, const int64_t,
                           const struct futhark_u8_1d *,
                           const struct futhark_opaque_state *);
struct futhark_f32_1d *futhark_new_f32_1d(struct futhark_context *,
                                          const float *, int64_t);
int futhark_free_f32_1d(struct futhark_context *, struct futhark_f32_1d *);

int futhark_entry_load_obj_vertices(
    struct futhark_context *, struct futhark_opaque_state **, const int64_t,
    const struct futhark_f32_1d *, const struct futhark_f32_1d *,
    const struct futhark_f32_1d *, const struct futhark_opaque_state *);
int futhark_entry_load_obj_normals(struct futhark_context *,
                                   struct futhark_opaque_state **,
                                   const int64_t, const struct futhark_f32_1d *,
                                   const struct futhark_f32_1d *,
                                   const struct futhark_f32_1d *,
                                   const struct futhark_opaque_state *);
int futhark_entry_load_obj_texcoords(struct futhark_context *,
                                     struct futhark_opaque_state **,
                                     const int64_t,
                                     const struct futhark_f32_1d *,
                                     const struct futhark_f32_1d *,
                                     const struct futhark_opaque_state *);
int futhark_entry_load_obj_vertices(
    struct futhark_context *, struct futhark_opaque_state **, const int64_t,
    const struct futhark_f32_1d *, const struct futhark_f32_1d *,
    const struct futhark_f32_1d *, const struct futhark_opaque_state *);
int futhark_entry_load_obj_normals(struct futhark_context *,
                                   struct futhark_opaque_state **,
                                   const int64_t, const struct futhark_f32_1d *,
                                   const struct futhark_f32_1d *,
                                   const struct futhark_f32_1d *,
                                   const struct futhark_opaque_state *);
int futhark_entry_load_obj_texcoords(struct futhark_context *,
                                     struct futhark_opaque_state **,
                                     const int64_t,
                                     const struct futhark_f32_1d *,
                                     const struct futhark_f32_1d *,
                                     const struct futhark_opaque_state *);

int futhark_entry_load_obj_vertex_indices(struct futhark_context *,
                                          struct futhark_opaque_state **,
                                          const int64_t,
                                          const struct futhark_i64_1d *,
                                          const struct futhark_opaque_state *);
struct futhark_i64_1d *futhark_new_i64_1d(struct futhark_context *,
                                          const int64_t *, int64_t);
int futhark_free_i64_1d(struct futhark_context *, struct futhark_i64_1d *);
#endif

static bool load_bin(struct futhark_context *futctx,
                     struct futhark_opaque_state **futstate, std::int64_t index,
                     std::filesystem::path &path);

static bool load_obj(struct futhark_context *futctx,
                     struct futhark_opaque_state **futstate, std::int64_t index,
                     std::filesystem::path &path);

extern "C" bool load_files(struct futhark_context *futctx,
                           struct futhark_opaque_state **futstate) {
  namespace fs = std::filesystem;

  try {
    std::unique_ptr<char[]> file_names = nullptr;
    {
      struct futhark_u8_1d *input_file_names = nullptr;
      FUT_CHECK(futctx,
                futhark_entry_input_file_names(futctx, &input_file_names));
      size_t file_names_len = futhark_shape_u8_1d(futctx, input_file_names)[0];
      file_names.reset(new char[file_names_len + 1]);
      assert(file_names != nullptr);
      FUT_CHECK(futctx,
                futhark_values_u8_1d(futctx, input_file_names,
                                     (unsigned char *)file_names.get()));
      FUT_CHECK(futctx, futhark_context_sync(futctx));
      file_names[file_names_len] = '\0';
      FUT_CHECK(futctx, futhark_free_u8_1d(futctx, input_file_names));
    }
    std::stringstream ss{std::string(file_names.get())};

    std::string item;
    std::int64_t index = 0;
    while (std::getline(ss, item, ',')) {
      fs::path path = fs::current_path();
      path /= item;
      if (!fs::exists(path)) {
        std::cerr << "file path " << path << " does not exist\n";
        return false;
      }
      if (!fs::is_regular_file(path)) {
        std::cerr << "file path " << path << " is not a regular file\n";
        return false;
      }
      if (path.extension() == ".bin") {
        if (!load_bin(futctx, futstate, index, path)) {
          return false;
        }
      } else if (path.extension() == ".obj") {
        if (!load_obj(futctx, futstate, index, path)) {
          return false;
        }
      } else {
        std::cerr << "unsupported file type: " << path.extension() << "\n";
        return false;
      }
      index += 1;
    }
  } catch (const std::exception &e) {
    std::cerr << "exception: " << e.what() << "\n";
    return false;
  }
  return true;
}

static bool load_bin(struct futhark_context *futctx,
                     struct futhark_opaque_state **futstate, std::int64_t index,
                     std::filesystem::path &path) {
  std::int64_t data_dim = 0;
  std::unique_ptr<std::uint8_t[]> data = nullptr;
  try {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
      std::cerr << "failed to open " << path << "\n";
      return false;
    }
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    if (size < 0) {
      std::cerr << "failed to get size of file " << path << "\n";
      return false;
    }
    file.seekg(0, std::ios::beg);
    data_dim = static_cast<std::int64_t>(size);
    data.reset(new uint8_t[static_cast<std::size_t>(size)]);
    if (!file.read(reinterpret_cast<char *>(data.get()), size)) {
      std::cerr << "failed to read file " << path << "\n";
      return false;
    }
  } catch (const std::exception &e) {
    std::cerr << "exception: " << e.what() << "\n";
    return false;
  }

  struct futhark_u8_1d *file_content =
      futhark_new_u8_1d(futctx, data.get(), data_dim);
  assert(file_content != nullptr);
  struct futhark_opaque_state *new_state = nullptr, *old_state = *futstate;
  FUT_CHECK(futctx, futhark_entry_load_bin(futctx, &new_state, index,
                                           file_content, old_state));
  *futstate = new_state;
  FUT_CHECK(futctx, futhark_free_u8_1d(futctx, file_content));

  return true;
}

static bool load_obj(struct futhark_context *futctx,
                     struct futhark_opaque_state **futstate, std::int64_t index,
                     std::filesystem::path &path) {
  std::vector<std::int64_t> vis;
  std::vector<float> vx, vy, vz;
  std::vector<float> nx, ny, nz;
  std::vector<float> tu, tv;

  {
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = "./"; // Path to material files

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path, reader_config)) {
      if (!reader.Error().empty()) {
        std::cerr << "TinyObjReader: " << reader.Error();
      }
      return false;
    }

    if (!reader.Warning().empty()) {
      std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto &shapes = reader.GetShapes();
    auto &attrib = reader.GetAttrib();

    assert(attrib.vertices.size() % 3 == 0);
    const size_t num_vertices = attrib.vertices.size() / 3;
    vx.reserve(num_vertices);
    vy.reserve(num_vertices);
    vz.reserve(num_vertices);

    for (size_t i = 0; i < num_vertices; i++) {
      vx.push_back(attrib.vertices[3 * i + 0]);
      vy.push_back(attrib.vertices[3 * i + 1]);
      vz.push_back(attrib.vertices[3 * i + 2]);
    }

    size_t total_vertex_indices = 0;
    for (const auto &shape : shapes) {
      for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
        total_vertex_indices += shape.mesh.num_face_vertices[f];
        assert(shape.mesh.num_face_vertices[f] == 3 &&
               "assuming only triangles are loaded");
      }
    }
    vis.reserve(total_vertex_indices);

    assert(attrib.normals.size() % 3 == 0);
    const size_t num_normals = attrib.normals.size() / 3;
    nx.reserve(num_normals);
    ny.reserve(num_normals);
    nz.reserve(num_normals);

    assert(attrib.texcoords.size() % 2 == 0);
    const size_t num_texcoords = attrib.texcoords.size() / 2;
    tu.reserve(num_texcoords);
    tv.reserve(num_texcoords);

    for (size_t s = 0; s < shapes.size(); s++) {
      size_t index_offset = 0;
      for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
        size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

        for (size_t v = 0; v < fv; v++) {
          tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

          assert(idx.vertex_index >= 0);
          vis.push_back(std::int64_t(idx.vertex_index));

          if (idx.normal_index >= 0) {
            nx.push_back(attrib.normals[3 * size_t(idx.normal_index) + 0]);
            ny.push_back(attrib.normals[3 * size_t(idx.normal_index) + 1]);
            nz.push_back(attrib.normals[3 * size_t(idx.normal_index) + 2]);
          }
          if (idx.texcoord_index >= 0) {
            tu.push_back(attrib.texcoords[2 * size_t(idx.texcoord_index) + 0]);
            tv.push_back(attrib.texcoords[2 * size_t(idx.texcoord_index) + 1]);
          }
        }
        index_offset += fv;
      }
    }
  }

  if (vis.size() > 0) {
    struct futhark_i64_1d *vis_arr =
        futhark_new_i64_1d(futctx, vis.data(), vis.size());

    struct futhark_opaque_state *new_state = nullptr, *old_state = *futstate;
    FUT_CHECK(futctx, futhark_entry_load_obj_vertex_indices(
                          futctx, &new_state, index, vis_arr, old_state));
    *futstate = new_state;
    FUT_CHECK(futctx, futhark_free_i64_1d(futctx, vis_arr));
  }

  if (vx.size() > 0) {
    struct futhark_f32_1d *vx_arr =
        futhark_new_f32_1d(futctx, vx.data(), vx.size());
    struct futhark_f32_1d *vy_arr =
        futhark_new_f32_1d(futctx, vy.data(), vy.size());
    struct futhark_f32_1d *vz_arr =
        futhark_new_f32_1d(futctx, vz.data(), vz.size());

    assert(vx_arr != nullptr);
    assert(vy_arr != nullptr);
    assert(vz_arr != nullptr);

    struct futhark_opaque_state *new_state = nullptr, *old_state = *futstate;
    FUT_CHECK(futctx,
              futhark_entry_load_obj_vertices(futctx, &new_state, index, vx_arr,
                                              vy_arr, vz_arr, old_state));
    *futstate = new_state;
    FUT_CHECK(futctx, futhark_free_f32_1d(futctx, vx_arr));
    FUT_CHECK(futctx, futhark_free_f32_1d(futctx, vy_arr));
    FUT_CHECK(futctx, futhark_free_f32_1d(futctx, vz_arr));
  }
  if (nx.size() > 0) {
    struct futhark_f32_1d *nx_arr =
        futhark_new_f32_1d(futctx, nx.data(), nx.size());
    struct futhark_f32_1d *ny_arr =
        futhark_new_f32_1d(futctx, ny.data(), ny.size());
    struct futhark_f32_1d *nz_arr =
        futhark_new_f32_1d(futctx, nz.data(), nz.size());

    assert(nx_arr != nullptr);
    assert(ny_arr != nullptr);
    assert(nz_arr != nullptr);

    struct futhark_opaque_state *new_state = nullptr, *old_state = *futstate;
    FUT_CHECK(futctx,
              futhark_entry_load_obj_normals(futctx, &new_state, index, nx_arr,
                                             ny_arr, nz_arr, old_state));
    *futstate = new_state;
    FUT_CHECK(futctx, futhark_free_f32_1d(futctx, nx_arr));
    FUT_CHECK(futctx, futhark_free_f32_1d(futctx, ny_arr));
    FUT_CHECK(futctx, futhark_free_f32_1d(futctx, nz_arr));
  }
  if (tu.size() > 0) {
    struct futhark_f32_1d *tu_arr =
        futhark_new_f32_1d(futctx, tu.data(), tu.size());
    struct futhark_f32_1d *tv_arr =
        futhark_new_f32_1d(futctx, tv.data(), tv.size());

    assert(tu_arr != nullptr);
    assert(tv_arr != nullptr);

    struct futhark_opaque_state *new_state = nullptr, *old_state = *futstate;
    FUT_CHECK(futctx,
              futhark_entry_load_obj_texcoords(futctx, &new_state, index,
                                               tu_arr, tv_arr, old_state));
    *futstate = new_state;
    FUT_CHECK(futctx, futhark_free_f32_1d(futctx, tu_arr));
    FUT_CHECK(futctx, futhark_free_f32_1d(futctx, tv_arr));
  }
  return true;
}
