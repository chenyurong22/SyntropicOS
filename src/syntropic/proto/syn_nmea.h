/**
 * @file syn_nmea.h
 * @brief NMEA 0183 / GNSS sentence parser and encoder.
 * @ingroup syn_protocol
 *
 * Provides a zero-allocation, streaming byte-at-a-time NMEA 0183 parser,
 * sentence checksum verification, and structured data decoders for standard
 * GNSS sentences:
 *   - GGA: Global Positioning System Fix Data
 *   - RMC: Recommended Minimum Specific GNSS Data
 *   - VTG: Course Over Ground & Ground Speed
 *   - GSA: GNSS DOP & Active Satellites
 *   - ZDA: UTC Time & Date
 */

#ifndef SYN_NMEA_H
#define SYN_NMEA_H

#if __has_include("syn_config.h")
#include "syn_config.h"
#endif

#if !defined(SYN_USE_NMEA) || SYN_USE_NMEA

#include "../common/syn_defs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SYN_GNSS_USE_FIXED_POINT
#define SYN_GNSS_USE_FIXED_POINT 0 /**< Enable fixed-point GNSS calculations if non-zero */
#endif

#ifndef SYN_NMEA_USE_FIXED_POINT
#define SYN_NMEA_USE_FIXED_POINT SYN_GNSS_USE_FIXED_POINT /**< NMEA fixed-point mode flag */
#endif

#if (SYN_GNSS_USE_FIXED_POINT != SYN_NMEA_USE_FIXED_POINT)
#error \
    "SyntropicOS Config Error: Mismatched GNSS numeric mode! SYN_NMEA_USE_FIXED_POINT must match SYN_GNSS_USE_FIXED_POINT."
#endif

/**
 * @brief Convert microdegrees (1e-7 deg) to decimal degrees.
 * @param udeg Microdegrees input value.
 * @return Decimal degrees equivalent.
 */
static inline double syn_udeg_to_deg(int32_t udeg)
{
    return (double)udeg / 10000000.0;
}

/**
 * @brief Convert decimal degrees to microdegrees (1e-7 deg).
 * @param deg Decimal degrees input value.
 * @return Microdegrees (1e-7 deg) equivalent.
 */
static inline int32_t syn_deg_to_udeg(double deg)
{
    return (int32_t)(deg * 10000000.0 + (deg >= 0.0 ? 0.5 : -0.5));
}

#define SYN_NMEA_MAX_SENTENCE_LEN 82 /**< Max NMEA 0183 sentence length */

/** @brief NMEA Sentence Type Enum */
typedef enum {
    SYN_NMEA_SENTENCE_UNKNOWN = 0,
    SYN_NMEA_SENTENCE_GGA,
    SYN_NMEA_SENTENCE_RMC,
    SYN_NMEA_SENTENCE_VTG,
    SYN_NMEA_SENTENCE_GSA,
    SYN_NMEA_SENTENCE_ZDA,
} SYN_NMEA_SentenceType;

/** @brief GPS Fix Quality Enum (from GGA) */
typedef enum {
    SYN_NMEA_FIX_INVALID = 0,
    SYN_NMEA_FIX_GPS = 1,
    SYN_NMEA_FIX_DGPS = 2,
    SYN_NMEA_FIX_PPS = 3,
    SYN_NMEA_FIX_RTK = 4,
    SYN_NMEA_FIX_FLOAT_RTK = 5,
    SYN_NMEA_FIX_ESTIMATED = 6,
} SYN_NMEA_FixQuality;

/** @brief Parsed NMEA GGA (Fix Data) Structure */
typedef struct {
    uint8_t hours;         /**< UTC hours (0..23) */
    uint8_t minutes;       /**< UTC minutes (0..59) */
    uint8_t seconds;       /**< UTC seconds (0..59) */
    uint16_t milliseconds; /**< UTC milliseconds (0..999) */
#if SYN_NMEA_USE_FIXED_POINT
    int32_t lat_udeg; /**< Micro-degrees (+N, -S, 1e-7 deg LSB) */
    int32_t lon_udeg; /**< Micro-degrees (+E, -W, 1e-7 deg LSB) */
#else
    double latitude;  /**< Decimal degrees (+N, -S) */
    double longitude; /**< Decimal degrees (+E, -W) */
#endif
    SYN_NMEA_FixQuality fix_quality; /**< Fix quality indicator */
    uint8_t num_satellites;          /**< Number of satellites in view/use */
    float hdop;                      /**< Horizontal Dilution of Precision */
    float altitude_m;                /**< Antenna altitude above mean sea level in meters */
    bool valid;                      /**< True if frame parsed successfully */
} SYN_NMEA_GGA;

/** @brief Parsed NMEA RMC (Recommended Minimum Data) Structure */
typedef struct {
    uint8_t hours;         /**< UTC hours (0..23) */
    uint8_t minutes;       /**< UTC minutes (0..59) */
    uint8_t seconds;       /**< UTC seconds (0..59) */
    uint16_t milliseconds; /**< UTC milliseconds (0..999) */
    bool status_valid;     /**< 'A' = valid, 'V' = receiver warning */
#if SYN_NMEA_USE_FIXED_POINT
    int32_t lat_udeg; /**< Micro-degrees (+N, -S, 1e-7 deg LSB) */
    int32_t lon_udeg; /**< Micro-degrees (+E, -W, 1e-7 deg LSB) */
#else
    double latitude;  /**< Decimal degrees (+N, -S) */
    double longitude; /**< Decimal degrees (+E, -W) */
#endif
    float speed_knots; /**< Speed over ground in knots */
    float course_deg;  /**< Course over ground in true degrees */
    uint8_t day;       /**< Day of month (1..31) */
    uint8_t month;     /**< Month of year (1..12) */
    uint16_t year;     /**< Full year (e.g. 2026) */
    bool valid;        /**< True if frame parsed successfully */
} SYN_NMEA_RMC;

/** @brief Parsed NMEA VTG (Velocity & Course) Structure */
typedef struct {
    float course_true_deg; /**< True track course in degrees */
    float speed_knots;     /**< Speed over ground in knots */
    float speed_kph;       /**< Speed over ground in km/h */
    bool valid;            /**< True if frame parsed successfully */
} SYN_NMEA_VTG;

/** @brief Parsed NMEA GSA (DOP & Active Satellites) Structure */
typedef struct {
    char mode;        /**< 'M' = Manual, 'A' = Automatic */
    uint8_t fix_type; /**< 1 = No fix, 2 = 2D fix, 3 = 3D fix */
    float pdop;       /**< Position Dilution of Precision */
    float hdop;       /**< Horizontal Dilution of Precision */
    float vdop;       /**< Vertical Dilution of Precision */
    bool valid;       /**< True if frame parsed successfully */
} SYN_NMEA_GSA;

/** @brief Parsed NMEA ZDA (UTC Date & Time) Structure */
typedef struct {
    uint8_t hours;         /**< UTC hours (0..23) */
    uint8_t minutes;       /**< UTC minutes (0..59) */
    uint8_t seconds;       /**< UTC seconds (0..59) */
    uint16_t milliseconds; /**< UTC milliseconds (0..999) */
    uint8_t day;           /**< Day of month (1..31) */
    uint8_t month;         /**< Month of year (1..12) */
    uint16_t year;         /**< Full year (e.g. 2026) */
    bool valid;            /**< True if frame parsed successfully */
} SYN_NMEA_ZDA;

/** @brief Streaming NMEA Parser State Machine */
typedef struct {
    char buf[SYN_NMEA_MAX_SENTENCE_LEN + 1]; /**< Sentence assembly line buffer */
    uint8_t pos;                             /**< Current buffer write index */
    bool in_sentence;                        /**< True if start byte '$' received */
} SYN_NMEA_Parser;

/**
 * @brief Initialize an NMEA streaming parser.
 * @param parser Pointer to parser state machine.
 */
void syn_nmea_parser_init(SYN_NMEA_Parser *parser);

/**
 * @brief Feed a byte into the NMEA streaming parser.
 *
 * @param parser Pointer to parser instance.
 * @param byte   Received character.
 * @param out_sentence Output buffer for completed NMEA sentence string (if returns true).
 * @return true if a complete, valid NMEA sentence was received.
 */
bool syn_nmea_parser_feed(SYN_NMEA_Parser *parser, char byte, char *out_sentence);

/**
 * @brief Calculate XOR checksum of an NMEA sentence payload (between '$' and '*').
 * @param sentence  NMEA sentence string.
 * @return 8-bit XOR checksum value.
 */
uint8_t syn_nmea_checksum(const char *sentence);

/**
 * @brief Validate an NMEA sentence string (checks start '$', '*XX' checksum, and CRLF).
 * @param sentence NMEA sentence string.
 * @return true if valid sentence and matching checksum.
 */
bool syn_nmea_validate(const char *sentence);

/**
 * @brief Identify sentence type from NMEA string.
 * @param sentence NMEA sentence string.
 * @return SYN_NMEA_SentenceType enum.
 */
SYN_NMEA_SentenceType syn_nmea_get_type(const char *sentence);

/**
 * @brief Parse NMEA DDMM.MMMM coordinate string and direction indicator.
 * @param nmea_coord  Coordinate string (e.g. "4807.038").
 * @param dir         Direction char ('N', 'S', 'E', 'W').
 * @return Decimal degrees (+N/+E, -S/-W).
 */
double syn_nmea_parse_coord(const char *nmea_coord, char dir);

/**
 * @brief Parse a $GPGGA / $GNGGA sentence.
 * @param sentence NMEA sentence string.
 * @param gga      Pointer to destination GGA struct.
 * @return true on success.
 */
bool syn_nmea_parse_gga(const char *sentence, SYN_NMEA_GGA *gga);

/**
 * @brief Parse a $GPRMC / $GNRMC sentence.
 * @param sentence NMEA sentence string.
 * @param rmc      Pointer to destination RMC struct.
 * @return true on success.
 */
bool syn_nmea_parse_rmc(const char *sentence, SYN_NMEA_RMC *rmc);

/**
 * @brief Parse a $GPVTG / $GNVTG sentence.
 * @param sentence NMEA sentence string.
 * @param vtg      Pointer to destination VTG struct.
 * @return true on success.
 */
bool syn_nmea_parse_vtg(const char *sentence, SYN_NMEA_VTG *vtg);

/**
 * @brief Parse a $GPGSA / $GNGSA sentence.
 * @param sentence NMEA sentence string.
 * @param gsa      Pointer to destination GSA struct.
 * @return true on success.
 */
bool syn_nmea_parse_gsa(const char *sentence, SYN_NMEA_GSA *gsa);

/**
 * @brief Parse a $GPZDA / $GNZDA sentence.
 * @param sentence NMEA sentence string.
 * @param zda      Pointer to destination ZDA struct.
 * @return true on success.
 */
bool syn_nmea_parse_zda(const char *sentence, SYN_NMEA_ZDA *zda);

#ifdef __cplusplus
}
#endif

#endif /* SYN_USE_NMEA */

#endif /* SYN_NMEA_H */
