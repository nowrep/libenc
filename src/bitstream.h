#pragma once

#include <vector>
#include <stdint.h>

class bitstream
{
public:
   bitstream();
   virtual ~bitstream();

   void reset();
   uint32_t size() const;
   const uint8_t *data() const;

   void ue(uint32_t value);
   void se(int32_t value);
   void ui(uint32_t value, uint8_t bits);

   void byte_align();
   void trailing_bits();

private:
   std::vector<uint8_t> buf;
   uint32_t buf_pos = 0;
   uint32_t shifter = 0;
   uint32_t bits_in_shifter = 0;
   uint32_t bits_output = 0;
};
