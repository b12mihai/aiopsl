#ifndef _FSL_DPLIB_SYS_H
#define _FSL_DPLIB_SYS_H

#ifdef __linux__
#ifdef __uboot__

#define dmb()           __asm__ __volatile__ ("" : : : "memory")
#define __iormb()       dmb()
#define __iowmb()       dmb()
#define __arch_getq(a)                  (*(volatile unsigned long *)(a))
#define __arch_putq(v, a)                (*(volatile unsigned long *)(a) = (v))
#define readq(c)        ({ u64 __v = __arch_getq(c); __iormb(); __v; })
#define writeq(v, c)     ({ u64 __v = v; __iowmb(); __arch_putq(__v, c); __v; })
#include <common.h>
#include <errno.h>
#include <asm/io.h>

#else

#include <linux/errno.h>
#include <asm/io.h>

#endif /* __uboot__ */

#ifndef ENOTSUP
#define ENOTSUP		95
#endif

#define PTR_TO_UINT(_ptr)       ((uintptr_t)(_ptr))
#define PTR_MOVE(_ptr, _offset)	(void *)((uint8_t *)(_ptr) + (_offset))

#define ioread64(_p)	    readq(_p)
#define iowrite64(_v, _p)   writeq(_v, _p)

#else /* __linux__ */

#include "common/types.h"
#include "common/errors.h"
#include "common/io.h"


int dplib_send(void *regs,
	int auth,
	uint16_t cmd_id,
	uint16_t size,
	int pri,
	void *cmd_data);

static inline uint64_t virt_to_phys(void *vaddr)
{
	return (uint64_t)PTR_TO_UINT(vaddr);
}

#endif /* __linux__ */

#if (!defined(DECLARE_UINT_CODEC))

#define MAKE_UMASK64(_width) \
	((uint64_t)((_width) < 64 ? ((uint64_t)1 << (_width)) - 1 : -1))

static inline uint64_t u64_enc(int lsoffset, int width, uint64_t val)
{
	return (uint64_t)(((uint64_t)val & MAKE_UMASK64(width)) << lsoffset);
}
static inline uint64_t u64_dec(uint64_t val, int lsoffset, int width)
{
	return (uint64_t)((val >> lsoffset) & MAKE_UMASK64(width));
}

#endif

#endif /* _FSL_DPLIB_SYS_H */
