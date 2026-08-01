/**
 * @file test_geo.c
 * @brief Unity tests for syn_geo — geodetic & 3D coordinate transformations.
 */

#include "mocks/mock_port.h"
#include "syntropic/syntropic.h"
#include "syntropic/util/syn_geo.h"
#include "unity/unity.h"

#include <math.h>

/* ── Test Suite ────────────────────────────────────────────────────────── */

static void test_wgs84_to_ecef_prime_meridian_equator(void)
{
    /* (0, 0, 0) -> ECEF (a, 0, 0) = (6378137.0, 0, 0) */
    double x, y, z;
    SYN_Status st = syn_geo_wgs84_to_ecef(0.0, 0.0, 0.0, &x, &y, &z);

    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_DOUBLE_WITHIN(1e-3, 6378137.0, x);
    TEST_ASSERT_DOUBLE_WITHIN(1e-3, 0.0, y);
    TEST_ASSERT_DOUBLE_WITHIN(1e-3, 0.0, z);
}

static void test_wgs84_to_ecef_north_pole(void)
{
    /* (90 N, 0, 0) -> ECEF (0, 0, b) where b = a * sqrt(1-e2) ~ 6356752.314 */
    double x, y, z;
    SYN_Status st = syn_geo_wgs84_to_ecef(90.0, 0.0, 0.0, &x, &y, &z);

    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_DOUBLE_WITHIN(1e-3, 0.0, x);
    TEST_ASSERT_DOUBLE_WITHIN(1e-3, 0.0, y);
    TEST_ASSERT_DOUBLE_WITHIN(1e-1, 6356752.314, z);
}

static void test_ecef_to_wgs84_round_trip(void)
{
    /* Round trip test for a known city coordinate (e.g. San Francisco ~ 37.7749 N, -122.4194 W, 30m
     * alt) */
    double orig_lat = 37.774929;
    double orig_lon = -122.419416;
    double orig_alt = 30.0;

    double x, y, z;
    syn_geo_wgs84_to_ecef(orig_lat, orig_lon, orig_alt, &x, &y, &z);

    double lat, lon, alt;
    SYN_Status st = syn_geo_ecef_to_wgs84(x, y, z, &lat, &lon, &alt);

    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_DOUBLE_WITHIN(1e-7, orig_lat, lat);
    TEST_ASSERT_DOUBLE_WITHIN(1e-7, orig_lon, lon);
    TEST_ASSERT_DOUBLE_WITHIN(1e-4, orig_alt, alt);
}

static void test_wgs84_to_enu_identity_at_origin(void)
{
    /* Converting reference origin to ENU should yield (0, 0, 0) */
    double ref_lat = 37.7749;
    double ref_lon = -122.4194;
    double ref_alt = 10.0;

    SYN_ENU enu;
    SYN_Status st =
        syn_geo_wgs84_to_enu(ref_lat, ref_lon, ref_alt, ref_lat, ref_lon, ref_alt, &enu);

    TEST_ASSERT_EQUAL(SYN_OK, st);
    TEST_ASSERT_DOUBLE_WITHIN(1e-4, 0.0, enu.east_m);
    TEST_ASSERT_DOUBLE_WITHIN(1e-4, 0.0, enu.north_m);
    TEST_ASSERT_DOUBLE_WITHIN(1e-4, 0.0, enu.up_m);
}

static void test_wgs84_to_enu_10m_north(void)
{
    /* Reference origin */
    double ref_lat = 37.774929;
    double ref_lon = -122.419416;
    double ref_alt = 0.0;

    /* 1 degree latitude ~ 111,139 meters. 10 meters north ~ +0.00008993 degrees */
    double target_lat = ref_lat + 0.000089932;

    SYN_ENU enu;
    syn_geo_wgs84_to_enu(target_lat, ref_lon, ref_alt, ref_lat, ref_lon, ref_alt, &enu);

    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, enu.east_m);
    TEST_ASSERT_DOUBLE_WITHIN(0.05, 10.0, enu.north_m);
    TEST_ASSERT_DOUBLE_WITHIN(0.05, 0.0, enu.up_m);
}

static void test_haversine_distance(void)
{
    /* Known baseline: London Heathrow (51.4700 N, -0.4543 W) to JFK Airport (40.6413 N, -73.7781 W)
     * ~ 5555 km */
    double dist_m = syn_geo_haversine_m(51.4700, -0.4543, 40.6413, -73.7781);
    TEST_ASSERT_DOUBLE_WITHIN(10000.0, 5555000.0, dist_m);
}

static void test_3d_distance(void)
{
    SYN_ENU p1 = {.east_m = 0.0, .north_m = 0.0, .up_m = 0.0};
    SYN_ENU p2 = {.east_m = 3.0, .north_m = 4.0, .up_m = 12.0};

    /* 3^2 + 4^2 + 12^2 = 9 + 16 + 144 = 169 -> sqrt = 13 */
    double dist = syn_geo_3d_distance_m(&p1, &p2);
    TEST_ASSERT_DOUBLE_WITHIN(1e-5, 13.0, dist);
}

static void test_pos_from_gga_fix_qualities(void)
{
    SYN_NMEA_GGA gga = {
        .latitude = 37.7749, .longitude = -122.4194, .altitude_m = 15.5f, .valid = true};
    SYN_GeoPos pos;

    /* RTK Fixed -> 0.01m */
    gga.fix_quality = SYN_NMEA_FIX_RTK;
    syn_geo_pos_from_gga(&gga, &pos);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.01f, pos.accuracy_m);

    /* RTK Float -> 0.20m */
    gga.fix_quality = SYN_NMEA_FIX_FLOAT_RTK;
    syn_geo_pos_from_gga(&gga, &pos);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.20f, pos.accuracy_m);

    /* DGPS -> 1.00m */
    gga.fix_quality = SYN_NMEA_FIX_DGPS;
    syn_geo_pos_from_gga(&gga, &pos);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.00f, pos.accuracy_m);

    /* GPS -> 2.50m */
    gga.fix_quality = SYN_NMEA_FIX_GPS;
    syn_geo_pos_from_gga(&gga, &pos);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 2.50f, pos.accuracy_m);

    /* Invalid / Default -> 50.0m */
    gga.fix_quality = SYN_NMEA_FIX_INVALID;
    syn_geo_pos_from_gga(&gga, &pos);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 50.00f, pos.accuracy_m);
}

static void test_geo_null_params(void)
{
    double val;

    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_geo_wgs84_to_ecef(0, 0, 0, NULL, &val, &val));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_geo_ecef_to_wgs84(0, 0, 0, NULL, &val, &val));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_geo_ecef_to_enu(0, 0, 0, 0, 0, 0, NULL));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_geo_wgs84_to_enu(0, 0, 0, 0, 0, 0, NULL));
    TEST_ASSERT_EQUAL(SYN_INVALID_PARAM, syn_geo_pos_from_gga(NULL, NULL));

    /* NULL 3D distance check (line 162) */
    TEST_ASSERT_EQUAL_DOUBLE(0.0, syn_geo_3d_distance_m(NULL, NULL));

    /* Polar region (p < 1e-12) ECEF to WGS84 coverage (lines 71-74) */
    double lat_p, lon_p, alt_p;
    SYN_Status st_p1 = syn_geo_ecef_to_wgs84(0.0, 0.0, 6356752.314, &lat_p, &lon_p, &alt_p);
    TEST_ASSERT_EQUAL(SYN_OK, st_p1);
    TEST_ASSERT_DOUBLE_WITHIN(1e-3, 90.0, lat_p);
    SYN_Status st_p2 = syn_geo_ecef_to_wgs84(0.0, 0.0, -6356752.314, &lat_p, &lon_p, &alt_p);
    TEST_ASSERT_EQUAL(SYN_OK, st_p2);
    TEST_ASSERT_DOUBLE_WITHIN(1e-3, -90.0, lat_p);

    /* Haversine boundary clamping (a > 1.0, a < 0.0) (lines 161, 163) */
    double dist_opposite = syn_geo_haversine_m(0.0, 0.0, 0.0, 180.0);
    TEST_ASSERT_DOUBLE_WITHIN(100.0, 20037508.34, dist_opposite);
    double dist_clamp1 = syn_geo_haversine_m(89.99999, 0.0, -89.99999, 180.0);
    TEST_ASSERT_TRUE(dist_clamp1 > 0.0);
    double dist_clamp2 = syn_geo_haversine_m(89.9999999999, 0.0, -89.9999999999, 180.0);
    TEST_ASSERT_TRUE(dist_clamp2 > 0.0);
}

void run_geo_tests(void)
{
    RUN_TEST(test_wgs84_to_ecef_prime_meridian_equator);
    RUN_TEST(test_wgs84_to_ecef_north_pole);
    RUN_TEST(test_ecef_to_wgs84_round_trip);
    RUN_TEST(test_wgs84_to_enu_identity_at_origin);
    RUN_TEST(test_wgs84_to_enu_10m_north);
    RUN_TEST(test_haversine_distance);
    RUN_TEST(test_3d_distance);
    RUN_TEST(test_pos_from_gga_fix_qualities);
    RUN_TEST(test_geo_null_params);
}
