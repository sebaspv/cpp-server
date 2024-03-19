#include <cstdlib>
#include <vector>
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

int findtext(const std::string& pat, const std::string& txt){
    int M = pat.size();
    int N = txt.size();
    int lastindex = -1;
    bool found = false;
    for (int i = 0; i <= N - M; i++) {
        int j;
        for (j = 0; j < M; j++)
            if (txt[i + j] != pat[j])
                break;
 
        if (j == M){
          lastindex = i;
        }
    }
    return lastindex;
}

int main(int argc, char **argv) {
  std::cout << "Logs from your program will appear here!\n";

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }

  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);

  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }

  struct sockaddr_in client_addr;

  socklen_t client_addr_len = sizeof(client_addr);

  bind(server_fd, (const struct sockaddr *) &client_addr, client_addr_len);
  listen(server_fd, connection_backlog);
  int connection = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  
  char req[4096];
  int bytes = recv(connection, req, 4096, 0);
  std::string request(req, bytes);
  int spacecount = 0;
  std::string route;
  for (auto letter: request){
    if (letter == ' '){
      spacecount++;
    }
    if (spacecount == 1){
      route.push_back(letter);
    }
    else if (spacecount == 2){
      break;
    }
  }
  std::string content;
  int slashcount = 0;
  for (auto letter: route){
    if (letter == '/'){
      slashcount++;
      continue;
    }
    if (slashcount == 1){
      content.push_back(letter);
    }
  }

  if (route.size() < 6 && route != " /"){
    const char* msg = "HTTP/1.1 404 Not Found\r\n\r\n";
    send(connection, msg, strlen(msg), 0);
  }
  else {
    std::string echoroute(route.begin(), route.begin() + 6);
    int agent = findtext("GET /user-agent", request);
    if (echoroute == " /echo"){
      std::string afterroute(route.begin() + 7, route.end());
      std::string msg = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(afterroute.size()) + "\r\n\r\n" + afterroute;
      const char * response = msg.c_str();
      send(connection, response, strlen(response), 0);
    }
    else if (agent != -1){
      std::string searching = "User-Agent: ";
      int tosearch = searching.size();
      int curlcontent = findtext(searching, request);
      std::string agentheader(request.begin() + curlcontent + tosearch, request.end());
      std::string finroute;
      for (auto aa: agentheader){
        if (aa == '\r'){
          break;
        }
        else {
          finroute.push_back(aa);
        }
      }
      std::string msg = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(finroute.size()) + "\r\n\r\n" + finroute;
      const char * response = msg.c_str();
      send(connection, response, strlen(response), 0);
    }
    else if (route == " /"){
      std::string msg = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(content.size()) + "\r\n\r\n" + content;
      const char * response = msg.c_str();
      send(connection, response, strlen(response), 0);
    }
    else {
      const char* msg = "HTTP/1.1 404 Not Found\r\n\r\n";
      send(connection, msg, strlen(msg), 0);
    }
  }
  
  close(server_fd);

  return 0;
}
