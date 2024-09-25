#pragma once

enum enc_hevc_profile {
   ENC_HEVC_PROFILE_MAIN = 1,
   ENC_HEVC_PROFILE_MAIN_10 = 2,
};

enum enc_hevc_tier {
   ENC_HEVC_TIER_MAIN = 0,
   ENC_HEVC_TIER_HIGH = 1,
};

struct enc_hevc_encoder_params {
   // Profile.
   enum enc_hevc_profile profile;

   // Tier.
   enum enc_hevc_tier tier;

   // Level.
   uint8_t level;
};
