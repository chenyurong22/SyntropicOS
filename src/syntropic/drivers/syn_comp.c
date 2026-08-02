#if __has_include("syn_config.h")
#include "syn_config.h"
#endif

#if !defined(SYN_USE_COMP) || SYN_USE_COMP

/**
 * @file syn_comp.c
 * @brief High-Speed Rail-to-Rail Analog Comparator Driver implementation.
 */

#include "../util/syn_assert.h"
#include "syn_comp.h"

SYN_Status syn_comp_init(SYN_COMP *comp, uint8_t comp_id, SYN_COMP_InvertingInput inv_in)
{
    if (comp == NULL) {
        return SYN_INVALID_PARAM;
    }

    comp->comp_id = comp_id;
    comp->inv_in = inv_in;
    comp->enabled = false;

    return syn_port_comp_init(comp_id, inv_in);
}

bool syn_comp_read(const SYN_COMP *comp)
{
    if (comp == NULL) {
        return false;
    }

    return syn_port_comp_read(comp->comp_id);
}

SYN_Status syn_comp_enable(SYN_COMP *comp, bool enable)
{
    if (comp == NULL) {
        return SYN_INVALID_PARAM;
    }

    comp->enabled = enable;
    return syn_port_comp_enable(comp->comp_id, enable);
}

#endif /* SYN_USE_COMP */
