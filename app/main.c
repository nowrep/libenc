#include <enc/enc.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <getopt.h>
#include <inttypes.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_drm.h>

AVFormatContext *av_ctx;
AVCodecContext *av_codec_ctx;
int av_stream_idx;
AVFrame *av_frame;
AVFrame *av_drm_frame;

static int create_decoder(const char *device_path, const char *file_path)
{
   AVBufferRef *hw_dev_ctx;
   if (av_hwdevice_ctx_create(&hw_dev_ctx, AV_HWDEVICE_TYPE_VAAPI, device_path, NULL, 0) != 0) {
      fprintf(stderr, "Failed to create hwdevice context %s\n", device_path);
      return 1;
   }

   if (avformat_open_input(&av_ctx, file_path, NULL, NULL) != 0) {
      fprintf(stderr, "Failed to open input %s\n", file_path);
      return 1;
   }

   if (avformat_find_stream_info(av_ctx, NULL) != 0) {
      fprintf(stderr, "Failed to find stream info\n");
      return 1;
   }

   const AVCodec *av_codec;
   av_stream_idx = av_find_best_stream(av_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &av_codec, 0);
   if (av_stream_idx < 0) {
      fprintf(stderr, "Failed to find video stream\n");
      return 1;
   }

   av_codec_ctx = avcodec_alloc_context3(av_codec);
   if (!av_codec_ctx) {
      fprintf(stderr, "Failed to allocate codec context\n");
      return 1;
   }

   av_codec_ctx->hw_device_ctx = hw_dev_ctx;

   AVStream *av_stream = av_ctx->streams[av_stream_idx];

   if (avcodec_parameters_to_context(av_codec_ctx, av_stream->codecpar) < 0) {
      fprintf(stderr, "Failed to set codec parameters\n");
      return 1;
   }

   if (avcodec_open2(av_codec_ctx, av_codec, NULL) != 0) {
      fprintf(stderr, "Failed to open codec\n");
      return 1;
   }

   av_frame = av_frame_alloc();
   av_drm_frame = av_frame_alloc();

   return 0;
}

static bool decode_frame(struct enc_dmabuf *dmabuf)
{
   AVFrame *frame = NULL;
   AVPacket *packet = av_packet_alloc();

   while (av_read_frame(av_ctx, packet) == 0) {
      if (packet->stream_index != av_stream_idx) {
         av_packet_unref(packet);
         continue;
      }

      int ret = avcodec_send_packet(av_codec_ctx, packet);
      av_packet_unref(packet);
      if (ret == AVERROR_EOF)
         break;
      if (ret != 0) {
         fprintf(stderr, "Failed to send packet\n");
         break;
      }

      ret = avcodec_receive_frame(av_codec_ctx, av_frame);
      if (ret == AVERROR(EAGAIN))
         continue;
      if (ret != 0) {
         fprintf(stderr, "Failed to receive frame\n");
         break;
      }

      frame = av_frame;
      break;
   }

   av_packet_free(&packet);

   if (frame) {
      av_frame_unref(av_drm_frame);
      av_drm_frame->format = AV_PIX_FMT_DRM_PRIME;
      if (av_hwframe_map(av_drm_frame, frame, AV_HWFRAME_MAP_READ) < 0) {
         fprintf(stderr, "Failed to map drm frame\n");
         return false;
      }
      AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor *)av_drm_frame->data[0];
      dmabuf->fd = desc->objects[0].fd;
      dmabuf->width = frame->width;
      dmabuf->height = frame->height;
      dmabuf->modifier = desc->objects[0].format_modifier;
      dmabuf->num_layers = desc->nb_layers;
      for (int32_t i = 0; i < desc->nb_layers; i++) {
         dmabuf->layers[i].drm_format = desc->layers[i].format;
         dmabuf->layers[i].num_planes = desc->layers[i].nb_planes;
         for (int32_t j = 0; j < desc->layers[i].nb_planes; j++) {
            dmabuf->layers[i].offset[j] = desc->layers[i].planes[j].offset;
            dmabuf->layers[i].pitch[j] = desc->layers[i].planes[j].pitch;
         }
      }
      return true;
   }

   return false;
}

static int help(void)
{
   printf("Usage: encapp [options] input output\n");
   printf("\n");
   printf("Options:\n");
   printf("  --device DEVICE                      Set device to use, default = /dev/dri/renderD128\n");
   printf("  --codec CODEC                        Set codec (h264), default = h264\n");
   printf("  --refs NUM_REFS                      Set number of references, default = 1\n");
   printf("  --gop GOP_SIZE                       Set group of pictures size, default = 60\n");
   printf("  --fps FRAME_RATE                     Set frame rate, default = 30.0\n");
   printf("  --rc RATE_CONTROL                    Set rate control mode (cqp, cbr, vbr, qvbr), default = cqp\n");
   printf("  --bitrate BITRATE_KBPS               Set bit rate in kbps, default = 10000\n");
   printf("  --qp QP                              Set QP, default = 22\n");
   printf("  --intra-refresh                      Enable intra refresh, default = false\n");
   printf("  --rc-layers NUM_LAYERS               Set number of rate control layers, default = 1\n");
   printf("  --hierarchy LEVEL                    Set reference hierarchy level, default = 1\n");
   printf("\n");
   printf("  --maxframes NUM_FRAMES               Set maximum number of encoded frames\n");
   printf("  --norefs FRAMES                      List of comma separated frames to not be referenced\n");
   printf("  --dropframes FRAMES                  List of comma separated frames to drop in output\n");
   printf("  --no-invalidate                      Don't invalidate dropped frames\n");
   return 1;
}

static struct option long_options[] = {
   {"device",           required_argument, NULL, ':'},
   {"codec",            required_argument, NULL, ':'},
   {"refs",             required_argument, NULL, ':'},
   {"gop",              required_argument, NULL, ':'},
   {"fps",              required_argument, NULL, ':'},
   {"rc",               required_argument, NULL, ':'},
   {"bitrate",          required_argument, NULL, ':'},
   {"qp",               required_argument, NULL, ':'},
   {"maxframes",        required_argument, NULL, ':'},
   {"norefs",           required_argument, NULL, ':'},
   {"dropframes",       required_argument, NULL, ':'},
   {"intra-refresh",    no_argument,       NULL, ':'},
   {"no-invalidate",    no_argument,       NULL, ':'},
   {"rc-layers",        required_argument, NULL, ':'},
   {"hierarchy",        required_argument, NULL, ':'},
   {NULL, 0, NULL, 0},
};

const char *opt_device = "/dev/dri/renderD128";
enum enc_codec opt_codec = ENC_CODEC_H264;
uint8_t opt_refs = 1;
uint32_t opt_gop = 60;
float opt_fps = 30.0;
enum enc_rate_control_mode opt_rc = ENC_RATE_CONTROL_MODE_CQP;
uint32_t opt_bitrate = 10000;
uint16_t opt_qp = 22;
uint64_t opt_maxframes = UINT64_MAX;
char *opt_norefs = NULL;
char *opt_dropframes = NULL;
bool opt_intra_refresh = false;
bool opt_invalidate = true;
uint32_t opt_rc_layers = 1;
uint8_t opt_hierarchy = 1;

int next_noref(void)
{
   if (!opt_norefs)
      return -1;
   static char *save = NULL;
   char *tok = strtok_r(save ? NULL : opt_norefs, ",", &save);
   return tok ? atoi(tok) : -1;
}

int next_dropframe(void)
{
   if (!opt_dropframes)
      return -1;
   static char *save = NULL;
   char *tok = strtok_r(save ? NULL : opt_dropframes, ",", &save);
   return tok ? atoi(tok) : -1;
}

struct enc_rate_control_params rc_params[4];

uint8_t hierarchy_levels[][10] = {
   // 2 levels
   { 0, 1, 0xff },
   // 3 levels
   { 0, 2, 1, 2, 0xff },
   // 4 levels
   { 0, 3, 2, 3, 1, 3, 2, 3, 0xff },
};

int main(int argc, char *argv[])
{
   while (true) {
      int option_index = 0;
      int c = getopt_long(argc, argv, "", long_options, &option_index);
      if (c == -1)
         break;
      if (c == '?')
         return help();
      switch (option_index) {
      case 0:
         opt_device = optarg;
         break;
      case 1:
         if (!strcmp(optarg, "h264")) {
            opt_codec = ENC_CODEC_H264;
         } else {
            fprintf(stderr, "Invalid codec value '%s'\n", optarg);
            return 1;
         }
         break;
      case 2:
         opt_refs = atoi(optarg);
         break;
      case 3:
         opt_gop = atoi(optarg);
         break;
      case 4:
         opt_fps = atof(optarg);
         break;
      case 5:
         if (!strcmp(optarg, "cqp")) {
            opt_rc = ENC_RATE_CONTROL_MODE_CQP;
         } else if (!strcmp(optarg, "cbr")) {
            opt_rc = ENC_RATE_CONTROL_MODE_CBR;
         } else if (!strcmp(optarg, "vbr")) {
            opt_rc = ENC_RATE_CONTROL_MODE_VBR;
         } else if (!strcmp(optarg, "qvbr")) {
            opt_rc = ENC_RATE_CONTROL_MODE_QVBR;
         } else {
            fprintf(stderr, "Invalid rate control '%s'\n", optarg);
            return 1;
         }
         break;
      case 6:
         opt_bitrate = atoi(optarg);
         break;
      case 7:
         opt_qp = atoi(optarg);
         break;
      case 8:
         opt_maxframes = atoi(optarg);
         break;
      case 9:
         opt_norefs = optarg;
         break;
      case 10:
         opt_dropframes = optarg;
         break;
      case 11:
         opt_intra_refresh = true;
         break;
      case 12:
         opt_invalidate = false;
         break;
      case 13:
         opt_rc_layers = atoi(optarg);
         if (opt_rc_layers > 4)
            opt_rc_layers = 4;
         break;
      case 14:
         opt_hierarchy = atoi(optarg);
         break;
      default:
         fprintf(stderr, "Unhandled option %d\n", option_index);
         return 1;
      }
   }

   if (argc - optind < 2)
      return help();

   const char *input = argv[optind];
   const char *output = argv[optind + 1];

   if (create_decoder(opt_device, input) != 0) {
      fprintf(stderr, "Failed to initialize decode context\n");
      return 2;
   }

   struct enc_dmabuf dmabuf;
   if (!decode_frame(&dmabuf)) {
      fprintf(stderr, "Failed to decode frame\n");
      return 2;
   }

   struct enc_dev_params dev_params = {
      .device_path = opt_device,
   };
   struct enc_dev *dev = enc_dev_create(&dev_params);
   if (!dev) {
      fprintf(stderr, "Failed to create encoder device %s\n", opt_device);
      return 2;
   }

   for (uint32_t i = 0; i < opt_rc_layers; i++) {
      rc_params[i].frame_rate = opt_fps / (1 << i);
      rc_params[i].bit_rate = opt_bitrate / (1 << i) * 1000;
      rc_params[i].peak_bit_rate = opt_bitrate * (opt_rc == ENC_RATE_CONTROL_MODE_CBR ? 1000.0 : 1500.0);
      rc_params[i].vbv_buffer_size = opt_bitrate * 1000;
      rc_params[i].vbv_initial_fullness = opt_bitrate * 1000;
      rc_params[i].min_qp = 1;
      rc_params[i].max_qp = 51;
   }

   struct enc_encoder_params encoder_params = {
      .dev = dev,
      .codec = opt_codec,
      .width = dmabuf.width,
      .height = dmabuf.height,
      .bit_depth = 8,
      .num_refs = opt_refs,
      .gop_size = opt_gop,
      .rc_mode = opt_rc,
      .num_rc_layers = opt_rc_layers,
      .rc_params = rc_params,
      .intra_refresh = opt_intra_refresh,
      .h264 = {
         .profile = ENC_H264_PROFILE_HIGH,
         .level_idc = 100,
      },
   };
   struct enc_encoder *enc = enc_encoder_create(&encoder_params);
   assert(enc);

   bool quiet = false;

   FILE *fout = NULL;
   if (strcmp(output, "-") == 0) {
      fout = stdout;
      quiet = true;
   } else {
      fout = fopen(output, "w");
   }

   uint64_t ref_num = 0;
   uint64_t frame_num = 0;
   uint64_t noref_frame = next_noref();
   uint64_t drop_frame = next_dropframe();
   uint8_t hierarchy_level = 0;
   uint8_t hierarchy_idx = 0;
   uint64_t hierarchy_frames[4] = {0, 0, 0, 0};

   struct enc_frame_feedback feedback;
   struct enc_frame_params frame_params = {
      .qp = opt_qp,
      .feedback = &feedback,
   };

   do {
      struct enc_surface_params surface_params = {
         .dev = dev,
         .format = ENC_FORMAT_NV12,
         .width = dmabuf.width,
         .height = dmabuf.height,
         .dmabuf = &dmabuf,
      };
      struct enc_surface *surf = enc_surface_create(&surface_params);
      if (!surf) {
         fprintf(stderr, "Failed to import dmabuf\n");
         return 2;
      }

      frame_params.surface = surf;
      frame_params.not_referenced = false;
      frame_params.num_ref_list0 = 0;

      if (opt_hierarchy > 1) {
         hierarchy_level = hierarchy_levels[opt_hierarchy - 2][hierarchy_idx++];
         if (hierarchy_level == 0xff) {
            hierarchy_level = 0;
            hierarchy_idx = 1;
            for (uint32_t i = 1; i < 4; i++)
               hierarchy_frames[i] = hierarchy_frames[0];
         }
         if (hierarchy_level == opt_hierarchy - 1)
            frame_params.not_referenced = true;
         if (hierarchy_level == 0) {
            if (hierarchy_frames[0] != UINT64_MAX) {
               frame_params.num_ref_list0 = 1;
               frame_params.ref_list0[0] = hierarchy_frames[0];
            }
         } else {
            uint64_t max_id = 0;
            for (uint8_t i = 0; i < hierarchy_level; i++) {
               if (hierarchy_frames[i] != UINT64_MAX && hierarchy_frames[i] >= max_id) {
                  max_id = hierarchy_frames[i];
                  frame_params.num_ref_list0 = 1;
                  frame_params.ref_list0[0] = max_id;
               }
            }
         }
         frame_params.temporal_id = hierarchy_level % encoder_params.num_rc_layers;
      } else if (encoder_params.num_rc_layers > 1) {
         frame_params.temporal_id = (frame_params.temporal_id + 1) % encoder_params.num_rc_layers;
      }

      if (noref_frame > 0 && noref_frame == frame_num) {
         noref_frame = next_noref();
         frame_params.not_referenced = true;
      }

      struct enc_task *task = enc_encoder_encode_frame(enc, &frame_params);
      assert(enc_task_wait(task, UINT64_MAX));

      if (feedback.reference)
         ref_num++;

      if (drop_frame > 0 && drop_frame == ref_num) {
         drop_frame = next_dropframe();
         if (opt_invalidate)
            frame_params.invalidate_refs[frame_params.num_invalidate_refs++] = feedback.frame_id;
      } else {
         frame_params.num_invalidate_refs = 0;
         uint32_t size = 0;
         uint8_t *data = NULL;
         while (enc_task_get_bitstream(task, &size, &data))
            fwrite(data, size, 1, fout);
      }

      enc_task_destroy(task);
      enc_surface_destroy(surf);

      if (opt_hierarchy > 1) {
         if (feedback.frame_type == ENC_FRAME_TYPE_IDR) {
            hierarchy_idx = 1;
            for (uint32_t i = 0; i < 4; i++)
               hierarchy_frames[i] = feedback.frame_id;
         } else if (feedback.reference) {
            hierarchy_frames[hierarchy_level] = feedback.frame_id;
         }
      }

      if (!quiet) {
         fprintf(stderr, "\r Frame = %"PRIu64" (%"PRIu64") ref=%u level=%u ...", frame_num, feedback.frame_id, feedback.reference, hierarchy_level);
         fflush(stderr);
      }

      frame_num++;
   } while (decode_frame(&dmabuf) && frame_num < opt_maxframes);

   if (!quiet) {
      printf("\n Done\n");
   }

   fclose(fout);

   enc_encoder_destroy(enc);
   enc_dev_destroy(dev);
   return 0;
}
