#include "adifall.ext"#include "epump.h"#include "edgesrv.h"#include "edge_io.h"
int sock_get_pending (SOCKET fd){    int  pending = 0;    int  ret = 0;
    if (fd == INVALID_SOCKET) return 0;
#if defined(FIONREAD)#ifdef WINDOWS        ret = ioctlsocket(fd, FIONREAD, (u_long *)&pending);#endif#ifdef UNIX        ret = ioctl(fd, FIONREAD, &pending);#endif#endif    if (ret >= 0) return pending;    return ret;}
int edge_body_recvfrom (void * vbody, void * pdev){    EdgeMgmt  *mgmt = (EdgeMgmt*)vbody;
	    uint8       rcvbuf[4096];    uint8     * pbuf = NULL;    int         toberead = 0;    struct sockaddr_in addr;    int         len = sizeof(addr);    int         ret = 0;    uint8       alloc = 0; 

		    if (!mgmt) return -1;
    while (1) {         toberead = sock_get_pending(iodev_fd(pdev));        if (toberead > sizeof(rcvbuf)) {             pbuf = kzalloc(toberead+1);            alloc = 1;        } else {             toberead = sizeof(rcvbuf);            pbuf = rcvbuf;             alloc = 0;        }
         memset(&addr, 0, sizeof(addr));        ret = recvfrom(iodev_fd(pdev), pbuf, toberead, 0,                        (struct sockaddr *)&addr, (socklen_t *)&len);        if (ret <= 0) {            if (alloc) { kfree(pbuf); pbuf = NULL; }            break;        }
 
#ifdef _DEBUG
printf("Recvfrom %s:%d %d bytes\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), ret);
//printOctet(stdout, pbuf, 0, ret > 200 ? 200 : ret, 2);
#endif        edge_udpdata_parse(mgmt, pbuf, ret, &addr, pdev);        if (alloc) { kfree(pbuf); pbuf = NULL; }    }
     return 0;}

int edge_udpdata_parse(void * vbody, uint8 * pbuf, int buflen, struct sockaddr_in * addr, void * pdev){    EdgeMgmt    *mgmt = (EdgeMgmt *)vbody;
   uint64       sessid = 0;    uint64       runid  = 0;    if(buflen < sizeof(int)){        printf("edge_udpdata_parse [%s][%d] small message\n",pbuf,buflen);        return -1;    }    printf("edge_udpdata_parse [%c][%c][%c][%c]\n",pbuf[0],pbuf[1],pbuf[2],pbuf[3]);    if(strncmp(pbuf,"type",strlen("type")) == 0){        push_sess_para(pbuf,buflen,&sessid,&runid);        // open a push session        push_sess_restart(mgmt,sessid,runid);        // update addr        push_sess_addr(mgmt,sessid,addr);                // response hb        push_sess_sendkcp(mgmt,sessid,"hb",strlen("hb"));                return 0;    }    if(pbuf[0] == 'A' && pbuf[1] == 'V' && pbuf[2] == 'K' && pbuf[3] == 'C' ){        printf("#################\n");        push_sess_input(mgmt,pbuf,buflen,addr);    }    return 0;}

