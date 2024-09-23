#pragma once

#include "bitstream.h"

class bitstream_av1 : public bitstream
{
public:
   struct seq {
      uint8_t seq_profile;
      uint8_t still_picture : 1;
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
      uint8_t enable_restoration : 1;
      uint8_t high_bitdepth : 1;
      uint8_t color_description_present_flag : 1;
      uint8_t color_primaries;
      uint8_t transfer_characteristics;
      uint8_t matrix_coefficients;
      uint8_t color_range : 1;
      uint8_t chroma_sample_position;
      uint8_t separate_uv_delta_q : 1;
      uint8_t film_grain_params_present : 1;
   };

   struct frame {
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
      uint8_t use_ref_frame_mvs : 1;
      uint8_t disable_frame_end_update_cdf : 1;
      uint8_t uniform_tile_spacing_flag : 1;
      // tile_info()
      uint8_t base_q_idx;
   };

   bitstream_av1();

   void write_seq(const seq &seq);
   void write_frame(const frame &frame);

private:
   void write_obu(uint8_t type, const bitstream &bs);
};
