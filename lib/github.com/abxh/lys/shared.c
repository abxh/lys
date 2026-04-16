

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "shared.h"

#ifndef PROGNAME
struct futhark_context_config *futhark_context_config_new();
struct futhark_context *futhark_context_new(struct futhark_context_config *);
void futhark_context_config_set_cache_file(struct futhark_context_config *,
                                           const char *);
struct futhark_u8_1d;
const int64_t *futhark_shape_u8_1d(struct futhark_context *,
                                   struct futhark_u8_1d *);
#endif

char *get_gpu_device_name(struct futhark_context *ctx) {
#if defined(FUTHARK_BACKEND_opencl)

  cl_device_id device;
  assert(clGetCommandQueueInfo(futhark_context_get_command_queue(ctx),
                               CL_QUEUE_DEVICE, sizeof(cl_device_id), &device,
                               NULL) == CL_SUCCESS);

  size_t name_size;
  assert(clGetDeviceInfo(device, CL_DEVICE_NAME, 0, NULL, &name_size) ==
         CL_SUCCESS);

  char *name = malloc(name_size);
  assert(clGetDeviceInfo(device, CL_DEVICE_NAME, name_size, name, NULL) ==
         CL_SUCCESS);

  return name;

#elif defined(FUTHARK_BACKEND_cuda)

  int device;
  assert(cudaGetDevice(&device) == cudaSuccess);

  cudaDeviceProp props;
  assert(cudaGetDeviceProperties(&props, device) == cudaSuccess);

  char *name = malloc(strlen(props.name) + 1);
  strcpy(name, props.name);

  return name;

#elif defined(FUTHARK_BACKEND_hip)

  int device;
  assert(hipGetDevice(&device) == hipSuccess);

  hipDeviceProp_t props;
  assert(hipGetDeviceProperties(&props, device) == hipSuccess);

  char *name = malloc(strlen(props.name) + 1);
  strcpy(name, props.name);

  return name;

#else

  (void)ctx;
  return NULL;

#endif
}

void lys_setup_futhark_context(const char *cache_path, const char *deviceopt,
                               _Bool device_interactive,
                               struct futhark_context_config **futcfg,
                               struct futhark_context **futctx,
                               char **device_name) {
  *futcfg = futhark_context_config_new();
  assert(*futcfg != NULL);

#if defined(FUTHARK_BACKEND_opencl) || defined(FUTHARK_BACKEND_cuda) ||        \
    defined(FUTHARK_BACKEND_hip)
  if (deviceopt) {
    futhark_context_config_set_device(*futcfg, deviceopt);
  }
#else
  (void)deviceopt;
#endif

#ifdef FUTHARK_BACKEND_opencl
  if (device_interactive) {
    futhark_context_config_select_device_interactively(*futcfg);
  }
#else
  (void)device_interactive;
#endif

  if (cache_path) {
    futhark_context_config_set_cache_file(*futcfg, cache_path);
  }

  *futctx = futhark_context_new(*futcfg);
  assert(*futctx != NULL);

  *device_name = get_gpu_device_name(*futctx);
}

void prepare_text(struct futhark_context *futctx, struct lys_text *text) {
  struct futhark_u8_1d *text_format_array;
  FUT_CHECK(futctx, futhark_entry_text_format(futctx, &text_format_array));
  size_t text_format_len = futhark_shape_u8_1d(futctx, text_format_array)[0];

  text->text_format = malloc(sizeof(char) * (text_format_len + 1));
  assert(text->text_format != NULL);
  FUT_CHECK(futctx, futhark_values_u8_1d(futctx, text_format_array,
                                         (unsigned char *)text->text_format));
  FUT_CHECK(futctx, futhark_context_sync(futctx));

  text->text_format[text_format_len] = '\0';
  FUT_CHECK(futctx, futhark_free_u8_1d(futctx, text_format_array));

  text->sum_names = (char ***)malloc(sizeof(char **) * n_printf_arguments());
  assert(text->sum_names != NULL);

  text->text_buffer_len = text_format_len;
  size_t i_arg = -1;
  for (size_t i = 0; i < text_format_len; i++) {
    if (text->text_format[i] == '%' && i + 1 < text_format_len &&
        text->text_format[i + 1] != '%') {
      i_arg++;
      if (i_arg >= n_printf_arguments()) {
        fprintf(stderr,
                "number of parameters between futhark function and format "
                "string do not match!\n");
        abort();
      }

      if (text->text_format[i + 1] == '[') {
        text->text_format[i + 1] = 's';
        size_t end_pos;
        size_t n_choices = 1;
        bool found_end = false;
        for (end_pos = i + 2; end_pos < text_format_len; end_pos++) {
          if (text->text_format[end_pos] == '|') {
            n_choices++;
          } else if (text->text_format[end_pos] == ']') {
            found_end = true;
            break;
          }
        }
        assert(found_end);
        text->sum_names[i_arg] =
            (char **)malloc(sizeof(char *) * (n_choices + 1));
        assert(text->sum_names[i_arg] != NULL);
        text->sum_names[i_arg][n_choices] = NULL;
        char *temp_choice =
            (char *)malloc(sizeof(char) * (end_pos - i - n_choices));
        assert(temp_choice != NULL);
        size_t choice_cur = 0;
        size_t i_choice = 0;
        for (size_t j = i + 2; j < end_pos + 1; j++) {
          if (text->text_format[j] == '|' || text->text_format[j] == ']') {
            temp_choice[choice_cur] = '\0';
            text->sum_names[i_arg][i_choice] =
                (char *)malloc(sizeof(char) * (choice_cur + 1));
            assert(text->sum_names[i_arg][i_choice] != NULL);
            strncpy(text->sum_names[i_arg][i_choice], temp_choice,
                    choice_cur + 1);
            choice_cur = 0;
            i_choice++;
          } else {
            temp_choice[choice_cur] = text->text_format[j];
            choice_cur++;
          }
        }
        free(temp_choice);
        size_t shift_left = end_pos - i - 1;
        for (size_t j = end_pos + 1; j < text_format_len; j++) {
          text->text_format[j - shift_left] = text->text_format[j];
        }
        text_format_len -= shift_left;
        text->text_format[text_format_len] = '\0';
        i++;
      } else {
        text->sum_names[i_arg] = NULL;
        text->text_buffer_len += 20; // estimate
      }
    }
  }

  text->text_buffer = malloc(sizeof(char) * text->text_buffer_len);
  assert(text->text_buffer != NULL);
  text->text_buffer[0] = '\0';

  text->show_text = true;
}
