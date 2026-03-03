#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <utility>

#include "shared.h"

#ifdef PROGHEADER
#include PROGHEADER
#else
#error "undefined PROGHEADER"
int futhark_entry_text_content(struct futhark_context *, const float in0,
                               const struct futhark_opaque_state *in1);
#endif

using printf_out_t = union {
  std::int64_t val;
  char *sum_name;
};

template <std::size_t... i, std::size_t N = sizeof...(i)>
static void build_text_entry(struct futhark_context *futctx,
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

static constexpr size_t n_printf_arguments_constexpr() {
  return function_arity<decltype(&futhark_entry_text_content)>::value - 3;
}

extern "C" size_t n_printf_arguments() {
  return n_printf_arguments_constexpr();
}

extern "C" void build_text(struct futhark_context *futctx,
                           struct futhark_opaque_state *futstate, char *dest,
                           size_t dest_len, const char *format, float fps,
                           char ***sum_names) {
  constexpr auto N = n_printf_arguments_constexpr();

  std::array<printf_out_t, N> out;
  build_text_entry(futctx, futstate, dest, dest_len, format, fps, sum_names,
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
    std::strncpy(cache_path_buffer, path_str.c_str(),
                 sizeof(cache_path_buffer));

    std::cout << "Using cache at: " << cache_path_buffer << "\n";

    return cache_path_buffer;

  } catch (const fs::filesystem_error &e) {
    return nullptr;
  }
}
