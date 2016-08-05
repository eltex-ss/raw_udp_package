#include "common.h"

#include <math.h>

/*====================================*/
/*                                    */
/*  Constants.                        */
/*                                    */
#define IP_UDP_PACKAGE_SIZE 60

/*====================================*/
/*                                    */
/*  Structures.                       */
/*                                    */
struct UdpPseudoHeader {
  uint32_t ip_s;
  uint32_t ip_d;
  uint8_t nulls;
  uint8_t protocol;
  uint16_t udp_l; 
};

struct UdpHeader {
  uint16_t port_s;
  uint16_t port_d;
  uint16_t length;
  uint16_t checksum;
};

struct UdpPackage {
  struct UdpHeader header;
  uint8_t data[MESSAGE_SIZE];
};

/*====================================*/
/*                                    */
/*  Global variables.                 */
/*                                    */
static int sock;

/*====================================*/
/*                                    */
/*  Functions.                        */
/*                                    */
void CloseSocket(void)
{
  close(sock);
}

uint16_t CalcCheckSum(uint16_t bytes[], size_t size)
{
  uint32_t sum = 0;
  while (size > 1) {
    sum += *(uint16_t *) bytes++;
    size -= 2;
  }
  if (size > 0)
    sum += *(uint8_t *) bytes;
  while (sum >> 16)
    sum = (sum & 0xffff) + (sum >> 16);
  return ~sum;
}

int IsAddressCorrect(char *ip_address)
{
  struct sockaddr_in net_address;
  int result;
  result = inet_pton(AF_INET, ip_address, &net_address);
  return result == 1;
}

int main(int argc, char **argv)
{
  struct sockaddr_in server_address;
  socklen_t server_length;

  struct UdpPseudoHeader *pseudo_header;
  struct UdpHeader *udp_header;
  struct UdpPackage *udp_package;
  char *message;                   /*  Udp data buffer. */
  
  uint8_t ip_udp_package[IP_UDP_PACKAGE_SIZE];  /*  Receiving package. */
  uint8_t pseudo_ip_udp_package[UDP_PACKAGE_SIZE + 12]; /*  Udp package buf. */

  /*  Input data check. */
  if (argc == 2) {
    if (!IsAddressCorrect(argv[1])) {
      printf("Incorrect address\n");
      exit(1);
    }
  } else {
    printf("Incorrect usage\n");
    printf("Should be: ./client ip_destination\n");
    printf("Example: ./client 127.0.0.1\n");
    exit(1);
  }

  /*  Data initialization. */
  pseudo_header = (struct UdpPseudoHeader *) pseudo_ip_udp_package;
  udp_package = (struct UdpPackage *)
                (pseudo_ip_udp_package + sizeof(struct UdpPseudoHeader));
  udp_header = (struct UdpHeader *) udp_package;
  message = (char *)(udp_package->data);

  /*  Create raw socket. */
  sock = CreateSocket(RAW_SOCK);
  atexit(CloseSocket);

  memset(message, 0, MESSAGE_SIZE);
  sprintf(message, "Hello!");
  server_length = sizeof(struct sockaddr_in);

  /*================================*/
  /*                                */
  /*  Udp package.                  */
  /*                                */

  /*  Filling pseudo header of udp package. */ 
  pseudo_header = (struct UdpPseudoHeader *) pseudo_ip_udp_package;
  pseudo_header->ip_s = inet_addr("192.168.2.44");
  pseudo_header->ip_d = inet_addr(argv[1]);
  pseudo_header->nulls = 0;
  pseudo_header->protocol = 17;
  pseudo_header->udp_l = htons(UDP_PACKAGE_SIZE);

  /*  Filling udp header. */
  udp_header->port_s = htons(SERVER_PORT + 1);
  udp_header->port_d = htons(SERVER_PORT);
  udp_header->length = htons(UDP_PACKAGE_SIZE);
  udp_header->checksum = 0;

  udp_header->checksum = CalcCheckSum((uint16_t *)pseudo_ip_udp_package,
                                       UDP_PACKAGE_SIZE + 12);

  /*  Filling server address structure. */
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(SERVER_PORT);
  server_address.sin_addr.s_addr = inet_addr(argv[1]);

  if (sendto(sock, udp_package, UDP_PACKAGE_SIZE, 0,
             (struct sockaddr *) &server_address, server_length) < 0) {
    perror("sendto error");
    exit(1);
  }
  while (1) {
    uint8_t header_size;
    uint8_t first_octet;
    uint16_t port_d;
    size_t udp_offset = 0;
    
    if (recvfrom(sock, ip_udp_package, IP_UDP_PACKAGE_SIZE, 0,
                 (struct sockaddr *) &server_address, &server_length) < 0) {
      perror("recvfrom error");
      exit(1);
    }

    memmove(&first_octet, ip_udp_package, 1); /*  Read first byte. */
    header_size = first_octet & 0xf; /* 0xf = 00001111b */
    udp_offset = (header_size > 5) ? 24 : 20; 

    memmove(&port_d, ip_udp_package + udp_offset + 2, 2);
    port_d = ntohs(port_d);
    if (port_d == 9999) {
      sprintf(message, "%s", ip_udp_package + udp_offset + 8);
      printf("%s\n", message);
      break;
    }
  }

  return 0;
}
