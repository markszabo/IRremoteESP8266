// Copyright 2020 Victor Mukayev (vitos1k)
/// @file
/// @brief Support for the MilesTag2 IR protocol for LaserTag gaming
/// @see http://hosting.cmalton.me.uk/chrism/lasertag/MT2Proto.pdf

// Supports:
//   Brand: Theoretically MilesTag2 supported hardware

//TODO: This implementation would support only short SHOT packets(14bits) and MSGs = 24bits. Support for long MSGs > 24bits is TODO

#include <algorithm>
#include "IRrecv.h"
#include "IRsend.h"
#include "IRutils.h"

// Constants
// Constants are similar to Sony SIRC protocol, bassically they are very similar, just nbits are varying

const uint16_t kMilesHdrMark = 2400;
const uint16_t kMilesSpace = 600;
const uint16_t kMilesOneMark = 1200;
const uint16_t kMilesZeroMark = 600;
const uint16_t kMilesRptLength = 32000;
const uint16_t kMilesMinGap = 32000;
const uint16_t kMilesStdFreq = 38000;  // kHz
const uint16_t kMilesStdDuty = 25;



#if SEND_MILESTAG2
/// Send a MilesTag2 formatted message.
/// Status: NEEDS TESTING
/// @param[in] data The message to be sent.
/// @param[in] nbits The number of bits of message to be sent.
/// @param[in] repeat The number of times the command is to be repeated.
void IRsend::sendMilesShot(const uint64_t data, const uint16_t nbits,
                      const uint16_t repeat)
{
    _sendMiles(data, nbits,repeat);
}

void IRsend::sendMilesMsg(const uint64_t data, const uint16_t nbits,
                      const uint16_t repeat)
{
    _sendMiles(data, nbits,repeat);
}

void IRsend::_sendMiles(const uint64_t data, const uint16_t nbits,
                      const uint16_t repeat) {
  enableIROut(kMilesStdFreq, kMilesStdDuty);
    // We always send a message, even for repeat=0, hence '<= repeat'.
  for (uint16_t r = 0; r <= repeat; r++) {

    // Header
    mark(kMilesHdrMark);
    //space(kMilesSpace);
    // Data
    if (nbits == 0)  // If we are asked to send nothing, just return.
      return;
    // Send the MSB first.    
    // Send the supplied data.
    for (uint64_t mask = 1ULL << (nbits - 1); mask; mask >>= 1)
      if (data & mask) {  // Send a 1
        space(kMilesSpace);
        mark(kMilesOneMark);        
      } else {  // Send a 0        
        space(kMilesSpace);
        mark(kMilesZeroMark);
      }
      space(kMilesRptLength);
  }
}
#endif  // SEND_MILESTAG2

#if DECODE_MILESTAG2
/// Decode the supplied MilesTag2 message.
/// Status: NEEDS TESTING
/// @param[in,out] results Ptr to the data to decode & where to store the result
/// @param[in] offset The starting index to use when attempting to decode the
///   raw data. Typically/Defaults to kStartOffset.
/// @param[in] nbits The number of data bits to expect.
/// @param[in] strict Flag indicating if we should perform strict matching.
/// @return True if it can decode it, false if it can't.
/// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/706
bool IRrecv::decodeMiles(decode_results *results, uint16_t offset,
                        const uint16_t nbits, const bool strict) {
  /*
  uint16_t gap_pos = 0;
  if (results->rawlen >= (2 * nbits + 1)) //we got alot more data than we thought, let's find last GAP and work from it
  {
      for (uint16_t ind = 0 ; ind < results->rawlen; ind++)
      {
          if (matchAtLeast(*(results->rawbuf+ind),34000)) gap_pos = ind;
      }
  }

  if (results->rawlen>gap_pos) offset = gap_pos+1;
  */
  
  // Compliance
  if (strict) {
    switch (nbits) {  // Check we've been called with a correct bit size.
      case 14:
      case 24:      
        break;
      default:
        DPRINT("incorrect nbits:");
        DPRINTLN(nbits);
        return false;  // The request doesn't strictly match the protocol defn.
    }
  }
  uint64_t data = 0;
  // Header
  if (!matchMark(*(results->rawbuf + offset++), kMilesHdrMark, kUseDefTol, kMarkExcess)) return 0;
  // Data
  uint16_t shift = 0;
  for (shift = 0; shift < nbits * 2;shift += 2) {
    // Is the bit a '1'?
    if (matchMark(*(results->rawbuf + 1 + offset + shift), kMilesOneMark, kUseDefTol, kMarkExcess) &&
        matchSpace(*(results->rawbuf + offset + shift), kMilesSpace, kUseDefTol, kMarkExcess)) {
      data = (data << 1) | 1;
    } else if (matchMark(*(results->rawbuf + 1 + offset + shift), kMilesZeroMark, kUseDefTol, kMarkExcess) &&
               matchSpace(*(results->rawbuf + offset + shift), kMilesSpace, kUseDefTol, kMarkExcess)) {
      data <<= 1;  // The bit is a '0'.
    } else {      
      return false;  // It's neither, so fail.
    }
  }  
  // Success
  results->bits = nbits;
  results->value = data;
  switch (nbits)
  {
    case 14:
      results->decode_type = decode_type_t::MILESTAG2SHOT;
      break;
    case 24:
      results->decode_type = decode_type_t::MILESTAG2MSG;
      break;
    default:
      return false;
  }  
  results->command = 0;
  results->address = 0;
  return true;
}
#endif  // DECODE_MILESTAG2