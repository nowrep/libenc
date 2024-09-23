#pragma once

enum enc_av1_profile {
   ENC_AV1_PROFILE_0 = 0,
};

struct enc_av1_encoder_params {
   // Profile.
   enum enc_av1_profile profile;

   // Matches AV1 spec.
   uint8_t seq_level_idx;
};
