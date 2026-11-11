#ifndef __SPI_MEM_H
#define __SPI_MEM_H


#define SPI_MEM_OP_CMD(__opcode, __buswidth)                    \
        {                                                       \
                .buswidth = __buswidth,                         \
                .opcode = __opcode,                             \
                .nbytes = 1,                                    \
        }

#define SPI_MEM_OP_ADDR(__nbytes, __val, __buswidth)            \
        {                                                       \
                .nbytes = __nbytes,                             \
                .val = __val,                                   \
                .buswidth = __buswidth,                         \
        }

#define SPI_MEM_OP_NO_ADDR      { 0 }

#define SPI_MEM_OP_DUMMY(__nbytes, __buswidth)                  \
        {                                                       \
                .nbytes = __nbytes,                             \
                .buswidth = __buswidth,                         \
        }

#define SPI_MEM_OP_NO_DUMMY     { 0 }

#define SPI_MEM_OP_DATA_IN(__nbytes, __buf, __buswidth)         \
        {                                                       \
                .dir = SPI_MEM_DATA_IN,                         \
                .nbytes = __nbytes,                             \
                .buf.in = __buf,                                \
                .buswidth = __buswidth,                         \
        }

#define SPI_MEM_OP_DATA_OUT(__nbytes, __buf, __buswidth)        \
        {                                                       \
                .dir = SPI_MEM_DATA_OUT,                        \
                .nbytes = __nbytes,                             \
                .buf.out = __buf,                               \
                .buswidth = __buswidth,                         \
        }

#define SPI_MEM_OP_NO_DATA      { 0 }

/**
 * enum spi_mem_data_dir - describes the direction of a SPI memory data
 *                         transfer from the controller perspective
 * @SPI_MEM_NO_DATA: no data transferred
 * @SPI_MEM_DATA_IN: data coming from the SPI memory
 * @SPI_MEM_DATA_OUT: data sent to the SPI memory
 */
enum spi_mem_data_dir {
        SPI_MEM_NO_DATA,
        SPI_MEM_DATA_IN,
        SPI_MEM_DATA_OUT,
};

struct spi_mem_op {
        struct {
                uint8_t nbytes;
                uint8_t buswidth;
                uint8_t dtr : 1;
                uint16_t opcode;
        } cmd;

        struct {
                uint8_t nbytes;
                uint8_t buswidth;
                uint8_t dtr : 1;
                uint64_t val;
        } addr;

        struct {
                uint8_t nbytes;
                uint8_t buswidth;
                uint8_t dtr : 1;
        } dummy;

        struct {
                uint8_t buswidth;
                uint8_t dtr : 1;
                enum spi_mem_data_dir dir;
                unsigned int nbytes;
                union {
                        void *in;
                        void *out;
                } buf;
        } data;
};

#define SPI_MEM_OP(__cmd, __addr, __dummy, __data)          \
        {                                                       \
                .cmd = __cmd,                                   \
                .addr = __addr,                                 \
                .dummy = __dummy,                               \
                .data = __data,                                 \
        }

struct spi_clk {
        uint32_t in;
        uint32_t out;
};

int spi_exec_mem_op(struct spi_clk *clk, const struct spi_mem_op *op);

#endif
