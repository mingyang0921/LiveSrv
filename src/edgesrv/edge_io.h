
#ifndef __EDGE_IO_H_
#define __EDGE_IO_H_

#ifdef __cplusplus
extern "C" {
#endif

int is_ikcp_head(uint8 *buf, int len);
int is_push_head(uint8 *buf, int len);
int is_pull_head(uint8 *buf, int len);

int head_info_get(uint8 *buf, int len, uint8 *head, uint32 *ts, uint8 *type, uint32 *size);

int edge_body_recvfrom (void * vbody, void * pdev);

int edge_udpdata_parse(void * vbody, uint8 * pbuf, int buflen, struct sockaddr_in * addr, void * pdev);


#ifdef __cplusplus
}
#endif

#endif

