#ifndef _PULL_SESS_H_
#define _PULL_SESS_H_ 


#ifdef __cplusplus
extern "C" {
#endif

#define t_pull_kcp_sess_check 2001


typedef struct Pull_sess {

	ulong              sessid;
    uint64             runid;
    uint64             pushid;

    uint8              ifHasInfo;

	struct in_addr     peerip;
	uint16             peerport;

    CRITICAL_SECTION   kcpCS;
    void             * kcp;
    void             * kcp_timer;

	time_t             ctime;
	time_t             stamp;

	void             * mgmt;
	
} PullSess;


int    pull_sess_cmp_sessid     (void * a, void * b);
ulong  pull_sess_id_hash_func   (void * key);
int    pull_sess_sort_by_time (void * a, void * b);

int    pull_sess_init    (void * vsess);
int    pull_sess_free    (void * vsess);

void * pull_sess_fetch   (void *vmgmt);
int    pull_sess_recycle (void * vsess);

int    pull_sess_restart(void * vmgmt, uint64 sessid, uint64 runid);

void * pull_sess_open    (void  *vmgmt, ulong sessid, uint64 runid);
int    pull_sess_close   (void *vsess);

int    pull_mgmt_onlyinfo (void *vmgmt, ulong sessid, uint8 *buf, int buflen);
int    pull_mgmt_stream   (void *vmgmt, ulong sessid, uint8 *buf, int buflen, uint8 *info, int infolen);

int    pull_sess_check (void * vsess);
int    pull_body_sendto   (const char *buf, int len, void *kcp, void *user);


int    pull_sess_para(uint8 *buf, int buflen, uint64 *sessid, uint64 *runid, uint64 *pushid);
int    pull_sess_addr(void *vmgmt, uint64 sessid, struct sockaddr_in * addr);


int    pull_sess_input(void *vmgmt, uint64 sessid, uint8 *pbuf, int buflen);

#ifdef _cplusplus
}
#endif

#endif



