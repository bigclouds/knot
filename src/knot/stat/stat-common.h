/*!
 * \file stat-common.h
 *
 * \author Lubos Slovak <lubos.slovak@nic.cz>
 *
 * \brief Common macros for stat.
 *
 * \addtogroup statistics
 * @{
 */

#ifndef _KNOTD_STAT_COMMON_H_
#define _KNOTD_STAT_COMMON_H_

#include <stdio.h>

//#define STAT_COMPILE
#define ST_DEBUG

#define ERR_ALLOC_FAILED fprintf(stderr, "Allocation failed at %s:%d\n", \
				  __FILE__, __LINE__)

#ifdef ST_DEBUG
#define dbg_st(msg...) fprintf(stderr, msg)
#else
#define dbg_st(msg...)
#endif

#endif /* _KNOTD_STAT_COMMON_H_ */

/*! @} */
