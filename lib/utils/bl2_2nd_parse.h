#ifndef __BL2_2ND_PARSE_H__
#define __BL2_2ND_PARSE_H__

#include "sys/types.h"

#define		MAGIC_2ND	(0x474d4943)    // "CIMG"
#define		CRC_TAG	(0x5a54)
#define		BOOT_FLAG_2ND	(uint32_t *)(0x0E000028)	//set 0 is success, other is fail

struct head {
	uint32_t magic;
	uint32_t version;
	uint32_t file_sz;
	uint32_t file_crc_value;
	uint32_t chunk_crc_offset;
	uint32_t chunk_crc_size;
	uint32_t chunk_crc_value;
	uint32_t part_offset;
	uint32_t part_size;
	uint8_t reserve[128-36];
};

void set_boot_flag_2nd(uint32_t flag);
int header_verify_2nd(void);
int verify_2nd(uint8_t *data);
void *get_head_2nd(void);
#endif /* __BL2_2ND_PARSE_H__ */
