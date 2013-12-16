/**************************************************************************//**
 Copyright 2013 Freescale Semiconductor, Inc.

 @File          fsl_ldpaa.h

 @Description   This file contains LDPAA general definitions.
 *//***************************************************************************/
#ifndef __FSL_LDPAA_ARCH_H
#define __FSL_LDPAA_ARCH_H


/**************************************************************************//**
 @Group         ldpaa_g  LDPAA API

 @Description   TODO

 @{
 *//***************************************************************************/

/** FD FLC running sum */
#define FD_FLC_RUNNING_SUM	FD_FLC_DS_AS_CS_OFFSET + 2
	/** FD APP Index offset for DPNI driver */
#define FD_FLC_APPIDX_OFFSET	FD_FLC_DS_AS_CS_OFFSET + 1

/* TODO - FRC error code definitions */
#define FD_FRC_ERROR_MASK          0x00ff0000
#define FD_FRC_DPNI_MTU_ERROR_CODE 0x00010000

/**************************************************************************//**
 @Collection    LDPAA FD GETTER/SETTER MACROs

 @{
*//***************************************************************************/

/* FD fields Getters and Setters */
#undef LDPAA_FD_GET_ADDR
/**< Macro to get FD ADDRESS field.*/
/* Todo - 64/49 bit address. Phys to Virt? */
#undef LDPAA_FD_SET_ADDR
/**< Macro to set FD ADDRESS field */
/* Todo - 64/49 bit address. Phys to Virt? */
#undef LDPAA_FD_GET_LENGTH
/**< Macro to get FD LENGTH field */
#undef LDPAA_FD_SET_LENGTH
/**< Macro to set FD LENGTH field */
#undef LDPAA_FD_GET_MEM
/**< Macro to get FD MEM field */
#undef LDPAA_FD_SET_MEM
/**< Macro to set FD MEM field */
#undef LDPAA_FD_GET_BPID
/**< Macro to get FD BPID field */
#undef LDPAA_FD_SET_BPID
/**< Macro to set FD BPID field */
#undef LDPAA_FD_GET_IVP
/**< Macro to get FD IVP field */
#undef LDPAA_FD_SET_IVP
/**< Macro to set FD IVP field */
#undef LDPAA_FD_GET_BMT
/**< Macro to get FD BMT field */
#undef LDPAA_FD_SET_BMT
/**< Macro to set FD BMT field */
#undef LDPAA_FD_GET_OFFSET
/**< Macro to get FD OFFSET field */
#undef LDPAA_FD_SET_OFFSET
/**< Macro to set FD OFFSET field */
#undef LDPAA_FD_GET_FMT
/**< Macro to get FD FMT field */
#undef LDPAA_FD_SET_FMT
/**< Macro to set FD FMT field */
#undef LDPAA_FD_GET_SL
/**< Macro to get FD SL field */
#undef LDPAA_FD_SET_SL
/**< Macro to set FD SL field */
#undef LDPAA_FD_GET_FRC
/**< Macro to get FD FRC field */
#undef LDPAA_FD_SET_FRC
/**< Macro to set FD FRC field */
#undef LDPAA_FD_GET_ERR
/**< Macro to get FD ERR field */
#undef LDPAA_FD_SET_ERR
/**< Macro to set FD ERR field */
#undef LDPAA_FD_GET_VA
/**< Macro to get FD VA field */
#undef LDPAA_FD_SET_VA
/**< Macro to set FD VA field */
#undef LDPAA_FD_GET_CBMT
/**< Macro to get FD CBMT field */
#undef LDPAA_FD_SET_CBMT
/**< Macro to set FD CBMT field */
#undef LDPAA_FD_GET_ASAL
/**< Macro to get FD ASAL field */
#undef LDPAA_FD_SET_ASAL
/**< Macro to set FD ASAL field */
#undef LDPAA_FD_GET_PTV2
/**< Macro to get FD PTV2 field */
#undef LDPAA_FD_SET_PTV2
/**< Macro to set FD PTV2 field */
#undef LDPAA_FD_GET_PTV1
/**< Macro to get FD PTV1 field */
#undef LDPAA_FD_SET_PTV1
/**< Macro to set FD PTV1 field */
#undef LDPAA_FD_GET_PTA
/**< Macro to get FD PTA field */
#undef LDPAA_FD_SET_PTA
/**< Macro to set FD PTA field */
#undef LDPAA_FD_GET_DROPP
/**< Macro to get FD DROPP field */
#undef LDPAA_FD_SET_DROPP
/**< Macro to set FD DROPP field */
#undef LDPAA_FD_GET_SC
/**< Macro to get FD SC field */
#undef LDPAA_FD_SET_SC
/**< Macro to set FD SC field */
#undef LDPAA_FD_GET_DD
/**< Macro to get FD DD field */
#undef LDPAA_FD_SET_DD
/**< Macro to set FD DD field */
#undef LDPAA_FD_GET_CS
/**< Macro to get FD CS field */
#undef LDPAA_FD_SET_CS
/**< Macro to set FD CS field */
#undef LDPAA_FD_GET_AS
/**< Macro to get FD AS field */
#undef LDPAA_FD_SET_AS
/**< Macro to set FD AS field */
#undef LDPAA_FD_GET_DS
/**< Macro to get FD DS field */
#undef LDPAA_FD_SET_DS
/**< Macro to set FD DS field */
#undef LDPAA_FD_GET_FLC
/**< Macro to get FD FLC field */
#undef LDPAA_FD_SET_FLC
/**< Macro to set FD FLC field */


	/** Macro to get FD ADDRESS field.*/
	/* Todo - 64/49 bit address. Phys to Virt? */
#define LDPAA_FD_GET_ADDR(_fd)					\
	(uint64_t)({register uint64_t *__rR = 0;		\
	uint64_t addr;						\
	/*uint32_t addr1, addr2;*/					\
	/* Todo - use swap intrinsic with long long when supported (e_ldbrw) */\
	/*__ldwbrw(addr >> 32, addr, ((uint32_t)(((char *)_fd) +	\
			FD_ADDR_OFFSET)),0);*/				\
	/*__ldwbrw(addr1, addr2, ((uint32_t)(((char *)_fd) + FD_ADDR_OFFSET)),\
			0); \*						\
	__e_ldwbrw_d(addr, ((uint32_t)(((char *)_fd) + FD_ADDR_OFFSET)));\
	/*__ldwbrw(addr@hiword, addr@loword, ((uint32_t)(((char *)_fd) + \
		FD_ADDR_OFFSET)), 0);*/					\
	/*__rR = (uint64_t *) (((uint64_t)(addr1 << 32)) | addr2) ;*/	\
	__rR = (uint64_t *) (addr); })
	/** Macro to get FD LENGTH field */
#define LDPAA_FD_GET_LENGTH(_fd)					\
	(uint32_t)({register uint32_t *__rR = 0;			\
	uint32_t length = (LW_SWAP(((char *)_fd) + FD_MEM_LENGTH_OFFSET));\
	if (LDPAA_FD_GET_SL(_fd)) {					\
		length &= FD_LENGTH_MEM_MASK; }				\
	__rR = (uint32_t *) length; })
	/** Macro to get FD MEM field */
#define LDPAA_FD_GET_MEM(_fd)						\
	(uint32_t)(LW_SWAP_MASK_SHIFT(((char *)_fd) + FD_MEM_LENGTH_OFFSET,\
			FD_MEM_MASK, FD_MEM_SHIFT))
	/** Macro to get FD BPID field */
#define LDPAA_FD_GET_BPID(_fd)						\
	(LH_SWAP_MASK(((char *)_fd) + FD_BPID_OFFSET,			\
				FD_BPID_MASK))
	/** Macro to get FD IVP field */
#define LDPAA_FD_GET_IVP(_fd)						\
	(uint8_t)(uint32_t)({register uint8_t *__rR = 0;		\
	uint8_t ivp = *((uint8_t *) (((char *)_fd) + FD_BMP_IVP_OFFSET));\
	ivp &= FD_IVP_MASK;						\
	ivp >>= FD_IVP_SHIFT;						\
	__rR = (uint8_t *) ivp; })
	/** Macro to get FD BMT field */
#define LDPAA_FD_GET_BMT(_fd)						\
	(uint8_t)(uint32_t)({register uint8_t *__rR = 0;		\
	uint8_t bmt = *((uint8_t *) (((char *)_fd) + FD_BMP_IVP_OFFSET));\
	bmt &= FD_BMT_MASK;						\
	bmt >>= FD_BMT_SHIFT;						\
	__rR = (uint8_t *) bmt; })
	/** Macro to get FD OFFSET field */
#define LDPAA_FD_GET_OFFSET(_fd)					\
	(LH_SWAP_MASK((((char *)_fd) + FD_OFFSET_OFFSET),		\
			FD_OFFSET_MASK))
	/** Macro to get FD FMT field */
#define LDPAA_FD_GET_FMT(_fd)						\
	(uint8_t)(uint32_t)({register uint8_t *__rR = 0;		\
	uint8_t fmt = *((uint8_t *) (((char *)_fd) + FD_SL_FMT_OFFSET));\
	fmt &= FD_FMT_MASK;						\
	fmt >>= FD_FMT_SHIFT;						\
	__rR = (uint8_t *) fmt; })
	/** Macro to get FD SL field */
#define LDPAA_FD_GET_SL(_fd)						\
	(uint8_t)(uint32_t)({register uint8_t *__rR = 0;		\
	uint8_t sl = *((uint8_t *) (((char *)_fd) + FD_SL_FMT_OFFSET));\
	sl &= FD_SL_MASK;						\
	sl >>= FD_SL_SHIFT;						\
	__rR = (uint8_t *) sl; })
	/** Macro to get FD FRC field */
#define LDPAA_FD_GET_FRC(_fd)						\
	(uint32_t)({register uint32_t *__rR = 0;			\
	uint32_t frc = (LW_SWAP(((char *)_fd) + FD_FRC_OFFSET));	\
	__rR = (uint32_t *) frc; })
	/** Macro to get FD ERR field */
#define LDPAA_FD_GET_ERR(_fd)						\
	(uint8_t)(uint32_t)({register uint8_t *__rR = 0;		\
	uint8_t err = *((uint8_t *) (((char *)_fd) + FD_ERR_OFFSET));	\
	__rR = (uint8_t *) err; })
	/** Macro to get FD VA field */
#define LDPAA_FD_GET_VA(_fd)						\
	(uint8_t)(uint32_t)({register uint8_t *__rR = 0;		\
	uint8_t va = *((uint8_t *) (((char *)_fd) + FD_CBMT_VA_OFFSET));\
	va &= FD_VA_MASK;						\
	va >>= FD_VA_SHIFT;						\
	__rR = (uint8_t *) va; })
	/** Macro to get FD CBMT field */
#define LDPAA_FD_GET_CBMT(_fd)						\
	(uint8_t)(uint32_t)({register uint8_t *__rR = 0;		\
	uint8_t cbmt = *((uint8_t *) (((char *)_fd) + FD_CBMT_VA_OFFSET));\
	cbmt &= FD_CBMT_MASK;						\
	cbmt >>= FD_CBMT_SHIFT;						\
	__rR = (uint8_t *) cbmt; })
	/** Macro to get FD ASAL field */
#define LDPAA_FD_GET_ASAL(_fd)						\
	(uint8_t)(uint32_t)({register uint8_t *__rR = 0;		\
	uint8_t asal = *((uint8_t *) (((char *)_fd) + FD_PTA_PVT_ASA_OFFSET));\
	asal &= FD_ASAL_MASK;						\
	__rR = (uint8_t *) asal; })
	/** Macro to get FD PTV2 field */
#define LDPAA_FD_GET_PTV2(_fd)						\
	(uint8_t)(uint32_t)({register uint8_t *__rR = 0;		\
	uint8_t ptv2 = *((uint8_t *) (((char *)_fd) + FD_PTA_PVT_ASA_OFFSET));\
	ptv2 &= FD_PTV2_MASK;						\
	ptv2 >>= FD_PTV2_SHIFT;						\
	__rR = (uint8_t *) ptv2; })
	/** Macro to get FD PTV1 field */
#define LDPAA_FD_GET_PTV1(_fd)						\
	(uint8_t)(uint32_t)({register uint8_t *__rR = 0;		\
	uint8_t ptv1 = *((uint8_t *) (((char *)_fd) + FD_PTA_PVT_ASA_OFFSET));\
	ptv1 &= FD_PTV1_MASK;						\
	ptv1 >>= FD_PTV1_SHIFT;						\
	__rR = (uint8_t *) ptv1; })
	/** Macro to get FD PTA field */
#define LDPAA_FD_GET_PTA(_fd)						\
	(uint8_t)(uint32_t)({register uint8_t *__rR = 0;		\
	uint8_t pta = *((uint8_t *) (((char *)_fd) + FD_PTA_PVT_ASA_OFFSET));\
	pta &= FD_PTA_MASK;						\
	pta >>= FD_PTA_SHIFT;						\
	__rR = (uint8_t *) pta; })
	/** Macro to get FD DROPP field */
#define LDPAA_FD_GET_DROPP(_fd)						\
	(uint8_t)(uint32_t)({register uint8_t *__rR = 0;		\
	uint8_t dropp = *((uint8_t *) (((char *)_fd) + FD_DD_SC_DROPP_OFFSET));\
	dropp &= FD_DROPP_MASK;						\
	__rR = (uint8_t *) dropp; })
	/** Macro to get FD SC field */
#define LDPAA_FD_GET_SC(_fd)						\
	(uint8_t)(uint32_t)({register uint8_t *__rR = 0;		\
	uint8_t sc = *((uint8_t *) (((char *)_fd) + FD_DD_SC_DROPP_OFFSET));\
	sc &= FD_SC_MASK;						\
	sc >>= FD_SC_SHIFT;						\
	__rR = (uint8_t *) sc; })
	/** Macro to get FD DD field */
#define LDPAA_FD_GET_DD(_fd)						\
	(uint8_t)(uint32_t)({register uint8_t *__rR = 0;		\
	uint8_t dd = *((uint8_t *) (((char *)_fd) + FD_DD_SC_DROPP_OFFSET));\
	dd &= FD_DD_MASK;						\
	dd >>= FD_DD_SHIFT;						\
	__rR = (uint8_t *) dd; })
	/** Macro to get FD CS field */
#define LDPAA_FD_GET_CS(_fd)						\
	(uint8_t)(uint32_t)({register uint8_t *__rR = 0;		\
	uint8_t cs = *((uint8_t *) (((char *)_fd) + FD_FLC_DS_AS_CS_OFFSET));\
	cs &= FD_CS_MASK;						\
	__rR = (uint8_t *) cs; })
	/** Macro to get FD AS field */
#define LDPAA_FD_GET_AS(_fd)						\
	(uint8_t)(uint32_t)({register uint8_t *__rR = 0;		\
	uint8_t as = *((uint8_t *) (((char *)_fd) + FD_FLC_DS_AS_CS_OFFSET));\
	as &= FD_AS_MASK;						\
	as >>= FD_AS_SHIFT;						\
	__rR = (uint8_t *) as; })
	/** Macro to get FD DS field */
#define LDPAA_FD_GET_DS(_fd)						\
	(uint8_t)(uint32_t)({register uint8_t *__rR = 0;		\
	uint8_t ds = *((uint8_t *) (((char *)_fd) + FD_FLC_DS_AS_CS_OFFSET));\
	ds &= FD_DS_MASK;						\
	ds >>= FD_DS_SHIFT;						\
	__rR = (uint8_t *) ds; })
	/** Macro to get FD FLC field */
	/* Todo - load flc to a 64bit _val - use swap intrinsic with long long
	 * once supported (e_ldbrw) */
#define LDPAA_FD_GET_FLC(_fd)


	/** Macro to set FD ADDRESS field */
	/* Todo - 64/49 bit address. Phys to Virt? */
#define LDPAA_FD_SET_ADDR(_fd, _val)
	/** Macro to set FD LENGTH field */
#define LDPAA_FD_SET_LENGTH(_fd, _val)					\
	({uint32_t length = (uint32_t)					\
		(LW_SWAP(((char *)_fd) + FD_MEM_LENGTH_OFFSET));	\
	if (LDPAA_FD_GET_SL(_fd)) {					\
		length &= FD_MEM_MASK;					\
		__e_rlwimi(length, _val, 0, 14, 31);			\
		STW_SWAP(length, FD_MEM_LENGTH_OFFSET);			\
	} else								\
		STW_SWAP(_val, FD_MEM_LENGTH_OFFSET);			\
	})
	/** Macro to set FD MEM field */
#define LDPAA_FD_SET_MEM(_fd, _val)
	/** Macro to set FD BPID field */
#define LDPAA_FD_SET_BPID(_fd, _val)
	/** Macro to set FD IVP field */
#define LDPAA_FD_SET_IVP(_fd, _val)
	/** Macro to set FD BMT field */
#define LDPAA_FD_SET_BMT(_fd, _val)
	/** Macro to set FD OFFSET field */
#define LDPAA_FD_SET_OFFSET(_fd, _val)
	/** Macro to set FD FMT field */
#define LDPAA_FD_SET_FMT(_fd, _val)
	/** Macro to set FD SL field */
#define LDPAA_FD_SET_SL(_fd, _val)
	/** Macro to set FD FRC field */
#define LDPAA_FD_SET_FRC(_fd, _val)					\
	STW_SWAP(_val, FD_FRC_OFFSET);
	/** Macro to set FD ERR field */
#define LDPAA_FD_SET_ERR(_fd, _val)
	/** Macro to set FD VA field */
#define LDPAA_FD_SET_VA(_fd, _val)
	/** Macro to set FD ASAL field */
#define LDPAA_FD_SET_ASAL(_fd, _val)					\
	((*((uint8_t *)(((char *)_fd) + FD_PTA_PVT_ASA_OFFSET))) =	\
	((*((uint8_t *)(((char *)_fd) + FD_PTA_PVT_ASA_OFFSET)) &	\
		~FD_ASAL_MASK) | (_val & FD_ASAL_MASK)))
	/** Macro to set FD CBMT field */
#define LDPAA_FD_SET_CBMT(_fd, _val)
	/** Macro to set FD PTV2 field */
#define LDPAA_FD_SET_PTV2(_fd, _val)					\
	((*((uint8_t *)(((char *)_fd) + FD_PTA_PVT_ASA_OFFSET))) =	\
	((*((uint8_t *)(((char *)_fd) + FD_PTA_PVT_ASA_OFFSET)) &	\
			~FD_PTV2_MASK) | (_val & FD_PTV2_MASK)))
	/** Macro to set FD PTV1 field */
#define LDPAA_FD_SET_PTV1(_fd, _val)					\
	((*((uint8_t *)(((char *)_fd) + FD_PTA_PVT_ASA_OFFSET))) =	\
	((*((uint8_t *)(((char *)_fd) + FD_PTA_PVT_ASA_OFFSET)) &	\
			~FD_PTV1_MASK) | (_val & FD_PTV1_MASK)))
	/** Macro to set FD PTA field */
#define LDPAA_FD_SET_PTA(_fd, _val)					\
	((*((uint8_t *)(((char *)_fd) + FD_PTA_PVT_ASA_OFFSET))) =	\
	((*((uint8_t *)(((char *)_fd) + FD_PTA_PVT_ASA_OFFSET)) &	\
			~FD_PTA_MASK) | (_val & FD_PTA_MASK)))
	/** Macro to set FD DROPP field */
#define LDPAA_FD_SET_DROPP(_fd, _val)
	/** Macro to set FD SC field */
#define LDPAA_FD_SET_SC(_fd, _val)
	/** Macro to set FD DD field */
#define LDPAA_FD_SET_DD(_fd, _val)
	/** Macro to set FD CS field */
#define LDPAA_FD_SET_CS(_fd, _val)
	/** Macro to set FD AS field */
#define LDPAA_FD_SET_AS(_fd, _val)
	/** Macro to set FD DS field */
#define LDPAA_FD_SET_DS(_fd, _val)
	/** Macro to set FD FLC field */
	/* Todo - set flc with a 64bit _val - use swap intrinsic with long long
	 * once supported (e_stdbrw) */
#define LDPAA_FD_SET_FLC(_fd, _val)

/* Additional FD Macros */
	/** Macro to update FD LENGTH */
#define LDPAA_FD_UPDATE_LENGTH(_fd, _from_size, _to_size)		\
	({uint32_t length;						\
	__lwbrx(length, ((char *)_fd) + FD_MEM_LENGTH_OFFSET);		\
	if (_from_size >= _to_size)					\
		length += (_from_size - _to_size);			\
	else								\
		length -= (_to_size - _from_size);			\
	STW_SWAP(length, ((char *)_fd) + FD_MEM_LENGTH_OFFSET); })

/* @} *//* end of LDPAA FD MACROs */

/** @} *//* end of ldpaa_g group */


#endif /* __FSL_LDPAA_H */