// SPDX-License-Identifier: GPL-2.0-only
/*
 * SPI testing utility (using spidev driver)
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/ioctl.h>
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#define GB02MAC2422 0x4000 /* receive with 8 wires */
#define GB02MAC2420 0x2000 /* transmit with 8 wires */
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static void pabort(const char *s)
{
	if (errno != 0)
		perror(s);
	else
		printf("%s\n", s);

	abort();
}

static const char *device = "/dev/spidev0.0";
static uint32_t	   mode;
static uint8_t	   bits = 8;
static char		*input_file;
static char		*output_file;
static uint32_t	   speed = 16600000;
static uint16_t	   delay;
static int		   verbose;
static int		   transfer_size;
static int		   iterations;
static int		   interval = 5; /* interval in seconds for showing transfer rate */

static uint8_t default_tx[] = {
	0x02,
	0x02,
	0x55,
	0xaa,
};

static uint8_t default_rx[ARRAY_SIZE(default_tx)] = {
	0,
};
static char *input_tx;

static void GB02FUNC1672(const void *src, size_t length, size_t line_size, char *prefix)
{
	int					 i		 = 0;
	const unsigned char *address = src;
	const unsigned char *line	 = address;
	unsigned char		 c;

	printf("%s | ", prefix);
	while (length-- > 0) {
		printf("%02X ", *address++);
		if (!(++i % line_size) || (length == 0 && i % line_size)) {
			if (length == 0) {
				while (i++ % line_size)
					printf("__ ");
			}
			printf(" |");
			while (line < address) {
				c = *line++;
				printf("%c", (c < 32 || c > 126) ? '.' : c);
			}
			printf("|\n");
			if (length > 0)
				printf("%s | ", prefix);
		}
	}
}

/*
 *  Unescape - process hexadecimal escape character
 *      converts shell input "\x23" -> 0x23
 */
static int unescape(char *_dst, char *_src, size_t len)
{
	int			 ret = 0;
	int			 match;
	char		 *src = _src;
	char		 *dst = _dst;
	unsigned int ch;

	while (*src) {
		if (*src == '\\' && *(src + 1) == 'x') {
			match = sscanf(src + 2, "%2x", &ch);
			if (!match)
				pabort("malformed input string");

			src += 4;
			*dst++ = (unsigned char)ch;
		} else {
			*dst++ = *src++;
		}
		ret++;
	}
	return ret;
}

static void transfer(int fd, uint8_t const *tx, uint8_t const *rx, size_t len)
{
	int						ret;
	int						out_fd;
	struct GB02STR183 tr = {
		.tx_buf		   = (unsigned long)tx,
		.rx_buf		   = (unsigned long)rx,
		.len		   = len,
		.delay_usecs   = delay,
		.speed_hz	   = speed,
		.bits_per_word = bits,
	};

	if (mode & GB02MAC2420)
		tr.tx_nbits = 8;
	else if (mode & GB02MAC2412)
		tr.tx_nbits = 4;
	else if (mode & GB02MAC2410)
		tr.tx_nbits = 2;
	if (mode & GB02MAC2422)
		tr.rx_nbits = 8;
	else if (mode & GB02MAC2416)
		tr.rx_nbits = 4;
	else if (mode & GB02MAC2414)
		tr.rx_nbits = 2;
	if (!(mode & GB02MAC2404)) {
		if (mode & (GB02MAC2420 | GB02MAC2412 | GB02MAC2410))
			tr.rx_buf = 0;
		else if (mode & (GB02MAC2422 | GB02MAC2416 | GB02MAC2414))
			tr.tx_buf = 0;
	}

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");

	if (verbose)
		GB02FUNC1672(tx, len, 32, "TX");

	if (output_file) {
		out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (out_fd < 0)
			pabort("could not open output file");

		ret = write(out_fd, rx, len);
		if (ret != len)
			pabort("not all bytes written to output file");

		close(out_fd);
	}

	if (verbose)
		GB02FUNC1672(rx, len, 32, "RX");
}

static void GB02FUNC1548(const char *prog)
{
	printf("Usage: %s [-DsbdlHOLC3vpNR24SI]\n", prog);
	puts("  -D --device   device to use (default /dev/spidev0.0)\n"
	     "  -s --speed    max speed (Hz)\n"
	     "  -d --delay    delay (usec)\n"
	     "  -b --bpw      bits per word\n"
	     "  -i --input    input data from a file (e.g. \"test.bin\")\n"
	     "  -o --output   output data to a file (e.g. \"results.bin\")\n"
	     "  -l --loop     loopback\n"
	     "  -H --cpha     clock phase\n"
	     "  -O --cpol     clock polarity\n"
	     "  -L --lsb      least significant bit first\n"
	     "  -C --cs-high  chip select active high\n"
	     "  -3 --3wire    SI/SO signals shared\n"
	     "  -v --verbose  Verbose (show tx buffer)\n"
	     "  -p            Send data (e.g. \"1234\\xde\\xad\")\n"
	     "  -N --no-cs    no chip select\n"
	     "  -R --ready    slave pulls low to pause\n"
	     "  -2 --dual     dual transfer\n"
	     "  -4 --quad     quad transfer\n"
	     "  -8 --octal    octal transfer\n"
	     "  -S --size     transfer size\n"
	     "  -I --iter     iterations\n");
	exit(1);
}

static void GB02FUNC1551(int argc, char *argv[])
{
	while (1) {
		static const struct option lopts[] = {
			{"device", 1, 0, 'D'},	{"speed", 1, 0, 's'},	{"delay", 1, 0, 'd'},
			{"bpw", 1, 0, 'b'},		{"input", 1, 0, 'i'},	{"output", 1, 0, 'o'},
			{"loop", 0, 0, 'l'},	{"cpha", 0, 0, 'H'},	{"cpol", 0, 0, 'O'},
			{"lsb", 0, 0, 'L'},		{"cs-high", 0, 0, 'C'}, {"3wire", 0, 0, '3'},
			{"no-cs", 0, 0, 'N'},	{"ready", 0, 0, 'R'},	{"dual", 0, 0, '2'},
			{"verbose", 0, 0, 'v'}, {"quad", 0, 0, '4'},	{"octal", 0, 0, '8'},
			{"size", 1, 0, 'S'},	{"iter", 1, 0, 'I'},	{NULL, 0, 0, 0},
		};
		int c;

		c = getopt_long(argc, argv, "D:s:d:b:i:o:lHOLC3NR248p:vS:I:", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'D':
			device = optarg;
			break;
		case 's':
			speed = atoi(optarg);
			break;
		case 'd':
			delay = atoi(optarg);
			break;
		case 'b':
			bits = atoi(optarg);
			break;
		case 'i':
			input_file = optarg;
			break;
		case 'o':
			output_file = optarg;
			break;
		case 'l':
			mode |= GB02MAC2404;
			break;
		case 'H':
			mode |= GB02MAC2386;
			break;
		case 'O':
			mode |= GB02MAC2388;
			break;
		case 'L':
			mode |= GB02MAC2400;
			break;
		case 'C':
			mode |= GB02MAC2398;
			break;
		case '3':
			mode |= GB02MAC2402;
			break;
		case 'N':
			mode |= GB02MAC2406;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'R':
			mode |= GB02MAC2408;
			break;
		case 'p':
			input_tx = optarg;
			break;
		case '2':
			mode |= GB02MAC2410;
			break;
		case '4':
			mode |= GB02MAC2412;
			break;
		case '8':
			mode |= GB02MAC2420;
			break;
		case 'S':
			transfer_size = atoi(optarg);
			break;
		case 'I':
			iterations = atoi(optarg);
			break;
		default:
			GB02FUNC1548(argv[0]);
		}
	}
	if (mode & GB02MAC2404) {
		if (mode & GB02MAC2410)
			mode |= GB02MAC2414;
		if (mode & GB02MAC2412)
			mode |= GB02MAC2416;
		if (mode & GB02MAC2420)
			mode |= GB02MAC2422;
	}
}

static void GB02FUNC1681(int fd, char *str)
{
	size_t	 size = strlen(str);
	uint8_t *tx;
	uint8_t *rx;

	tx = malloc(size);
	if (!tx)
		pabort("can't allocate tx buffer");

	rx = malloc(size);
	if (!rx)
		pabort("can't allocate rx buffer");

	size = unescape((char *)tx, str, size);
	transfer(fd, tx, rx, size);
	free(rx);
	free(tx);
}

static void GB02FUNC1683(int fd, char *filename)
{
	ssize_t		bytes;
	struct stat sb;
	int			tx_fd;
	uint8_t	*tx;
	uint8_t	*rx;

	if (stat(filename, &sb) == -1)
		pabort("can't stat input file");

	tx_fd = open(filename, O_RDONLY);
	if (tx_fd < 0)
		pabort("can't open input file");

	tx = malloc(sb.st_size);
	if (!tx)
		pabort("can't allocate tx buffer");

	rx = malloc(sb.st_size);
	if (!rx)
		pabort("can't allocate rx buffer");

	bytes = read(tx_fd, tx, sb.st_size);
	if (bytes != sb.st_size)
		pabort("failed to read input file");

	transfer(fd, tx, rx, sb.st_size);
	free(rx);
	free(tx);
	close(tx_fd);
}

static uint64_t _read_count;
static uint64_t _write_count;

static void GB02FUNC1685(void)
{
	static uint64_t prev_read_count, prev_write_count;
	double			rx_rate, tx_rate;

	rx_rate = ((_read_count - prev_read_count) * 8) / (interval * 1000.0);
	tx_rate = ((_write_count - prev_write_count) * 8) / (interval * 1000.0);

	printf("rate: tx %.1fkbps, rx %.1fkbps\n", rx_rate, tx_rate);

	prev_read_count	 = _read_count;
	prev_write_count = _write_count;
}

static void GB02FUNC1687(int fd, int len)
{
	uint8_t *tx;
	uint8_t *rx;
	int		 i;

	tx = malloc(len);
	if (!tx)
		pabort("can't allocate tx buffer");
	for (i = 0; i < len; i++)
		tx[i] = random();

	rx = malloc(len);
	if (!rx)
		pabort("can't allocate rx buffer");

	transfer(fd, tx, rx, len);

	_write_count += len;
	_read_count += len;

	if (mode & GB02MAC2404) {
		if (memcmp(tx, rx, len)) {
			fprintf(stderr, "transfer error !\n");
			GB02FUNC1672(tx, len, 32, "TX");
			GB02FUNC1672(rx, len, 32, "RX");
			exit(1);
		}
	}

	free(rx);
	free(tx);
}

int main(int argc, char *argv[])
{
	int		 ret = 0;
	uint32_t request;
	int fd;

	GB02FUNC1551(argc, argv);

	if (input_tx && input_file)
		pabort("only one of -p and --input may be selected");

	fd = open(device, O_RDWR);
	if (fd < 0)
		pabort("can't open device");

	/*
	 * spi mode
	 */
	/* WR is make a request to assign 'mode' */
	request = mode;
	ret		= ioctl(fd, SPI_IOC_WR_MODE32, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	/* RD is read what mode the device actually is in */
	ret = ioctl(fd, SPI_IOC_RD_MODE32, &mode);
	if (ret == -1)
		pabort("can't get spi mode");
	/* Drivers can reject some mode bits without returning an error.
	 * Read the current value to identify what mode it is in, and if it
	 * differs from the requested mode, warn the user.
	 */
	if (request != mode)
		printf("WARNING device does not support requested mode 0x%x\n", request);

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");

	printf("spi mode: 0x%x\n", mode);
	printf("bits per word: %u\n", bits);
	printf("max speed: %u Hz (%u kHz)\n", speed, speed / 1000);

	if (input_tx)
		GB02FUNC1681(fd, input_tx);
	else if (input_file)
		GB02FUNC1683(fd, input_file);
	else if (transfer_size) {
		struct timespec last_stat;

		clock_gettime(CLOCK_MONOTONIC, &last_stat);

		while (iterations-- > 0) {
			struct timespec current;

			GB02FUNC1687(fd, transfer_size);

			clock_gettime(CLOCK_MONOTONIC, &current);
			if (current.tv_sec - last_stat.tv_sec > interval) {
				GB02FUNC1685();
				last_stat = current;
			}
		}
		printf("total: tx %.1fKB, rx %.1fKB\n", _write_count / 1024.0, _read_count / 1024.0);
	} else
		transfer(fd, default_tx, default_rx, sizeof(default_tx));

	close(fd);

	return ret;
}
