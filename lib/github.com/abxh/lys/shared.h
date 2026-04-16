#pragma once

#ifdef PROGHEADER
#include PROGHEADER
#else
struct futhark_context_config;
struct futhark_context;
struct futhark_opaque_state;
char *futhark_context_get_error(struct futhark_context *);
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct lys_text {
  char *text_format;
  char *text_buffer;
  size_t text_buffer_len;
  bool show_text;
  char ***sum_names;
};

const char *get_cache_path(const char *progname);

void lys_setup_futhark_context(const char *cache_path, const char *deviceopt,
                               bool device_interactive,
                               struct futhark_context_config **futcfg,
                               struct futhark_context **futctx,
                               char **opencl_device_name);

int64_t lys_time_us();
int64_t lys_wall_time();

#define FUT_CHECK(ctx, x) _fut_check(ctx, x, __FILE__, __LINE__)
static inline void _fut_check(struct futhark_context *ctx, int res,
                              const char *file, int line) {
  if (res != 0) {
    fprintf(stderr, "%s:%d: Futhark error %d: %s\n", file, line, res,
            futhark_context_get_error(ctx));
    exit(EXIT_FAILURE);
  }
}

void futhark_context_config_set_device_wrapper(
    struct futhark_context_config *cfg, const char *s);

void futhark_context_config_select_device_interactively_wrapper(
    struct futhark_context_config *cfg);

size_t n_printf_arguments();

void prepare_text(struct futhark_context *futctx, struct lys_text *text);

void build_text(struct futhark_context *futctx,
                struct futhark_opaque_state *futstate, char *dest,
                size_t dest_len, const char *format, float fps,
                char ***sum_names);

bool load_files(struct futhark_context *futctx,
                struct futhark_opaque_state **futstate);

#ifdef __cplusplus
}
#endif
