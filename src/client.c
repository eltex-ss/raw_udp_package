#include "common.h"

#include <math.h>

#define IP_UDP_PACKAGE_SIZE 60

static int sock;

void CloseSocket(void)
{
  close(sock);
}

int main(void)
{
  struct sockaddr_in server_address;
  socklen_t server_length;
  char message[MESSAGE_SIZE];               /*  Udp data buffer. */
  char udp_package[UDP_PACKAGE_SIZE];       /*  Sending package. */
  char ip_udp_package[IP_UDP_PACKAGE_SIZE]; /*  Receiving package. */
  uint16_t port_d,            /*  Port-destination. */
           port_s,            /*  Port-source. */
           package_length,    /*  Udp-package length. */
           checksum;          /*  Udp-package checksum. */

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

  /*  Filling udp fields. */
  port_d = htons(SERVER_PORT);
  port_s = 10001;
  package_length = htons(UDP_PACKAGE_SIZE);
  checksum = 0;
  memset(udp_package, 0, UDP_PACKAGE_SIZE);
  
  memcpy(udp_package, &port_s, 2);
  memcpy(udp_package + 2, &port_d, 2);
  memcpy(udp_package + 4, &package_length, 2);
  memcpy(udp_package + 6, &checksum, 2);
  memcpy(udp_package + 8, message, MESSAGE_SIZE);

  /*  Filling server address structure. */
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(SERVER_PORT);
  server_address.sin_addr.s_addr = inet_addr("192.168.2.44");

  if (sendto(sock, udp_package, UDP_PACKAGE_SIZE, 0,
             (struct sockaddr *) &server_address, server_length) < 0) {
    perror("sendto error");
    exit(1);
  }
  while (1) {
    char header_size;
    char first_octet;
    int udp_offset = 0;
    
    if (recvfrom(sock, ip_udp_package, IP_UDP_PACKAGE_SIZE, 0,
                 (struct sockaddr *) &server_address, &server_length) < 0) {
      perror("recvfrom error");
      exit(1);
    }

    memcpy(&first_octet, ip_udp_package, 1); /*  Read first byte. */
    header_size = first_octet & 15; /* 15d = 00001111b */
    udp_offset = (header_size > 5) ? 24 : 20; 

    memcpy(&port_d, ip_udp_package + udp_offset + 2, 2);
    port_d = ntohs(port_d);
    if (port_d == 9999) {
      sprintf(message, "%s", ip_udp_package + udp_offset + 8);
      printf("%s\n", message);
      break;
    }
  }

  return 0;
}
