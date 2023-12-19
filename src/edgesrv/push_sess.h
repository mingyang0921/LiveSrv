#ifndef _PUSH_SESS_H_
#define _PUSH_SESS_H_ 


#ifdef __cplusplus
extern "C" {
#endif

#define t_push_kcp_sess_check 2000

typedef struct Push_sess {
	ulong              sessid;
    uint64             runid;

    uint8              addid[64];

	struct in_addr     peerip;
	uint16             peerport;

    uint8              mdinfo[256];//media info
    int                mdinfo_len;

    CRITICAL_SECTION   kcpCS;
    void             * kcp;
    void             * kcp_timer;

    CRITICAL_SECTION   pullCS;
    arr_t            * pull_list;

	time_t             ctime;
	time_t             stamp;

	void             * mgmt;
} PushSess;


int   push_sess_cmp_sessid     (void *a, void *b);
ulong push_sess_id_hash_func   (void *key);
int   push_sess_sort_by_time (void *a, void *b);

int   push_sess_init    (void *vsess);
int   push_sess_free    (void *vsess);

void *push_sess_fetch   (void *vmgmt);
int   push_sess_recycle (void *vsess);

void *push_sess_open    (void *vmgmt, ulong sessid, uint64 runid);
int   push_sess_close   (void *vsess);
int   push_sess_restart (void *vmgmt, uint64 sessid, uint64 runid);

int   push_sess_para(uint8 *pbuf, int buflen, uint64 *sessid, uint64 *runid);

int   push_sess_check (void *vsess);

int   push_sess_input(void *vmgmt, uint64 sessid, uint8 *pbuf, int buflen);

int   push_sess_add_list(void *vmgmt, ulong sessid, ulong pullid);
int   push_sess_get_list(void *vsess,void * arr);
int   push_sess_build_list(void *vsess);

int   push_body_sendto   (const char *buf, int len, void *kcp, void *user);

int   push_sess_addr(void *vmgmt, uint64 sessid, struct sockaddr_in * addr);
int   push_sess_sendkcp(void *vmgmt, uint64 sessid, uint8 *buf, int buflen);
int   push_sess_response(void *vsess, uint8 *buf, int buflen);

int   push_sess_addlist(void *vmgmt, ulong sessid, ulong pullid);

#ifdef _cplusplus
}
#endif

#endif




