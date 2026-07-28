/**
 * @file syn_transport_udp.h
 * @brief Bridge binding syn_port_udp_* API to native syn_udp stack.
 * @ingroup syn_net
 */

#ifndef SYN_TRANSPORT_UDP_H
#define SYN_TRANSPORT_UDP_H

#include "syntropic/net/syn_udp.h"
#include "syntropic/port/syn_port_socket.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bind native syn_udp instance to the syn_port_udp_* socket interface.
 *
 * @param udp Pointer to native SYN_UDP stack instance.
 */
void syn_transport_udp_set_instance(SYN_UDP *udp);

/**
 * @brief Get the active native SYN_UDP stack instance.
 * @return Pointer to SYN_UDP instance, or NULL.
 */
SYN_UDP *syn_transport_udp_get_instance(void);

#ifdef __cplusplus
}
#endif

#endif /* SYN_TRANSPORT_UDP_H */
