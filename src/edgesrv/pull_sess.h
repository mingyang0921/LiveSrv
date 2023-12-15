#ifndef _PULL_SESS_H_
#define _PULL_SESS_H_ 


#ifdef __cplusplus
extern "C" {
#endif

typedef struct Pull_sess {

	ulong              sessid;

    uint64             runid;

    uint8              sps[128];
    int                spslen;
    uint8              pps[128];
    int                ppslen;


	void             * video;      // save rtp video package
	void             * audio;      // save rtp audio package

	CRITICAL_SECTION   naluCS;
	arr_t           * nalu_list;  // save nalu 
	uint32             lastnaluid;
    uint32             lastaunaluid;


	void             * hearttimer;  
    void             * logtimer;    // every 1 minites save once log
    void             * runtimer;

	time_t             ctime;
	time_t             stamp;

	void             * mgmt;
	
} PullSess;


int    pull_sess_cmp_sessid     (void * a, void * b);
ulong  pull_sess_id_hash_func   (void * key);
int    pull_sess_sort_by_time (void * a, void * b);

int    pull_sess_init    (void * vsess);
int    pull_sess_free    (void * vsess);

void * pull_sess_fetch   (void *vmgmt,uint8 nodelay,uint8 interval,uint8 resend,uint8 nc);
int    pull_sess_recycle (void * vsess);
int    pull_sess_restart(void * vmgmt, uint64 sessid, uint64 runid);


void * pull_sess_open    (void  *vmgmt);
int    pull_sess_close   (void *vsess);


int    pull_update_check (void * vsess);

#ifdef _cplusplus
}
#endif

#endif



