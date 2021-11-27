#include <iostream>
#include <fstream>

// C library headers
#include <stdio.h>
#include <string.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

using namespace std;


extern uint8_t _binary_083AD_bin_start[];

int serial_port;

// Configure USB->serial adapter
void init_serial() {

	// Create new termios struct, we call it 'tty' for convention
	// No need for "= {0}" at the end as we'll immediately write the existing
	// config to this struct
	struct termios tty;

	// Read in existing settings, and handle any error
	// NOTE: This is important! POSIX states that the struct passed to tcsetattr()
	// must have been initialized with a call to tcgetattr() overwise behaviour
	// is undefined
	if(tcgetattr(serial_port, &tty) != 0) {
		printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
	}

	tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
	//tty.c_cflag |= PARENB;  // Set parity bit, enabling parity
	//tty.c_cflag &= ~PARODD;  // Clear parity odd bit
	//tty.c_cflag |= INPCK; // Enable parity checking

	tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
	tty.c_cflag &= ~CSIZE; // Clear all the size bits
	tty.c_cflag |= CS8; // 8 bits per byte (most common)

	tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
	//tty.c_cflag |= CRTSCTS;  // Enable RTS/CTS hardware flow control

	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO; // Disable echo
	tty.c_lflag &= ~ECHOE; // Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo
	tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK|BRKINT|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
	tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
	tty.c_cc[VMIN] = 0;

	cfsetispeed(&tty, B9600);
	cfsetospeed(&tty, B9600);

	// Save tty settings, also checking for error
	if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
		printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
	}
}

// Print sample of new firmware to ensure it's compiled in properly.
void dump_new_firmware() {
	for (uint32_t i = 0; i < 32; i++)
	{
		printf("%06X - 0x%02X\n", i, _binary_083AD_bin_start[i]);
	}
}

void fetch_rdid() {
	char read_buf[256] = {0};
	const char msg[] = {'i'};
	write(serial_port, msg, 1);
	int n = read(serial_port, &read_buf, 3);
	printf("Read: %d\n%x \n%x \n%x\n", n, read_buf[0], read_buf[1], read_buf[2]);
	printf("RDID should contain 0xc2 0x20 and 0x17.\n");
}

void fetch_status() {
	char read_buf[256] = {0};
	const char msg[] = {'s'};
	write(serial_port, msg, 1);
	int n = read(serial_port, &read_buf, 1);
	printf("Read: %d\n%x\n", n, read_buf[0]);
	printf("STATUS:\n[7]: Reserved\n[6]: Quad Enable\n[5-2]: Block Protect\n[1]: Write Enable\n[0]: Write in Progress\n");
}

// Extract firmware and write to a file
void dump_chip() {
	char read_buf[0x100];
	const char msg[] = {'d'};
	const char next[] = {'n'};
	const char all[] = {'a'};
	const char done[] = {'e'};
	ofstream dataFile("dump.bin", ios::out | ios::binary);

	write(serial_port, msg, 1);
	int n = read(serial_port, read_buf, 1);
	if (n == -1) {
		printf("Error reading d confirm.\n");
		return;
	}
	if (read_buf[0] != 'd') {
		printf("Error confirming dump.\n");
		return;
	}

	for (uint32_t i = 0; i < 0x800000; i++) {
		write(serial_port, next, 1);
		int n = read(serial_port, read_buf, 0x1);
		dataFile.write(read_buf, 0x1);
		if (n == -1) {
			printf("Error while reading from serial!\n");
			break;
		}
		if (i%0x1000 == 0) {
			printf("0x%x\n", i);
		}

	}

//	write(serial_port, all, 1);
//	for (uint32_t i = 0; i < 0x800000/0x100; i++) {
//		int n = read(serial_port, read_buf, 0x100);
//		dataFile.write(read_buf, 0x100);
//		if (n == -1) {
//			printf("Error while reading from serial!\n");
//			break;
//		}
//		printf("Fetching page #%d\n", i);
//	}

	write(serial_port, done, 1);

	dataFile.close();
//	printf("Read: 0x%04X\n", (uint8_t *)read_buf);
return;
}

void erase_chip()
{
	char result = '\0';
	const char msg[] = {'e'};
	write(serial_port, msg, 1);
	do {
		int n = read(serial_port, &result, 1);
		if (n == -1) {
			printf("Error while confirming erase!\n");
			break;
		}
	} while (result != 'e');

	printf("Chip should be erased now.\n");
	return;
}

void program_chip()
{
	const char cmd[] = {'p'};
	const char next[] = {'n'};
	const char done[] = {'z'};
	char result = '\0';
	int n = 0;

	// Send program command
	write(serial_port, cmd, 1);
	n = read(serial_port, &result, 1);
	if (n == -1) {
		printf("Error while confirming prog command!\n");
		return;
	}
	if (result != 'p') {
		printf("Error getting ack for p command.\n");
		return;
	}

	// Send new firmware to chip
	for (uint32_t i = 0; i < (0x800000/0x100); i++)
	{
		// Generate address bytes
		uint8_t addr3 = (((i*0x100)>>16)&0xff);
		uint8_t addr2 = (((i*0x100)>>8)&0xff);
//		uint8_t addr1 = (i&0xff);

		// Send that new page is being written
		write(serial_port,&next, 1);
		n = read(serial_port, &result, 1);
		if (n == -1) {
			printf("Error while confirming next page!\n");
			return;
		}
		if ((uint8_t)result != next[0]) {
			printf("Error getting ack for next page.\n");
			return;
		}

//		printf("Sending Address: %x, %x\n", addr3, addr2);

		// Send address byte 3
		write(serial_port,&addr3, 1);
		n = read(serial_port, &result, 1);
		if (n == -1) {
			printf("Error while confirming addr3!\n");
			return;
		}
		if ((uint8_t)result != addr3) {
			printf("Error getting ack for addr3.\n");
			return;
		}

		// Send address byte 2
		write(serial_port,&addr2, 1);
		n = read(serial_port, &result, 1);
		if (n == -1) {
			printf("Error while confirming addr2!\n");
			return;
		}
		if ((uint8_t)result != addr2) {
			printf("Error getting ack for addr2.\n");
			return;
		}

//		// Send address byte 1
//		write(serial_port,&addr1, 1);
//		n = read(serial_port, &result, 1);
//		if (n == -1) {
//			printf("Error while confirming addr1!\n");
//			return;
//		}
//		if ((uint8_t)result != addr1) {
//			printf("Error getting ack for addr1.\n");
//			return;
//		}

		// Send page to chip
		for (uint32_t d = 0; d < 0x100; d++)
		{
			uint32_t addr = (i*0x100)+d;
			uint8_t byte = _binary_083AD_bin_start[addr];
			write(serial_port, &byte, 1);
			n = read(serial_port, &result, 1);
			if (n == -1) {
				printf("Error while confirming page byte!\n");
				return;
			}
			if ((uint8_t)result != byte) {
				printf("Error getting ack for page byte.\n");
				return;
			}
//			else {
//				printf("[%x - %x = %x]  ", addr, byte, (uint8_t)result);
//			}
		}

//		// Send byte to chip
//		uint8_t byte = _binary_083AD_bin_start[i];
//		write(serial_port, &byte, 1);
//		n = read(serial_port, &result, 1);
//		if (n == -1) {
//			printf("Error while confirming page byte!\n");
//			return;
//		}
//		if ((uint8_t)result != byte) {
//			printf("Error getting ack for page byte.\n");
//			return;
//		}
//		else {
//			printf("[%x - %x = %x]  ", i, byte, (uint8_t)result);
//		}

		do {
			n = read(serial_port, &result, 1);
			if (n == -1) {
				printf("Error while confirming page write!\n");
				break;
			}
		} while (result != 's');
//		printf("Page data sent.\n");

		do {
			n = read(serial_port, &result, 1);
			if (n == -1) {
				printf("Error while confirming page write complete!\n");
				break;
			}
		} while (result != 'd');
//		printf("Page written.\n");
	}

	// Send done command
	write(serial_port,&done, 1);
	n = read(serial_port, &result, 1);
	if (n == -1) {
		printf("Error while confirming done page!\n");
		return;
	}
	if ((uint8_t)result != done[0]) {
		printf("Error getting ack for done page.\n");
		return;
	}

	return;
}

// Main loop
int main()
{
	char input;

//	serial_port = open("/dev/ttyUSB0", O_RDWR);
	serial_port = open("/dev/ttyACM0", O_RDWR);

	// Check for errors
	if (serial_port < 0) {
		printf("Error %i from open: %s\n", errno, strerror(errno));
	}

//	init_serial();

	while (1) {
		printf("\n\n============ Main Menu ============\n");
		printf("(1) Print new firmware sample.\n");
		printf("(2) Fetch the RDID register.\n");
		printf("(3) Fetch the STATUS register.\n");
		printf("(4) Dump firmware from chip to dump.bin.\n");
		printf("(5) Erase chip.\n");
		printf("(6) Program chip with 083AD firmware.\n");
		printf("(q) Exit.\n");
		printf("> ");
		input = getchar();

		if (input == '\r' || input == '\n')
			continue;

		switch(input) {
			case '1':
				dump_new_firmware();
				break;
			case '2':
				fetch_rdid();
				break;
			case '3':
				fetch_status();
				break;
			case '4':
				dump_chip();
				break;
			case '5':
				erase_chip();
				break;
			case '6':
				program_chip();
				break;
			case 'q':
				close(serial_port);
				return 0;
			default:
				printf("Unrecognized option.\n");
				break;
		}
	}

    return 0;
}
