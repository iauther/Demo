/******************************************************************************
 * umqtt.c - MQTT packet client for microcontrollers.
 *
 * Copyright (c) 2016, Joseph Kroesche (tronics.kroesche.io)
 * All rights reserved.
 *
 * This software is released under the FreeBSD license, found in the
 * accompanying file LICENSE.txt and at the following URL:
 *      http://www.freebsd.org/copyright/freebsd-license.html
 *
 * This software is provided as-is and without warranty.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "umqtt.h"

/**
 *
 * @addtogroup umqtt_api uMQTT API
 * @{
 *
 * Functions you call to operate MQTT client
 * -----------------------------------------
 *
 * Function Name              | Description
 * ---------------------------|------------
 * umqtt_New()                | Create and initialize umqtt instance
 * umqtt_Delete()             | de-initialize umqtt instace (frees resources)
 * umqtt_Run()                | main run loop
 * umqtt_Connect()            | establish protocol connection to MQTT broker
 * umqtt_Disconnect()         | protocol disconnect from MQTT broker
 * umqtt_Publish()            | publish a topic
 * umqtt_Subscribe()          | subscribe to topic(s)
 * umqtt_Unsubscribe()        | unsubscribe from topic(s)
 * umqtt_GetErrorString()     | get string representation of error code
 * umqtt_GetConnectedStatus() | determine if connected
 *
 * The following are available but you don't need to call these directly,
 * they are called from umqtt_Run() when needed.
 *
 * Function Name              | Description
 * ---------------------------|------------
 * umqtt_PingReq()            | send ping request to MQTT broker
 * umqtt_DecodePacket()       | decode a MQTT packet and perform actions
 *
 * Functions you must implement
 * ----------------------------
 * These are populated into a @ref umqtt_TransportConfig_t structure and passed
 * to umqtt_New():
 *
 * Function Name      | Description
 * -------------------|------------
 * malloc_t()         | memory allocation for packets
 * free_t()           | free allocated memory
 * netReadPacket_t()  | read a packet from the network
 * netWritePacket_t() | write a packet to the network
 *
 * Optional functions to implement
 * -------------------------------
 * These callback functions are used to notify the application when certain
 * events occur.  These are populated into a @ref umqtt_Callbacks_t
 * structure and passed to umqtt_New().  Unimplemented callback functions
 * can be set to NULL.
 *
 * Function Name  | Description
 * ---------------|------------
 * ConnackCb_t()  | CONNACK received from broker to acknowledge a CONNECT
 * PublishCb_t()  | PUBLISH received from broker
 * PubackCb_t()   | PUBACK received from broker in response to PUBLISH with QoS != 0
 * SubackCb_t()   | SUBACK received in response to SUBSCRIBE
 * UnsubackCb_t() | UNSUBACK received in response to UNSUBSCRIBE
 * PingrespCb_t() | PINGRESP received in response to PINGREQ
 *
 * Run loop and timing
 * -------------------
 * You must call umqtt_Run() repeatedly from your main run loop.  It is not
 * required that timing be exact between calls.  You must maintain a
 * source of millisecond tick values (like a tick timer) and pass the
 * millisecond ticks to umqtt_Run() each time it is called.  This is how
 * `umqtt` keeps track of timing for timeouts and retries.
 *
 * Not thread safe
 * ---------------
 * These functions are not thread safe.  You musn't call them from different
 * threads unless you provide your own resource lock wrapper(s).
 *
 */

/*
 * MQTT packet types
 */
#define UMQTT_TYPE_CONNECT 1
#define UMQTT_TYPE_CONNACK 2
#define UMQTT_TYPE_PUBLISH 3
#define UMQTT_TYPE_PUBACK 4
#define UMQTT_TYPE_SUBSCRIBE 8
#define UMQTT_TYPE_SUBACK 9
#define UMQTT_TYPE_UNSUBSCRIBE 10
#define UMQTT_TYPE_UNSUBACK 11
#define UMQTT_TYPE_PINGREQ 12
#define UMQTT_TYPE_PINGRESP 13
#define UMQTT_TYPE_DISCONNECT 14

/*
 * MQTT fixed header flags
 */
#define UMQTT_FLAG_DUP 0x08
#define UMQTT_FLAG_RETAIN 0x01
#define UMQTT_FLAG_QOS 0x06
#define UMQTT_FLAG_QOS_SHIFT 1

/*
 * flags used in connect packet (different from fixed header flags)
 */
#define UMQTT_CONNECT_FLAG_USER 0x80
#define UMQTT_CONNECT_FLAG_PASS 0x40
#define UMQTT_CONNECT_FLAG_WILL_RETAIN 0x20
#define UMQTT_CONNECT_FLAG_WILL_QOS 0x18
#define UMQTT_CONNECT_FLAG_WILL 0x04
#define UMQTT_CONNECT_FLAG_CLEAN 0x02
#define UMQTT_CONNECT_FLAG_QOS_SHIFT 3

/*
 * Defines the retry timeout and number of retries before giving up.
 */
#define UMQTT_RETRY_TIMEOUT 5000
#define UMQTT_RETRIES 10

// error handling convenience
#define RETURN_IF_ERR(c,e) do{if(c){return (e);}}while(0)

/*
 * Provide names for all error codes for debug convenience.
 */
static const char *errCodeStrings[] =
{
    "UMQTT_ERR_OK",
    "UMQTT_ERR_PACKET_ERROR",
    "UMQTT_ERR_BUFSIZE",
    "UMQTT_ERR_PARM",
    "UMQTT_ERR_NETWORK",
    "UMQTT_ERR_CONNECT_PENDING",
    "UMQTT_ERR_CONNECTED",
    "UMQTT_ERR_DISCONNECTED",
    "UMQTT_ERR_TIMEOUT",
};

/*
 * Defines the interal packet buffer structure.  This is a packet header
 * that is used to track state and does not include the actual MQTT packet
 * and payload.  This structure is added to the front of the MQTT packet
 * and is not visible to the client program.
 */
typedef struct PktBuf
{
    struct PktBuf *next;    // next packet in a list
    uint16_t packetId;      // packet ID of this packet
    uint32_t ticks;         // ticks when this packet was last sent
    unsigned int ttl;       // time-to-live, remaining retries
} PktBuf_t;

/*
 * umqtt instance data structure.  This is allocated and populated when
 * the client calls "New"
 * @todo consider using the pending packet list to track ping requests
 * instead of keeping a separate field here
 */
typedef struct
{
    uint16_t packetId;      // last used packet ID on this instance
    void *pUser;            // caller supplied data pointer
    struct PktBuf pktList;  // pending packet list
    uint32_t ticks;         // ticks when run was last called
    uint32_t pingTicks;     // ticks when last ping request was sent
    bool isConnected;       // this client instance is protocol-connected
    bool connectIsPending;  // connect req was send but waiting for ack
    uint16_t keepAlive;     // keep alive interval in seconds
    umqtt_TransportConfig_t *pNet;  // network instance
    umqtt_Callbacks_t *pCb; // pointer to callbacks
} umqtt_Instance_t;


/*
 * @internal
 *
 * Allocate a new packet
 *
 * @param this umqtt instance
 * @param remainingLength the packet length not including header and
 * remaining length fields
 *
 * This function will allocate a new packet and return a pointer to a buffer
 * to use for assembling a MQTT packet.  It will add additional length
 * to account for the required header byte and up to 4 bytes for remaining
 * length.  It also adds the internal packet tracking structure to the
 * front of the MQTT packet but this is hidden from the returned pointer.
 *
 * @return pointer to uint8_t buffer of sufficient length for MQTT packet,
 * or NULL
 */
static uint8_t *
newPacket(const umqtt_Instance_t *this, size_t remainingLength)
{
    if (!this)
    {
        return NULL;
    }
    remainingLength += 1 + 4; // 1 hdr byte plus up to 4 len bytes
    uint8_t *buf = this->pNet->pfnmalloc(remainingLength + sizeof(PktBuf_t));
    if (buf)
    {
        buf += sizeof(PktBuf_t);
        return buf;
    }
    else
    {
        return NULL;
    }
}

/*
 * @internal
 *
 * Free a packet
 *
 * @param this umqtt instance
 * @param pbuf the MQTT packet that was allocated with newPacket()
 *
 * Frees a previously allocated MQTT packet.  Assumes this packet is
 * not in the pending list.  If it is, then things will go bad.
 */
static void
deletePacket(const umqtt_Instance_t *this, uint8_t *pbuf)
{
    if (pbuf && this)
    {
        pbuf -= sizeof(PktBuf_t);
        PktBuf_t *pkt = (PktBuf_t *)pbuf;
        pkt->next = NULL;
        this->pNet->pfnfree(pbuf);
    }
}

/*
 * @internal
 *
 * Add a packet to the pending packet list.
 *
 * @param this umqtt instance
 * @param pbuf MQTT packet buffer to add to the queue
 * @param packetId packet ID of this packet
 * @param ticks tick count when this packet is enqueued
 *
 * This function will add the MQTT packet to the list of pending packets.
 * The caller supplies the packet ID and the current tick count for the
 * packet.  These are saved with the packet to facilitate lookup later.
 */
static void
enqueuePacket(umqtt_Instance_t *this, uint8_t *pbuf, uint16_t packetId, uint32_t ticks)
{
    if (pbuf && this)
    {
        pbuf -= sizeof(PktBuf_t);
        PktBuf_t *pkt = (PktBuf_t *)pbuf;
        pkt->next = this->pktList.next;
        this->pktList.next = pkt;
        pkt->ticks = ticks;
        pkt->packetId = packetId;
        pkt->ttl = UMQTT_RETRIES;
    }
}

/*
 * @internal
 *
 * Remove a packet from the pending packet list, using the packet ID
 *
 * @param this umqtt instance
 * @param packetId the packet ID of the packet to remove
 *
 * Searches the pending packet list/queue for a packet with matching
 * packet ID.  If found, the packet is delinked from the list and returned
 * to the caller.
 *
 * @return Pointer to the dequeued packet or NULL.
 */
static uint8_t *
dequeuePacketById(umqtt_Instance_t *this, uint16_t packetId)
{
    if (!this)
    {
        return NULL;
    }
    PktBuf_t *pPrev = &this->pktList;
    PktBuf_t *pPkt = pPrev->next;
    while (pPkt)
    {
        if (packetId == pPkt->packetId)
        {
            pPrev->next = pPkt->next;
            pPkt->next = NULL;
            uint8_t *buf = (uint8_t *)pPkt;
            buf += sizeof(PktBuf_t);
            return buf;
        }
        pPrev = pPkt;
        pPkt = pPkt->next;
    }
    return NULL;
}

/*
 * @internal
 *
 * Remove a packet from the pending packet list, using the MQTT packet type
 *
 * @param this umqtt instance
 * @param type the MQTT packet type to remove
 *
 * Searches the pending packet list/queue for a packet with matching
 * packet type.  If found, the packet is delinked from the list and returned
 * to the caller.  If there is more than one packet with the same type, it
 * will only dequeue and return the first one that matches.
 *
 * @return Pointer to the dequeued packet or NULL.
 */
static uint8_t *
dequeuePacketByType(umqtt_Instance_t *this, uint8_t type)
{
    if (!this)
    {
        return NULL;
    }
    PktBuf_t *pPrev = &this->pktList;
    PktBuf_t *pPkt = pPrev->next;
    while (pPkt)
    {
        uint8_t *buf = (uint8_t *)pPkt;
        buf += sizeof(PktBuf_t);
        if ((type << 4) == buf[0])
        {
            pPrev->next = pPkt->next;
            pPkt->next = NULL;
            return buf;
        }
        pPrev = pPkt;
        pPkt = pPkt->next;
    }
    return NULL;
}

/*
 * @internal
 *
 * Removes and frees all packets from the pending packet list.
 *
 * @param this umqtt instance
 */
static void
freeAllQueuedPackets(umqtt_Instance_t *this)
{
    if (this)
    {
        PktBuf_t *pNext = this->pktList.next;
        while (pNext)
        {
            PktBuf_t *pPkt = pNext;
            pNext = pPkt->next;
            pPkt->next = NULL;
            this->pNet->pfnfree(pPkt);
        }
    }
}

/**
 * Get string representing an error code.
 *
 * @param err is the error code to decode
 *
 * This function is useful for debugging.  It can be used to print
 * a meaningful string for an error code.

 * @return a human readable string representation of the error code
 */
const char *
umqtt_GetErrorString(umqtt_Error_t err)
{
    return errCodeStrings[err];
}

/* @internal
 *
 * Encode length into MQTT remaining length format
 *
 * @param length length to encode
 * @param pEncodeBuf buffer to store encoded length
 *
 * Does not validate parameters.  Assumes length is valid
 * and buffer is large enough to hold encoded length.
 *
 * @return the count of bytes that hold the length
 */
static uint32_t
umqtt_EncodeLength(uint32_t length, uint8_t *pEncodeBuf)
{
    uint8_t encByte;
    uint32_t idx = 0;
    do
    {
        encByte = length % 128;
        length /= 128;
        if (length > 0)
        {
            encByte |= 0x80;
        }
        pEncodeBuf[idx] = encByte;
        ++idx;;
    } while (length > 0);
    return idx; // return count of bytes
}

/* @internal
 *
 * Decode remaining length field from MQTT packet.
 *
 * @param pLength storage to location of decoded length
 * @param pEncodedLength buffer holding the encoded length field
 *
 * The caller supplies storage for the decoded length through the
 * _pLength_ parameter.  The number of bytes used to hold the encoded
 * length in the packet is returned to the caller.  No parameter
 * validation is performed.
 *
 * @return count of bytes of the encoded length
 */
static uint32_t
umqtt_DecodeLength(uint32_t *pLength, const uint8_t *pEncodedLength)
{
    uint8_t encByte;
    uint32_t count = 0;
    uint32_t length = 0;
    uint32_t mult = 1;
    do
    {
        encByte = pEncodedLength[count];
        length += (encByte & 0x7F) * mult;
        ++count;
        mult *= 128;
    } while (encByte & 0x80);

    *pLength = length;
    return count;
}

/* @internal
 *
 * Encode a data block into an MQTT packet
 *
 * @param pInBuf pointer to buffer containing data to encode
 * @param inBufLen count of bytes in input buffer
 * @param pOutBuf pointer to a buffer where the data block will be encoded
 *
 * This function assumes that the buffer pointed at by _pOutBuf_ is large
 * enough to hold the encoded data block.  This function will take the data
 * and length provided and encode it into a buffer as an MQTT data block.
 *
 * @return count of bytes that were encoded
 */
static uint32_t
umqtt_EncodeData(const uint8_t *pInBuf, uint32_t inBufLen, uint8_t *pOutBuf)
{
    *pOutBuf++ = inBufLen >> 8;
    *pOutBuf++ = inBufLen & 0xFF;
    memcpy(pOutBuf, pInBuf, inBufLen);
    return inBufLen + 2;
}

/**
 * Initiate MQTT protocol Connect
 *
 * @param h umqtt instance handle from umqtt_New()
 * @param cleanSession true to establish new session, false to resume old session
 * @param willRetain true if published will message should be retained
 * @param willQos the QoS level to be used for the will message (if used)
 * @param keepAlive the keep alive interval in seconds
 * @param clientId the name of the MQTT client to use for the session
 * @param willTopic optional topic name for will, or NULL
 * @param willPayload optional will payload, or NULL
 * @param willPayloadLen length of the will payload message
 * @param username optional authentication user name, or NULL
 * @param password optional authentication password, or NULL
 *
 * @return UMQTT_ERR_OK if successful, or an error code if an error occurred
 *
 * This function will create a MQTT Connect packet and attempt to send it
 * to the MQTT broker.  The network connection to the MQTT server port should
 * already be established.  When the function returns, the Connect packet
 * has been sent or there was an error.  Upon return, the MQTT connection
 * is pending, it is not yet established.  The connection is not established
 * until the MQTT broker sends a Connack packet, which can be detected by
 * either using the Connack callback function, or when umqtt_GetConnectedStatus()
 * returns UMQTT_ERR_CONNECTED.
 *
 * Possible return codes:
 *
 * Code                      | Reason
 * --------------------------|-------
 * UMQTT_ERR_OK              | Connect packet was transmitted
 * UMQTT_ERR_PARM            | detected an error in a function parameter
 * UMQTT_ERR_BUFSIZE         | memory allocation failed
 * UMQTT_ERR_NETWORK         | error writing packet to network
 * UMQTT_ERR_CONNECTED       | MQTT connection is already established
 * UMQTT_ERR_CONNECT_PENDING | MQTT connection is already in progress
 *
 * __Example__
 *
 * ~~~~~~~~.c
 * umqtt_Handle_t h; // previously acquired instance handle
 * char clientName[] = "myMqttClient";
 * char willTopic[] = "myWillTopic";
 * uint8_t willPayload[] = (uint8_t *)"myWillMessage";
 *
 * umqtt_Error_t err;
 * err = umqtt_Connect(h, true, false, 0, 30, // clean session, 30 secs keep alive
 *                     clientName, willTopic,
 *                     willPayload, strlen(willPayload),
 *                     NULL, NULL); // no username or password
 * if (err != UMQTT_ERR_OK)
 * {
 *     // handle error
 * }
 * else
 * {
 *     // connect is in progress
 * }
 * ~~~~~~~~
 */
umqtt_Error_t
umqtt_Connect(umqtt_Handle_t h,
              bool cleanSession, bool willRetain, uint8_t willQos,
              uint16_t keepAlive, const char *clientId,
              const char *willTopic, const uint8_t *willPayload, uint32_t willPayloadLen,
              const char *username, const char *password)
{
    uint8_t connectFlags = 0;
    uint32_t idx = 0;
    umqtt_Instance_t *this = h;

    // initial parameter check
    RETURN_IF_ERR((this == NULL) || (clientId == NULL), UMQTT_ERR_PARM);
    size_t clientIdLen = strlen(clientId);
    size_t willTopicLen = willTopic ? strlen(willTopic) : 0;
    size_t usernameLen = username ? strlen(username) : 0;
    size_t passwordLen = password ? strlen(password) : 0;

    // already connected
    RETURN_IF_ERR(this->isConnected, UMQTT_ERR_CONNECTED);
    RETURN_IF_ERR(this->connectIsPending, UMQTT_ERR_CONNECT_PENDING);

    // calculate the "remaining length" for the packet based on
    // the various input fields.
    RETURN_IF_ERR(clientIdLen == 0, UMQTT_ERR_PARM);
    uint16_t remainingLength = 10 + 2 + clientIdLen;
    if (willTopicLen)
    {
        connectFlags |= UMQTT_CONNECT_FLAG_WILL;
        remainingLength += 2 + willTopicLen;
        // if there is a will topic there should be a will message
        RETURN_IF_ERR(willPayload == NULL, UMQTT_ERR_PARM);
        remainingLength += 2 + willPayloadLen;
    }
    if (usernameLen)
    {
        connectFlags |= UMQTT_CONNECT_FLAG_USER;
        remainingLength += 2 + usernameLen;
    }
    if (passwordLen)
    {
        connectFlags |= UMQTT_CONNECT_FLAG_PASS;
        remainingLength += 2 + passwordLen;
    }

    // allocate buffer needed for encode
    uint8_t *buf = newPacket(this, remainingLength);
    RETURN_IF_ERR(buf == NULL, UMQTT_ERR_BUFSIZE);

    // allocate second buffer just to hold packet timeout
    uint8_t *tmoBuf = newPacket(this, 0);
    if (tmoBuf == NULL)
    {
        deletePacket(this, buf);
        return UMQTT_ERR_BUFSIZE;
    }

    // encode the remaining length into the appropriate position in the buffer
    uint32_t lenSize = umqtt_EncodeLength(remainingLength, &buf[1]);

    // compute final length of packet with all data and headers
    remainingLength += 1 + lenSize;

    // encode the packet type and adjust index ahead to
    // point at variable header
    buf[0] = UMQTT_TYPE_CONNECT << 4;
    idx = 1 + lenSize;
    tmoBuf[0] = buf[0];

    // encode protocol name
    const uint8_t *protocolName = (const uint8_t *)"MQTT";
    idx += umqtt_EncodeData(protocolName, 4, &buf[idx]);

    // protocol level, connect flags and keepalive
    connectFlags |= willRetain ? UMQTT_CONNECT_FLAG_WILL_RETAIN : 0;
    connectFlags |= cleanSession ? UMQTT_CONNECT_FLAG_CLEAN : 0;
    connectFlags |= (willQos << UMQTT_CONNECT_FLAG_QOS_SHIFT) & UMQTT_CONNECT_FLAG_WILL_QOS;
    buf[idx++] = 4;
    buf[idx++] = connectFlags;
    buf[idx++] = keepAlive >> 8;
    buf[idx++] = keepAlive & 0xFF;
    this->keepAlive = keepAlive;

    // client id
    idx += umqtt_EncodeData((const uint8_t *)clientId, clientIdLen, &buf[idx]);

    // will topic and message
    if (willTopicLen)
    {
        // check data pointers
        idx += umqtt_EncodeData((const uint8_t *)willTopic, willTopicLen, &buf[idx]);
        idx += umqtt_EncodeData(willPayload, willPayloadLen, &buf[idx]);
    }

    // username
    if (usernameLen)
    {
        idx += umqtt_EncodeData((const uint8_t *)username, usernameLen, &buf[idx]);
    }

    // password
    if (passwordLen)
    {
        idx += umqtt_EncodeData((const uint8_t *)password, passwordLen, &buf[idx]);
    }

    // attempt to send the packet on the network
    int len = this->pNet->pfnNetWritePacket(this->pNet->hNet, buf, remainingLength, false);
    // no matter what, we dont need this packet any more so free it
    deletePacket(this, buf);

    // check for error sending on the network
    if (len != remainingLength)
    {
        // also need to delete the timeout packet since we dont need it now
        deletePacket(this, tmoBuf);
        return UMQTT_ERR_NETWORK; // network error
    }

    // if we make it here then we are attempting a connection and dont know
    // yet if there is a connection.  Enqueue the timeout packet for the
    // purpose of tracking the timeout until we get a CONNACK.
    // This packet doesnt have a packet ID so just use 0
    enqueuePacket(this, tmoBuf, 0, this->ticks);
    this->connectIsPending = true;

    // return success - connect attempt is in flight
    return UMQTT_ERR_OK;
}

/**
 * Disconnect MQTT protocol
 *
 * @param h umqtt instance handle from umqtt_New()
 *
 * @return UMQTT_ERR_OK or an error code
 *
 * This function attempts to send a MQTT protocol disconnect packet, frees
 * up all pending packets and marks the instance as disconnected.
 */
umqtt_Error_t
umqtt_Disconnect(umqtt_Handle_t h)
{
    umqtt_Instance_t *this = h;
    static const uint8_t disconnectPacket[2] = { UMQTT_TYPE_DISCONNECT << 4, 0 };

    // initial parameter check
    RETURN_IF_ERR(h == NULL, UMQTT_ERR_PARM);

    // clean out packet queue
    freeAllQueuedPackets(this);

    // attempt to send disconnect packet
    int len = this->pNet->pfnNetWritePacket(this->pNet->hNet, disconnectPacket,
                                            2, false);

    // clear connection status no matter what
    this->isConnected = false;
    this->connectIsPending = false;

    if (len != 2)
    {
        return UMQTT_ERR_NETWORK; // network error
    }

    return UMQTT_ERR_OK;
}

/**
 * Send MQTT protocol Publish packet
 *
 * @param h umqtt instance handle from umqtt_New()
 * @param topic topic name to publish
 * @param payload payload or message for the topic (can be NULL)
 * @param payloadLen number of bytes in the payload
 * @param qos QoS (quality of service) level for this topic
 * @param shouldRetain true if MQTT broker should retain this topic
 * @param pId pointer to storage for assigned packet ID (optional)
 *
 * @return UMQTT_ERR_OK if successful, or an error code if an error occurred
 *
 * This function is used to publish a topic with an optional payload.
 * A publish packet will be assembled and sent to the connected UMQTT
 * broker.  If the QoS > 0 then the packet will be held and resent as
 * needed until an acknowledgment is received.  If this happens, then the
 * Publish packet will have a packet ID.  The _pId_ parameter can be used
 * to get the packet ID if needed.  Otherwise, this parameter can be set
 * to NULL.  If the QoS is 0, then there is no packet ID.
 *
 * If a callback function was provided for Puback, then the Puback callback
 * will be called when the publish packet is acknowledged.
 *
 * A payload is not required.  MQTT can publish only a topic name with
 * no payload.  If a payload is not used then the parameter can be set
 * to NULL.  The payload can be binary data and not necessarily a string.
 *
 * @note At this time, the umqtt library does not support QoS level 2
 *
 * __Example__
 *
 * ~~~~~~~~.c
 * umqtt_Handle_t h; // previously acquired instance handle
 * char topicName[] = "myTopic";
 * uint8_t payload[] = (uint8_t *)"myPayloadMessage";
 * uint16_t msgId;
 *
 * umqtt_Error_t err;
 * err = umqtt_Publish(h, topicName, payload, strlen(payload),
 *                     1, true, *msgId);
 * if (err == UMQTT_ERR_OK)
 * {
 *     // Publish packet has been sent with QoS 1
 *     // Packet is pending acknowledgment
 *     // msgId now contains the packet ID that was used
 * }
 * else // error occurred
 * {
 *     // handle publish error
 * }
 * ~~~~~~~~
 */
umqtt_Error_t
umqtt_Publish(umqtt_Handle_t h,
              const char *topic, const uint8_t *payload, uint32_t payloadLen,
              uint32_t qos, bool shouldRetain, uint16_t *pId)
{
    uint8_t flags = 0;
    uint32_t idx = 0;
    umqtt_Instance_t *this = h;

    // initial parameter check
    RETURN_IF_ERR((this == NULL) || (topic == NULL), UMQTT_ERR_PARM);
    size_t topicLen = strlen(topic);
    RETURN_IF_ERR((payloadLen != 0) && (payload == NULL), UMQTT_ERR_PARM);

    RETURN_IF_ERR(!this->isConnected, UMQTT_ERR_DISCONNECTED);

    // calculate the "remaining length" for the packet based on
    // the various input fields.
    uint16_t remainingLength = (qos ? 2 : 0) + 2 + topicLen;
    remainingLength += payload ? payloadLen: 0;

    // allocate buffer needed to encode packet
    uint8_t *buf = newPacket(this, remainingLength);
    RETURN_IF_ERR(buf == NULL, UMQTT_ERR_BUFSIZE);

    // encode the remaining length into the appropriate position in the buffer
    uint32_t lenSize = umqtt_EncodeLength(remainingLength, &buf[1]);

    // compute final length of packet with all data and headers
    remainingLength += 1 + lenSize;

    // encode the packet type and adjust index ahead to
    // point at variable header
    buf[0] = UMQTT_TYPE_PUBLISH << 4;
    idx = 1 + lenSize;

    // header flags
    // @todo dup flag needs to be set by retransmit
//    flags |= isDup ? UMQTT_FLAG_DUP : 0;
    flags |= shouldRetain ? UMQTT_FLAG_RETAIN : 0;
    flags |= (qos << UMQTT_FLAG_QOS_SHIFT) & UMQTT_FLAG_QOS;
    buf[0] |= flags;

    // topic name
    idx += umqtt_EncodeData((const uint8_t *)topic, topicLen, &buf[idx]);

    // if QOS then also need packet ID
    if (qos != 0)
    {
        ++this->packetId;
        if (this->packetId == 0)
        {
            this->packetId = 1;
        }
        buf[idx++] = this->packetId >> 8;
        buf[idx++] = this->packetId & 0xFF;
        if (pId)
        {
            *pId = this->packetId;
        }
    }
    else
    {
        if (pId)
        {
            *pId = 0;
        }
    }

    // payload message
    if (payloadLen)
    {
        memcpy(&buf[idx], payload, payloadLen);
        idx += payloadLen;
    }

    int len = this->pNet->pfnNetWritePacket(this->pNet->hNet, buf, remainingLength, false);
    if (len == remainingLength)
    {
        // if qos is non-zero then we need to hang on to the packet until
        // it is acked, so save the packetId and put it in the wait list
        if (qos != 0)
        {
            enqueuePacket(this, buf, this->packetId, this->ticks);
        }
        else
        {
            deletePacket(this, buf);
        }
    }
    else
    {
        deletePacket(this, buf);
        return UMQTT_ERR_NETWORK; // network error
    }

    return UMQTT_ERR_OK;
}

/**
 * Subscribe to topics.
 *
 * @param h umqtt instance handle from umqtt_New()
 * @param count number of topics in the list of topics to subscribe
 * @param topics array of topic names to subscribe
 * @param qoss array of QoS values to use for subscribed topics
 * @param pId pointer to storage for assigned packet ID (optional)
 *
 * @return UMQTT_ERR_OK if successful, or an error code if an error occurred
 *
 * This function is used to send a subscribe request to an MQTT broker.
 * It can be used to subscribe to one or more topics at one time.  Each
 * subscribed topic can be assigned a QoS level.
 *
 * A subscribe packet will be held pending until it is acknowledged by the
 * MQTT broker.  If the caller provided the _pId_ parameter, then the packet
 * ID will be saved for the caller.  If the caller does not need the packet
 * ID, then _pId_ parameter can be NULL.
 *
 * If the caller provided a Suback callback function, then it will be
 * notified when the subscribe request is acknowledged.
 *
 * __Example__
 *
 * ~~~~~~~~.c
 * umqtt_Handle_t h; // previously acquired instance handle
 * char *topics[] = { "topic1", "topic2" };
 * uint8_t qoss[] = { 0, 1 };
 * uint16_t msgId;
 *
 * umqtt_Error_t err;
 * err = umqtt_Subscribe(h, 2, topics, qoss, &msgId);
 * if (err == UMQTT_ERR_OK)
 * {
 *     // Subscribe packet has been sent with requested topics
 *     // Packet is pending acknowledgment
 *     // msgId now contains the packet ID that was used
 * }
 * else // error occurred
 * {
 *     // handle subscribe error
 * }
 * ~~~~~~~~
 */
umqtt_Error_t
umqtt_Subscribe(umqtt_Handle_t h,
                uint32_t count, char *topics[], uint8_t qoss[],
                uint16_t *pId)
{
    uint32_t idx = 0;
    umqtt_Instance_t *this = h;

    // initial parameter check
    RETURN_IF_ERR((this == NULL), UMQTT_ERR_PARM);
    RETURN_IF_ERR((count == 0), UMQTT_ERR_PARM);
    RETURN_IF_ERR(topics == NULL, UMQTT_ERR_PARM);
    RETURN_IF_ERR(qoss == NULL, UMQTT_ERR_PARM);

    RETURN_IF_ERR(!this->isConnected, UMQTT_ERR_DISCONNECTED);

    // calculate the "remaining length" for the packet based on
    // the various input fields.
    uint16_t remainingLength = 2; // packet id
    for (uint32_t i = 0; i < count; i++)
    {
        RETURN_IF_ERR(topics[i] == NULL, UMQTT_ERR_PARM);
        RETURN_IF_ERR(qoss[i] > 2, UMQTT_ERR_PARM);
        remainingLength += 2 + 1; // topic length field plus qos
        remainingLength += strlen(topics[i]);
    }

    // allocate buffer needed to encode packet
    uint8_t *buf = newPacket(this, remainingLength);
    RETURN_IF_ERR(buf == NULL, UMQTT_ERR_BUFSIZE);

    // encode the remaining length into the appropriate position in the buffer
    uint32_t lenSize = umqtt_EncodeLength(remainingLength, &buf[1]);

    // compute final length of packet with all data and headers
    remainingLength += 1 + lenSize;

    // encode the packet type and adjust index ahead to
    // point at variable header
    buf[0] = (UMQTT_TYPE_SUBSCRIBE << 4) | 0x02;
    idx = 1 + lenSize;

    // packet id
    ++this->packetId;
    if (this->packetId == 0)
    {
        this->packetId = 1;
    }
    buf[idx++] = this->packetId >> 8;
    buf[idx++] = this->packetId & 0xFF;
    if (pId)
    {
        *pId = this->packetId;
    }

    // encode each topic in topic array provided by caller
    for (uint32_t i = 0; i < count; i++)
    {
        idx += umqtt_EncodeData((const uint8_t *)topics[i], strlen(topics[i]), &buf[idx]);
        buf[idx++] = qoss[i];
    }

    int len = this->pNet->pfnNetWritePacket(this->pNet->hNet, buf, remainingLength, false);
    if (len == remainingLength)
    {
        // need to save the packet to wait for ack
        enqueuePacket(this, buf, this->packetId, this->ticks);
    }
    else
    {
        deletePacket(this, buf);
        return UMQTT_ERR_NETWORK; // network error
    }

    return UMQTT_ERR_OK;
}

/**
 * Unsubscribe from topics.
 *
 * @param h umqtt instance handle from umqtt_New()
 * @param count count of topics in topic list
 * @param topics array of topic names to unsubscribe
 * @param pId pointer to storage for assigned packet ID (optional)
 *
 * @return UMQTT_ERR_OK if successful, or an error code if an error occurred
 *
 * This function is used to send an unsubscribe request to an MQTT broker.
 * It can be used to unsubscribe from one or more topics at one time.
 *
 * An unsubscribe packet will be held pending until it is acknowledged by the
 * MQTT broker.  If the caller provided the _pId_ parameter, then the packet
 * ID will be saved for the caller.  If the caller does not need the packet
 * ID, then _pId_ parameter can be NULL.
 *
 * If the caller provided a Unsuback callback function, then it will be
 * notified when the unsubscribe request is acknowledged.
 *
 * __Example__
 *
 * ~~~~~~~~.c
 * umqtt_Handle_t h; // previously acquired instance handle
 * char *topics[] = { "topic1", "topic2" };
 * uint16_t msgId;
 *
 * umqtt_Error_t err;
 * err = umqtt_Unsubscribe(h, 2, topics, &msgId);
 * if (err == UMQTT_ERR_OK)
 * {
 *     // Unsubscribe packet has been sent with topic list
 *     // Packet is pending acknowledgment
 *     // msgId now contains the packet ID that was used
 * }
 * else // error occurred
 * {
 *     // handle unsubscribe error
 * }
 * ~~~~~~~~
 */
umqtt_Error_t
umqtt_Unsubscribe(umqtt_Handle_t h,
                       uint32_t count, const char *topics[], uint16_t *pId)
{
    uint32_t idx = 0;
    umqtt_Instance_t *this = h;

    // initial parameter check
    RETURN_IF_ERR(topics == NULL, UMQTT_ERR_PARM);
    RETURN_IF_ERR(count == 0, UMQTT_ERR_PARM);

    RETURN_IF_ERR(!this->isConnected, UMQTT_ERR_DISCONNECTED);

    // calculate the "remaining length" for the packet based on
    // the various input fields.
    uint16_t remainingLength = 2; // packet id
    for (uint32_t i = 0; i < count; i++)
    {
        RETURN_IF_ERR(topics[i] == NULL, UMQTT_ERR_PARM);
        remainingLength += 2; // topic length field
        remainingLength += strlen(topics[i]);
    }

    // allocate buffer needed to encode packet
    uint8_t *buf = newPacket(this, remainingLength);
    RETURN_IF_ERR(buf == NULL, UMQTT_ERR_BUFSIZE);

    // encode the remaining length into the appropriate position in the buffer
    uint32_t lenSize = umqtt_EncodeLength(remainingLength, &buf[1]);

    // compute final length of packet with all data and headers
    remainingLength += 1 + lenSize;

    // encode the packet type and adjust index ahead to
    // point at variable header
    buf[0] = (UMQTT_TYPE_UNSUBSCRIBE << 4) | 0x02;
    idx = 1 + lenSize;

    // packet id
    ++this->packetId;
    if (this->packetId == 0)
    {
        this->packetId = 1;
    }
    buf[idx++] = this->packetId >> 8;
    buf[idx++] = this->packetId & 0xFF;
    if (pId)
    {
        *pId = this->packetId;
    }

    // encode each topic in topic array provided by caller
    for (uint32_t i = 0; i < count; i++)
    {
        idx += umqtt_EncodeData((const uint8_t *)topics[i], strlen(topics[i]), &buf[idx]);
    }

    int len = this->pNet->pfnNetWritePacket(this->pNet->hNet, buf, remainingLength, false);
    if (len == remainingLength)
    {
        // need to save the packet to wait for ack
        enqueuePacket(this, buf, this->packetId, this->ticks);
    }
    else
    {
        deletePacket(this, buf);
        return UMQTT_ERR_NETWORK; // network error
    }

    return UMQTT_ERR_OK;
}

/**
 * Send a Ping (keep-alive) to the MQTT broker
 *
 * @param h umqtt instance handle from umqtt_New()
 *
 * @return UMQTT_ERR_OK if successful, or an error code if an error occurred
 *
 * This function is used to send a Ping Request to the MQTT broker.  The
 * client is required to periodically send Ping messages to keep the
 * connection alive.  This function is provided as a convenience but should
 * not normally ever need to be called by the client application.  The
 * keep-alive/ping process is handled by the umqtt_Run() function.
 */
umqtt_Error_t
umqtt_PingReq(umqtt_Handle_t h)
{
    umqtt_Instance_t *this = h;
    static const uint8_t pingreqPacket[2] = { UMQTT_TYPE_PINGREQ << 4, 0 };

    // initial parameter check
    RETURN_IF_ERR(h == NULL, UMQTT_ERR_PARM);

    // attempt to send pingreq packet
    int len = this->pNet->pfnNetWritePacket(this->pNet->hNet, pingreqPacket,
                                            2, false);
    if (len != 2)
    {
        return UMQTT_ERR_NETWORK; // network error
    }

    return UMQTT_ERR_OK;
}

/**
 * Decode incoming MQTT packet.
 *
 * @param h umqtt instance handle from umqtt_New()
 * @param pIncoming buffer holding incoming UMQTT packet
 * @param incomingLen number of bytes in the incoming buffer
 *
 * @return UMQTT_ERR_OK if successful, or an error code if an error occurred
 *
 * This function is used to decode incoming MQTT packets from the server.
 * It is normally called from umqtt_Run() and the client application does
 * not need to call this directly. However, it is provided for completeness.
 *
 * The caller passes a buffer containing an MQTT protocol packet that was
 * received from the network.  The packet will be decoded and if valid the
 * appropriate action taken.  The action depends on the packet:
 *
 * Type     | Action
 * ---------|-------
 * CONNACK  | Free pending connect, notify client if callback is provided
 * PUBLISH  | Extract publish topic, notify client through callback
 * PUBACK   | Free pending Publish, notify client if callback is provided
 * SUBACK   | Free pending Subscribe, notify client if callback is provided
 * UNSUBACK | Free pending Unsubscribe, notify client if callback is provided
 * PINGRESP | No action except notify client if a callback is provided
 */
umqtt_Error_t
umqtt_DecodePacket(umqtt_Handle_t h, const uint8_t *pIncoming, uint32_t incomingLen)
{
    umqtt_Error_t err = UMQTT_ERR_OK;

    // basic parameter check
    if ((h == NULL) || (pIncoming == NULL) || (incomingLen == 0))
    {
        return UMQTT_ERR_PARM;
    }

    // get instance data from handle
    umqtt_Instance_t *this = h;

    // start processing the packet if it contains data
    if (incomingLen)
    {
        // extract the packet type, flags and length
        uint8_t type = pIncoming[0] >> 4;
        uint8_t flags = pIncoming[0] & 0x0F;
        uint32_t remainingLen;
        uint32_t lenCount = umqtt_DecodeLength(&remainingLen, &pIncoming[1]);
        // make sure MQTT packet length is consistent with
        // the supplied network packet
        if ((remainingLen + 1 + lenCount) != incomingLen)
        {
            return UMQTT_ERR_PACKET_ERROR;
        }

        // process the packet type
        // only client related - not implementing server
        switch (type)
        {
            // CONNACK - pass ack info to callback
            case UMQTT_TYPE_CONNACK:
            {
                // sanity check
                RETURN_IF_ERR(remainingLen != 2, UMQTT_ERR_PACKET_ERROR);
                // extract parameters from connack packet
                bool sessionPresent = pIncoming[2] & 1 ? true : false;
                uint8_t returnCode = pIncoming[3];

                // remove any pending connects from the wait queue
                uint8_t *buf;
                do
                {
                    buf = dequeuePacketByType(this, UMQTT_TYPE_CONNECT);
                    if (buf)
                    {
                        deletePacket(this, buf);
                    }
                } while (buf);

                // update the connection state
                // if return code is 0 then client is connected
                this->connectIsPending = false;
                this->isConnected = (returnCode == 0);
                this->pingTicks = this->ticks;

                // notify client of connack
                if (this->pCb->connackCb)
                {
                    this->pCb->connackCb(h, this->pUser, sessionPresent, returnCode);
                }
                break;
            }

            // PUBLISH - extract published topic, payload and options
            case UMQTT_TYPE_PUBLISH:
            {
                const char *pTopic;
                const uint8_t *pMsg = NULL;
                uint8_t pktId[2] = {0, 0};

                // make sure there is a callback function
                // @todo do we need to process packet even if no callback?
                // what if qos != 0 we still need to reply to sender
                if (this->pCb->publishCb)
                {
                    // extract publish options
                    bool dup = flags & UMQTT_FLAG_DUP ? true : false;
                    bool retain = flags & UMQTT_FLAG_RETAIN ? true : false;
                    uint8_t qos = (flags & UMQTT_FLAG_QOS) >> UMQTT_FLAG_QOS_SHIFT;
                    RETURN_IF_ERR(qos > 2, UMQTT_ERR_PACKET_ERROR);

                    // find the topic length and value
                    // make sure remaining packet length is long enough
                    uint32_t idx = 1 + lenCount;
                    uint16_t topicLen = (pIncoming[idx] << 8) + pIncoming[idx + 1];
                    idx += 2;
                    RETURN_IF_ERR((topicLen + 2U) > remainingLen, UMQTT_ERR_PACKET_ERROR);

                    // extract the topic length and buf pointer
                    pTopic = (const char *)&pIncoming[idx];
                    remainingLen -= topicLen + 2;
                    idx += topicLen;

                    // for non-0 QoS, extract the packet id
                    // @todo check for qos 2 and signal error?
                    if (qos != 0)
                    {
                        if (remainingLen >= 2)
                        {
                            pktId[0] = pIncoming[idx++];
                            pktId[1] = pIncoming[idx++];
                            remainingLen -= 2;
                        }
                        else
                        {
                            return UMQTT_ERR_PACKET_ERROR;
                        }
                    }

                    // continue extracting if there is a topic payload
                    if (remainingLen != 0)
                    {
                        // remainder of packet is the payload message
                        pMsg = &pIncoming[idx];
                    }

                    // callback to provide the publish info to the app
                    this->pCb->publishCb(h, this->pUser, dup, retain, qos, pTopic, topicLen, pMsg, remainingLen);

                    // if QoS is non-0, prepare a reply packet and
                    // notify through the callback
                    // (note this only works for QoS 1 right now)
                    if (qos != 0)
                    {
                        uint8_t pubackdat[4];
                        pubackdat[0] = UMQTT_TYPE_PUBACK << 4;
                        pubackdat[1] = 2;
                        pubackdat[2] = pktId[0];
                        pubackdat[3] = pktId[1];
                        int msgLen = this->pNet->pfnNetWritePacket(this->pNet->hNet, pubackdat, 4, false);
                        RETURN_IF_ERR(msgLen != 4, UMQTT_ERR_NETWORK);
                    }
                }

                break;
            }

            // PUBACK - server is acking the client publish, notify client
            case UMQTT_TYPE_PUBACK:
            {
                // sanity check
                RETURN_IF_ERR(remainingLen != 2, UMQTT_ERR_PACKET_ERROR);
                uint16_t pktId = (pIncoming[2] << 8) + pIncoming[3];

                // remove pending publish packet with this packet ID
                uint8_t *buf;
                do
                {
                    buf = dequeuePacketById(this, pktId);
                    if (buf)
                    {
                        deletePacket(this, buf);
                    }
                } while (buf); // should not ever repeat

                if (this->pCb->pubackCb)
                {
                    this->pCb->pubackCb(this, this->pUser, pktId);
                }
                break;
            }

            // SUBACK - server is acking the client subscribe,
            // notify client
            case UMQTT_TYPE_SUBACK:
            {
                // sanity check
                RETURN_IF_ERR(remainingLen < 3, UMQTT_ERR_PACKET_ERROR);
                uint16_t pktId = (pIncoming[2] << 8) + pIncoming[3];

                // remove pending subscribe packet with this packet ID
                uint8_t *buf;
                do
                {
                    buf = dequeuePacketById(this, pktId);
                    if (buf)
                    {
                        deletePacket(this, buf);
                    }
                } while (buf); // should not ever repeat

                if (this->pCb->subackCb)
                {
                    uint16_t topicCount = remainingLen - 2;
                    const uint8_t *topicList = &pIncoming[4];
                    this->pCb->subackCb(this, this->pUser, topicList, topicCount, pktId);
                }
                break;
            }

            // UNSUBACK - notify client
            case UMQTT_TYPE_UNSUBACK:
            {
                // sanity check
                RETURN_IF_ERR(remainingLen != 2, UMQTT_ERR_PACKET_ERROR);
                uint16_t pktId = (pIncoming[2] << 8) + pIncoming[3];

                // remove pending unsub packet with this packet ID
                uint8_t *buf;
                do
                {
                    buf = dequeuePacketById(this, pktId);
                    if (buf)
                    {
                        deletePacket(this, buf);
                    }
                } while (buf); // should not ever repeat

                if (this->pCb->unsubackCb)
                {
                    this->pCb->unsubackCb(this, this->pUser, pktId);
                }
                break;
            }

            // PINGRESP - notify client
            case UMQTT_TYPE_PINGRESP:
            {
                // sanity check
                RETURN_IF_ERR(remainingLen != 0, UMQTT_ERR_PACKET_ERROR);
                if (this->pCb->pingrespCb)
                {
                    this->pCb->pingrespCb(this, this->pUser);
                }
                break;
            }

            // unexpected packet type is an error
            default:
            {
                err = UMQTT_ERR_PACKET_ERROR;
                break;
            }
        }
        return err;
    }

    // packet length 0 is an error
    else
    {
        return UMQTT_ERR_PACKET_ERROR;
    }
}

/**
 * Get the status of the connection.
 *
 * @param h umqtt instance handle from umqtt_New()
 *
 * Use this function to determine the state of connectedness of the
 * UMQTT client.  It will return one of the following:
 *
 * * UMQTT_ERR_CONNECTED
 * * UMQTT_ERR_CONNECT_PENDING
 * * UMQTT_ERR_DISCONNECTED
 *
 * @return return code indicating the connection state
 */
umqtt_Error_t
umqtt_GetConnectedStatus(umqtt_Handle_t h)
{
    if (h == NULL)
    {
        return UMQTT_ERR_PARM;
    }
    umqtt_Instance_t *this = h;

    if (this->isConnected)              { return UMQTT_ERR_CONNECTED; }
    else if (this->connectIsPending)    { return UMQTT_ERR_CONNECT_PENDING; }
    else                                { return UMQTT_ERR_DISCONNECTED; }
}

/**
 * Create and initialize a umqtt client instance.
 *
 * @param pTransport structure defining the MQTT transport interface
 * @param pCallbacks structure holding the callback functions
 * @param pUser optional caller defined data pointer that will be passed in callbacks
 *
 * @return _umqtt_ instance handle that should be used for all other
 * function calls, or NULL if there is an error.
 *
 * This function will allocate a _umqtt_ instance and initialize it.  No
 * actual MQTT operations are performed.  The caller must provide the
 * network access functions through the pTransport parameter.  Callback
 * functions are used for notification and are optional.  If used, callback
 * functions are provided by the pCallbacks parameter.  The entire parameter
 * can be NULL if no callbacks are used, or individual callbacks can be
 * NULL within the structure.
 *
 * __Example__
 * ~~~~~~~~.c
 * void PublishCb(umqtt_Handle_t h, void *pUser, bool dup, bool retain,
 *                const char *topic, uint16_t topicLen,
 *                const uint8_t *msg, uint16_t msgLen)
 * {
 *     // publish handler
 * }
 *
 * umqtt_TransportConfig_t transport =
 * {
 *     myNetworkHandle, // optional, if network has a handle or instance
 *     malloc, free,
 *     myNetReadFunction, myNetWriteFunction
 * };
 * umqtt_Callbacks_t callbacks = // only publish callback is defined
 *     { NULL, PublishCb, NULL, NULL, NULL, NULL };
 *
 * umqtt_Handle_t h;
 * h = umqtt_New(&transport, &callbacks, NULL);
 * if (h == NULL)
 * {
 *     // handle error
 * }
 * ~~~~~~~~
 */
umqtt_Handle_t
umqtt_New(umqtt_TransportConfig_t *pTransport, umqtt_Callbacks_t *pCallbacks, void *pUser)
{
    if (!pTransport)
    {
        return NULL;
    }
    if (!pTransport->pfnmalloc || !pTransport->pfnfree
     || !pTransport->pfnNetReadPacket || !pTransport->pfnNetWritePacket
     || !pTransport->hNet)
    {
        return NULL;
    }
    umqtt_Instance_t *this = pTransport->pfnmalloc(sizeof(umqtt_Instance_t));
    if (!this)
    {
        return NULL;
    }
    this->pNet = pTransport;
    this->pCb = pCallbacks;
    this->pUser = pUser;
    this->packetId = 0;
    this->pktList.next = NULL;
    this->pktList.packetId = 0;
    this->pktList.ticks = 0;
    this->ticks = 0;
    this->pingTicks = 0;
    this->isConnected = false;
    this->connectIsPending = false;
    this->keepAlive = 0;
    return this;
}

/**
 * Clean up and free umqtt client instance.
 *
 * @param h umqtt instance handle from umqtt_New()
 *
 * This function is used to free all allocated memory by the umqtt
 * instance.  It should be called as part of an orderly shutdown.
 */
void
umqtt_Delete(umqtt_Handle_t h)
{
    if (h)
    {
        umqtt_Instance_t *this = h;
        freeAllQueuedPackets(this);
        void (*pfnfree)(void *ptr) = this->pNet->pfnfree;
        memset(h, 0, sizeof(umqtt_Instance_t));
        pfnfree(h);
    }
}

/**
 * Main loop processing for the umqtt client instance
 *
 * @param h umqtt instance handle from umqtt_New()
 * @param msTicks milliseconds tick count
 *
 * @return UMQTT_ERR_OK if everything is normal, or an error code if
 * something goes wrong
 *
 * This function should be called repeatedly from the application main loop.
 * The application must maintain an incrementing millisecond tick counter
 * and pass that to the Run function.  This tick value is used to keep track
 * of internal timeouts.  The Run function performs the following actions:
 *
 * - check for any incoming packets, decode and process
 * - check for ping timeout and send ping packet if needed
 * - check for timed out pending packets and resend or expire
 *
 * The Run function can encounter several kinds of errors while peforming
 * its process.  If nothing goes wrong it will return UMQTT_ERR_OK.  If
 * something goes wrong, it will return the most recent error code, even
 * if multiple errors are encountered.  For this reason, the caller cannot
 * conclusively know the cause of a problem based on error code.  Instead,
 * the presence of a non-OK return value means that something has gone
 * wrong and the caller should probably initiate a recovery procedure.  Even
 * when errors are encountered, the Run function attempts to carry out all
 * required actions and does not automatically disconnect or change internal
 * state.  The following error codes can be returned:
 *
 * Code                      | Reason
 * --------------------------|-------
 * UMQTT_ERR_OK              | Run() did not encounter any problem
 * UMQTT_ERR_PARM            | detected an error in a function parameter
 * UMQTT_ERR_BUFSIZE         | memory allocation failed
 * UMQTT_ERR_NETWORK         | error reading or writing the network
 * UMQTT_ERR_TIMEOUT         | a pending packet has timed out
 *
 * In the case of UMQTT_ERR_TIMEOUT, it means either that no CONNACK was
 * received in response to a Connect attempt, or that one of the other
 * pending packet types has completely expired all of its retry attempts.
 * In any case this probably indicates something has gone wrong with the
 * connection to the MQTT broker.
 */
umqtt_Error_t
umqtt_Run(umqtt_Handle_t h, uint32_t msTicks)
{
    umqtt_Error_t err = UMQTT_ERR_OK;
    umqtt_Instance_t *this = h;
    RETURN_IF_ERR(this == NULL, UMQTT_ERR_PARM);

    this->ticks = msTicks;

    // if connected or connect is pending, then need to process incoming
    if (this->connectIsPending || this->isConnected)
    {
        // attempt to read from the network
        // assumes always a whole packet is given
        // cannot handle partial packets
        uint8_t *pBuf;
        int len;
        len = this->pNet->pfnNetReadPacket(this->pNet->hNet, &pBuf);

        // check for network error.  if so then remember error
        // but keep going because there is more processing to do
        if (len < 0)
        {
            err = UMQTT_ERR_NETWORK;
        }

        // non-zero means something was received, so decode the packet
        // free it when we are finished
        else if (len)
        {
            err = umqtt_DecodePacket(h, pBuf, len);
            this->pNet->pfnfree(pBuf);
        }

        // if connected, then need to check for ping timeout
        if (this->isConnected)
        {
            // use half of keepalive for ping timeout
            // keepAlive * 1000 / 2 ==> keepAlive * 500
            if ((this->ticks - this->pingTicks) > (this->keepAlive * 500))
            {
                this->pingTicks = this->ticks;
                err = umqtt_PingReq(h);
            }
        }
    }

    // iterate through list of queued messages
    PktBuf_t *pPrev = &this->pktList;
    PktBuf_t *pPkt = this->pktList.next;
    while (pPkt)
    {
        bool unlinkAndFree;
        unlinkAndFree = false;
        // check if the packet is past the retry timeout
        if ((msTicks - pPkt->ticks) >= UMQTT_RETRY_TIMEOUT)
        {
            // get the payload part of the packet buffer
            uint8_t *buf = (uint8_t *)pPkt;
            buf += sizeof(PktBuf_t);
            uint8_t type = buf[0] >> 4;

            // check for connect packet
            // if a connect packet times out, we dont retry
            if (type == UMQTT_TYPE_CONNECT)
            {
                // mark packet to be unlinked and freed
                unlinkAndFree = true;
                err = UMQTT_ERR_TIMEOUT;
                this->connectIsPending = false;
            }

            // all other packet type use the same processing
            else
            {
                // if the packet has more life, then retry it
                if (pPkt->ttl)
                {
                    // reduce retry count and reset the timeout ticks
                    --pPkt->ttl;
                    pPkt->ticks = this->ticks;
                    // get the packet length, adjust for header
                    uint32_t remLen;
                    uint32_t lenBytes = umqtt_DecodeLength(&remLen, &buf[1]);
                    remLen += 1 + lenBytes;
                    // attempt to re-send the packet
                    uint32_t writeLen = this->pNet->pfnNetWritePacket(this->pNet->hNet,
                                                                 buf, remLen, false);
                    // if there is an error then return error,
                    // but packet is not deleted so it will be tried again
                    if (writeLen != remLen)
                    {
                        err = UMQTT_ERR_NETWORK;
                    }
                }

                // life expired for this packet dont retry again
                else
                {
                    // unlink it from the list and free packet memory
                    unlinkAndFree = true;
                    err = UMQTT_ERR_TIMEOUT;
                }
            }
        }

        // if marked for deletion, update pointers and free packet
        if (unlinkAndFree)
        {
            pPrev->next = pPkt->next;
            pPkt->next = NULL;
            this->pNet->pfnfree(pPkt);
            pPkt = pPrev->next;
        }

        // packet not to be deleted, advance to the next in the list
        else
        {
            pPrev = pPkt;
            pPkt = pPkt->next;
        }
    }
    return err;
}

/**
 * @}
 */

