#include <pcap/pcap.h>
#include <stdio.h>
#include "srtp.h"
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "netDataHead.h"


bool packetIsUdp(const u_char *packetHead, int packetLen)
{
    struct iphdr* ipHeader = (struct iphdr*)(packetHead+sizeof(struct ether_header));

    if(ipHeader->protocol == 17){
    return true;
    }else{
    return false;
    }

}

bool packetIsNeedPort(const u_char *udpHeader, int packetLen, int clientPort, bool& recv)
{
    struct udphdr* header = (struct udphdr*)udpHeader;
    if(ntohs(header->portDst) == clientPort){
        recv = false;
        return true;
       
    } else if(ntohs(header->portSrc) == clientPort){
        recv = true;
        return true;
    }else{
        return false;
    }
}



bool packetIsRtp(const u_char *udpDataHeader, int packetLen )
{
    auto header = const_cast<RTPHeader*>(reinterpret_cast<const RTPHeader*>(udpDataHeader));

    return (
        (packetLen >= 12) &&
        // DOC: https://tools.ietf.org/html/draft-ietf-avtcore-rfc5764-mux-fixes
        (udpDataHeader[0] > 127 && udpDataHeader[0] < 192) &&
        // RTP Version must be 2.
        (header->version == 2)
    );
}

bool packetIsRtcp(const u_char *udpDataHeader, int packetLen )
{
    auto header = const_cast<RTCPCommonHeader*>(reinterpret_cast<const RTCPCommonHeader*>(udpDataHeader));
	
    return (
        (packetLen >= 4) &&
        // DOC: https://tools.ietf.org/html/draft-ietf-avtcore-rfc5764-mux-fixes
        (udpDataHeader[0] > 127 && udpDataHeader[0] < 192) &&
        // RTP Version must be 2.
        (header->version == 2) &&
        // RTCP packet types defined by IANA:
        // http://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml#rtp-parameters-4
        // RFC 5761 (RTCP-mux) states this range for secure RTCP/RTP detection.
        (header->packetType >= 192 && header->packetType <= 223)
    );
}


bool createSrtpSession(srtp_t &srtpSeesion, int cryptoSuit, const char* keyFileName, int recv)
{
    srtp_err_status_t status;
    srtp_policy_t policy;
    memset(&policy, 0, sizeof(srtp_policy_t));
    int keyLen;

    switch (cryptoSuit)
	  {
			case 1:  // AEAD_AES_256_GCM
			{
				srtp_crypto_policy_set_aes_gcm_256_16_auth(&policy.rtp);
				srtp_crypto_policy_set_aes_gcm_256_16_auth(&policy.rtcp);
        keyLen = 44;

				break;
			}
			case 2:   // AEAD_AES_128_GCM
			{
				srtp_crypto_policy_set_aes_gcm_128_16_auth(&policy.rtp);
				srtp_crypto_policy_set_aes_gcm_128_16_auth(&policy.rtcp);
        keyLen = 28;
				break;
			}

			case 3:  // AES_CM_128_HMAC_SHA1_80
			{
				srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtp);
				srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtcp);
        keyLen = 30;
				break;
			}

			case 4: //AES_CM_128_HMAC_SHA1_32:
			{
				srtp_crypto_policy_set_aes_cm_128_hmac_sha1_32(&policy.rtp);
				// NOTE: Must be 80 for RTCP.
				srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtcp);
        keyLen = 30;
				break;
			}

      default:
        printf("srtp crypto suit[%d] error\n", cryptoSuit);
        return false;
    }

    uint8_t rKey[keyLen]; 
    memset(rKey, 0, keyLen);
    FILE* keyFile = fopen(keyFileName, "rb");
    if(keyFile == NULL){
      printf("open key file fail!\n");
      return false;
    }
    int n = fread(rKey, 1, keyLen, keyFile);
    fclose(keyFile);
    if(n != keyLen)
    {
      printf("read key file[%s] error\n", keyFileName);
      return false;
    }

    policy.ssrc.value = 0;
    // TODO: adjust window_size
    policy.window_size = 8192;
    policy.allow_repeat_tx = 1;
    policy.next = NULL;
    if(recv == 1){
      policy.ssrc.type = ssrc_any_inbound;
    }else{
      policy.ssrc.type = ssrc_any_outbound;
    }
    policy.key = rKey;

    status = srtp_create(&srtpSeesion, &policy);
    if(status != srtp_err_status_ok)
    {
      printf("srtp_create failed!\n");
      return false;
    }

    return true;
}


int ether_size = sizeof(struct ether_header);
int ip_size = sizeof(struct iphdr);
int udp_size = sizeof(struct udphdr);

int main(int argc, char* argv[])
{
    if(argc < 6){
      printf("./proc pcap recvkeyFile recvcryptoSuit sendKeyFile sendcryptoSuit outPcap clientPort");
      return 0;
    }

    // 1. 生成srtpsession
    srtp_err_status_t status = srtp_init();
    if(status){
      printf("srtp_init fail!\n");
      return 0;
    }

    srtp_t recv_ctx;
    if(false == createSrtpSession(recv_ctx, atoi(argv[3]), argv[2], 1))
    {
      return 0;
    }
    srtp_t send_ctx;
    if(false == createSrtpSession(send_ctx, atoi(argv[5]), argv[4], 0))
    {
      return 0;
    }

    // 2. 打开pcap文件
    char errbuf[PCAP_ERRBUF_SIZE] = ""; // PCAP_ERRBUF_SIZE为512字节
    pcap_t *pcap_ptr = pcap_open_offline(argv[1], errbuf);
     if (!pcap_ptr) {
        // 如果ptr为空，则说明抓包文件有问题，不是pcap或者pcapng
        printf("open pcap file failed!\n");
        return 0;
    }
    
    pcap_t *pcap_dead = pcap_open_dead(DLT_EN10MB, 1500);
    if (!pcap_dead) {
        // 如果ptr为空，则说明抓包文件有问题，不是pcap或者pcapng
        printf("open dead pcap file failed!\n");
        return 0;
    }
    pcap_dumper_t* save_file = pcap_dump_open(pcap_dead, argv[6]);
    if(save_file == NULL)
    {
      printf("save file failed!\n");
      return 0;
    }
   
    // 3. 进行解密
    struct pcap_pkthdr pkthdr = { 0 };
    u_char packet[1500] = {0};
    int totalCount = 0;
    int decodeSuc = 0;
    int decodeFail = 0;
    int unRTPorRTCPcnt = 0;
    int recvCnt = 0;
    int sendCnt = 0;
    while (true) {
        const u_char *pkt_buff = pcap_next(pcap_ptr, &pkthdr);  // 循环读取文件
        if (!pkt_buff) {
            printf("pcapng read over.");
            break;
        }
        if (pkthdr.caplen > 1500) {        // 读取文件异常
            continue;
        }

        if(packetIsUdp(pkt_buff, pkthdr.caplen) == false){
            continue;
        }
        
        bool recvFlag;
        srtp_t* srtpSesson = NULL;
        if(packetIsNeedPort(pkt_buff+(ether_size+ip_size), pkthdr.len-(ether_size+ip_size), atoi(argv[7]), recvFlag))
        {
            if(recvFlag){
                recvCnt++;
                srtpSesson = &recv_ctx;
            }else{
                sendCnt++;
                srtpSesson = &send_ctx;
            }

            totalCount++;
            memcpy(packet, pkt_buff, pkthdr.len);
            u_char* ptr = packet + ether_size + ip_size + udp_size;
            int rtpDateLen = pkthdr.len - (ether_size + ip_size + udp_size);
            int orgRtpLen = rtpDateLen;
            if(packetIsRtcp(ptr, rtpDateLen)) {
                status = srtp_unprotect_rtcp(*srtpSesson, (void*)ptr, &rtpDateLen);
            }else if(packetIsRtp(ptr, rtpDateLen)){
                status = srtp_unprotect(*srtpSesson, (void*)ptr, &rtpDateLen);
            }else{
                // 不要保持stun或者dtls包，不然wireshark会把rtp显示为srtp
                //pcap_dump((u_char*)save_file, &pkthdr, packet);
                unRTPorRTCPcnt++;
                continue;
            }
            
            if(status != srtp_err_status_ok)
            {
                decodeFail++;
                continue;
            }else{
                decodeSuc++;
            }
    
            ((struct iphdr*)(packet+ether_size))->totalLength = htons(ntohs(((struct iphdr*)(packet+ether_size))->totalLength) - (orgRtpLen-rtpDateLen));
            ((struct udphdr*)(packet+ether_size+ip_size))->length = htons(ntohs(((struct udphdr*)(packet+ether_size+ip_size))->length) - (orgRtpLen-rtpDateLen));
            pkthdr.caplen -= orgRtpLen-rtpDateLen;
            pkthdr.len -= orgRtpLen-rtpDateLen;
            
            pcap_dump((u_char*)save_file, &pkthdr, packet);
            
            
        }
    }

    printf("total: %d, suc: %d, fail: %d, unRTP: %d send: %d, recv: %d\n",totalCount, decodeSuc, decodeFail, unRTPorRTCPcnt, sendCnt, recvCnt);
    pcap_close(pcap_ptr);   // 必须关闭句柄 不然内存泄漏
    pcap_dump_close(save_file);

}
