#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <math.h>

int main()
{
	int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd == -1) {
		printf("Failed to created socket: %s\n", strerror(errno));
		return 1;
	}

	struct    sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(9999);

	int ret = bind(fd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	if (ret == -1) {
		printf("Failed to bind socket: %s\n", strerror(errno));
		return 1;
	}

	unsigned num_half_cycles = 0;
	unsigned num_samples = 0;
	double sum = 0.0;
	double last_val = 100.0;

	while (1) { 
		uint8_t buf[1500];
		ret = recv(fd, buf, sizeof(buf), 0);
		if (ret < 0) {
			printf("Failed to recv data: %s\n", strerror(errno));
			return 1;
		}

		uint16_t id = buf[1] << 8 | buf[0];
		
		for (int offset = 2; offset + 3 <= ret; offset += 3) {
			uint16_t rval = buf[offset+1] | (buf[offset+2]<<8);
			int16_t rvali = rval;
			float val = rvali * (2.048 / 32768.0);

			if (val >= 0.0 && last_val < 0.0) {
				num_half_cycles++;
				if (num_half_cycles == 50) {
					printf("S %6u %8f\n", num_samples, 5.0*sqrt(sum/num_samples));
					num_samples = 0;
					sum = 0;
					num_half_cycles = 0;
				}
			}

			sum += val*val;
			num_samples++;
			printf("I %04x %03d %7u %8.5f %8u %4u %8f\n", id, buf[offset], rval, val, num_samples, num_half_cycles, sqrt(sum/num_samples));

			last_val = val;
		}

	}
}
