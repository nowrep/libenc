#pragma once

enum enc_av1_profile {
   ENC_AV1_PROFILE_0 = 0,
};

enum enc_av1_tier {
   ENC_AV1_TIER_MAIN = 0,
   ENC_AV1_TIER_HIGH = 1,
};

struct enc_av1_encoder_params {
   // Profile.
   enum enc_av1_profile profile;

   // Tier.
   enum enc_av1_tier tier;

   // Level.
   uint8_t level;
};
