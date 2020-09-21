#include <err.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <queue>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define PORT 80
#define THREADS 4

using namespace std;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_cond = PTHREAD_COND_INITIALIZER;
sem_t freeWorkers;
queue<int> requests;
int logFlag = 0;
int offset = 0;
char *logName;

void *connection(void *arg) {
  while (1) {
    // Put thread to sleep when inititalized
    pthread_mutex_lock(&mutex);
    if (requests.size() == 0) {
      pthread_cond_wait(&condition_cond, &mutex);
    }

    // Critical section, popping request off of global requests queue
    int requestSock = requests.front();
    if (requests.empty()) {
      pthread_mutex_unlock(&mutex);
      sem_post(&freeWorkers);
      continue;
    }
    requests.pop();
    pthread_mutex_unlock(&mutex);
    // End of critical section

    // Asgn1 code copy and pasted for the rest of this function
    // Read HTTP header
    char buffer[1024];
    read(requestSock, buffer, 1024);

    char file_name[27];
    char request[4];

    sscanf(buffer, "%s %s", request, file_name);
    // Remove the beginning '/' of the file name
    char *ps = file_name;
    if (ps[0] == '/') {
      ps++;
    }

    // Check if name is valid
    if (strlen(ps) != 27) {
      // Log this request if requested
      if (logFlag == 1) {
        // Open log file and write log message
        int lFile = open(logName, O_RDWR);
        char buff[100];
        int logContent;
        logContent = sprintf(
            buff, "FAIL: GET %s HTTP/1.1 --- response 400\n========\n", ps);

        // Get size of log message to calculate offset
        int messSize = snprintf(
            NULL, 0, "FAIL: GET %s HTTP/1.1 --- response 400\n========\n", ps);

        // Get offset and adjust for other threads
        pthread_mutex_lock(&mutex);
        int threadBuff = offset;
        offset += messSize;
        pthread_mutex_unlock(&mutex);

        // Actually write it into log file
        pwrite(lFile, buff, strlen(buff), threadBuff);
      }
      char errMsg[27] = "HTTP/1.1 400 Bad Request\r\n";
      send(requestSock, errMsg, strlen(errMsg), 0);
      close(requestSock);
      sem_post(&freeWorkers);
      continue;
    }
    for (int i = 0; i < strlen(ps); i++) {
      if (!((ps[i] >= 'a' && ps[i] <= 'z') || (ps[i] >= 'A' && ps[i] <= 'Z') ||
            (ps[i] >= '0' && ps[i] <= '9') || ps[i] == '-' || ps[i] == '_')) {
        // Log this request if requested
        if (logFlag == 1) {
          // Open log file and write log message
          int lFile = open(logName, O_RDWR);
          char buff[100];
          int logContent;
          logContent = sprintf(
              buff, "FAIL: GET %s HTTP/1.1 --- response 400\n========\n", ps);

          // Get size of log message to calculate offset
          int messSize = snprintf(
              NULL, 0, "FAIL: GET %s HTTP/1.1 --- response 400\n========\n",
              ps);

          // Get offset and adjust for other threads
          pthread_mutex_lock(&mutex);
          int threadBuff = offset;
          offset += messSize;
          pthread_mutex_unlock(&mutex);

          // Actually write it into log file
          pwrite(lFile, buff, strlen(buff), threadBuff);
        }
        char errMsg[27] = "HTTP/1.1 400 Bad Request\r\n";
        send(requestSock, errMsg, strlen(errMsg), 0);
        close(requestSock);
        sem_post(&freeWorkers);
        continue;
      }
    }

    // Check if request is GET
    int result = strcmp(request, "GET");
    if (result == 0) {
      char text_buffer[16000];

      // Check if file exists
      if (access(ps, F_OK) != -1) {
        // Check if file is readable
        if (access(ps, R_OK) != 0) {
          // Log this request if requested
          if (logFlag == 1) {
            // Open log file and write log message
            int lFile = open(logName, O_RDWR);
            char buff[100];
            int logContent;
            logContent = sprintf(
                buff, "FAIL: GET %s HTTP/1.1 --- response 403\n========\n", ps);

            // Get size of log message to calculate offset
            int messSize = snprintf(
                NULL, 0, "FAIL: GET %s HTTP/1.1 --- response 403\n========\n",
                ps);

            // Get offset and adjust for other threads
            pthread_mutex_lock(&mutex);
            int threadBuff = offset;
            offset += messSize;
            pthread_mutex_unlock(&mutex);

            // Actually write it into log file
            pwrite(lFile, buff, strlen(buff), threadBuff);
          }
          char errMsg[25] = "HTTP/1.1 403 Forbidden\r\n";
          send(requestSock, errMsg, strlen(errMsg), 0);
          close(requestSock);
          sem_post(&freeWorkers);
          continue;
        }
        // Open current argument and get size
        int file = open(ps, O_RDWR);
        struct stat fileStat;
        stat(ps, &fileStat);
        int sizeOfFile = fileStat.st_size;

        // Send size of content to client
        char contentBuff[100];
        int content_length;

        content_length = sprintf(
            contentBuff, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n",
            sizeOfFile);
        send(requestSock, contentBuff, strlen(contentBuff), 0);

        // Log this request if requested
        if (logFlag == 1) {
          // Open log file and write log message
          int lFile = open(logName, O_RDWR);
          char buff[50];
          int logContent;
          logContent =
              sprintf(buff, "GET %s length %d\n========\n", ps, sizeOfFile);

          // Get size of log message to calculate offset
          int messSize =
              snprintf(NULL, 0, "GET %s length %d\n========\n", ps, sizeOfFile);

          // Get offset and adjust for other threads
          pthread_mutex_lock(&mutex);
          int threadBuff = offset;
          offset += messSize;
          pthread_mutex_unlock(&mutex);

          // Actually write it into log file
          pwrite(lFile, buff, strlen(buff), threadBuff);
        }

        // Read and then write the file, 32kb at a time until end of file
        do {
          if (sizeOfFile < sizeof text_buffer) {
            read(file, text_buffer, sizeOfFile);
            send(requestSock, text_buffer, sizeOfFile, 0);
          } else {
            read(file, text_buffer, sizeof text_buffer);
            send(requestSock, text_buffer, sizeOfFile, 0);
          }
          sizeOfFile -= sizeof text_buffer;
        } while (sizeOfFile > 0);
        close(file);
        close(requestSock);
        sem_post(&freeWorkers);
        continue;
      } else if (access(ps, F_OK) == -1) {
        // Log this request if requested
        if (logFlag == 1) {
          // Open log file and write log message
          int lFile = open(logName, O_RDWR);
          char buff[100];
          int logContent;
          logContent = sprintf(
              buff, "FAIL: GET %s HTTP/1.1 --- response 404\n========\n", ps);

          // Get size of log message to calculate offset
          int messSize = snprintf(
              NULL, 0, "FAIL: GET %s HTTP/1.1 --- response 404\n========\n",
              ps);

          // Get offset and adjust for other threads
          pthread_mutex_lock(&mutex);
          int threadBuff = offset;
          offset += messSize;
          pthread_mutex_unlock(&mutex);

          // Actually write it into log file
          pwrite(lFile, buff, strlen(buff), threadBuff);
        }
        char errMsg[25] = "HTTP/1.1 404 Not Found\r\n";
        send(requestSock, errMsg, strlen(errMsg), 0);
        close(requestSock);
        sem_post(&freeWorkers);
        continue;
      }

    } else if (strcmp(request, "PUT") == 0) {
      // Request is PUT, so get content-length
      char contentLenAsChar[20];
      char *temp;
      temp = strstr(buffer, "Content-Length:");
      sscanf(temp, "%s %s", contentLenAsChar, contentLenAsChar);
      int contentLen = atoi(contentLenAsChar);

      // Deal with small or large files
      char readBuff[16000];
      if (contentLen <= sizeof readBuff) {
        recv(requestSock, readBuff, contentLen, 0);
      } else {
        do {
          if (contentLen < sizeof readBuff) {
            recv(requestSock, readBuff, contentLen, 0);
          } else {
            recv(requestSock, readBuff, sizeof readBuff, 0);
          }
          contentLen -= sizeof readBuff;
        } while (contentLen > 0);
      }

      // Check if file already exists
      if (access(ps, F_OK) == 0) {
        // Check if file is writable
        if (access(ps, W_OK) != 0) {
          // Log this request if requested
          if (logFlag == 1) {
            // Open log file and write log message
            int lFile = open(logName, O_RDWR);
            char buff[100];
            int logContent;
            logContent = sprintf(
                buff, "FAIL: GET %s HTTP/1.1 --- response 403\n========\n", ps);

            // Get size of log message to calculate offset
            int messSize = snprintf(
                NULL, 0, "FAIL: GET %s HTTP/1.1 --- response 403\n========\n",
                ps);

            // Get offset and adjust for other threads
            pthread_mutex_lock(&mutex);
            int threadBuff = offset;
            offset += messSize;
            pthread_mutex_unlock(&mutex);

            // Actually write it into log file
            pwrite(lFile, buff, strlen(buff), threadBuff);
          }
          char errMsg[25] = "HTTP/1.1 403 Forbidden\r\n";
          send(requestSock, errMsg, strlen(errMsg), 0);
          close(requestSock);
          sem_post(&freeWorkers);
          continue;
        }
        // overwrite the file
        int file = open(ps, O_RDWR | O_TRUNC);
        write(file, readBuff, contentLen);

        // Log this request if requested
        if (logFlag == 1) {
          // Open log file
          int lFile = open(logName, O_RDWR);
          char hex[100];

          // Convert content to hex
          for (int i = 0, j = 0; i < contentLen; ++i, j += 2) {
            sprintf(hex + j, "%02x", readBuff[i] & 0xff);
          }
          char buff[10000];
          int logContent;
          logContent = sprintf(buff, "PUT %s length %d\n%s\n========\n", ps,
                               contentLen, hex);
          int messSize = snprintf(NULL, 0, "PUT %s length %d\n%s\n========\n",
                                  ps, contentLen, hex);

          // Get offset and adjust for other threads
          pthread_mutex_lock(&mutex);
          int threadBuff = offset;
          offset += messSize;
          pthread_mutex_unlock(&mutex);

          // Write to log file
          pwrite(lFile, buff, strlen(buff), threadBuff);
        }
        char header[39] = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        send(requestSock, header, strlen(header), 0);
        close(requestSock);
        sem_post(&freeWorkers);
        continue;
      } else {
        // Create file and write content into it
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        int file = creat(ps, mode);
        write(file, readBuff, contentLen);

        // Log this request if requested
        if (logFlag == 1) {
          // Open log file
          int lFile = open(logName, O_RDWR);
          char hex[100];

          // Convert content to hex
          for (int i = 0, j = 0; i < contentLen; ++i, j += 2) {
            sprintf(hex + j, "%02x", readBuff[i] & 0xff);
          }
          char buff[10000];
          int logContent;
          logContent = sprintf(buff, "PUT %s length %d\n%s\n========\n", ps,
                               contentLen, hex);
          int messSize = snprintf(NULL, 0, "PUT %s length %d\n%s\n========\n",
                                  ps, contentLen, hex);

          // Get offset and adjust for other threads
          pthread_mutex_lock(&mutex);
          int threadBuff = offset;
          offset += messSize;
          pthread_mutex_unlock(&mutex);

          // Write to log file
          pwrite(lFile, buff, strlen(buff), threadBuff);
        }
        char header[44] = "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n";
        send(requestSock, header, strlen(header), 0);
        close(requestSock);
        sem_post(&freeWorkers);
        continue;
      }

    } else {
      // Not 'GET' or 'PUT'
      // Log this request if requested
      if (logFlag == 1) {
        // Open log file and write log message
        int lFile = open(logName, O_RDWR);
        char buff[100];
        int logContent;
        logContent = sprintf(
            buff, "FAIL: GET %s HTTP/1.1 --- response 500\n========\n", ps);

        // Get size of log message to calculate offset
        int messSize = snprintf(
            NULL, 0, "FAIL: GET %s HTTP/1.1 --- response 500\n========\n", ps);

        // Get offset and adjust for other threads
        pthread_mutex_lock(&mutex);
        int threadBuff = offset;
        offset += messSize;
        pthread_mutex_unlock(&mutex);

        // Actually write it into log file
        pwrite(lFile, buff, strlen(buff), threadBuff);
      }
      char header[39] = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
      send(requestSock, header, strlen(header), 0);
      close(requestSock);
      sem_post(&freeWorkers);
      continue;
    }
    // Thread is free again
    sem_post(&freeWorkers);
  }
}

int main(int argc, char *argv[]) {
  // Create socket
  int server = socket(AF_INET, SOCK_STREAM, 0);
  int opt = 1;
  setsockopt(server, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
             sizeof(opt));
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;

  //-------------------------------------------------------------------------
  // Parse command line arguments
  // ./httpserver -N 6 -l logfile.txt localhost 8080

  int opt2;
  int threadNum = -1;
  int new_port = -1;
  int logFile;

  while ((opt2 = getopt(argc, argv, "l:N:p:")) != -1) {
    switch (opt2) {
    case 'l':
      // Set log file name
      logName = optarg;
      logFlag = 1;

      logFile = creat(logName, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      break;
    case 'N':
      // Set # of threads
      threadNum = atoi(optarg);
      break;
    case 'p':
      // Set port
      new_port = atoi(optarg);
      break;
    }
  }

  //-------------------------------------------------------------------------

  // Thread creation, default is 4
  if (threadNum <= 0) {
    threadNum = THREADS;
  }

  sem_init(&freeWorkers, 0, threadNum);
  pthread_t thread_id[threadNum];

  for (int i = 0; i < threadNum; i++) {
    pthread_create(&thread_id[i], NULL, connection, NULL);
  }

  //-------------------------------------------------------------------------

  // Setting port #
  if (new_port > 0) {
    address.sin_port = htons(new_port);
  } else {
    address.sin_port = htons(PORT);
  }

  // Attach socket to port
  bind(server, (struct sockaddr *)&address, sizeof(address));
  listen(server, 3);
  int addLen = sizeof(address);
  int sock = accept(server, (struct sockaddr *)&address, (socklen_t *)&addLen);

  do {
    requests.push(sock);
    int len = requests.size();
    // Assign a thread to do a request, if there are none available
    // then we will wait
    if (!requests.empty()) {
      sem_wait(&freeWorkers);
      pthread_cond_signal(&condition_cond);
    }

  } while ((sock = accept(server, (struct sockaddr *)&address,
                          (socklen_t *)&addLen)) > 0);
}