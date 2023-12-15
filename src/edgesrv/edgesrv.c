
#include "adifall.ext"
#include "epump.h"
#include "ejet.h"

#include "pull_sess.h"
#include "push_sess.h"
#include "edge_io.h"
#include "edgesrv.h"


int edge_mgmt_conf (void * vmgmt, void * hconf)
{
    EdgeMgmt   * mgmt = (EdgeMgmt *)vmgmt;
    uint8       varsect[64];	
	uint8     * pstr = NULL;
	int         i = 0;

    if (!mgmt) return -1;
    if (!hconf) hconf = mgmt ->hconf;

    mgmt->nodeid = conf_get_int(hconf, "Edge Service Node", "Edge Nodeid");

    mgmt->udp_port = conf_get_int(hconf, "Edge Service Node", "UDP Listen Port");
    if(mgmt->udp_port <= 0 ) mgmt->udp_port = 54321;

    mgmt->logpath = conf_get_string(hconf, "Edge Service Node", "Log Path");
    if (mgmt->logpath == NULL || strlen(mgmt->logpath) < 1)
        mgmt->logpath = ".";

    mgmt->logstatus = conf_get_int(hconf, "Sks Service Node", "Log Status");
    if(mgmt->logstatus <= 0 ) mgmt->logstatus = 0;

    return 0;
}


void * edge_mgmt_init ( void * hconf, void *pcore, void * http_mgmt)
{
    EdgeMgmt   *mgmt = NULL;

    mgmt = kzalloc(sizeof(*mgmt));
    if (!mgmt){
        return NULL;
    }

    mgmt->pcore = pcore;
    mgmt->hconf = conf_mgmt_init(hconf);
    mgmt->http_mgmt = http_mgmt;
    mgmt->frame_pool = http_get_frame_pool(http_mgmt);
    
    edge_mgmt_conf(mgmt,mgmt->hconf); 

    InitializeCriticalSection(&mgmt->logfpCS);
    mgmt->logdate = 0;
    mgmt->logfp = NULL;

    InitializeCriticalSection(&mgmt->pushCS);  
    mgmt->push_list = arr_new(16);
    mgmt->push_table = ht_only_new(200,push_sess_cmp_sessid);
    ht_set_hash_func(mgmt->push_table, push_sess_id_hash_func);
    mgmt->pushaddr_table = ht_only_new(200,addr_sess_cmp_sessid);
    
    InitializeCriticalSection(&mgmt->pullCS);
    mgmt->pull_list = arr_new(16);
    mgmt->pull_table = ht_only_new(200,pull_sess_cmp_sessid);
    ht_set_hash_func(mgmt->pull_table, pull_sess_id_hash_func);


    if (!mgmt->push_pool) {
        mgmt->push_pool = bpool_init(NULL);
        bpool_set_initfunc(mgmt->push_pool, push_sess_init);
        bpool_set_freefunc(mgmt->push_pool, push_sess_free);
        bpool_set_unitsize(mgmt->push_pool, sizeof(PushSess));
        bpool_set_allocnum(mgmt->push_pool, 4096);
    }

    if (!mgmt->pull_pool) {
        mgmt->pull_pool = bpool_init(NULL);
        bpool_set_initfunc(mgmt->pull_pool, pull_sess_init);
        bpool_set_freefunc(mgmt->pull_pool, pull_sess_free);
        bpool_set_unitsize(mgmt->pull_pool, sizeof(PullSess));
        bpool_set_allocnum(mgmt->pull_pool, 4096);
    }   

    edge_udp_socket_start(mgmt);
    
    return mgmt; 

}

int edge_mgmt_clean (void * vmgmt)
{
    EdgeMgmt * mgmt = (EdgeMgmt *)vmgmt;

    if (!mgmt) return -1;

    edge_udp_socket_stop(mgmt);

    if (mgmt->logfp) fclose(mgmt->logfp);
    DeleteCriticalSection(&mgmt->logfpCS);  

    DeleteCriticalSection(&mgmt->pushCS);
    ht_free(mgmt->addr_table);

    DeleteCriticalSection(&mgmt->pushCS);
    arr_pop_free(mgmt->push_list, push_sess_free);
    ht_free(mgmt->push_table); 
    
    DeleteCriticalSection(&mgmt->pullCS);
    arr_pop_free(mgmt->pull_list, pull_sess_free);
    ht_free(mgmt->pull_table);

    if (mgmt->push_pool)  {
        bpool_clean(mgmt->push_pool);
        mgmt->push_pool = NULL;  
    }

    if (mgmt->pull_pool)  {
        bpool_clean(mgmt->pull_pool);
        mgmt->pull_pool = NULL;  
    }

    kfree(mgmt);
    return 0;

}

int edge_udp_socket_start(void *vmgmt)
{
	EdgeMgmt  *mgmt = (EdgeMgmt*)vmgmt;
    int       ret =0;

    if(!mgmt) return -1;

    mgmt->listendev_udp = epudp_client2(mgmt->pcore,
                              NULL, (uint16)mgmt->udp_port,
                              NULL, &ret, 
                              edge_body_pump, mgmt);
	
	return 0;
}

int edge_udp_socket_stop(void *vmgmt)
{
	EdgeMgmt  *mgmt = (EdgeMgmt*)vmgmt;

	if(!mgmt) return -1;

	if(mgmt->listendev_udp){
		iodev_close(mgmt->listendev_udp);
		mgmt->listendev_udp = NULL; 
	}
	
	return 0;
}

int edge_body_pump (void * vbody, void * vobj, int event, int fdtype)
{
    EdgeMgmt    *body = (EdgeMgmt*)vbody;
    int          cmd = 0;
	ulong        sessid = 0;
    void        *sess = NULL;

#ifdef _DEBUG
printf("\nEdge_Ifac_Pump: "); 
if (event == IOE_CONNECTED) printf("IOE_CONNECTED ");
else if (event == IOE_CONNFAIL) printf("IOE_CONNFAIL ");
else if (event == IOE_ACCEPT) printf("IOE_ACCEPT ");
else if (event == IOE_READ) printf("IOE_READ ");
else if (event == IOE_WRITE) printf("IOE_WRITE ");
else if (event == IOE_TIMEOUT) printf("IOE_TIMEOUT ");
else printf("Unknown ");
printf(" Type: ");
if (fdtype == FDT_LISTEN) printf("FDT_LISTEN");
else if (fdtype == FDT_USOCK_LISTEN) printf("FDT_USOCK_LISTEN");
else if (fdtype == FDT_CONNECTED) printf("FDT_CONNECTED");
else if (fdtype == FDT_ACCEPTED) printf("FDT_ACCEPTED");
else if (fdtype == FDT_UDPSRV) printf("FDT_UDPSRV");
else if (fdtype == FDT_UDPCLI) printf("FDT_UDPCLI");
else if (fdtype == FDT_TIMER) printf("FDT_TIMER");
else if (fdtype == FDT_USERCMD) printf("FDT_USERCMD");
else printf("Unknown Type");
if (event != IOE_TIMEOUT) printf(" FD: %d", iodev_fd(vobj));
printf("\n");
#endif
 
    switch (event) {
    case IOE_ACCEPT:
        break;

    case IOE_INVALID_DEV:
        break;
 
    case IOE_READ:
		if(fdtype == FDT_UDPSRV ||  fdtype == FDT_UDPCLI ){
			return edge_body_recvfrom (body,vobj);
		}
        break;
 
    case IOE_WRITE:
        break;
 
    case IOE_TIMEOUT:
        cmd = iotimer_cmdid(vobj);
        if(cmd == t_push_kcp_sess_check) {
                sessid = (ulong)iotimer_para(vobj);         
                sess = push_mgmt_sess_get(body, sessid);
                if (sess) {
                    push_sess_check (sess);
                }
            return 0;
        }
        
	
        break;
    default:
       return -1;
    }
 
    return -1;
}

void *push_mgmt_sess_get  (void * vmgmt, ulong sessid)
{
	EdgeMgmt  *mgmt = (EdgeMgmt *)vmgmt;
	PushSess  *sess = NULL;	

	if(!mgmt) return NULL;

	EnterCriticalSection(&mgmt->pushCS);
	sess = ht_get(mgmt->push_table,&sessid);
	LeaveCriticalSection(&mgmt->pushCS);		

	return sess;
}

int  push_mgmt_sess_add  (void * vmgmt, void *vsess)
{
	EdgeMgmt  *mgmt = (EdgeMgmt *)vmgmt;	
	PushSess  *sess = (PushSess *)vsess;	
	PushSess  *tmp  = NULL;

	if(!mgmt || !sess) return -1;

	EnterCriticalSection(&mgmt->pushCS);
	tmp = ht_get(mgmt->push_table,&sess->sessid);
	if(!tmp){
		ht_set(mgmt->push_table,&sess->sessid,sess);
		arr_push(mgmt->push_list, sess);
	}
	LeaveCriticalSection(&mgmt->pushCS);		

	return 0;
}


void *push_mgmt_sess_del  (void * vmgmt, ulong sessid)
{
    EdgeMgmt    *mgmt = (EdgeMgmt *)vmgmt;
    PushSess    *sess = NULL;
 
    if (!mgmt) return NULL;
 
    EnterCriticalSection(&mgmt->pushCS);
    sess  = ht_delete(mgmt->push_table, &sessid);
    if (sess ){
        arr_delete_ptr(mgmt->push_list, sess);
	}
    LeaveCriticalSection(&mgmt->pushCS);

    return sess;
}

void *pull_mgmt_sess_get  (void * vmgmt, ulong sessid)
{
	EdgeMgmt  *mgmt = (EdgeMgmt *)vmgmt;
	PushSess  *sess = NULL;	

	if(!mgmt) return NULL;

	EnterCriticalSection(&mgmt->pullCS);
	sess = ht_get(mgmt->pull_table,&sessid);
	LeaveCriticalSection(&mgmt->pullCS);		

	return sess;
}

int  pull_mgmt_sess_add  (void * vmgmt, void *vsess)
{
	EdgeMgmt  *mgmt = (EdgeMgmt *)vmgmt;	
	PushSess  *sess = (PushSess *)vsess;
	PushSess  *tmp  = NULL;

	if(!mgmt || !sess) return -1;

	EnterCriticalSection(&mgmt->pullCS);
	tmp = ht_get(mgmt->pull_table,&sess->sessid);
	if(!tmp){
		ht_set(mgmt->push_table,&sess->sessid,sess);
		arr_push(mgmt->pull_list, sess);
	}
	LeaveCriticalSection(&mgmt->pullCS);		

	return 0;
}


void *pull_mgmt_sess_del  (void * vmgmt, ulong sessid)
{
    EdgeMgmt    *mgmt = (EdgeMgmt *)vmgmt;
    PushSess    *sess = NULL;
 
    if (!mgmt) return NULL;
 
    EnterCriticalSection(&mgmt->pullCS);
    sess  = ht_delete(mgmt->pull_table, &sessid);
    if (sess ){
        arr_delete_ptr(mgmt->pull_list, sess);
	}
    LeaveCriticalSection(&mgmt->pullCS);

    return sess;
}

int  pull_mgmt_get_list(void *vmgmt, void * arr)
{
    EdgeMgmt   *mgmt = (EdgeMgmt *)vmgmt;
    arr_t      *explist = (arr_t *)arr;
	void       *tmp = NULL;
	int         num = 0;
	int         i  = 0;

	if(!mgmt) return -1;
	
	EnterCriticalSection(&mgmt->pullCS);
	num =arr_num(mgmt->pull_list);
	for(i=0;i<num;i++){
		tmp = arr_value(mgmt->pull_list,i);
		if(tmp && arr){
			arr_push(explist, tmp);
		}
	}
	LeaveCriticalSection(&mgmt->pullCS);	

	return 0;
}

int addr_sess_cmp_sessid (void * a, void * b)
{
    ulong    va = (ulong)a;
    ulong    vb = (ulong)b;	

    if(va == vb) return 0;

    return -1;
}

int    addr_mgmt_sess_add(void *vmgmt, uint8 *addr, ulong sessid)
{
	EdgeMgmt  *mgmt = (EdgeMgmt *)vmgmt;	
    void      *id = NULL;

	if(!mgmt) return -1;

	EnterCriticalSection(&mgmt->addrCS);
	id = ht_get(mgmt->addr_table,addr);
	if(!id){
		ht_set(mgmt->addr_table,addr,sessid);
	}
	LeaveCriticalSection(&mgmt->addrCS);

	return 0;
}

int    addr_mgmt_sess_get(void *vmgmt, uint8 *addr, ulong *sessid)
{
	EdgeMgmt  *mgmt = (EdgeMgmt *)vmgmt;
    void      *id = NULL;

	if(!mgmt) return -1;

	EnterCriticalSection(&mgmt->addrCS);
	id = ht_get(mgmt->addr_table,addr);
	LeaveCriticalSection(&mgmt->addrCS);
    
    *sessid = id;

    return 0;
}

int    addr_mgmt_sess_del(void *vmgmt, uint8 *addr)
{
    EdgeMgmt    *mgmt = (EdgeMgmt *)vmgmt;
 
    if (!mgmt) return -1;
 
    EnterCriticalSection(&mgmt->addrCS);
    ht_delete(mgmt->addr_table,addr);
    LeaveCriticalSection(&mgmt->addrCS);

    return 0;
}

static void itimeofday(long *sec, long *usec)
{
	#if defined(__unix)
	struct timeval time;
	gettimeofday(&time, NULL);
	if (sec) *sec = time.tv_sec;
	if (usec) *usec = time.tv_usec;
	#else
	static long mode = 0, addsec = 0;
	BOOL retval;
	static IINT64 freq = 1;
	IINT64 qpc;
	if (mode == 0) {
		retval = QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
		freq = (freq == 0)? 1 : freq;
		retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
		addsec = (long)time(NULL);
		addsec = addsec - (long)((qpc / freq) & 0x7fffffff);
		mode = 1;
	}
	retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
	retval = retval * 2;
	if (sec) *sec = (long)(qpc / freq) + addsec;
	if (usec) *usec = (long)((qpc % freq) * 1000000 / freq);
	#endif
}


int64 iclock64(void)
{
	long s, u;
	int64 value;
	itimeofday(&s, &u);
	value = ((int64)s) * 1000 + (u / 1000);
	return value;
}

int32 iclock()
{
	return (int32)(iclock64() & 0xfffffffful);
}

