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

int main(int argc, char **argv, char **envp) {
	uid_t uid = getuid();
	if (uid != 0)
	{
		printf("You need to run this tool as root.\n");
		return 1;
	}

	if (argc != 2)
	{
		printf("Usage: %s <i2c_dev>\nFor example: %s /dev/i2c-0\n",
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
	unsigned char reg;
	unsigned short val;
	unsigned int ureg;
	unsigned int uval;


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


	for (reg = 0x02; reg <= 0x6e; reg += 2)
	{
		val = read_i2c(reg);
		ureg = reg;
		uval = val;
		printf("%02x: %04x\n", ureg, uval);
	}

	return 0;
}
