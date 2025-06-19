#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <openssl/md5.h>
#include "sm2_sign_and_verify.h"
#include "sm2_create_key_pair.h"

#define GB02MAC1025 'U'  
#define UPGRADE_MCU_CMD_START _IOW(GB02MAC1025, 1u, int)
#define UPGRADE_MCU_CMD_GET_PROGRESS _IOW(GB02MAC1025, 2u, int)
#define UPGRADE_MCU_CMD_SET_IMAGE_INFO _IOW(GB02MAC1025, 3u, int)

//#define GB02_UPGRADE_MCU_SUB_IMAGE_OFFSET (0xB0000000 )
#define GB02MAC1026 (1024 * 4)    /* 4KB */
#define APP_FILE_NAME ("upgrade_app.bin")
#define UEFI_FIRMWARE_NAME 	("upgrade_uefi.bin")
#define VBIOS_VERSION_PROC_FILE_PATH "cat /proc/gb/version"
#define VRAM_SIZE_PROC_FILE_PATH "/proc/gbgpuinfo"

//#define PUBLIC_KEY_FILE_NAME	"sietium_key.pub"
//#define USER_ID "sietium"
#define GB02MAC2040	100
#define GB02MAC2041 65
#define GB02MAC2042  20

#define KB (1UL<<10)
#define MB (1UL<<20)
#define GB (1UL<<30)

/* Reserved for MCU */
#define GB02MAC489             (2*MB)
/* Reserved for AUDIO */
#define GB02MAC490           (18*MB)
/* Reserved for HDMAC */
#define GB02MAC491      (0*MB)
/* Reserved for V2V */
#define GB02MAC492        (1*MB)
/* Reserved for MMU */
#define GB02MAC493             (64*MB)
/* Reserved for DMA LL TABLE */
#define GB02MAC494        (24*MB)

//#define max(a,b) ((a) > (b) ? (a) : (b))
//#define min(a,b) ((a) < (b) ? (a) : (b))

/*typedef signed   char      int8_t;
typedef unsigned char      uint8_t;
typedef signed   short     int16_t;
typedef unsigned short     uint16_t;
typedef signed   int       int32_t;
typedef unsigned int       uint32_t;
 */
typedef enum {
	U_UPGRADE_TYPE_MCU_APP = 1,		/* upgrade MCU's APP image */
	U_UPGRADE_TYPE_UEFI_FW, 		/* upgrade UEFI firmware */
	U_UPGRADE_TYPE_MAX
} upgrade_mcu_sub_type_e;

struct GB02STR114 {
	uint8_t filename[0x100];	/* upgrade image name */
	unsigned long long addr; /* the address of the sub image in VRAM */
	uint32_t size; /* the size of the sub image */
	uint16_t crc16;
	uint8_t checksum;
	uint8_t type; /* MCU app, UEFI or others */
};

/* gb02 vram memory footprint */
struct GB02STR178 {
	uint64_t vpu_ram_size;
	uint64_t gpu_ram_size;
	uint32_t mcu_ram_size;
	uint32_t audio_ram_size;
	uint32_t hdmac_ram_size;
	uint32_t perf_cnt_size;
	uint32_t mmu_size;
	uint32_t dma_ll_tbl;
};

static struct GB02STR178 vram_mem = {
		.vpu_ram_size = 0,
		.gpu_ram_size = 0,
		.mcu_ram_size = GB02MAC489,
		.audio_ram_size = GB02MAC490,
		.hdmac_ram_size = GB02MAC491,
		.perf_cnt_size = GB02MAC492,
		.mmu_size = GB02MAC493,
		.dma_ll_tbl = GB02MAC494
};


/**
 * @brief  Update CRC16 for input byte
 * @param  crc_in input value
 * @param  input byte
 * @retval None
 */
uint16_t UpdateCRC16(uint16_t crc_in, uint8_t byte)
{
	uint32_t crc = crc_in;
	uint32_t in = byte | 0x100;

	do {
		crc <<= 1;
		in <<= 1;
		if(in & 0x100)
			++crc;
		if(crc & 0x10000)
			crc ^= 0x1021;
	} while(!(in & 0x10000));

	return crc & 0xffffu;
}

/**
 * @brief  Cal CRC16 for Packet
 * @param  data
 * @param  length
 * @retval None
 */
uint16_t Cal_CRC16(const uint8_t* p_data, uint32_t size)
{
	uint32_t crc = 0;
	const uint8_t* dataEnd = p_data+size;

	while(p_data < dataEnd)
		crc = UpdateCRC16(crc, *p_data++);

	crc = UpdateCRC16(crc, 0);
	crc = UpdateCRC16(crc, 0);

	return crc&0xffffu;
}

/**
 * @brief  Calculate Check sum for Packet
 * @param  p_data Pointer to input data
 * @param  size length of input data
 * @retval uint8_t checksum value
 */
uint8_t CalcChecksum(const uint8_t *p_data, uint32_t size)
{
	uint32_t sum = 0;
	const uint8_t *p_data_end = p_data + size;

	while (p_data < p_data_end )
	{
		sum += *p_data++;
	}

	return (sum & 0xffu);
}

int GB02FUNC1538(int fd, uint32_t size, uint16_t *crc16, uint8_t *checksum)
{
	char *buffer;
	int ret = 0;

	buffer = malloc(size);
	if (!buffer) {
		return -1;
	}

	lseek(fd, 0, SEEK_SET);
	ret = read(fd, buffer, size);
	if (ret != size) {
		printf("read image failed(ret = %d, size = %d)\n", ret, size);
		free(buffer);
		return -1;
	}

	*crc16 = Cal_CRC16(buffer, size);
	*checksum = CalcChecksum(buffer, size);
	free(buffer);
	//printf("0x%x ---0x%x",*crc16, *checksum);
	return 0;
}


static uint64_t GB02FUNC1539(uint64_t vram_size)
{
	uint64_t vpu_size = 0;
	if(vram_size==32*GB)
	{
		vpu_size = 8*GB;
	}
	else
	{
		vpu_size = vram_size * 0.5;
	}
	return vpu_size;
}

static uint64_t GB02FUNC1541(struct GB02STR178 *vram, uint64_t vram_size)
{
	uint64_t reserved_size = 0;

	if(vram==NULL)
		return 0;

	vram->vpu_ram_size  = GB02FUNC1539(vram_size);
	printf("vpu ram size:%llu\n", vram->vpu_ram_size);
	reserved_size = vram->vpu_ram_size + vram->mcu_ram_size +
			vram->audio_ram_size + vram->hdmac_ram_size +
			vram->perf_cnt_size + vram->mmu_size + vram->dma_ll_tbl;
	printf("reserved ram size:%llu\n", reserved_size);

	vram->gpu_ram_size = vram_size - reserved_size;
	printf("gpu ram size:%llu\n", vram->gpu_ram_size);
	return vram->gpu_ram_size;
}

static uint64_t GB02FUNC1543(struct GB02STR178 *vram, uint64_t vram_size)
{
	if(vram==NULL)
	{
		return 0;
	}
	uint64_t vpu_size = GB02FUNC1539(vram_size);
	uint64_t gpu_size = GB02FUNC1541(vram, vram_size);
	uint64_t mcu_addr_offset = vpu_size + gpu_size;
	return mcu_addr_offset;
}


uint64_t GB02FUNC1544(char* fname)
{
	char line[100] = {0, };
	char file_name[100] = {0,};
	uint32_t vram_size = 0;
	char *file_rp = NULL;
	FILE *file = NULL;

	if((access(fname, F_OK)) == -1) {
		printf("the file(%s) doesn't exist.\n", fname);
		return -1;		
	} else {
		file_rp = realpath(fname, NULL);
		strcpy(file_name, file_rp);
	}
	if((access(file_name, R_OK)) == -1) {
		printf("the file(%s) can't be read.\n", file_name);
		return -1;
	} else {
		printf("file name: %s.\n", file_name);
	}

	file = fopen(file_name,"r");
	if (NULL == file) {
		printf("fopen failed.\n");
		return -1;
	}
	while (fgets(line, sizeof(line), file)) {		

		if (sscanf(line,"Memory Size:%dM", &vram_size) == 1) {
			printf("vram size: %d MBytes\n", vram_size);	
			break;
		}
	}
	fclose(file);
	return vram_size*MB;	
}

int GB02FUNC1547( unsigned char *image_buf, size_t size,  unsigned char* md5)
{
	int ret = 0;
	MD5_CTX md5_ctx;

	if(image_buf==NULL)
		return -1;

	ret = MD5_Init(&md5_ctx);
	if(ret!=1)
		return -1;

	ret = MD5_Update(&md5_ctx, image_buf, size );
	if(ret!=1)
		return -1;

	ret = MD5_Final(md5, &md5_ctx);
	if(ret!=1)
		return -1;
	return 0;	
}

static int GB02FUNC1549(char* image_name, char* sig_name, char delim)
{
	int ret = -1;
	uint8_t len = 0;
	char* subfix = NULL;
	char tmp[GB02MAC2040];
	if(image_name==NULL) {
		printf("image name memory access error...\n");
		return -1;
	}
	subfix = strrchr(image_name, delim);
	if(subfix==NULL)
		return -1;
 	len = subfix-image_name;
//	printf("image_name size:%d, dot pos:%d",strlen(image_name), len);
	memcpy(tmp, image_name, len);
	tmp[len] = '\0';
//	printf("sig name:%s\n", tmp);
	sprintf(sig_name, "%s%s",tmp,".sig");
	printf("\n\nimage sig :%s",sig_name);
	return 0;
}

static int GB02FUNC1552(char* fpath)
{	
	if(fpath==NULL)	return -1;
	/* check whether the file exist */
	if((access(fpath, F_OK))==-1) {
		printf("file (%s) does't exist.", fpath);
		return -1;
	}
	/* judge whether we get the read permission of the file. */
	if((access(fpath, R_OK)) == -1) {
		printf("The file(%s) can't be read.\n", fpath);
		return -1;
	} 
	return 0;
}

int GB02FUNC1555(char* fname)
{
	int fd = 0;
	size_t size;
	fd = open(fname, O_RDONLY);
	if(fd == -1) {
		printf("open file (%s) error\n", fname);
		return -1;
	}
	size = lseek(fd, 0, SEEK_END);

	if(size==0) {
		printf("file (%s) is empty\n", fname);		
		return -1;
	}
	printf("file:%s, size: %ld bytes\n", fname, size);
	close(fd);
	return size;
}

int GB02FUNC1558(char *fname, size_t offset,unsigned char* user_id)
{
	FILE* fp;
	unsigned char line[GB02MAC2042+1];
	if(fname==NULL)
		return -1;

	fp = fopen(fname, "r");
	if(NULL == fp) {
		printf("open file %s error!\n", fname);
		return -1;
	}
	//	printf("\nopen file (%s) success.", fname);
	fseek(fp, offset, SEEK_SET); /* 定位到文件开头 */
	fgets(line, GB02MAC2042+1,fp);
	strcpy(user_id, line);
	printf("\n+++ read user id start...");
	printf("\nuser_id:%s",user_id);
	printf("len:%d",strlen(line));
	fclose(fp);
	return strlen(line);
}

int GB02FUNC1560(char *image_sig, size_t offset, char* pub_key)
{
	int fd_image_sig;
	if(image_sig==NULL) {
		return -1;
	}
	fd_image_sig = open(image_sig, O_RDONLY);
	if(fd_image_sig==-1) {
		printf("open file (%s) error\n", image_sig);
		return -1;
	}

	//printf("\nopen file (%s) success.", image_sig);
	lseek(fd_image_sig, offset, SEEK_SET);

	printf("\n+++ read public key start...");
	ssize_t read_nb = read(fd_image_sig, pub_key , GB02MAC2041);
	if(read_nb==-1) {
		printf("read signature file (%s)--pub_key error\n", image_sig);
		return -1;
	}
	printf("\n==============pub key data===============\n");	
	for(int i=0;i<GB02MAC2041;i++) {
		printf("0x%x ",pub_key[i]);
	}
	close(fd_image_sig);
	return read_nb;
}

int GB02FUNC1561(char* image_sig, size_t offset, SM2_SIGNATURE_STRUCT *sm2_sig )
{
	int fd_image_sig;
	if(image_sig==NULL) {
		return -1;
	}
	fd_image_sig = open(image_sig, O_RDONLY);
	if(fd_image_sig==-1) {
		printf("open file (%s) error\n", image_sig);
		return -1;
	}

	//	printf("\nopen file (%s) success.", image_sig);
	lseek(fd_image_sig, offset, SEEK_SET);

	ssize_t read_nb = read(fd_image_sig, sm2_sig->r_coordinate, sizeof(sm2_sig->r_coordinate));
	if(read_nb==-1)	{
		printf("read signature file (%s)--r_coordinate error\n", image_sig);
		return -1;
	}
	printf("\n+++ read signatue data start ...", image_sig);
	printf("\n================r_coordinate==============\n");
	for(int i=0;i<sizeof(sm2_sig->r_coordinate);i++) {
		printf("0x%x ",sm2_sig->r_coordinate[i]);
	}
	read_nb = read(fd_image_sig, sm2_sig->s_coordinate, sizeof(sm2_sig->s_coordinate));

	if(read_nb==-1)	{
		printf("read signature file (%s)--s_coordinate error\n", image_sig);
		return -1;
	}

	printf("\n================p_coordinate==============\n");
	for(int i=0;i<sizeof(sm2_sig->s_coordinate);i++) {
		printf("0x%x ",sm2_sig->s_coordinate[i]);
	}
	printf("\nread signature file success.");
	return 0;
}

int GB02FUNC1564(char* image_name)
{
	int error_code = -1;	
	size_t image_size;
	unsigned char user_id[GB02MAC2042+1]={0,};
	unsigned int user_id_len = 0;
	int fd_image = 0;
	unsigned char* tmp_buf = NULL;
	unsigned char pub_key[GB02MAC2041] = {0};
	int ret_size;
	char image_sig[GB02MAC2040];
	SM2_SIGNATURE_STRUCT sm2_sig;
	unsigned char md[MD5_DIGEST_LENGTH];

	/* malloc memory for image file data */
	image_size = GB02FUNC1555(image_name);
	/* read contents of image file to buffer*/
	tmp_buf = malloc(image_size);	
	if(tmp_buf==NULL) {
		printf("tmp_buf malloc error\n");
		error_code = -1;
		goto ERROR_1;
	}

	/* read image file data to tmp_buf */
	fd_image = open(image_name, O_RDONLY);

	if(fd_image == -1) {
		printf("open file (%s) error\n", image_name);
		goto ERROR_0;
		return -1;
	}	
	lseek(fd_image, 0, SEEK_SET);
	ret_size = read(fd_image,tmp_buf,image_size);
	if(ret_size!=image_size) {
		printf("file (%s) read error\n",image_name);				
		error_code = -1;
		goto ERROR_0;
	}

	/* create md5 string for image file data */
	memset(md, 0, MD5_DIGEST_LENGTH);
	int ret = GB02FUNC1547(tmp_buf, image_size, md);
	if(ret<0) {
		error_code = -2;
		printf("image file's md5 digest error...\r\n"); 
		goto ERROR_0;		
	}	

	printf("\ndigest print start...\n");
	for(int i=0;i<MD5_DIGEST_LENGTH;i++) {
		printf("0x%x ",md[i]);
	}
	printf("\ndigest print end...");

	/* find the signature file of image file in current path */
	ret = GB02FUNC1549(image_name, image_sig, '.');	
	if(ret<0) {
		error_code = -3;
		goto ERROR_0;		
	}

	ret = GB02FUNC1552(image_sig);
	if(ret<0) {
		error_code = -4;
		goto ERROR_0;
	}

	/* get user_id from image.sig file */
	user_id_len = GB02FUNC1558(image_sig, 0, user_id);

	int pub_key_size = GB02FUNC1560(image_sig, user_id_len, pub_key);

	printf("\npub key size:%d\n",pub_key_size);	
	if(pub_key_size!=GB02MAC2041) {
		error_code = -6;
		goto ERROR_0;			
	}
	ret = GB02FUNC1561(image_sig, pub_key_size+user_id_len, &sm2_sig );
	if(ret<0) {
		error_code = -7;
		goto ERROR_0;	
	}
	/* start to verify the signature of image file*/	
	ret = GB02FUNC1686(md, MD5_DIGEST_LENGTH, (unsigned char*)user_id, user_id_len-1, pub_key, &sm2_sig);
	if (ret!=0)	{
		printf("\nverify image signature failed ...\n");
		error_code = -8;
		goto ERROR_0;
	} else {
		printf("\nverify image signature passed ...\n");
		error_code  = 0;
	}
	ERROR_0:
	free(tmp_buf);
	ERROR_1:
	close(fd_image);
	return error_code;
}

int main(int argc, char *argv[])
{
	int  fd,fd_image = 0, ret = 0, i = 0;
	char image_file[100] = {0}, *image_file_rp;
	unsigned int count = 0, image_len = 0;
	char *buffer;
	uint32_t wrote;
	fd_set readfd;
	struct timeval timeout;
	struct GB02STR114 image_info;
	uint8_t type = 0;	

	if(argc>=2) {
		type = atoi(argv[1]);
		switch(type) {
			case 1:
			case 2:
				if(argc<3) {
					printf("\nParameter count mismatch,please check!\n");
					printf("\nUsage : ./upgrade [upgrade type] [upgrade image file]");
					printf("\nupgrade type : 1 --- VBIOS firmware;  2 --- UEFI firmware; 3 -- display version;\n\n");
					return -1;
				}
				break;
			case 3:
			{
				system(VBIOS_VERSION_PROC_FILE_PATH);
				return 0;
			}
		}
	} else {
		printf("\nParameter count mismatch,please check!\n");
		printf("\nUsage : ./upgrade [upgrade type] [upgrade image file]");
		printf("\nupgrade type : 1 --- VBIOS firmware;  2 --- UEFI firmware; 3 -- display version;\n\n");	
		return -1;		
	}

	strcpy(image_file, argv[2]);	

	if((access(image_file, F_OK)) == -1) {
		printf("the image_file(%s) doesn't exist.\n", image_file);
		return -1;		
	} else {
		image_file_rp = realpath(image_file, NULL);
		strcpy(image_file, image_file_rp);
	}

	/* judge whether we get the read permission of the image_file. */
	if((access(image_file, R_OK)) == -1) {
		printf("the image_file(%s) can't be read.\n", image_file);
		return -1;
	} else {
		printf("image name: %s.\n", image_file);
	}	

	ret = GB02FUNC1564(image_file);	
	if(ret!=0) {
		return -1;
	}	

	/* get MCU's sub image */
	fd_image = open(image_file, O_RDONLY);
	if (fd_image == -1) {
		printf("open file(%s) failed\n", image_file);
		return -1;
	}
	image_len = lseek(fd_image, 0, SEEK_END);
	printf("image size: %d.\n", image_len);
	lseek(fd_image, 0, SEEK_SET);

	fd = open("/dev/gb02_fw", O_RDWR);
	if (fd == -1) {
		printf("open gbfpga failed\n");
		close(fd_image);
		return -1;
	}
	lseek(fd, 0, SEEK_SET);

	buffer = malloc(image_len);
	if (!buffer) {
		close(fd_image);
		close(fd);
		return -1;
	}

	/* read the new image */
	count = read(fd_image, buffer, image_len);
	if (count != image_len) {
		printf("read image failed. count = %d, image_len = %d\n", count, image_len);
	}
	/* write upgrade image to VRAM */
	wrote = write(fd, buffer, image_len);
	if (wrote != image_len) {
		printf("read image failed. wrote = %d, image_len = %d\n", wrote, image_len);
	}

	/* image file name, size, and CRC16/checksum content */
	memset(&image_info, 0, sizeof(image_info));
	uint64_t vram_size = GB02FUNC1544(VRAM_SIZE_PROC_FILE_PATH);
	image_info.addr = GB02FUNC1543(&vram_mem, vram_size);
	printf("image addr:0x%llx\n",image_info.addr);
	image_info.size = image_len;

	if ((type >= U_UPGRADE_TYPE_MCU_APP) && (type < U_UPGRADE_TYPE_MAX)) {
		image_info.type = type;
	} else {
		printf("Invalid upgrade image type(%d)!\n", type);
		free(buffer);
		close(fd_image);
		close(fd);
		return -1;
	}

	if (image_info.type == U_UPGRADE_TYPE_MCU_APP) {
		strcpy(image_info.filename, APP_FILE_NAME);
	} else {
		strcpy(image_info.filename, UEFI_FIRMWARE_NAME);
	}
	/* calculate CRC16 and checksum of the image */
	ret = GB02FUNC1538(fd_image, image_info.size,
			&(image_info.crc16), &(image_info.checksum));
	if (ret < 0) {
		printf("Failed to calculate CRC(%d)!\n", ret);
		free(buffer);
		close(fd_image);
		close(fd);
		return -1;
	}
	printf("Start to set upgrade image info!\n");
	ret = ioctl(fd, UPGRADE_MCU_CMD_SET_IMAGE_INFO, &image_info);
	if (ret < 0) {
		printf("ioctl failed with code %d\n", ret);
		free(buffer);
		close(fd_image);
		close(fd);
		return -1;
	}

	/* start to upgrade MCU's sub */
	printf("Start to Upgrade %s!\n",
			image_info.type == U_UPGRADE_TYPE_MCU_APP ? "VBIOS firmware" : "option ROM firmware");
	ret = ioctl(fd, UPGRADE_MCU_CMD_START, NULL);
	if (ret < 0) {
		printf("ioctl failed with code %d\n", ret);
		free(buffer);
		close(fd_image);
		close(fd);
		return -1;
	}

	/* wait for upgrading finish */

	for(i = 0;i < 10;i++)
	{
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		FD_ZERO(&readfd);
		FD_SET(fd, &readfd);

		ret = select(fd + 1, &readfd, NULL, NULL, &timeout);
		if(ret == -1) {
			printf("select error(%d)\n", ret);
		}
		else if(ret) {
			if(FD_ISSET(fd, &readfd))
			{
				printf("image upgrade finished,please reboot system.\n");
				break;
			}
		} else if (!ret)
			printf("select timeout\n");
	}

	close(fd_image);
	close(fd);
	return 0;
}
