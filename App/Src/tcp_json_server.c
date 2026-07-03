#include "tcp_json_server.h"

#include <stdio.h>
#include <string.h>

#include "cmsis_os2.h"
#include "fault_command.h"
#include "lwip/api.h"
#include "lwip/err.h"

#define TCP_JSON_SERVER_STACK_SIZE 2048U
#define TCP_JSON_SERVER_LINE_SIZE 256U

static osThreadId_t tcpJsonServerHandle;

typedef struct
{
  struct netconn *conn;
} TcpJsonServer_OutputContext;

static void TcpJsonServer_Write(const char *text, void *context)
{
  TcpJsonServer_OutputContext *output = (TcpJsonServer_OutputContext *)context;

  if ((output == NULL) || (output->conn == NULL) || (text == NULL))
  {
    return;
  }

  (void)netconn_write(output->conn, text, strlen(text), NETCONN_COPY);
}

static void TcpJsonServer_HandleLine(struct netconn *conn, const char *line)
{
  TcpJsonServer_OutputContext output = {conn};

  FaultCommand_ExecuteLine(line, TcpJsonServer_Write, &output);
}

static void TcpJsonServer_HandleClient(struct netconn *conn)
{
  struct netbuf *buf;
  char line[TCP_JSON_SERVER_LINE_SIZE];
  uint16_t lineLength = 0U;

  const char *hello =
    "{\"ok\":true,\"service\":\"bms-fault-simulator\",\"hint\":\"send one JSON command per line\"}\r\n";

  (void)netconn_write(conn, hello, strlen(hello), NETCONN_COPY);

  while (netconn_recv(conn, &buf) == ERR_OK)
  {
    void *data;
    u16_t len;

    do
    {
      netbuf_data(buf, &data, &len);

      for (u16_t index = 0U; index < len; index++)
      {
        char ch = ((char *)data)[index];

        if ((ch == '\r') || (ch == '\n'))
        {
          if (lineLength > 0U)
          {
            line[lineLength] = '\0';
            TcpJsonServer_HandleLine(conn, line);
            lineLength = 0U;
          }
          continue;
        }

        if (lineLength < (TCP_JSON_SERVER_LINE_SIZE - 1U))
        {
          line[lineLength++] = ch;
        }
        else
        {
          lineLength = 0U;
          (void)netconn_write(conn,
                              "{\"ok\":false,\"error\":\"line_too_long\"}\r\n",
                              38U,
                              NETCONN_COPY);
        }
      }
    } while (netbuf_next(buf) >= 0);

    netbuf_delete(buf);
  }
}

static void TcpJsonServer_Task(void *argument)
{
  struct netconn *server;
  err_t err;

  (void)argument;

  server = netconn_new(NETCONN_TCP);
  if (server == NULL)
  {
    printf("[tcp] netconn_new failed\r\n");
    osThreadExit();
  }

  err = netconn_bind(server, IP_ADDR_ANY, TCP_JSON_SERVER_PORT);
  if (err != ERR_OK)
  {
    printf("[tcp] bind %u failed err=%d\r\n", TCP_JSON_SERVER_PORT, err);
    netconn_delete(server);
    osThreadExit();
  }

  err = netconn_listen(server);
  if (err != ERR_OK)
  {
    printf("[tcp] listen failed err=%d\r\n", err);
    netconn_delete(server);
    osThreadExit();
  }

  printf("[tcp] JSON server listening on port %u\r\n", TCP_JSON_SERVER_PORT);

  for (;;)
  {
    struct netconn *client = NULL;

    err = netconn_accept(server, &client);
    if ((err == ERR_OK) && (client != NULL))
    {
      printf("[tcp] client connected\r\n");
      TcpJsonServer_HandleClient(client);
      netconn_close(client);
      netconn_delete(client);
      printf("[tcp] client disconnected\r\n");
    }
    else
    {
      osDelay(100U);
    }
  }
}

void TcpJsonServer_Start(void)
{
  static const osThreadAttr_t attributes = {
    .name = "TcpJson",
    .stack_size = TCP_JSON_SERVER_STACK_SIZE,
    .priority = (osPriority_t)osPriorityBelowNormal,
  };

  if (tcpJsonServerHandle == NULL)
  {
    tcpJsonServerHandle = osThreadNew(TcpJsonServer_Task, NULL, &attributes);
  }
}
