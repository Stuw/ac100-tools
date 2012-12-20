#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

int fd;
unsigned short read_i2c(unsigned char reg) {
	int err;
	struct i2c_rdwr_ioctl_data dat;
	unsigned addr=0x3C>>1;
	unsigned short data;
	struct i2c_msg msgs[]= {
		{
			.addr = addr,
			.flags = 0,
			.len = 1,
			.buf = &reg,
		},
		{
			.addr = addr,
			.flags = I2C_M_RD,
			.len = 2,
			.buf =(unsigned char*) &data,
		}
	};
	dat.msgs=msgs;
	dat.nmsgs=2;

	err=ioctl(fd, I2C_RDWR, &dat);
	if(err==-1) {
		perror("I2C_RDWR");
		return 0;
	}
	//Make it upside down
	data=data<<8 | (data&0xff00)>>8;
	return data;
}

void write_i2c(unsigned char reg, unsigned short value) {
	int err;
	struct i2c_rdwr_ioctl_data dat;
	unsigned addr=0x3C>>1;
	unsigned short data;
	unsigned char buf[3];
	buf[0]=reg;
	//!!!!!!
	//Endian inversion !!!!
	buf[2]=value&0xff;
	buf[1]=value>>8;
	struct i2c_msg msgs[]= {
		{
			.addr = addr,
			.flags = 0,
			.len = 3,
			.buf = buf,
		}
	};
	dat.msgs=msgs;
	dat.nmsgs=1;

	err=ioctl(fd, I2C_RDWR, &dat);
	if(err==-1) {
		perror("I2C_RDWR");
		return;
	}
}


void init()
{
	write_i2c(0x02, 0x8383);
	write_i2c(0x04, 0x0c0c);
	write_i2c(0x10, 0xee00);
	write_i2c(0x12, 0xdcdc);
	write_i2c(0x1c, 0x0348);
	write_i2c(0x22, 0x0500);
	write_i2c(0x24, 0x00c4);
	write_i2c(0x26, 0x000f);
	write_i2c(0x3A, 0xcf63);
	write_i2c(0x3C, 0x23F0);
	write_i2c(0x3E, 0x8C00);
	write_i2c(0x5E, 0x0000);
}

int hex_digit(char c)
{
	if ('0' <= c && c <= '9')
		return (c - '0');
	if ('a' <= c && c <= 'f')
		return (10 + (c - 'a'));
	if ('A' <= c && c <= 'F')
		return (10 + (c - 'a'));

	return -1;
}

int hex_value(const char* sval)
{
	int x = -1;
	int val = 0;
	const char* ptr = sval;

	if (!sval)
		return -1;

	val = hex_digit(*ptr);
	++ptr;

	while (*ptr != '\0')
	{
		int x = hex_digit(*ptr);
		if (x == -1)
		{
			printf("Incorrect value: %s\n", sval);
			return -1;
		}

		++ptr;
		val = ((val << 4) | x);
	}

	return val;
}

int dec_value(const char* sval)
{
	int x = -1;
	int val = 0;
	const char* ptr = sval;

	if (!sval)
		return -1;

	while (*ptr != '\0')
	{
		int x = *ptr - '0';
		if (x < 0 || x > 9)
		{
			printf("Incorrect value: %s\n", sval);
			return -1;
		}

		++ptr;
		val = (val * 10 + x);
	}

	return val;
}

int p_write(int reg, int val)
{
	printf("-> %02x: %04x\n", reg, val);
	write_i2c(reg, val);
}


int p_read(int reg)
{
	unsigned int val = read_i2c(reg);
	printf("%02x: %04x\n", reg, val);
}


int p_ch_bit(int reg, int bit, int set)
{
	printf("-> %02x: %d to %d\n", reg, bit, set);
	unsigned int val = 0x8; //read_i2c(reg);
	printf("-> %02x: %04x before\n", reg, val);
	if (set)
		val = val | (1 << bit);
	else
		val = val & (~(1 << bit));
	printf("-> %02x: %04x after\n", reg, val);
	write_i2c(reg, val);
}


int main(int argc, char **argv, char **envp) {
	uid_t uid = getuid();
	if (uid != 0)
	{
		printf("You need to run this tool as root.\n");
		return 1;
	}

	if (argc < 2)
	{
		printf("Usage: %s <i2c_dev> <read|write> <reg> [value]\nFor example: %s /dev/i2c-0 r 3a\n",
				argv[0], argv[0]);
		printf("Usage: %s <i2c_dev> <set|clr> <reg> [bit]\nFor example: %s /dev/i2c-0 s 3a 1\n",
				argv[0], argv[0]);

		return 2;
	}

	fd=open(argv[1], O_RDWR);
	if (fd == -1)
	{
		perror("i2c device open");
		return 3;
	}

	int err;
	err=ioctl(fd, I2C_TENBIT, 0);
	if(err) {
		perror("I2C_TEN");
		return 4;
	}

	err=ioctl(fd, I2C_SLAVE, 0x3C);
	if(err) {
		perror("I2C_SLAVE");
		return 5;
	}

	if (argc == 2)
	{
		init();
		return 0;
	}

	const char* cmd = argv[2];
	switch (cmd[0])
	{
		case 'r':
		{
			int reg = hex_value(argv[3]);
			if (reg == -1)
				return 7;

			p_read(reg);
		}
		break;

		case 'w':
		{
			if (argc != 5)
				return -1;

			int reg = hex_value(argv[3]);
			if (reg == -1)
				return 8;

			int val = hex_value(argv[4]);
			if (val == -1)
				return 9;

			p_write(reg, val);
		}
		break;

		case 's':
		case 'c':
		{
			if (argc != 5)
				return -1;

			int reg = hex_value(argv[3]);
			if (reg == -1)
				return 8;

			int bit = dec_value(argv[4]);
			if (bit == -1 || (bit < 0 || bit > 15))
				return 9;

			p_ch_bit(reg, bit, cmd[0] == 's');
		}
		break;

		default:
			printf ("Unknown command\n");
			return 6;
	}

	return 0;
}
