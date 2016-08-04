#include "common.h"

static int sock;

void CloseSocket(void)
{
  close(sock);
}

int main(void)
{
  char message[MESSAGE_SIZE];
  struct sockaddr_in server_address,
                     client_address;
  socklen_t client_length;

  /*  Create udp socket. */
  sock = CreateSocket(UDP_SOCK);
  atexit(CloseSocket);

  /*  Fiiling server address. */
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(SERVER_PORT);
  server_address.sin_addr.s_addr = inet_addr("192.168.2.44");

  client_length = sizeof(struct sockaddr_in);
  if (bind(sock, (struct sockaddr *) &server_address,
           sizeof(struct sockaddr)) < 0) {
    perror("bind error");
    exit(1);
  }
  /*  Get message "Hello!". */
  if (recvfrom(sock, message, MESSAGE_SIZE, 0,
               (struct sockaddr *) &client_address, &client_length) < 0) {
    perror("recvfrom error");
    exit(1);
  }
  printf("Got message: %s\n", message);

  sprintf(message, "Hi!");
  client_address.sin_port = htons(9999);
  if (sendto(sock, message, MESSAGE_SIZE, 0,
             (struct sockaddr *) &client_address, client_length) < 0) {
    perror("sendto error");
    exit(1);
  }

  return 0;
}
