/**
 * @file syn_transform.h
 * @brief Fixed-point Q16.16 Coordinate System Transformations (Polar, Spherical, Cartesian).
 * @ingroup syn_util
 */

#ifndef SYN_TRANSFORM_H
#define SYN_TRANSFORM_H

#include "../common/syn_defs.h"
#include "syn_qmath.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert 2D Cartesian coordinates (x, y) to Polar coordinates (r, theta).
 * @param x      X coordinate in Q16.
 * @param y      Y coordinate in Q16.
 * @param r      Output radius r in Q16.
 * @param theta  Output angle theta in Q16 radians [-pi, pi].
 */
void syn_cart2pol(q16_t x, q16_t y, q16_t *r, q16_t *theta);

/**
 * @brief Convert 2D Polar coordinates (r, theta) to Cartesian coordinates (x, y).
 * @param r      Radius r in Q16.
 * @param theta  Angle theta in Q16 radians.
 * @param x      Output X coordinate in Q16.
 * @param y      Output Y coordinate in Q16.
 */
void syn_pol2cart(q16_t r, q16_t theta, q16_t *x, q16_t *y);

/**
 * @brief Convert 3D Cartesian coordinates (x, y, z) to Spherical coordinates (r, theta, phi).
 * @param x      X coordinate in Q16.
 * @param y      Y coordinate in Q16.
 * @param z      Z coordinate in Q16.
 * @param r      Output radius r in Q16.
 * @param theta  Output azimuth angle theta in Q16 radians [-pi, pi].
 * @param phi    Output polar angle (inclination) phi in Q16 radians [0, pi].
 */
void syn_cart2sph(q16_t x, q16_t y, q16_t z, q16_t *r, q16_t *theta, q16_t *phi);

/**
 * @brief Convert 3D Spherical coordinates (r, theta, phi) to Cartesian coordinates (x, y, z).
 * @param r      Radius r in Q16.
 * @param theta  Azimuth angle theta in Q16 radians.
 * @param phi    Polar angle (inclination) phi in Q16 radians.
 * @param x      Output X coordinate in Q16.
 * @param y      Output Y coordinate in Q16.
 * @param z      Output Z coordinate in Q16.
 */
void syn_sph2cart(q16_t r, q16_t theta, q16_t phi, q16_t *x, q16_t *y, q16_t *z);

#ifdef __cplusplus
}
#endif

#endif /* SYN_TRANSFORM_H */
