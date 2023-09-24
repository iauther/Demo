#include "myAmqp.h"

#ifdef USE_AMQP_CPP
#pragma comment(lib, "libuv.lib")
#pragma comment(lib, "libcrypto.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "amqpcpp.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Userenv.lib")


using namespace AMQP;

#define LOG_BUF_LEN   1000

class myHandler : public LibUvHandler
{
public:
    virtual void onData(Connection *connection, const char *data, size_t size)
    {
        
    }

    virtual void onReady(Connection *connection)
    {
        
    }

    virtual void onError(Connection *connection, const char *message)
    {
        
    }

    
    virtual void onClosed(Connection *connection)
    {
        
    }

    virtual void onConnected(TcpConnection* connection) override
    {
        
    }

    myHandler(uv_loop_t* loop) : LibUvHandler(loop) {}

    virtual ~myHandler() = default;

private:
    
};
static DWORD amqpThread(LPVOID lpParam)
{
    uv_loop_t* loop = (uv_loop_t*)lpParam;

    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
#elif defined USE_RABBITMQ_C
//https://code84.com/808395.html

#pragma comment(lib, "librabbitmq.lib")
//#pragma comment(lib, "D:/programs/OpenSSL-Win64/lib/VC/libssl64MDd.lib")
//#pragma comment(lib, "D:/programs/OpenSSL-Win64/lib/VC/libcrypto64MDd.lib")


static void error_exit(int x, char const* context) {
    if (x < 0) {
        LOGE("%s: %s\n", context, amqp_error_string2(x));
        exit(1);
    }
}

static void amqp_error_exit(amqp_rpc_reply_t x, char const* context) {
    switch (x.reply_type) {
    case AMQP_RESPONSE_NORMAL:
        return;

    case AMQP_RESPONSE_NONE:
        LOGE("%s: missing RPC reply type!\n", context);
        break;

    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
        LOGE("%s: %s\n", context, amqp_error_string2(x.library_error));
        break;

    case AMQP_RESPONSE_SERVER_EXCEPTION:
        switch (x.reply.id) {
        case AMQP_CONNECTION_CLOSE_METHOD: {
            amqp_connection_close_t* m =
                (amqp_connection_close_t*)x.reply.decoded;
            LOGE("%s: server connection error %uh, message: %.*s\n",
                context, m->reply_code, (int)m->reply_text.len,
                (char*)m->reply_text.bytes);
            break;
        }
        case AMQP_CHANNEL_CLOSE_METHOD: {
            amqp_channel_close_t* m = (amqp_channel_close_t*)x.reply.decoded;
            LOGE("%s: server channel error %uh, message: %.*s\n",
                context, m->reply_code, (int)m->reply_text.len,
                (char*)m->reply_text.bytes);
            break;
        }
        default:
            LOGE("%s: unknown server error, method id 0x%08X\n",
                context, x.reply.id);
            break;
        }
        break;
    }

    exit(1);
}

#define SUMMARY_EVERY_US 1000000
static uint64_t now_microseconds(void) {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    return (((uint64_t)ft.dwHighDateTime << 32) | (uint64_t)ft.dwLowDateTime) /
        10;
}
static DWORD amqpThread(LPVOID lpParam)
{
    uint64_t start_time = now_microseconds();
    int received = 0;
    int previous_received = 0;
    uint64_t previous_report_time = start_time;
    uint64_t next_summary_time = start_time + SUMMARY_EVERY_US;
    amqp_connection_state_t hconn = (amqp_connection_state_t)lpParam;

    amqp_frame_t frame;
    uint64_t now;
    amqp_rpc_reply_t ret;
    amqp_envelope_t envelope;

    while (1) {
        now = now_microseconds();
        if (now > next_summary_time) {
            int countOverInterval = received - previous_received;
            double intervalRate =
                countOverInterval / ((now - previous_report_time) / 1000000.0);
            LOGD("%d ms: Received %d - %d since last report (%d Hz)\n",
                (int)(now - start_time) / 1000, received, countOverInterval,
                (int)intervalRate);

            previous_received = received;
            previous_report_time = now;
            next_summary_time += SUMMARY_EVERY_US;
        }

        amqp_maybe_release_buffers(hconn);
        ret = amqp_consume_message(hconn, &envelope, NULL, 0);

        if (AMQP_RESPONSE_NORMAL != ret.reply_type) {
            if (AMQP_RESPONSE_LIBRARY_EXCEPTION == ret.reply_type &&
                AMQP_STATUS_UNEXPECTED_STATE == ret.library_error) {
                if (AMQP_STATUS_OK != amqp_simple_wait_frame(hconn, &frame)) {
                    return 1;
                }

                if (AMQP_FRAME_METHOD == frame.frame_type) {
                    switch (frame.payload.method.id) {
                    case AMQP_BASIC_ACK_METHOD:
                        /* if we've turned publisher confirms on, and we've published a
                         * message here is a message being confirmed.
                         */
                        break;
                    case AMQP_BASIC_RETURN_METHOD:
                        /* if a published message couldn't be routed and the mandatory
                         * flag was set this is what would be returned. The message then
                         * needs to be read.
                         */
                    {
                        amqp_message_t message;
                        ret = amqp_read_message(hconn, frame.channel, &message, 0);
                        if (AMQP_RESPONSE_NORMAL != ret.reply_type) {
                            return 1;
                        }

                        amqp_destroy_message(&message);
                    }

                    break;

                    case AMQP_CHANNEL_CLOSE_METHOD:
                        /* a channel.close method happens when a channel exception occurs,
                         * this can happen by publishing to an exchange that doesn't exist
                         * for example.
                         *
                         * In this case you would need to open another channel redeclare
                         * any queues that were declared auto-delete, and restart any
                         * consumers that were attached to the previous channel.
                         */
                        return 1;

                    case AMQP_CONNECTION_CLOSE_METHOD:
                        /* a connection.close method happens when a connection exception
                         * occurs, this can happen by trying to use a channel that isn't
                         * open for example.
                         *
                         * In this case the whole connection must be restarted.
                         */
                        return 1;

                    default:
                        LOGE("An unexpected method was received %u\n",
                            frame.payload.method.id);
                        return 1;
                    }
                }
            }

        }
        else {
            amqp_destroy_envelope(&envelope);
        }

        received++;
    }

    return 0;
}
#elif defined USE_AZURE_UMAQP_C
static DWORD amqpThread(LPVOID lpParam)
{
    return 0;
}


#endif


int myAmqp::conn(const char *host, int port)
{
    void* param = NULL;

#ifdef USE_AMQP_CPP
    uv_loop_t* loop = uv_default_loop();

#if 1
    myHandler handler(loop);
    //Address address("amqp://guest:guest@localhost/vhost");

    // create a AMQP connection object
    //TcpConnection  connection(&handler, Address(host));
    TcpConnection  connection(&handler, "");

    // and create a channel
    TcpChannel channel(&connection);

    // use the channel object to call the AMQP method you like
    //channel.declareExchange("my-exchange", fanout);
    channel.declareQueue("my-queue");
    //channel.bindQueue("my-exchange", "my-queue", "my-routing-key");

    param = loop;
#endif

#elif defined USE_RABBITMQ_C
    int status;
    amqp_rpc_reply_t reply;
    char const* exchange = "amq.direct";
    char const* bindingkey = "test queue";

    hconn = amqp_new_connection();

    socket = amqp_tcp_socket_new(hconn);
    if (!socket) {
        return -1;
    }

    status = amqp_socket_open(socket, host, AMQP_PROTOCOL_PORT);
    if (status) {
        LOGE("opening TCP socket failed");
        return -1;
    }

    reply = amqp_login(hconn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest");
    amqp_error_exit(reply, "login");

    amqp_channel_open(hconn, 1);
    amqp_error_exit(amqp_get_rpc_reply(hconn), "open channel");

    {
        amqp_queue_declare_ok_t* r = amqp_queue_declare(
            hconn, 1, amqp_empty_bytes, 0, 0, 0, 1, amqp_empty_table);
        amqp_error_exit(amqp_get_rpc_reply(hconn), "declare queue");
        queuename = amqp_bytes_malloc_dup(r->queue);
        if (queuename.bytes == NULL) {
            LOGE("Out of memory while copying queue name");
            return 1;
        }
    }

    amqp_queue_bind(hconn, 1, queuename, amqp_cstring_bytes(exchange),
        amqp_cstring_bytes(bindingkey), amqp_empty_table);
    amqp_error_exit(amqp_get_rpc_reply(hconn), "bind queue");

    amqp_basic_consume(hconn, 1, queuename, amqp_empty_bytes, 0, 1, 0,
        amqp_empty_table);
    amqp_error_exit(amqp_get_rpc_reply(hconn), "consume");

    param = hconn;
#elif defined USE_AZURE_UMAQP_C


#endif

    hThread = CreateThread(NULL, NULL, amqpThread, param, 0, NULL);

    return 0;
}


int myAmqp::disconn(void)
{
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

#ifdef USE_AMQP_CPP
    //channel.cancel()
    //channel.removeExchange
    //channel.unbindExchange
    //channel.unbindQueue
    //channel.removeQueue
    //channel.close();
    //connection.close();
#elif defined USE_RABBITMQ_C
    
    amqp_bytes_free(queuename);
    amqp_error_exit(amqp_channel_close(hconn, 1, AMQP_REPLY_SUCCESS),
        "Closing channel");
    amqp_error_exit(amqp_connection_close(hconn, AMQP_REPLY_SUCCESS),
        "Closing connection");
    error_exit(amqp_destroy_connection(hconn), "Ending connection");
#elif defined USE_AZURE_UMAQP_C


#endif

    return 0;
}


int myAmqp::read(void *data, int len)
{
#ifdef USE_AMQP_CPP
    TcpChannel channel(0);

    auto startCb = [](const std::string& consumertag) {

        //std::cout << "consume operation started" << std::endl;
    };

    // callback function that is called when the consume operation failed
    auto errorCb = [](const char* message) {

        //std::cout << "consume operation failed" << std::endl;
    };

    // callback operation when a message was received
    auto messageCb = [&channel](const AMQP::Message& message, uint64_t deliveryTag, bool redelivered) {

        //std::cout << "message received" << std::endl;

        // acknowledge the message
        //channel.ack(deliveryTag);
    };

    // start consuming from the queue, and install the callbacks
    channel.consume("my-queue")
        .onReceived(messageCb)
        .onSuccess(startCb)
        .onError(errorCb);
#elif defined USE_RABBITMQ_C

#elif defined USE_AZURE_UMAQP_C


#endif

    return 0;
}


int myAmqp::write(void *data, int len)
{
#ifdef USE_AMQP_CPP
    TcpChannel channel(0);

    channel.startTransaction();

    // publish a number of messages
    channel.publish("my-exchange", "my-key", "message");

    // commit the transactions, and set up callbacks that are called when
    // the transaction was successful or not
    channel.commitTransaction()
    .onSuccess([]() {
        // all messages were successfully published
    })
    .onError([](const char *message) {
        // none of the messages were published
        // now we have to do it all over again
    });
#elif defined USE_RABBITMQ_C
    amqp_bytes_t mb;

    mb.len = len;
    mb.bytes = data;
    error_exit(amqp_basic_publish(hconn, 1, amqp_cstring_bytes("amq.direct"),
        amqp_cstring_bytes("pub_queuen"), 0, 0, NULL, mb), "publish");

    return len;
#elif defined USE_AZURE_UMAQP_C


#endif
    
    return 0;
}


