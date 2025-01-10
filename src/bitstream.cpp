#include "bitstream.h"
#include "util.h"

bitstream::bitstream()
{
}

bitstream::~bitstream()
{
}

uint32_t bitstream::size() const
{
   return buf.size();
}

uint32_t bitstream::size_bits() const
{
   return size() * 8 + bits_in_shifter;
}

const uint8_t *bitstream::data() const
{
   return buf.data();
}

void bitstream::reset()
{
   buf.clear();
   shifter = 0;
   bits_in_shifter = 0;
   emulation_prevention = false;
}

void bitstream::ue(uint32_t value)
{
    uint32_t bits = 0;
    uint32_t tmp = ++value;
    while (tmp) {
        tmp >>= 1;
        bits++;
    }
    ui(0, bits - 1);
    ui(value, bits);
}

void bitstream::se(int32_t value)
{
    if (value <= 0)
        ue(-2 * value);
    else
        ue(2 * value - 1);
}

void bitstream::ui(uint32_t value, uint8_t bits)
{
   unsigned int bits_to_pack = 0;

   while (bits > 0) {
      uint32_t value_to_pack = value & (0xffffffff >> (32 - bits));
      bits_to_pack = bits > (32 - bits_in_shifter) ? (32 - bits_in_shifter) : bits;

      if (bits_to_pack < bits)
         value_to_pack = value_to_pack >> (bits - bits_to_pack);

      shifter |= value_to_pack << (32 - bits_in_shifter - bits_to_pack);
      bits -= bits_to_pack;
      bits_in_shifter += bits_to_pack;

      while (bits_in_shifter >= 8) {
         uint8_t output_byte = shifter >> 24;
         shifter <<= 8;
         if (emulation_prevention && num_zeros >= 2 && output_byte <= 3) {
            buf.push_back(3);
            num_zeros = 0;
         }
         buf.push_back(output_byte);
         num_zeros = output_byte ? 0 : num_zeros + 1;
         bits_in_shifter -= 8;
      }
   }
}

void bitstream::le(uint64_t value, uint8_t bytes)
{
   for (uint8_t i = 0; i < bytes; i++)
      ui((value >> (i * 8)) & 0xff, 8);
}

void bitstream::leb128(uint64_t value, uint8_t bytes)
{
   if (!bytes)
      bytes = (logbase2_ceil(value) + 7) / 7;

   for (uint8_t i = 0; i < bytes; i++) {
      uint8_t byte = (value >> (7 * i)) & 0x7f;
      if (i != bytes - 1)
         byte |= 0x80;
      ui(byte, 8);
   }
}

void bitstream::byte_align()
{
   uint32_t num_padding_zeros = (32 - bits_in_shifter) % 8;
   if (num_padding_zeros > 0)
      ui(0x0, num_padding_zeros);
}

void bitstream::trailing_bits()
{
   ui(0x1, 1);
   byte_align();
}

void bitstream::enable_emulation_prevention()
{
   emulation_prevention = true;
}
