#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

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
#if 1
			float val = rvali * (4.096 / 32768.0);
#else
			float val = rvali * (4.096 / 32768.0 / 0.185) - 1.65;
#endif
			printf("%04x %03d %6u %8f\n", id, buf[offset], rval, val);
		}

	}
}
