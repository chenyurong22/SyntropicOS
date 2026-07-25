/**
 * @file syn_geo.h
 * @brief Geodetic coordinate transformations and distance calculation library.
 *
 * Provides 64-bit precision WGS-84 ellipsoid coordinate transformations
 * between spherical GNSS coordinates (Latitude, Longitude, Altitude) and 3D
 * metric Cartesian frames:
 *   - ECEF (Earth-Centered Earth-Fixed): Global (X, Y, Z) in meters.
 *   - ENU  (East-North-Up): Local (East, North, Up) in meters relative to an
 *          origin reference station (Lat0, Lon0, Alt0).
 *
 * Preserves sub-millimeter mathematical precision, fully supporting 1 cm
 * RTK Fixed GNSS positioning data parsed from NMEA sentences.
 *
 * Usage:
 * @code
 *   SYN_GeoPos base, rover;
 *   // ... initialize base and rover positions ...
 *   SYN_ENU local_enu;
 *   syn_geo_wgs84_to_enu(rover.latitude, rover.longitude, rover.altitude_m,
 *                        base.latitude, base.longitude, base.altitude_m,
 *                        &local_enu);
 *   // local_enu.east_m, local_enu.north_m, local_enu.up_m are in meters!
 * @endcode
 * @ingroup syn_util
 */

#ifndef SYN_GEO_H
#define SYN_GEO_H

#if __has_include("syn_config.h")
#include "syn_config.h"
#endif

#if !defined(SYN_USE_GEO) || SYN_USE_GEO

#include "../common/syn_defs.h"
#include "../proto/syn_nmea.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── WGS-84 Ellipsoid Constants ────────────────────────────────────────── */

/** WGS-84 Semi-major axis in meters (a). */
#define SYN_GEO_WGS84_A 6378137.0

/** WGS-84 Inverse flattening (1/f). */
#define SYN_GEO_WGS84_INV_F 298.257223563

/** WGS-84 First eccentricity squared (e^2 = 2f - f^2). */
#define SYN_GEO_WGS84_E2 0.0066943799901413165

/* ── Data Structures ───────────────────────────────────────────────────── */

/**
 * @brief Structured Geographic Position with accuracy bound and fix quality.
 */
typedef struct {
    double latitude;              /**< Decimal degrees (+N, -S)            */
    double longitude;             /**< Decimal degrees (+E, -W)            */
    double altitude_m;            /**< Altitude above MSL/ellipsoid (m)    */
    float accuracy_m;             /**< Estimated 3D position error bound   */
    SYN_NMEA_FixQuality fix_type; /**< GPS, DGPS, RTK_FIXED, RTK_FLOAT     */
    bool valid;                   /**< true if position data is valid      */
} SYN_GeoPos;

/**
 * @brief Local East-North-Up (ENU) 3D Cartesian coordinates in meters.
 */
typedef struct {
    double east_m;  /**< Local East offset in meters  */
    double north_m; /**< Local North offset in meters */
    double up_m;    /**< Local Up offset in meters    */
} SYN_ENU;

/* ── API ────────────────────────────────────────────────────────────────── */

/**
 * @brief Convert WGS-84 Geodetic coordinates to Earth-Centered Earth-Fixed (ECEF).
 *
 * @param lat_deg  Latitude in decimal degrees (-90 to +90).
 * @param lon_deg  Longitude in decimal degrees (-180 to +180).
 * @param alt_m    Altitude above ellipsoid/MSL in meters.
 * @param out_x    Output ECEF X coordinate in meters. Must not be NULL.
 * @param out_y    Output ECEF Y coordinate in meters. Must not be NULL.
 * @param out_z    Output ECEF Z coordinate in meters. Must not be NULL.
 * @return SYN_OK on success, SYN_INVALID_PARAM if outputs are NULL.
 */
SYN_Status syn_geo_wgs84_to_ecef(double lat_deg, double lon_deg, double alt_m, double *out_x,
                                 double *out_y, double *out_z);

/**
 * @brief Convert Earth-Centered Earth-Fixed (ECEF) coordinates back to WGS-84 Geodetic.
 *
 * Uses Bowring's closed-form algorithm for high precision.
 *
 * @param x        ECEF X coordinate in meters.
 * @param y        ECEF Y coordinate in meters.
 * @param z        ECEF Z coordinate in meters.
 * @param out_lat  Output Latitude in decimal degrees. Must not be NULL.
 * @param out_lon  Output Longitude in decimal degrees. Must not be NULL.
 * @param out_alt  Output Altitude in meters. Must not be NULL.
 * @return SYN_OK on success, SYN_INVALID_PARAM if outputs are NULL.
 */
SYN_Status syn_geo_ecef_to_wgs84(double x, double y, double z, double *out_lat, double *out_lon,
                                 double *out_alt);

/**
 * @brief Convert global ECEF coordinates to a Local East-North-Up (ENU) frame.
 *
 * Computes metric Cartesian offsets (East, North, Up) relative to a reference
 * station origin (ref_lat_deg, ref_lon_deg, ref_alt_m).
 *
 * @param x            Target ECEF X in meters.
 * @param y            Target ECEF Y in meters.
 * @param z            Target ECEF Z in meters.
 * @param ref_lat_deg  Reference origin Latitude in decimal degrees.
 * @param ref_lon_deg  Reference origin Longitude in decimal degrees.
 * @param ref_alt_m    Reference origin Altitude in meters.
 * @param out_enu      Output ENU Cartesian coordinates. Must not be NULL.
 * @return SYN_OK on success, SYN_INVALID_PARAM if out_enu is NULL.
 */
SYN_Status syn_geo_ecef_to_enu(double x, double y, double z, double ref_lat_deg, double ref_lon_deg,
                               double ref_alt_m, SYN_ENU *out_enu);

/**
 * @brief Direct conversion from WGS-84 Geodetic to Local East-North-Up (ENU).
 *
 * Convenience function combining WGS84->ECEF and ECEF->ENU.
 *
 * @param lat_deg      Target Latitude in decimal degrees.
 * @param lon_deg      Target Longitude in decimal degrees.
 * @param alt_m        Target Altitude in meters.
 * @param ref_lat_deg  Reference origin Latitude in decimal degrees.
 * @param ref_lon_deg  Reference origin Longitude in decimal degrees.
 * @param ref_alt_m    Reference origin Altitude in meters.
 * @param out_enu      Output ENU Cartesian coordinates in meters.
 * @return SYN_OK on success, SYN_INVALID_PARAM if out_enu is NULL.
 */
SYN_Status syn_geo_wgs84_to_enu(double lat_deg, double lon_deg, double alt_m, double ref_lat_deg,
                                double ref_lon_deg, double ref_alt_m, SYN_ENU *out_enu);

/**
 * @brief Compute 2D surface geodesic distance using the Haversine formula.
 *
 * @param lat1_deg  Point 1 Latitude in decimal degrees.
 * @param lon1_deg  Point 1 Longitude in decimal degrees.
 * @param lat2_deg  Point 2 Latitude in decimal degrees.
 * @param lon2_deg  Point 2 Longitude in decimal degrees.
 * @return 2D Surface distance in meters.
 */
double syn_geo_haversine_m(double lat1_deg, double lon1_deg, double lat2_deg, double lon2_deg);

/**
 * @brief Compute 3D Euclidean Cartesian distance between two ENU points.
 *
 * @param p1  Point 1 ENU coordinates in meters. Must not be NULL.
 * @param p2  Point 2 ENU coordinates in meters. Must not be NULL.
 * @return 3D Euclidean distance in meters.
 */
double syn_geo_3d_distance_m(const SYN_ENU *p1, const SYN_ENU *p2);

/**
 * @brief Populate a SYN_GeoPos structure from a parsed NMEA GGA sentence.
 *
 * Assigns automatic accuracy estimate based on fix quality:
 *   - RTK_FIXED:   0.01 m (1 cm)
 *   - RTK_FLOAT:   0.20 m (20 cm)
 *   - DGPS:        1.00 m
 *   - GPS:         2.50 m
 *   - INVALID/EST: 50.0 m
 *
 * @param gga      Parsed NMEA GGA sentence. Must not be NULL.
 * @param out_pos  Output SYN_GeoPos struct. Must not be NULL.
 * @return SYN_OK on success, SYN_INVALID_PARAM if args are NULL.
 */
SYN_Status syn_geo_pos_from_gga(const SYN_NMEA_GGA *gga, SYN_GeoPos *out_pos);

#ifdef __cplusplus
}
#endif

#endif /* SYN_USE_GEO */

#endif /* SYN_GEO_H */
