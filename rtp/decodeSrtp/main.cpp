#include <pcap/pcap.h>
#include <stdio.h>
#include "srtp.h"
#include <string.h>
 #include <arpa/inet.h>


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


int main(int argc, char* argv[])
{
    if(argc < 4){
      printf("./proc pcap key outPcap");
      return 0;
    }

    char errbuf[PCAP_ERRBUF_SIZE] = ""; // PCAP_ERRBUF_SIZE为512字节
    int ether_size = sizeof(struct ether_header);
    int ip_size = sizeof(struct iphdr);
    int udp_size = sizeof(struct udphdr);
    int rtp_head = sizeof(RTPHeader) + 8; // 8字节扩展

    srtp_err_status_t status = srtp_init();
    if(status){
      printf("srtp_init fail!\n");
      return 0;
    }

    uint8_t rKey[31]; // todo read recv key 
    memset(rKey, 0, 31);
    FILE* keyFile = fopen(argv[2], "rb");
    if(keyFile == NULL){
      printf("open key file fail!\n");
      return 0;
    }
    fread(rKey, 30, 1, keyFile);
    fclose(keyFile);

    srtp_policy_t policy;
    memset(&policy, 0, sizeof(srtp_policy_t));
    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtp);
    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtcp);

    policy.ssrc.value = 0;
    // TODO: adjust window_size
    policy.window_size = 8192;
    policy.allow_repeat_tx = 1;
    policy.next = NULL;
    policy.ssrc.type = ssrc_any_inbound;
    policy.key = rKey;

    srtp_t recv_ctx;
    status = srtp_create(&recv_ctx, &policy);
    if(status != srtp_err_status_ok)
    {
      printf("srtp_create failed!\n");
      return 0;
    }

    pcap_t *pcap_ptr = pcap_open_offline(argv[1], errbuf);
    
    pcap_t *pcap_dead = pcap_open_dead(DLT_EN10MB, 1500);
    if (!pcap_dead) {
        // 如果ptr为空，则说明抓包文件有问题，不是pcap或者pcapng
        printf("open dead pcap file failed!\n");
        return 0;
    }
    pcap_dumper_t* save_file = pcap_dump_open(pcap_dead, argv[3]);


    if (!pcap_ptr) {
        // 如果ptr为空，则说明抓包文件有问题，不是pcap或者pcapng
        printf("open pcap file failed!\n");
        return 0;
    }
    struct pcap_pkthdr pkthdr = { 0 };
    u_char packet[1500] = {0};
    
    while (true) {
        const u_char *pkt_buff = pcap_next(pcap_ptr, &pkthdr);  // 循环读取文件
        if (!pkt_buff) {
            printf("pcapng read over.");
            break;
        }
        if (pkthdr.caplen > 1500) {        // 读取文件异常
            printf("read pcap body error.");
            break;
        }
        // pkt_buff执行抓包数据 可进行相关业务处理
        memcpy(packet, pkt_buff, pkthdr.len);
        u_char* ptr = packet + ether_size + ip_size + udp_size;
        int rtpDateLen = pkthdr.len - (ether_size + ip_size + udp_size);
        int orgRtpLen = rtpDateLen;
        status = srtp_unprotect(recv_ctx, (void*)ptr, &rtpDateLen);
        if(status != srtp_err_status_ok)
        {
          printf("decode fail\n");
          continue;
        }
  
        ((struct iphdr*)(packet+ether_size))->totalLength = htons(ntohs(((struct iphdr*)(packet+ether_size))->totalLength) - (orgRtpLen-rtpDateLen));
        ((struct udphdr*)(packet+ether_size+ip_size))->length = htons(ntohs(((struct udphdr*)(packet+ether_size+ip_size))->length) - (orgRtpLen-rtpDateLen));
        pkthdr.caplen -= orgRtpLen-rtpDateLen;
        pkthdr.len -= orgRtpLen-rtpDateLen;
        
        pcap_dump((u_char*)save_file, &pkthdr, packet);

    }
    pcap_close(pcap_ptr);   // 必须关闭句柄 不然内存泄漏
    pcap_dump_close(save_file);

}
