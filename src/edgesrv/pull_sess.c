
#include "adifall.ext"
#include "epump.h"

#include "ikcp.h"
#include "edgesrv.h"
#include "pull_sess.h"

int pull_sess_cmp_sessid (void * a, void * b)
{
    PullSess    * sess = (PullSess *)a;
    ulong        sessid = *(ulong *)b;	

    if (!sess ) return -1;

    if (sess->sessid > sessid) return 1;
    if (sess->sessid < sessid) return -1;

    return 0;
}
ulong pull_sess_id_hash_func (void * key)
{
    ulong sessid = *(ulong *)key;

	return sessid;
}

int pull_sess_sort_by_time (void * a, void * b)
{
    PullSess * sessa = *(PullSess **)a;
    PullSess * sessb = *(PullSess **)b;
 
    if (!sessa || !sessb) return -1;
 
    if (sessa->stamp > sessb->stamp) return -1;
    if (sessa->stamp < sessb->stamp) return 1;

    return 0;
}

int pull_sess_init (void * vsess)
{
    PullSess  * sess = (PullSess *)vsess;
    
    if (!sess) return -1;

    sess->runid = 0;
    sess->sessid = 0;
    sess->pushid = 0;

    sess->peerport = 0;
    sess->ifHasInfo = FALSE;
    sess->state = PULL_NULL;

	sess->kcp_timer = NULL;
    InitializeCriticalSection(&sess->kcpCS);
    sess->kcp = NULL;
    
    sess->mgmt = NULL;
    return 0;
}

int pull_sess_free(void *vsess)
{
    PullSess * sess = (PullSess *)vsess;

	if(!sess) return -1;

 	DeleteCriticalSection(&sess->kcpCS);

	kfree(sess);
	return 0;
}

void *pull_sess_fetch (void * vmgmt)
{
    EdgeMgmt  *mgmt  = (EdgeMgmt *)vmgmt;
    PullSess  *sess = NULL;
 
    if (!mgmt) return NULL;
 
    sess = bpool_fetch(mgmt->pull_pool);
    if (!sess) {
        sess = kzalloc(sizeof(*sess));
        pull_sess_init(sess);
    }
    if (!sess) return NULL;

	sess->kcp = ikcp_create(AVKCP_CONV,sess);
	if(!sess->kcp){
        bpool_recycle(mgmt->pull_pool, sess);
        return NULL;
    };

    ikcp_setoutput(sess->kcp, (int (*)(const char*, int, ikcpcb *, void* ))pull_body_sendto);
    ikcp_nodelay(sess->kcp, 1, 10, 2, 1);
    ikcp_wndsize(sess->kcp, 256, 1024);
    
    //sess->ikcp->interval = 10;
    //sess->ikcp->rx_minrto = 50;
    //sess->ikcp->fastresend = 1;
    //sess->ikcp->stream = 1;
    ikcp_setinfo(sess->kcp);
 
	sess->mgmt = mgmt;
	sess->ctime = time(&sess->stamp);

	return sess;
}

int  pull_sess_recycle (void *vsess)
{
    EdgeMgmt  *mgmt  = NULL;
    PullSess  *sess = (PullSess*)vsess;
    void      *tmp = NULL;
 
	if (!sess) return -1;
	
	mgmt = (EdgeMgmt *)sess->mgmt;
	sess->mgmt = NULL;

    if (sess->kcp_timer) {
        iotimer_stop(sess->kcp_timer);
        sess->kcp_timer = NULL;
    }

	if(sess->kcp) {
		tmp = sess->kcp;
		sess->kcp = NULL;
		ikcp_release(tmp);
		tmp=NULL;
	}
 
	if (!mgmt || !mgmt->push_pool)
		return pull_sess_free(sess);
	 
	bpool_recycle(mgmt->pull_pool, sess);

	return 0;
}


void *pull_sess_open (void *vmgmt, ulong sessid, uint64 runid)
{
    EdgeMgmt  *mgmt  = (EdgeMgmt*)vmgmt;
    PullSess  *sess = NULL;	

	if (!mgmt) return NULL;	

	EnterCriticalSection(&mgmt->pullCS);	
	sess = ht_get(mgmt->pull_table,&sessid);
	if(sess){
		time(&sess->stamp);
		LeaveCriticalSection(&mgmt->pullCS);	
		return sess;
	}

	sess = pull_sess_fetch (mgmt);
	if(!sess){
		LeaveCriticalSection(&mgmt->pullCS);
		return NULL;
	}
	sess->sessid = sessid;
    sess->runid  = runid;

	ht_set(mgmt->pull_table,&sess->sessid,sess);
	arr_push(mgmt->pull_list, sess);	

	LeaveCriticalSection(&mgmt->pullCS);

    pull_sess_check(sess);

	return sess;
}

int  pull_sess_close(void *vsess)
{
	PullSess    *sess = (PullSess *)vsess;
    EdgeMgmt    *mgmt = NULL;
	PullSess	*tmp  = NULL;

	if(!sess) return -1;

	mgmt = sess->mgmt;
	if(!mgmt) return -1;

	tmp = pull_mgmt_sess_del(mgmt,sess->sessid);
	if(tmp != sess) return -1;
	
	return pull_sess_recycle(sess);
}

int   pull_sess_restart (void *vmgmt, uint64 sessid, uint64 runid)
{
	PullSess    *sess = NULL;
    EdgeMgmt    *mgmt = (EdgeMgmt*)vmgmt;

    if(!mgmt) return -1;

    sess = pull_mgmt_sess_get(mgmt,sessid);
    if(!sess){
        sess = pull_sess_open (mgmt,sessid,runid);
        return 0;
    }

    if(sess->runid != runid){
        pull_sess_close(sess);
        pull_sess_open (mgmt,sessid,runid);
    }
    
    return 0;
}

int    pull_mgmt_stream(void *vmgmt, ulong sessid, uint8 *buf, int buflen, uint8 *info, int infolen)
{
    EdgeMgmt    *mgmt = (EdgeMgmt*)vmgmt;
    PullSess    *sess = NULL;

    if(!mgmt) return -1;

    sess = pull_mgmt_sess_get(mgmt,sessid);
    if(!sess) return -2;

    if(!sess->ifHasInfo){
        // head
        //printf("pull_sess_sendkcp 1 sess->state=%u\n",sess->state);
        pull_sess_sendkcp(mgmt,sessid,info,infolen);
        sess->ifHasInfo = TRUE;
        if(sess->state == PULL_NULL) sess->state = PULL_INFO;
        //printf("pull_sess_sendkcp 2 sess->state=%u\n",sess->state);
    }

    // send body
    if(sess->state == PULL_INFO){
        if(buf[0] == 'T' && (/*buf[4] == 'A' ||*/ buf[4] == 'V')){
            pull_sess_sendkcp(mgmt,sessid,buf,buflen);
            sess->state = PULL_STREAM;
            //printf("pull_sess_sendkcp 3 sess->state=%u\n",sess->state);
        }
    }else if(sess->state == PULL_STREAM){
        pull_sess_sendkcp(mgmt,sessid,buf,buflen);
        //printf("pull_sess_sendkcp 4 sess->state=%u\n",sess->state);
    }

    return 0;
}

int    pull_mgmt_onlyinfo(void *vmgmt, ulong sessid, uint8 *buf, int buflen)
{
    EdgeMgmt    *mgmt = (EdgeMgmt*)vmgmt;
    PullSess    *sess = NULL;

    if(!mgmt) return -1;

    sess = pull_mgmt_sess_get(mgmt,sessid);
    if(!sess) return -2;
    
    sess->ifHasInfo = TRUE;
    pull_sess_sendkcp(mgmt,sessid,buf,buflen);

    if(sess->state == PULL_NULL) sess->state = PULL_INFO;

    return 0;
}

int pull_sess_check (void * vsess)
{
    PullSess *sess = (PullSess*)vsess;
    EdgeMgmt *mgmt = NULL;

	uint32     interval = 0;
	uint32     next_time = 0;
	uint32     now_time = 0;
    time_t     curt = 0;

    if (!sess) return -1;

#ifdef _DEBUG
    //printf("pull_sess_check:sess=%p \n",sess);
#endif    
	
    mgmt = (EdgeMgmt *)sess->mgmt;
    if (!mgmt) return -2;

    if (sess->kcp_timer) {
        iotimer_stop(sess->kcp_timer);
        sess->kcp_timer = NULL;
    }

	curt = time(NULL);
    
	EnterCriticalSection(&sess->kcpCS);
	if(sess->kcp){
		ikcp_update(sess->kcp,iclock());
		now_time = iclock();
		next_time = ikcp_check(sess->kcp,now_time);
		if(next_time >now_time){
			interval = next_time - now_time;
		}else{
			interval = 10;
		}
	}else{
		interval = 10;
	}
	LeaveCriticalSection(&sess->kcpCS);

#ifdef _DEBUG
    //printf("kcp_sess_check:sess=%p interval=%d\n",sess,interval);
#endif   

    
    sess->kcp_timer = iotimer_start(mgmt->pcore,
                                  interval,
                                  t_pull_kcp_sess_check,
                                  (void *)sess->sessid,
                                  edge_body_pump,
                                  mgmt);
    return 0;
}


int   pull_sess_sendkcp(void *vmgmt, uint64 sessid, uint8 *buf, int buflen)
{
    PullSess    *sess = NULL;
    EdgeMgmt    *mgmt = (EdgeMgmt*)vmgmt;

    if(!mgmt) return -1;

    sess = pull_mgmt_sess_get(mgmt,sessid);
    if(sess){
        EnterCriticalSection(&sess->kcpCS);
        //push_sess_printf_data(buf,buflen);
        ikcp_send(sess->kcp, (const char*)buf, buflen);
        LeaveCriticalSection(&sess->kcpCS);
    } 

    return 0;
}

int pull_sess_response(void *vsess, uint8 *buf, int buflen)
{
    EdgeMgmt   *mgmt = NULL;
    PullSess   *sess = (PullSess *)vsess;
	int         ret = 0;
    
	struct sockaddr_in sock;	

	if(!sess) return -1;

    mgmt = sess->mgmt;
    if(!mgmt) return -2;

    memset(&sock,0,sizeof(sock));
 	sock.sin_family = AF_INET;
	sock.sin_addr = sess->peerip;
	sock.sin_port = htons(sess->peerport);
    
	ret = sendto (iodev_fd(mgmt->listendev_udp), buf,buflen, 0,(struct sockaddr *)&sock, sizeof(sock));	
    if(ret<0)printf("error: %s\n", strerror(errno));
    
	//printf("Pull SendTo %u.%u.%u.%u:%d %d(%d) bytes\n",0xFF & sock.sin_addr.s_addr >> 0,0xFF & sock.sin_addr.s_addr >> 8,
    //    0xFF & sock.sin_addr.s_addr >> 16,0xFF & sock.sin_addr.s_addr >> 24, ntohs(sock.sin_port), ret,buflen);
	

	return 0;
}


int pull_body_sendto   (const char *buf, int len, void *kcp, void *user)
{
    pull_sess_response(user,(uint8*)buf,len);
    return 0;
}

int pull_sess_para(uint8 *buf, int buflen, uint64 *sessid, uint64 *runid, uint64 *pushid)
{
    *sessid = 2000;
    *runid = 5000;
    *pushid = 1000;

    return 0;
}

int pull_sess_addr(void *vmgmt, uint64 sessid, struct sockaddr_in * addr)
{
    PullSess    *sess = NULL;
    EdgeMgmt    *mgmt = (EdgeMgmt*)vmgmt;
    uint8        baseaddr[64];

    if(!mgmt) return -1;

    sess = pull_mgmt_sess_get(mgmt,sessid);
    if(sess){
        sess->peerip = addr->sin_addr;
        sess->peerport = ntohs(addr->sin_port);

        memset(baseaddr,0,sizeof(addr));
        sprintf((char*)baseaddr,"%s:%u",inet_ntoa(sess->peerip),sess->peerport);
        
        addr_mgmt_sess_add(mgmt,baseaddr,sessid,FALSE);
    }
    
    return 0;
}

int    pull_sess_input(void *vmgmt, uint64 sessid, uint8 *pbuf, int buflen)
{
    PullSess    *sess = NULL;
    EdgeMgmt    *mgmt = (EdgeMgmt*)vmgmt;
    int          len = 0;
    uint8        buf[16*1024];

    if(!mgmt) return -1;

    sess = pull_mgmt_sess_get(mgmt,sessid);
    if(!sess) return -2;

    
    EnterCriticalSection(&sess->kcpCS);
    ikcp_input(sess->kcp,(const char*)pbuf,buflen);
    LeaveCriticalSection(&sess->kcpCS);

    memset(buf,0,sizeof(buf));
    EnterCriticalSection(&sess->kcpCS);
    len = ikcp_recv(sess->kcp, (char*)buf, sizeof(buf)-1);
    LeaveCriticalSection(&sess->kcpCS);

    if(len != -1) printf("pull_sess_input in %d to %s %d \n",buflen,buf,len);

    return 0;
}

