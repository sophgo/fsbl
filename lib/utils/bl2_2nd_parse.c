#include "bl2_2nd_parse.h"
#include "crc16.h"
#include "debug.h"


struct head header_2nd;
void print_head_2nd(void)
{
	ERROR("HEAD:\n"
			"   magic: 0x%x\n"
			"   version: 0x%x\n"
			"   file_sz: 0x%x\n"
			"   file_crc_value: 0x%x\n"
			"   chunk_crc_offset: 0x%x\n"
			"   chunk_crc_size: 0x%x\n"
			"   chunk_crc_value: 0x%x\n"
			"   part_offset: 0x%x\n"
			"   part_size: 0x%x\n",
			header_2nd.magic, header_2nd.version, header_2nd.file_sz, header_2nd.file_crc_value,
			header_2nd.chunk_crc_offset, header_2nd.chunk_crc_size, header_2nd.chunk_crc_value,
			header_2nd.part_offset, header_2nd.part_size);
}

void set_boot_flag_2nd(uint32_t flag)
{
#ifdef MIRROR_2ND_PACK_HEAD
    *BOOT_FLAG_2ND = flag;
#endif
}

int header_verify_2nd(void)
{
	if ((header_2nd.magic != MAGIC_2ND) || (header_2nd.file_sz == 0)
		|| (header_2nd.part_size == 0)) {
        ERROR("mirror header is bad\n");
		return -1;
	}
	print_head_2nd();
	return 0;
}

int verify_2nd(uint8_t *data)
{
	uint32_t crc = 0;

	if (data == NULL) {
		return -1;
	}
	crc = crc16_ccitt(0, (void *)(data + header_2nd.chunk_crc_offset), header_2nd.chunk_crc_size);
	crc |= (CRC_TAG << 16);
	if (crc != header_2nd.chunk_crc_value) {
		ERROR("mirror crc error: chunk_crc_value=0x%x, crc=0x%x\n", header_2nd.chunk_crc_value, crc);
		return -1;
	}

	return 0;
}

// return NULL ----> without 128 byte head
// return &header_2nd ----> with 128 byte head
void *get_head_2nd(void)
{
#ifdef MIRROR_2ND_PACK_HEAD
	return (void *)(&header_2nd);
#else
	return NULL;
#endif
}
