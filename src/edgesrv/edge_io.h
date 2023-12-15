
#ifndef __EDGE_IO_H_
#define __EDGE_IO_H_

#ifdef __cplusplus
extern "C" {
#endif


int edge_body_sendto   (const char *buf, int len, void *kcp, void *user);
int edge_body_recvfrom (void * vbody, void * pdev);

int edge_udpdata_parse(void * vbody, uint8 * pbuf, int buflen, struct sockaddr_in * addr, void * pdev);


#ifdef __cplusplus
}
#endif

#endif

