// Copyright 2020 Christian Nilsson
//
/// @note Unsupported:
///    - Auto/Max button press (special format)

#include "ir_Corona.h"
#include <algorithm>
#include <cstring>
#include "IRac.h"
#include "IRrecv.h"
#include "IRsend.h"
#include "IRtext.h"
#include "IRutils.h"

using irutils::addBoolToString;
using irutils::addLabeledString;
using irutils::addModeToString;
using irutils::addTempToString;
using irutils::addFanToString;
using irutils::minsToString;
using irutils::setBit;
using irutils::setBits;

// Constants
const uint16_t kCoronaAcHdrMark = 3500;
const uint16_t kCoronaAcHdrSpace = 1680;
const uint16_t kCoronaAcBitMark = 450;
const uint16_t kCoronaAcOneSpace = 1270;
const uint16_t kCoronaAcZeroSpace = 420;
const uint16_t kCoronaAcSpaceGap = 10800;
const uint16_t kCoronaAcFreq = 38000;  // Hz.
const uint16_t kCoronaAcOverheadShort = 3;
const uint16_t kCoronaAcOverhead = 11;  // full message
const uint8_t kCoronaTolerance = 5;  // +5%

#if SEND_CORONA_AC
/// Send a CoronaAc formatted message.
/// Status: BETA / Appears to be working.
/// Where data is:
///   uint8_t data[kCoronaAcStateLength] = {
///   0x28, 0x61, 0x3D, 0x19, 0xE6, 0x37, 0xC8,
///   0x28, 0x61, 0x6D, 0xFF, 0x00, 0xFF, 0x00,
///   0x28, 0x61, 0xCD, 0xFF, 0x00, 0xFF, 0x00};
///
/// @param[in] data An array of bytes containing the IR command.
/// @param[in] nbytes Nr. of bytes of data in the array.
///                   (>=kCoronaAcStateLength)
/// @param[in] repeat Nr. of times the message is to be repeated.
void IRsend::sendCoronaAc(const uint8_t data[],
                          const uint16_t nbytes, const uint16_t repeat) {
  if (nbytes < kCoronaAcSectionBytes) return;
  if (kCoronaAcSectionBytes < nbytes &&
      nbytes < kCoronaAcStateLength) return;
  for (uint16_t r = 0; r <= repeat; r++) {
    uint16_t pos = 0;
    // Data Section #1 - 3 loop
    // e.g.
    //   bits = 56; bytes = 7;
    // #1  *(data + pos) = {0x28, 0x61, 0x3D, 0x19, 0xE6, 0x37, 0xC8};
    // #2  *(data + pos) = {0x28, 0x61, 0x6D, 0xFF, 0x00, 0xFF, 0x00};
    // #3  *(data + pos) = {0x28, 0x61, 0xCD, 0xFF, 0x00, 0xFF, 0x00};
    for (uint8_t section = 0; section < kCoronaAcSections; section++) {
      sendGeneric(kCoronaAcHdrMark, kCoronaAcHdrSpace,
                  kCoronaAcBitMark, kCoronaAcOneSpace,
                  kCoronaAcBitMark, kCoronaAcZeroSpace,
                  kCoronaAcBitMark, kCoronaAcSpaceGap,
                  data + pos, kCoronaAcSectionBytes,
                  kCoronaAcFreq, false, kNoRepeat, kDutyDefault);
      pos += kCoronaAcSectionBytes;  // Adjust by how many bytes was sent
      // don't send more data then what we have
      if (nbytes <= pos)
        break;
    }
  }
}
#endif  // SEND_CORONA_AC

#if DECODE_CORONA_AC
/// Decode the supplied CoronaAc message.
/// Status: BETA / Appears to be working.
/// @param[in,out] results Ptr to the data to decode & where to store it
/// @param[in] offset The starting index to use when attempting to decode the
///   raw data. Typically/Defaults to kStartOffset.
/// @param[in] nbits The number of data bits to expect.
/// @param[in] strict Flag indicating if we should perform strict matching.
/// @return A boolean. True if it can decode it, false if it can't.
bool IRrecv::decodeCoronaAc(decode_results *results, uint16_t offset,
                            const uint16_t nbits, const bool strict) {
  bool isLong = results->rawlen >= kCoronaAcBits * 2;
  if (results->rawlen < 2 * nbits +
      (isLong ? kCoronaAcOverhead : kCoronaAcOverheadShort)
      - offset)
    return false;  // Too short a message to match.
  if (strict && nbits != kCoronaAcBits && nbits != kCoronaAcBitsShort)
    return false;

  uint16_t pos = 0;
  uint16_t used = 0;

  // Data Section #1 - 3 loop
  // e.g.
  //   bits = 56; bytes = 7;
  // #1  *(results->state + pos) = {0x28, 0x61, 0x3D, 0x19, 0xE6, 0x37, 0xC8};
  // #2  *(results->state + pos) = {0x28, 0x61, 0x6D, 0xFF, 0x00, 0xFF, 0x00};
  // #3  *(results->state + pos) = {0x28, 0x61, 0xCD, 0xFF, 0x00, 0xFF, 0x00};
  for (uint8_t section = 0; section < kCoronaAcSections; section++) {
    DPRINT(uint64ToString(section));
    used = matchGeneric(results->rawbuf + offset, results->state + pos,
                        results->rawlen - offset, kCoronaAcBitsShort,
                        kCoronaAcHdrMark, kCoronaAcHdrSpace,
                        kCoronaAcBitMark, kCoronaAcOneSpace,
                        kCoronaAcBitMark, kCoronaAcZeroSpace,
                        kCoronaAcBitMark, kCoronaAcSpaceGap, true,
                        _tolerance + kCoronaTolerance, kMarkExcess, false);
    if (used == 0) return false;  // We failed to find any data.
    // short versions section 0 is special
    if (strict && !IRCoronaAc::validSection(results->state, pos,
                                            isLong ? section : 3))
      return false;
    offset += used;  // Adjust for how much of the message we read.
    pos += kCoronaAcSectionBytes;  // Adjust by how many bytes of data was read
    // don't read more data then what we have
    if (results->rawlen <= offset)
      break;
  }

  // Re-check we got the correct size/length due to the way we read the data.
  if (strict && pos * 8 != kCoronaAcBits && pos * 8 != kCoronaAcBitsShort) {
    DPRINTLN("strict bit match fail");
    return false;
  }

  // Success
  results->decode_type = decode_type_t::CORONA_AC;
  results->bits = pos * 8;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}
#endif  // DECODE_CORONA_AC

/// Class constructor for handling detailed Corona A/C messages.
/// @param[in] pin GPIO to be used when sending.
/// @param[in] inverted Is the output signal to be inverted?
/// @param[in] use_modulation Is frequency modulation to be used?
/// @return An IRCoronaAc object.
IRCoronaAc::IRCoronaAc(const uint16_t pin, const bool inverted,
                       const bool use_modulation)
    : _irsend(pin, inverted, use_modulation) { stateReset(); }

/// Reset the internal state to a fixed known good state.
/// @note The state is powered off.
void IRCoronaAc::stateReset(void) {
  // known good state
  remote_state[kCoronaAcSectionD0Pos] = kCoronaAcSectionD0Base;
  remote_state[kCoronaAcSectionD1Pos] = 0x00;  // ensure no unset mem
  setTemp(kCoronaAcMinTemp);
  setMode(kCoronaAcModeCool);
  setFan(kCoronaAcFanAuto);
  setOnTimer(kCoronaAcTimerOff);
  setOffTimer(kCoronaAcTimerOff);
  checksum(remote_state);
}

/// Get the byte that identifies the section
/// @param[in] section index of the section 0-2,
///            3 and above is used as the special case for short message
/// @return the byte used for the section
uint8_t IRCoronaAc::getSectionByte(const uint8_t section) {
  // base byte
  uint8_t b = kCoronaAcSectionLabelBase;
  // 2 enabled bits shifted 0-2 bits depending on section
  if (section >= 3)
    return 0b10010000 | b;
  setBits(&b, 4, 4, 0b11 << section);
  return b;
}

/// Check that a CoronaAc Section part is valid with section byte and inverted
/// @param[in] state An array of bytes containing the section
/// @param[in] pos where to start in the state array
/// @param[in] section which section to work with
///            Used to get the section byte, and is validated against pos
/// @return true if section is valid, otherwise false
bool IRCoronaAc::validSection(const uint8_t state[], const uint16_t pos,
                              const uint8_t section) {
  // sanity check, pos must match section, section 4 is at pos 0
  if ((section % kCoronaAcSections) * kCoronaAcSectionBytes != pos)
    return false;
  // all individual sections has the same prefix
  if (state[pos + kCoronaAcSectionB0Pos] != kCoronaAcSectionB0) {
    DPRINT("State ");
    DPRINT(pos);
    DPRINT(" expected 0x28 was ");
    DPRINTLN(uint64ToString(state[pos + kCoronaAcSectionB0Pos], 16));
    return false;
  }
  if (state[pos + kCoronaAcSectionB1Pos] != kCoronaAcSectionB1) {
    DPRINT("State ");
    DPRINT(pos + kCoronaAcSectionB1Pos);
    DPRINT(" expected 0x61 was ");
    DPRINTLN(uint64ToString(state[pos + kCoronaAcSectionB1Pos], 16));
    return false;
  }

  // checking section byte
  if (state[pos + kCoronaAcSectionLabelPos] != getSectionByte(section)) {
    DPRINT("check 2 not matching, got ");
    DPRINT(uint64ToString(state[pos + kCoronaAcSectionLabelPos], 16));
    DPRINT(" expected ");
    DPRINTLN(uint64ToString(getSectionByte(section), 16));
    return false;
  }

  // checking inverts
  uint8_t d0invinv = ~state[pos + kCoronaAcSectionD0InvPos];
  if (state[pos + kCoronaAcSectionD0Pos] != d0invinv) {
    DPRINT("inverted 3 - 4 not matching, got ");
    DPRINT(uint64ToString(state[pos + kCoronaAcSectionD0Pos], 16));
    DPRINT(" vs ");
    DPRINT(uint64ToString(state[pos + kCoronaAcSectionD0InvPos], 16));
    DPRINT(" expected ");
    DPRINTLN(uint64ToString(~state[pos + kCoronaAcSectionD0Pos] & 0xff, 16));
    return false;
  }
  uint8_t d1invinv = ~state[pos + kCoronaAcSectionD1InvPos];
  if (state[pos + kCoronaAcSectionD1Pos] != d1invinv) {
    DPRINT("inverted 5 - 6 not matching, got ");
    DPRINT(uint64ToString(state[pos + kCoronaAcSectionD1Pos], 16));
    DPRINT(" vs ");
    DPRINT(uint64ToString(state[pos + kCoronaAcSectionD1InvPos], 16));
    DPRINT(" expected ");
    DPRINTLN(uint64ToString(~state[pos + kCoronaAcSectionD1Pos] & 0xff, 16));
    return false;
  }
  return true;
}

/// Calculate and set the check values for the internal state.
/// @param[in,out] data the array to be modified
void IRCoronaAc::checksum(uint8_t* data) {
  uint8_t pos;
  for (uint8_t section = 0; section < kCoronaAcSections; section++) {
    pos = section * kCoronaAcSectionBytes;
    data[pos + kCoronaAcSectionB0Pos] = kCoronaAcSectionB0;
    data[pos + kCoronaAcSectionB1Pos] = kCoronaAcSectionB1;
    data[pos + kCoronaAcSectionLabelPos] = getSectionByte(section);
    data[pos + kCoronaAcSectionD0InvPos] =
      ~data[pos + kCoronaAcSectionD0Pos];
    data[pos + kCoronaAcSectionD1InvPos] =
      ~data[pos + kCoronaAcSectionD1Pos];
  }
}

void IRCoronaAc::begin(void) { _irsend.begin(); }

#if SEND_CORONA_AC
void IRCoronaAc::send(const uint16_t repeat) {
  _irsend.sendCoronaAc(getRaw(), kCoronaAcStateLength, repeat);
}
#endif  // SEND_CORONA_AC

/// Get a copy of the internal state as a valid code for this protocol.
/// @return A valid code for this protocol based on the current internal state.
uint8_t* IRCoronaAc::getRaw(void) {
  checksum(remote_state);  // Ensure correct settings before sending.
  return remote_state;
}

/// Set the internal state from a valid code for this protocol.
/// @param state A valid code for this protocol.
void IRCoronaAc::setRaw(const uint8_t new_code[], const uint16_t length) {
  memcpy(remote_state, new_code, std::min(length, kCoronaAcStateLength));
}

/// Set the temp in deg C.
/// @param temp The desired temperature in Celsius.
void IRCoronaAc::setTemp(const uint8_t temp) {
  uint8_t degrees = std::max(temp, kCoronaAcMinTemp);
  degrees = std::min(degrees, kCoronaAcMaxTemp);
  setBits(&remote_state[kCoronaAcSectionD1Pos], kCoronaAcTempOffset,
          kCoronaAcTempSize, degrees - kCoronaAcMinTemp + 1);
}

/// Get the current temperature from the internal state.
/// @return The current temperatue in Celsius.
uint8_t IRCoronaAc::getTemp(void) {
  return GETBITS8(remote_state[kCoronaAcSectionD1Pos], kCoronaAcTempOffset,
      kCoronaAcTempSize) + kCoronaAcMinTemp - 1;
}

/// Change the power setting.
/// @param on true, the setting is on. false, the setting is off.
/// @note If changed, setPowerToggle is also needed,
///       unless timer is or was active
void IRCoronaAc::setPower(const bool on) {
  setBit(&remote_state[kCoronaAcSectionD1Pos], kCoronaAcPowerOffset, on);
  // setting power state resets timers that would cause the state
  if (on)
    setOnTimer(kCoronaAcTimerOff);
  else
    setOffTimer(kCoronaAcTimerOff);
}

/// Get the value of the current power setting.
/// @return true, the setting is on. false, the setting is off.
bool IRCoronaAc::getPower(void) {
  return GETBIT8(remote_state[kCoronaAcSectionD1Pos], kCoronaAcPowerOffset);
}

/// Change the power toggle setting.
/// @param on true, the setting is on. false, the setting is off.
/// @note this sets that the AC should change power,
///       use setPower to define if the AC should end up as on or off
void IRCoronaAc::setPowerToggle(const bool toggle) {
  setBit(&remote_state[kCoronaAcSectionD1Pos],
      kCoronaAcPowerToggleOffset, toggle);
}

/// Get the value of the current power toggle setting.
/// @return true, the setting is on. false, the setting is off.
bool IRCoronaAc::getPowerToggle(void) {
  return GETBIT8(remote_state[kCoronaAcSectionD1Pos],
      kCoronaAcPowerToggleOffset);
}

/// Change the power setting to On.
void IRCoronaAc::on(void) { setPower(true); }

/// Change the power setting to Off.
void IRCoronaAc::off(void) { setPower(false); }

/// Get the operating mode setting of the A/C.
/// @return The current operating mode setting.
uint8_t IRCoronaAc::getMode(void) {
  return GETBITS8(remote_state[kCoronaAcSectionD1Pos],
      kCoronaAcModeOffset, kCoronaAcModeSize);
}

/// Set the operating mode of the A/C.
/// @param mode The desired operating mode.
void IRCoronaAc::setMode(const uint8_t mode) {
  switch (mode) {
    case kCoronaAcModeCool:
    case kCoronaAcModeDry:
    case kCoronaAcModeFan:
    case kCoronaAcModeHeat:
      setBits(&remote_state[kCoronaAcSectionD1Pos],
              kCoronaAcModeOffset, kCoronaAcModeSize,
              mode);
      return;
    default:
      this->setMode(kCoronaAcModeCool);
  }
}

/// Convert a standard A/C mode into its native mode.
/// @param mode A stdAc::opmode_t mode to be converted to it's native equivalent
/// @return The corresponding native mode.
uint8_t IRCoronaAc::convertMode(const stdAc::opmode_t mode) {
  switch (mode) {
    case stdAc::opmode_t::kFan:  return kCoronaAcModeFan;
    case stdAc::opmode_t::kDry:  return kCoronaAcModeDry;
    case stdAc::opmode_t::kHeat: return kCoronaAcModeHeat;
    default: return kCoronaAcModeCool;
  }
}

/// Convert a native mode to it's common stdAc::opmode_t equivalent.
/// @param mode A native operation mode to be converted.
/// @return The corresponding common stdAc::opmode_t mode.
stdAc::opmode_t IRCoronaAc::toCommonMode(const uint8_t mode) {
  switch (mode) {
    case kCoronaAcModeFan:  return stdAc::opmode_t::kFan;
    case kCoronaAcModeDry:  return stdAc::opmode_t::kDry;
    case kCoronaAcModeHeat: return stdAc::opmode_t::kHeat;
    default: return stdAc::opmode_t::kCool;
  }
}

/// Get the operating speed of the A/C Fan
/// @return The current operating fan speed setting
uint8_t IRCoronaAc::getFan(void) {
  return GETBITS8(remote_state[kCoronaAcSectionD0Pos],
      kCoronaAcFanOffset, kCoronaAcFanSize);
}

/// Set the operating speed of the A/C Fan
/// @param speed The desired fan speed
void IRCoronaAc::setFan(const uint8_t speed) {
  if (speed > kCoronaAcFanHigh)
    setFan(kCoronaAcFanAuto);
  else
    setBits(&remote_state[kCoronaAcSectionD0Pos],
            kCoronaAcFanOffset, kCoronaAcFanSize, speed);
}

/// Change the powersave setting.
/// @param on true, the setting is on. false, the setting is off.
void IRCoronaAc::setEcono(const bool on) {
  setBit(&remote_state[kCoronaAcSectionD0Pos],
      kCoronaAcPowerSaveOffset, on);
}

/// Get the value of the current powersave setting.
/// @return true, the setting is on. false, the setting is off.
bool IRCoronaAc::getEcono(void) {
  return GETBIT8(remote_state[kCoronaAcSectionD0Pos],
      kCoronaAcPowerSaveOffset);
}

// Convert a standard A/C Fan speed into its native fan speed.
uint8_t IRCoronaAc::convertFan(const stdAc::fanspeed_t speed) {
  switch (speed) {
    case stdAc::fanspeed_t::kMin:
    case stdAc::fanspeed_t::kLow:    return kCoronaAcFanLow;
    case stdAc::fanspeed_t::kMedium: return kCoronaAcFanMedium;
    case stdAc::fanspeed_t::kHigh:
    case stdAc::fanspeed_t::kMax:    return kCoronaAcFanHigh;
    default:                         return kCoronaAcFanAuto;
  }
}

// Convert a native fan speed to it's common equivalent.
stdAc::fanspeed_t IRCoronaAc::toCommonFanSpeed(const uint8_t speed) {
  switch (speed) {
    case kCoronaAcFanHigh:    return stdAc::fanspeed_t::kHigh;
    case kCoronaAcFanMedium:  return stdAc::fanspeed_t::kMedium;
    case kCoronaAcFanLow:     return stdAc::fanspeed_t::kLow;
    default:                  return stdAc::fanspeed_t::kAuto;
  }
}

/// Set the Vertical Swing mode of the A/C.
/// @param on true, the setting is on. false, the setting is off.
/// @note This is a button press, and not a state
///       after sending it once you should turn it off
void IRCoronaAc::setSwingV(const bool on) {
  setBit(&remote_state[kCoronaAcSectionD0Pos],
      kCoronaAcSwingVToogleOffset, on);
}

/// Get the Vertical Swing mode of the A/C.
/// @return true, the setting is on. false, the setting is off.
bool IRCoronaAc::getSwingV(void) {
  return GETBIT64(remote_state[kCoronaAcSectionD0Pos],
      kCoronaAcSwingVToogleOffset);
}

/// Set the Timer time
/// @param nr_of_mins Number of minutes to set the timer to.
///   (non in range value is disable).
///   Valid is from 1 minute to 12 hours
void IRCoronaAc::_setTimer(const uint8_t section, const uint16_t nr_of_mins) {
  // default to off
  uint16_t hsecs = kCoronaAcTimerOff;
  if (1 <= nr_of_mins && nr_of_mins <= kCoronaAcTimerMax)
    hsecs = nr_of_mins * kCoronaAcTimerUnitsPerMin;

  uint8_t pos = section * kCoronaAcSectionBytes;
  remote_state[pos + kCoronaAcSectionD1Pos] = (hsecs & 0xff00) >> 8;
  remote_state[pos + kCoronaAcSectionD0Pos] = (hsecs & 0xff);
}

/// Get the current Timer time
/// @return The number of minutes it is set for. 0 means it's off.
/// @note The A/C protocol supports 2 second increments
uint16_t IRCoronaAc::_getTimer(const uint8_t section) {
  uint8_t pos = section * kCoronaAcSectionBytes;
  uint16_t hsecs = remote_state[pos + kCoronaAcSectionD1Pos] << 8
                 | remote_state[pos + kCoronaAcSectionD0Pos];

  if (hsecs == kCoronaAcTimerOff)
    return 0;

  return hsecs / kCoronaAcTimerUnitsPerMin;
}

/// Get the current On Timer time
/// @return The number of minutes it is set for. 0 means it's off.
uint16_t IRCoronaAc::getOnTimer(void) {
  return _getTimer(kCoronaAcOnTimerSection);
}

/// Set the On Timer time
/// @param nr_of_mins Number of minutes to set the timer to.
///   (0 or kCoronaAcTimerOff is disable).
void IRCoronaAc::setOnTimer(const uint16_t nr_of_mins) {
  _setTimer(kCoronaAcOnTimerSection, nr_of_mins);
  // if we set a timer value, clear the other timer
  if (getOnTimer() != 0)
    setOffTimer(kCoronaAcTimerOff);
}

/// Get the current Off Timer time
/// @return The number of minutes it is set for. 0 means it's off.
uint16_t IRCoronaAc::getOffTimer(void) {
  return _getTimer(kCoronaAcOffTimerSection);
}

/// Set the Off Timer time
/// @param nr_of_mins Number of minutes to set the timer to.
///   (0 or kCoronaAcTimerOff is disable).
void IRCoronaAc::setOffTimer(const uint16_t nr_of_mins) {
  _setTimer(kCoronaAcOffTimerSection, nr_of_mins);
  // if we set a timer value, clear the other timer
  if (getOffTimer() != 0)
    setOnTimer(kCoronaAcTimerOff);
}

/// Convert the internal state into a human readable string.
/// @return The current internal state expressed as a human readable String.
String IRCoronaAc::toString(void) {
  String result = "";
  result.reserve(120);  // Reserve some heap for the string to reduce fragging.
  result += addBoolToString(getPower(), kPowerStr, false);
  result += addBoolToString(getPowerToggle(), kPowerToggleStr);
  result += addModeToString(getMode(), 0xFF, kCoronaAcModeCool,
                            kCoronaAcModeHeat, kCoronaAcModeDry,
                            kCoronaAcModeFan);
  result += addTempToString(getTemp());
  result += addFanToString(getFan(), kCoronaAcFanHigh, kCoronaAcFanLow,
                           kCoronaAcFanAuto, kCoronaAcFanAuto,
                           kCoronaAcFanMedium);
  result += addBoolToString(getSwingV(), kSwingVToggleStr);
  result += addBoolToString(getEcono(), kEconoStr);
  result += addLabeledString(getOnTimer()
                             ? minsToString(getOnTimer()) : kOffStr,
                             kOnTimerStr);
  result += addLabeledString(getOffTimer()
                             ? minsToString(getOffTimer()) : kOffStr,
                             kOffTimerStr);
  return result;
}

/// Convert the A/C state to it's common stdAc::state_t equivalent.
/// @return A stdAc::state_t state.
stdAc::state_t IRCoronaAc::toCommon(const stdAc::state_t *prev) {
  stdAc::state_t result;
  if (prev != NULL) {
    result = *prev;
  } else {
    result.swingv = stdAc::swingv_t::kOff;
  }
  result.protocol = decode_type_t::CORONA_AC;
  result.model = -1;  // No models used.
  result.power = getPower();
  result.mode = toCommonMode(getMode());
  result.celsius = true;
  result.degrees = getTemp();
  result.fanspeed = toCommonFanSpeed(getFan());
  if (getSwingV()) {
    result.swingv = result.swingv != stdAc::swingv_t::kOff ?
        stdAc::swingv_t::kOff : stdAc::swingv_t::kAuto;  // Invert swing.
  }
  result.sleep = -1;
  // Not supported.
  result.swingh = stdAc::swingh_t::kOff;
  result.turbo = false;
  result.quiet = false;
  result.clean = false;
  result.filter = false;
  result.beep = false;
  result.econo = getEcono();
  result.light = false;
  result.clock = -1;
  return result;
}
