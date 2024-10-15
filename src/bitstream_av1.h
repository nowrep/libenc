#pragma once

#include "bitstream.h"

class bitstream_av1 : public bitstream
{
public:
   struct seq {
      uint8_t seq_profile;
      uint8_t still_picture : 1;
      uint8_t reduced_still_picture_header : 1;
      uint8_t timing_info_present_flag : 1;
      uint32_t num_units_in_display_tick;
      uint32_t time_scale;
      uint8_t equal_picture_interval : 1;
      uint32_t num_ticks_per_picture_minus_1;
      uint8_t operating_points_cnt_minus_1 : 1;
      uint16_t operating_point_idc[16];
      uint8_t seq_level_idx[16];
      uint8_t seq_tier[16];
      uint8_t frame_width_bits_minus_1;
      uint8_t frame_height_bits_minus_1;
      uint32_t max_frame_width_minus_1;
      uint32_t max_frame_height_minus_1;
      uint8_t use_128x128_superblock : 1;
      uint8_t enable_filter_intra : 1;
      uint8_t enable_intra_edge_filter : 1;
      uint8_t enable_interintra_compound : 1;
      uint8_t enable_masked_compound : 1;
      uint8_t enable_warped_motion : 1;
      uint8_t enable_dual_filter : 1;
      uint8_t enable_cdef : 1;
      uint8_t high_bitdepth : 1;
      uint8_t color_description_present_flag : 1;
      uint8_t color_primaries;
      uint8_t transfer_characteristics;
      uint8_t matrix_coefficients;
      uint8_t color_range : 1;
      uint8_t chroma_sample_position;
   };

   struct frame {
      uint8_t obu_extension_flag : 1;
      uint8_t temporal_id;
      uint8_t frame_type;
      uint8_t show_frame : 1;
      uint8_t showable_frame : 1;
      uint8_t error_resilient_mode : 1;
      uint8_t disable_cdf_update : 1;
      uint8_t primary_ref_frame;
      uint8_t refresh_frame_flags;
      uint8_t ref_frame_idx[7];
      uint8_t allow_high_precision_mv : 1;
      uint8_t is_filter_switchable : 1;
      uint8_t interpolation_filter;
      uint8_t is_motion_mode_switchable : 1;
      uint8_t disable_frame_end_update_cdf : 1;
      uint8_t uniform_tile_spacing_flag : 1;
      uint8_t base_q_idx;
      uint8_t tx_mode_select : 1;
      uint8_t reduced_tx_set;
   };

   struct frame_offsets {
      uint32_t obu_size;
      uint32_t base_q_idx;
      uint32_t segmentation_enabled;
      uint32_t loop_filter_params;
      uint32_t cdef_params;
      uint32_t cdef_params_size;
      uint32_t frame_header_end;
   };

   bitstream_av1(uint8_t obu_size_bytes);

   void write_temporal_delimiter();
   void write_seq(const seq &seq);
   frame_offsets write_frame(const frame &frame, const seq &seq);

private:
   uint32_t write_obu(uint8_t type, const bitstream &bs, uint8_t size_bytes = 0, uint8_t temporal_id = 0xff);

   uint8_t obu_size_bytes;
};
