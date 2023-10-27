/******************************************************************************
 * umqtt.h - MQTT packet client for microcontrollers.
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

#ifndef __UMQTT_H__
#define __UMQTT_H__

/**
 * @addtogroup umqtt_api
 * @{
 */

/**
 * Return codes for umqtt functions.
 */
typedef enum
{
    UMQTT_ERR_OK,           ///< normal return code, no errors
    UMQTT_ERR_PACKET_ERROR, ///< detected error in packet during decoding
    UMQTT_ERR_BUFSIZE,      ///< unable to allocate memory for a packet
    UMQTT_ERR_PARM,         ///< problem with function input parameter
    UMQTT_ERR_NETWORK,      ///< error writing or reading network
    UMQTT_ERR_CONNECT_PENDING, ///< connect is in progress
    UMQTT_ERR_CONNECTED,    ///< umqtt client is connected to MQTT broker
    UMQTT_ERR_DISCONNECTED, ///< umqtt client is not connected to MQTT broker
    UMQTT_ERR_TIMEOUT,      ///< a timeout occurred waiting on some reply
} umqtt_Error_t;

/**
 * umqtt instance handle, to be passed to all functions.  Obtained
 * from umqtt_New().
 */
typedef void * umqtt_Handle_t;

/**
 * Callback function for CONNACK connection acknowledgment.
 *
 * @param h umqtt instance handle
 * @param pUser client's optional user data pointer
 * @param sessionPresent MQTT session present flag
 * @param retCode return code for the MQTT connection attempt
 *
 * This function is called when a CONNACK is received in response to
 * umqtt_Connect().  The application can use this callback to know when
 * a connection is complete and ready to be used.
 */
typedef void (*ConnackCb_t)(umqtt_Handle_t h, void *pUser,
                            bool sessionPresent, uint8_t retCode);

/**
 * Callback function for Publish packets.
 *
 * @param h umqtt instance handle
 * @param pUser client's optional user data pointer
 * @param dup MQTT dup header flag was set in the packet
 * @param retain MQTT retain flag was set in the packet
 * @param qos QoS level for the packet
 * @param pTopic pointer to topic string
 * @param topicLen number of bytes in the topic string
 * @param pMsg pointer to topic message
 * @param msgLen number of bytes in the topic message
 *
 * This function is called when the umqtt client receives a publish
 * packet from the MQTT broker.  While this callback function is optional,
 * it is the only way for the umqtt client application to be notified of
 * publish messages.  If any of the packet contents such as the topic or
 * message needs to be retained, then this function must make a copy.
 * Once this function returns the pointers will be no longer valid.
 */
typedef void (*PublishCb_t)(umqtt_Handle_t h, void *pUser, bool dup, bool retain,
                            uint8_t qos, const char *pTopic, uint16_t topicLen,
                            const uint8_t *pMsg, uint16_t msgLen);

/**
 * Callback function for Puback packets.
 *
 * @param h umqtt instance handle
 * @param pUser client's optional user data pointer
 * @param pktId packet ID of the received packet
 *
 * This function is called when the umqtt client receives a Puback
 * packet in response to umqtt_Publish().  It is not necessary for the
 * application to use this callback unless it needs to track completion
 * of publish messages.  The _pktId_ parameter should match the message ID
 * that was returned when using umqtt_Publish().  This method could be used
 * to throttle publish actions (ensure that only one publish is sent at
 * a time).  The umqtt_Run() function takes care of tracking acknowledgments
 * and handling retries, so this function is just informative.
 */
typedef void (*PubackCb_t)(umqtt_Handle_t h, void *pUser, uint16_t pktId);

/**
 * Callback function for Suback packets.
 *
 * @param h umqtt instance handle
 * @param pUser client's optional user data pointer
 * @param retCodes array of return codes, one for each subscribe topic
 * @param retCount number of return codes in the retCodes array
 * @param pktId packet ID of the received packet
 *
 * This function is called when the umqtt client receives a Suback
 * packet in response to umqtt_Subscribe().  It is not necessary for the
 * application to use this callback unless it needs to track completion
 * of subscribe messages or needs confirmation of subscribe topic return
 * codes.  The _pktId_ parameter should match the message ID that was
 * returned when using umqtt_Subscribe().  The umqtt_Run() function takes
 * care of tracking acknowledgments and handling retries.
 */
typedef void (*SubackCb_t)(umqtt_Handle_t h, void *pUser, const uint8_t *retCodes,
                           uint16_t retCount, uint16_t pktId);

/**
 * Callback function for Unsuback packets.
 *
 * @param h umqtt instance handle
 * @param pUser client's optional user data pointer
 * @param pktId packet ID of the received packet
 *
 * This function is called when the umqtt client receives a Unsuback
 * packet in response to umqtt_Unsubscribe().  It is not necessary for the
 * application to use this callback unless it needs to track completion
 * of unsubscribe messages.  The _pktId_ parameter should match the message
 * ID that was returned when using umqtt_Unsubscribe().  The umqtt_Run()
 * function takes care of tracking acknowledgments and handling retries.
 */
typedef void (*UnsubackCb_t)(umqtt_Handle_t h, void *pUser, uint16_t pktId);

/**
 * Callback function for PINGRESP packets.
 *
 * @param h umqtt instance handle
 * @param pUser client's optional user data pointer
 *
 * This function is called when the umqtt client receives a PINGRESP
 * in response to umqtt_Pingreq().  There is no reason for the application
 * to use this callback but it is here for completeness.  It could be used
 * as a kind of heartbeat although the interval will not be reliable.
 * The umqtt_Run() function takes care of sending and receiving ping
 * messages at the appropriate time.
 */
typedef void (*PingrespCb_t)(umqtt_Handle_t h, void *pUser);

/**
 * Structure to hold callback functions.
 */
typedef struct
{
    /// Called when a CONNACK is received.
    ConnackCb_t connackCb;
    /// Called when PUBLISH packet is received.
    PublishCb_t publishCb;
    /// Called when PUBACK is received.
    PubackCb_t pubackCb;
    /// Called when SUBACK is received.
    SubackCb_t subackCb;
    /// Called when UNSUBACK is received.
    UnsubackCb_t unsubackCb;
    /// Called when PINGRESP is received.
    PingrespCb_t pingrespCb;
} umqtt_Callbacks_t;

/**
 * Memory allocation function provided by application.
 *
 * @param size the number of bytes to be allocated
 *
 * @return pointer to the allocated memory or NULL
 *
 * This function must be implemented by the application.  The umqtt library
 * uses this when it needs to request memory for building MQTT packets.
 * The application is free to implement this in any way.  It can just pass
 * through the system malloc, or it could use other memory allocator methods
 * (such as bget).  Or it can implement a simple list of fixed buffers and
 * just allocate one of those each time this function is called.
 */
typedef void *(*malloc_t)(size_t size);

/**
 * Memory free function provided by application.
 *
 * @param ptr pointer to the previously allocated memory buffer
 *
 * This function must be implemented by the application.  The umqtt library
 * uses this when it needs to free memory that was earlier allocated for
 * packet memory.
 */
typedef void (*free_t)(void *ptr);

/**
 * Read a packet from the network
 *
 * @param hNet is the network instance handle (not umqtt instance handle)
 * @param ppBuf pointer to a pointer to the received packet data
 *
 * @return number of bytes that were read from the network.  This will be
 * 0 if no data was read, or negative if there is an error.
 *
 * This function must be implemented by the application.  It is used by the
 * umqtt library when it needs to read a new packet from the network.
 * A double pointer is used (__ppBuf__) so that the umqtt library does not
 * need to make any additional copy of the data.  This function must allocate
 * the memory used to hold the packet in a method compatible with the
 * malloc_t() / free_t() functions.  The umqtt_Run() function will use the
 * free_t() function to free this packet after it has been decoded.
 *
 * The incoming packet must be a complete packet.  The `umqtt` library does
 * not handle partial packets or misaligned packets.
 */
typedef int (*netReadPacket_t)(void *hNet, uint8_t **ppBuf);

/**
 * Write a packet to the network
 *
 * @param hNet is the network instance handle (not umqtt instance handle)
 * @param pBuf pointer to the buffer holding the packet data
 * @param len count of byte in the buffer
 * @param isMore flag to indicate that there is additional data to send
 *
 * @return number of bytes that were written to the network.  This will be
 * 0 if no data was written or negative if there was an error.
 *
 * This function must be implemented by the application.  It is called by the
 * umqtt library when it needs to send a packet to the network.  The packet
 * must be all sent or none.  The umqtt library cannot handle a partial
 * transmit.  If anything less than the complete number of bytes are sent
 * it will be considered a network error.  The network write function can
 * optionally use the _isMore_ parameter to aggregate packet data before
 * sending it over the network link.  Usually _isMore_ is false.
 */
typedef int (*netWritePacket_t)(void *hNet, const uint8_t *pBuf, uint32_t len, bool isMore);

/**
 * Structure to define the network interface.
 */
typedef struct
{
    /// Network instance handle.  The meaning of this is application defined.
    /// It can be a data structure that holds network sockets or something
    /// similar.
    void *hNet;
    /// Application supplied function to allocate memory.
    malloc_t pfnmalloc;
    /// Application supplied function to free memory.
    free_t pfnfree;
    /// Application supplied function to read from the network.
    netReadPacket_t pfnNetReadPacket;
    /// Application supplied function to write to the network.
    netWritePacket_t pfnNetWritePacket;
} umqtt_TransportConfig_t;

/**
 * @}
 */

#ifdef __cplusplus
extern "C" {
#endif

extern umqtt_Error_t umqtt_Connect(umqtt_Handle_t h, bool cleanSession,
    bool willRetain, uint8_t willQos, uint16_t keepAlive, const char *clientId,
    const char *willTopic, const uint8_t *willPayload, uint32_t willPayloadLen,
    const char *username, const char *password);
extern umqtt_Error_t umqtt_Publish(umqtt_Handle_t h, const char *topic,
                                   const uint8_t *payload, uint32_t payloadLen,
                                   uint32_t qos, bool shouldRetain,
                                   uint16_t *pId);
extern umqtt_Error_t umqtt_Subscribe(umqtt_Handle_t h, uint32_t count,
                                     char *topics[], uint8_t qoss[],
                                     uint16_t *pId);
extern umqtt_Error_t umqtt_Unsubscribe(umqtt_Handle_t h, uint32_t count,
                                       const char *topics[], uint16_t *pId);
extern umqtt_Error_t umqtt_DecodePacket(umqtt_Handle_t h,
                                        const uint8_t *pIncoming, uint32_t incomingLen);
extern umqtt_Error_t umqtt_GetConnectedStatus(umqtt_Handle_t h);
extern umqtt_Error_t umqtt_Disconnect(umqtt_Handle_t h);
extern umqtt_Error_t umqtt_PingReq(umqtt_Handle_t h);
extern umqtt_Error_t umqtt_Run(umqtt_Handle_t h, uint32_t msTicks);
extern umqtt_Handle_t umqtt_New(umqtt_TransportConfig_t *pTransport,
                                         umqtt_Callbacks_t *pCallbacks, void *pUser);
extern void umqtt_Delete(umqtt_Handle_t h);
extern const char *umqtt_GetErrorString(umqtt_Error_t err);

#ifdef __cplusplus
extern }
#endif

#endif
