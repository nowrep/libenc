#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ENC_MAX_REFERENCES 16

enum enc_codec {
   ENC_CODEC_AV1 = 1,
   ENC_CODEC_H264 = 2,
   ENC_CODEC_HEVC = 3,
};

enum enc_format {
   ENC_FORMAT_NV12 = 0,
   ENC_FORMAT_P010 = 1,
   ENC_FORMAT_RGBA = 2,
   ENC_FORMAT_BGRA = 3,
   ENC_FORMAT_RGBA10 = 4,
   ENC_FORMAT_BGRA10 = 5,
};

enum enc_color_standard {
   ENC_COLOR_STANDARD_UNKNOWN = 0,
   ENC_COLOR_STANDARD_SRGB = 1,
   ENC_COLOR_STANDARD_BT601 = 2,
   ENC_COLOR_STANDARD_BT709 = 3,
};

enum enc_rate_control_mode {
   ENC_RATE_CONTROL_MODE_CQP = 0,
   ENC_RATE_CONTROL_MODE_CBR = 1,
   ENC_RATE_CONTROL_MODE_VBR = 2,
   ENC_RATE_CONTROL_MODE_QVBR = 3,
};

enum enc_frame_type {
   ENC_FRAME_TYPE_UNKNOWN = 0,
   ENC_FRAME_TYPE_IDR = 1,
   ENC_FRAME_TYPE_I = 2,
   ENC_FRAME_TYPE_P = 3,
};

struct enc_dev_params {
   // Device path. Example /dev/dri/renderD128.
   const char *device_path;

   // Existing VADisplay to use.
   void *va_display;
};

struct enc_dev *enc_dev_create(const struct enc_dev_params *params);
void enc_dev_destroy(struct enc_dev *dev);

struct enc_dmabuf {
   int fd;
   uint32_t width;
   uint32_t height;
   uint64_t modifier;
   uint32_t num_layers;
   struct {
      uint32_t drm_format;
      uint32_t num_planes;
      uint32_t offset[4];
      uint32_t pitch[4];
   } layers[4];
};

struct enc_surface_params {
   // Device.
   const struct enc_dev *dev;

   // Surface format.
   enum enc_format format;

   // Surface size.
   uint32_t width;
   uint32_t height;

   // Color standard. Used for format conversions.
   enum enc_color_standard color_standard;

   // Import dmabuf. Fd is owned by caller.
   const struct enc_dmabuf *dmabuf;
};

struct enc_surface *enc_surface_create(const struct enc_surface_params *params);
void enc_surface_destroy(struct enc_surface *surface);

// Exports surface to dmabuf. Fd is owned by caller.
bool enc_surface_export_dmabuf(struct enc_surface *surface, struct enc_dmabuf *dmabuf);

// Copy surface, performing format and color conversion if needed.
bool enc_surface_copy(struct enc_surface *input, struct enc_surface *output);

struct enc_rate_control_params {
   // Frame rate.
   float frame_rate;

   // Target bit rate.
   uint32_t bit_rate;

   // Peak bit rate (VBR and QVBR).
   uint32_t peak_bit_rate;

   // VBV buffer size (bits).
   uint32_t vbv_buffer_size;

   // VBV initial buffer fullness (bits).
   uint32_t vbv_initial_fullness;

   // Minimum QP.
   uint8_t min_qp;

   // Maximum QP.
   uint8_t max_qp;

   // QVBR quality
   uint8_t qvbr_quality;

   // Maximum frame size (bits).
   uint32_t max_frame_size;

   // Disable filler data.
   bool disable_filler_data;
};

#include <enc/enc_av1.h>
#include <enc/enc_h264.h>
#include <enc/enc_hevc.h>

struct enc_encoder_params {
   // Device.
   const struct enc_dev *dev;

   // Codec.
   enum enc_codec codec;

   // Video size.
   uint32_t width;
   uint32_t height;

   // Video bit depth (8 or 10 bit)
   uint8_t bit_depth;

   // Number of references. If set to 0, only IDR and I frames will be coded.
   uint8_t num_refs;

   // Interval between IDR frames. 0 = infinite.
   uint32_t gop_size;

   // Rate control mode.
   enum enc_rate_control_mode rc_mode;

   // Number of rate control layers.
   uint8_t num_rc_layers;

   // Rate control params (one for each layer).
   const struct enc_rate_control_params *rc_params;

   // Enable intra refresh, `gop_size` determines refresh interval.
   bool intra_refresh;

   // Enable low latency context.
   bool low_latency;

   union {
      struct enc_av1_encoder_params av1;
      struct enc_h264_encoder_params h264;
      struct enc_hevc_encoder_params hevc;
   };
};

struct enc_encoder *enc_encoder_create(const struct enc_encoder_params *params);
void enc_encoder_destroy(struct enc_encoder *encoder);

struct enc_frame_feedback {
   // Frame type.
   enum enc_frame_type frame_type;

   // Frame id.
   uint64_t frame_id;

   // Frame is reference.
   bool referenced;

   // Frame is long term reference.
   bool long_term;

   // Frame ids of references used to encode this frame.
   uint8_t num_ref_list0;
   uint64_t ref_list0[ENC_MAX_REFERENCES];

   // Frame ids of available references after encoding this frame.
   uint8_t num_refs;
   uint64_t ref[ENC_MAX_REFERENCES];
};

struct enc_frame_params {
   // Input surface.
   const struct enc_surface *surface;

   // Frame type.
   enum enc_frame_type frame_type;

   // If set, use references with specified frame id.
   uint8_t num_ref_list0;
   uint64_t ref_list0[ENC_MAX_REFERENCES];

   // If set, invalidates references with specified frame id.
   uint8_t num_invalidate_refs;
   uint64_t invalidate_refs[ENC_MAX_REFERENCES];

   // If set, this frame will not be used as reference.
   bool not_referenced;

   // If set, this frame will be used for long term reference.
   bool long_term;

   // Frame QP. Only valid with `ENC_RATE_CONTROL_MODE_CQP`.
   uint16_t qp;

   // Temporal id. Only valid when `num_rc_layers` > 1.
   uint8_t temporal_id;

   // Rate control params (one for each layer).
   const struct enc_rate_control_params *rc_params;

   // If set, encoder will output frame feedback info.
   struct enc_frame_feedback *feedback;
};

struct enc_task *enc_encoder_encode_frame(struct enc_encoder *encoder, const struct enc_frame_params *params);

void enc_task_destroy(struct enc_task *task);
bool enc_task_wait(struct enc_task *task, uint64_t timeout_ns);
// Should be called in a loop until it returns false.
bool enc_task_get_bitstream(struct enc_task *task, uint32_t *size, uint8_t **data);

#ifdef __cplusplus
}
#endif
