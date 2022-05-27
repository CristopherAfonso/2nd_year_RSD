//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//              This class processes an FTP transaction.
// 
//****************************************************************************

#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h> 
#include <iostream>
#include <dirent.h>

#include "common.h"

#include "ClientConnection.h"

// we declare this function here because the command "PASV" need it and 
// the function define_socket_TCP(int port) is implemented in FTPServer.cpp
int define_socket_TCP(int port);

ClientConnection::ClientConnection(int s) {
  int sock = (int)(s);
  char buffer[MAX_BUFF];
  control_socket = s;
  fd = fdopen(s, "a+"); // Check the Linux man pages to know what fdopen does.
  if (fd == NULL){
	  std::cout << "Connection closed" << std::endl;
  	fclose(fd);
	  close(control_socket);
	  ok = false;
	  return ;
  }
  ok = true;
  data_socket = -1;
  parar = false;  
};

ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket); 
}

int connect_TCP(uint32_t address, uint16_t port) {
  // Implement your code to define a socket here
  struct sockaddr_in sin;
  struct hostent *hent;
  int s; // this will be our socket descriptor

  memset(&sin, 0, sizeof(sin)); // we clean the struct sin
  sin.sin_family = AF_INET; // our socket family will be the internet protocols
  sin.sin_port = htons(port); // we convert port to host format from net format

  // we give the address client to struct sin, address could be directly a IP address
  sin.sin_addr.s_addr = address;

  // we convert address to char* to be given to function gethostbyname()
  char const *ptr_char_address = (std::to_string(address)).c_str();
  
  // we store the IP address of client with the DNS protocol
  hent = gethostbyname(ptr_char_address);

  if (hent) // if hent != NULL we copy the IP address in hent->h_addr to the struct sin
    memcpy(&sin.sin_addr, hent->h_addr, hent->h_length);
  else if ((sin.sin_addr.s_addr = inet_addr((char*)ptr_char_address)) == INADDR_NONE)
    errexit("Cannot solve the name \"%s\"\n", ptr_char_address);

  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0)
    errexit("Cannot create the socket: %s\n", strerror(errno));
  
  if (connect(s, (struct sockaddr*)&sin, sizeof(sin)) < 0)
    errexit("Cannot connect with %s: %s\n", address, strerror(errno));

  return s; // You must return the socket descriptor
}

void ClientConnection::stop() {
  close(data_socket);
  close(control_socket);
  parar = true;
}

#define COMMAND(cmd) strcmp(command, cmd)==0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You 
// are allowed to add auxiliary methods if necessary.
void ClientConnection::WaitForRequests() {
  if (!ok) {
	  return;
  }
  fprintf(fd, "220 Service ready\n");
  while (!parar) {
    fscanf(fd, "%s", command);
    if (COMMAND("USER")) {
	    fscanf(fd, "%s", arg);
	    fprintf(fd, "331 User name ok, need password\n");
    }
    else if (COMMAND("PWD")) {
	    
    }
    else if (COMMAND("PASS")) {
      fscanf(fd, "%s", arg);
      if(strcmp(arg,"1234") == 0){
        fprintf(fd, "230 User logged in\n");
      }
      else{
        fprintf(fd, "530 Not logged in.\n");
        parar = true;
      }
    }
    else if (COMMAND("PORT")) { // To be implemented by students
      // we create the necessary variables to store the data linked with
      // IP address client and the ports used for the program
	    int h1{0};
      int h2{0};
      int h3{0};
      int h4{0};
      int p1{0};
      int p2{0};
      fscanf(fd, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
      // we inicialize the variables necessary to create a socket for a client
      uint32_t address_client = (h1 << 24) | (h2 << 16) | (h3 << 8) | h4;
      uint16_t port_client = (p1 << 8) | p2;

      data_socket = connect_TCP(address_client, port_client);
      if (data_socket < 0)
        fprintf(fd, "421 Service not avalaible, closing control connection\n");
      else fprintf(fd, "200 Command okay.\n");
    }
    else if (COMMAND("PASV")) { // To be implemented by students
      int s{-1}; // this is our socket in this command
      int soc_name{-1};
      struct sockaddr_in address;
      socklen_t address_len{sizeof(address)};
      uint16_t port = address.sin_port; // we get the ports used for the communication
      uint8_t p1 = port;
      uint8_t p2 = port >> 8;

      // if the argument of define_socket_TCP is zero, the S.O will set a
      // random socket descriptor
      s = define_socket_TCP(0);
      // returns the current address to which the socket s is bound
      soc_name = getsockname(s, reinterpret_cast<sockaddr*>(&address), &address_len);
      fprintf(fd, "227 Entering passive mode (127,0,0,1,%d,%d)\n", p1, p2);
      fflush(fd); // forces a write of all user-space buffered data for the given output
      data_socket = accept(s, reinterpret_cast<sockaddr*>(&address), &address_len);
    }
    else if (COMMAND("STOR") ) { // To be implemented by students
	    FILE* file = fopen(arg, "wb");

      if (file == nullptr) fprintf(fd, "File empty\n");
      else {
        int data_written{0};
        int bytes_received{0};
        char buffer[1024];

        fprintf(fd, "150 File status okay; about to open data conection\n");
        fflush(fd);
        do {
          bytes_received = recv(data_socket, buffer, sizeof(buffer), 0);
          // writes bytes_received items of data, each size of 1 bytes long,  to  the
          // stream pointed to by file, obtaining them from the location given by buffer.
          data_written = fwrite(buffer, 1, bytes_received, file);
          if (data_written <= 0) {
            fprintf(fd, "fatal error with function fwrite()\n");
            fflush(fd);
            break;
          }
        } while (bytes_received == 1024);
        fprintf(fd, "226 Closing data connection\n");
        fflush(fd);
        close(data_socket);
        fclose(file);
      }
    }
    else if (COMMAND("RETR")) { // To be implemented by students
	    FILE* file = fopen(arg, "r");

      fscanf(fd, "%s", arg);
      if (file == nullptr) fprintf(fd, "File empty\n");
      else {
        int data_read{0};
        char buffer[1024];

        fprintf(fd, "150 File status okay; about to open data connection\n");
        do {
          // reads sizeof(buffer) items of data, each 1 bytes long, from the
          // stream pointed to by file, storing them at the location given by buffer.
          data_read = fread(buffer, 1, sizeof(buffer), file);
          send(data_socket, buffer, data_read, 0);
        } while (data_read == 1024);
        fprintf(fd, "226 Closing data connection\n");
        close(data_socket);
        fclose(file);
      }
    }
    else if (COMMAND("LIST")) { // To be implemented by students
	  	char directory[1024];
      struct dirent *entry;
      DIR *mydir;
      FILE *data = fdopen(data_socket, "a+");

      getcwd(directory, sizeof(directory));
      mydir = opendir(directory);
      if (mydir == NULL) {
        fprintf(fd, "450 Requested file action not taken\n");
        fflush(fd);
        exit(EXIT_FAILURE);
      }
      fprintf(fd, "125 Data connection already open; transfer starting\n");
      fflush(fd);
      while ((entry = readdir(mydir)) != NULL) {
        if (entry->d_name[0] != '.') {
          fprintf(data, "%s \n", entry->d_name);
          fflush(data);
        }
      }
      fclose(data);
      fprintf(fd, "250 Request file action okay, completed\n");
      fflush(fd);
      closedir(mydir);
    }
    else if (COMMAND("SYST")) {
      fprintf(fd, "215 UNIX Type: L8.\n");   
    }
    else if (COMMAND("TYPE")) {
	    fscanf(fd, "%s", arg);
	    fprintf(fd, "200 OK\n");   
    }
    else if (COMMAND("QUIT")) {
      fprintf(fd, "221 Service closing control connection. Logged out if appropriate.\n");
      close(data_socket);	
      parar = true;
      break;
    }
    else  {
	    fprintf(fd, "502 Command not implemented.\n"); fflush(fd);
	    printf("Comando : %s %s\n", command, arg);
	    printf("Error interno del servidor\n");
    }
  }
  fclose(fd); 
  return;  
};
