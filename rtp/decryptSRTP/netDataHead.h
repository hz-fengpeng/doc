#ifndef NETDATAHEAD_H
#define NETDATAHEAD_H

#include <stdint.h>

struct ether_header
{
    /** Destination MAC */
    uint8_t dstMac[6];
    /** Source MAC */
    uint8_t srcMac[6];
    /** EtherType */
    uint16_t etherType;
};

struct iphdr
{
    #if (BYTE_ORDER == LITTLE_ENDIAN)
        /** IP header length, has the value of 5 for IPv4 */
        uint8_t internetHeaderLength:4,
        /** IP version number, has the value of 4 for IPv4 */
        ipVersion:4;
    #else
        /** IP version number, has the value of 4 for IPv4 */
        uint8_t ipVersion:4,
        /** IP header length, has the value of 5 for IPv4 */
        internetHeaderLength:4;
    #endif
    /** type of service, same as Differentiated Services Code Point (DSCP)*/
    uint8_t typeOfService;
    /** Entire packet (fragment) size, including header and data, in bytes */
    uint16_t totalLength;
    /** Identification field. Primarily used for uniquely identifying the group of fragments of a single IP datagram*/
    uint16_t ipId;
        /** Fragment offset field, measured in units of eight-byte blocks (64 bits) */
    uint16_t fragmentOffset;
    /** An eight-bit time to live field helps prevent datagrams from persisting (e.g. going in circles) on an internet.  In practice, the field has become a hop count */
    uint8_t timeToLive;
    /** Defines the protocol used in the data portion of the IP datagram. Must be one of ::IPProtocolTypes */
    uint8_t protocol;
    /** Error-checking of the header */
    uint16_t headerChecksum;
    /** IPv4 address of the sender of the packet */
    uint32_t ipSrc;
    /** IPv4 address of the receiver of the packet */
    uint32_t ipDst;
    /*The options start here. */
};

struct udphdr
{
    /** Source port */
    uint16_t portSrc;
    /** Destination port */
    uint16_t portDst;
    /** Length of header and payload in bytes */
    uint16_t length;
    /**  Error-checking of the header and data */
    uint16_t headerChecksum;
};

typedef struct _RTPHeader
{
    //first byte
    #if BYTE_ORDER == LITTLE_ENDIAN
        unsigned int         CC:4;        /* CC field */
        unsigned int         X:1;         /* X field */
        unsigned int         P:1;         /* padding flag */
        unsigned int         version:2;
    #elif BYTE_ORDER == BIG_ENDIAN
        unsigned int         version:2;
        unsigned int         P:1;         /* padding flag */
        unsigned int         X:1;         /* X field */
        unsigned int         CC:4;        /* CC field*/
    #else
        #error "BYTE_ORDER should be big or little endian."
    #endif
    //second byte
    #if BYTE_ORDER == LITTLE_ENDIAN
        unsigned int         PT:7;     /* PT field */
        unsigned int         M:1;       /* M field */
    #elif BYTE_ORDER == BIG_ENDIAN
        unsigned int         M:1;         /* M field */
        unsigned int         PT:7;       /* PT field */
    #else
        #error "G_BYTE_ORDER should be big or little endian."
    #endif
    uint16_t              seq_num;      /* length of the recovery */
    uint32_t              TS;                   /* Timestamp */
    uint32_t              ssrc;
} RTPHeader; //12 bytes

typedef struct RTCPCommonHeader
{
    #if BYTE_ORDER == LITTLE_ENDIAN
        uint8_t count : 5;
        uint8_t padding : 1;
        uint8_t version : 2;
    #elif BYTE_ORDER == BIG_ENDIAN
        uint8_t version : 2;
        uint8_t padding : 1;
        uint8_t count : 5;
    #endif
    uint8_t packetType;
    uint16_t length;
}RTCPCommonHeader;

#endif