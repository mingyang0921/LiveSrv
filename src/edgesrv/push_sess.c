
#include "adifall.ext"
#include "epump.h"

#include "ikcp.h"
#include "edgesrv.h"
#include "push_sess.h"

int push_sess_cmp_sessid (void *a, void *b)
{
    PushSess    *sess = (PushSess *)a;
    ulong        sessid = *(ulong *)b;	

    if (!sess ) return -1;

    if (sess->sessid > sessid) return 1;
    if (sess->sessid < sessid) return -1;

    return 0;
}
ulong push_sess_id_hash_func (void *key)
{
    ulong sessid = *(ulong *)key;

	return sessid;
}

int push_sess_sort_by_time (void *a, void *b)
{
    PushSess *sessa = *(PushSess **)a;
    PushSess *sessb = *(PushSess **)b;
 
    if (!sessa || !sessb) return -1;
 
    if (sessa->stamp > sessb->stamp) return -1;
    if (sessa->stamp < sessb->stamp) return 1;

    return 0;
}

int push_sess_init (void * vsess)
{
    PushSess  *sess = (PushSess *)vsess;
    
    if (!sess) return -1;

    sess->runid = 0;
    sess->sessid = 0;

	sess->kcp_timer = NULL;

    memset(sess->mdinfo,0,sizeof(sess->mdinfo));
    sess->mdinfo_len = 0;

    sess->pull_list = arr_new(16);
    InitializeCriticalSection(&sess->pullCS);

    InitializeCriticalSection(&sess->kcpCS);

    sess->kcp = NULL;
    sess->mgmt = NULL;
	
    return 0;
}

int push_sess_free(void *vsess)
{
    PushSess *sess = (PushSess*)vsess;

	if(!sess) return -1;

 	DeleteCriticalSection(&sess->kcpCS);
 	DeleteCriticalSection(&sess->pullCS);
    if (sess->pull_list) {
        while (arr_num(sess->pull_list) > 0) {
            arr_pop(sess->pull_list);
        }
    }
    sess->pull_list = NULL;

	kfree(sess);

	return 0;
}

void *push_sess_fetch (void * vmgmt)
{
    EdgeMgmt  *mgmt  = (EdgeMgmt *)vmgmt;
    PushSess  *sess = NULL;
 
    if (!mgmt) return NULL;
 
    sess = bpool_fetch(mgmt->push_pool);
    if (!sess) {
        sess = kzalloc(sizeof(*sess));
        push_sess_init(sess);
    }
    if (!sess) return NULL;

	sess->kcp = ikcp_create(AVKCP_CONV,sess);
	if(!sess->kcp){
        bpool_recycle(mgmt->push_pool, sess);
        return NULL;
    };

    ikcp_setoutput(sess->kcp, push_body_sendto);
    ikcp_nodelay(sess->kcp, 1, 10, 2, 1);
    ikcp_wndsize(sess->kcp, 256, 1024);
    
    //sess->ikcp->interval = 10;
    //sess->ikcp->rx_minrto = 50;
    //sess->ikcp->fastresend = 1;
    //sess->ikcp->stream = 1;
 
	sess->mgmt = mgmt;
	sess->ctime = time(&sess->stamp);

	return sess;
}

int  push_sess_recycle (void *vsess)
{
    EdgeMgmt  *mgmt  = NULL;
    PushSess  *sess = (PushSess*)vsess;
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
		return push_sess_free(sess);
	
	while (arr_num(sess->pull_list) > 0) {
		arr_pop(sess->pull_list);
	}
	arr_zero(sess->pull_list);
	 
	bpool_recycle(mgmt->push_pool, sess);

	return 0;
}

void *push_sess_open (void *vmgmt, ulong sessid, uint64 runid)
{
    EdgeMgmt  *mgmt  = (EdgeMgmt*)vmgmt;
    PushSess  *sess = NULL;	

	if (!mgmt) return NULL;	

	EnterCriticalSection(&mgmt->pushCS);	
	sess = ht_get(mgmt->push_table,&sessid);
	if(sess){
		time(&sess->stamp);
		LeaveCriticalSection(&mgmt->pushCS);	
		return sess;
	}

	sess = push_sess_fetch (mgmt);
	if(!sess){
		LeaveCriticalSection(&mgmt->pushCS);
		return NULL;
	}
	sess->sessid = sessid;
    sess->runid  = runid;

	ht_set(mgmt->push_table,&sess->sessid,sess);
	arr_push(mgmt->push_list, sess);	

	LeaveCriticalSection(&mgmt->pushCS);

    push_sess_check(sess);

	return sess;
}

int  push_sess_close(void *vsess)
{
	PushSess    *sess = (PushSess *)vsess;
    EdgeMgmt    *mgmt = NULL;
	PushSess	*tmp  = NULL;

	if(!sess) return -1;

	mgmt = sess->mgmt;
	if(!mgmt) return -1;

	tmp = push_mgmt_sess_del(mgmt,sess->sessid);
	if(tmp != sess) return -1;
	
	return push_sess_recycle(sess);
}

int   push_sess_restart (void *vmgmt, uint64 sessid, uint64 runid)
{
	PushSess    *sess = NULL;
    EdgeMgmt    *mgmt = (EdgeMgmt*)vmgmt;
	PushSess	*tmp  = NULL;

    if(!mgmt) return -1;

    sess = push_mgmt_sess_get(mgmt,sessid);
    if(!sess){
        sess = push_sess_open (mgmt,sessid,runid);
        return 0;
    }

    if(sess->runid != runid){
        push_sess_close(sess);
        push_sess_open (mgmt,sessid,runid);
    }
    
    return 0;
}

int push_sess_para(uint8 *pbuf, int buflen, ulong *sessid, ulong *runid)
{
    *sessid = 1000;
    *runid = 5000;
    
    return 0;
}


int push_sess_check (void * vsess)
{
    PushSess *sess = (PushSess*)vsess;
    EdgeMgmt *mgmt = NULL;

	uint32     interval = 0;
	uint32     next_time = 0;
	uint32     now_time = 0;
    time_t     curt = 0;

    if (!sess) return -1;

#ifdef _DEBUG
    printf("kcp_sess_check:sess=%p \n",sess);
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
                                  t_push_kcp_sess_check,
                                  (void *)sess->sessid,
                                  edge_body_pump,
                                  mgmt);
    return 0;
}


int push_sess_addr(void *vmgmt, uint64 sessid, struct sockaddr_in * addr)
{
    PushSess    *sess = NULL;
    EdgeMgmt    *mgmt = (EdgeMgmt*)vmgmt;
    uint8        baseaddr[64];

    if(!mgmt) return -1;

    sess = push_mgmt_sess_get(mgmt,sessid);
    if(sess){
        sess->peerip = addr->sin_addr;
        sess->peerport = ntohs(addr->sin_port);

        memset(baseaddr,0,sizeof(addr));
        sprintf(baseaddr,"%s:%u",inet_ntoa(sess->peerip),sess->peerport);
        
        addr_mgmt_sess_add(mgmt,baseaddr,sessid);
    }
    
    return 0;
}

int   push_sess_sendkcp(void *vmgmt, uint64 sessid, uint8 *buf, int buflen)
{
    PushSess    *sess = NULL;
    EdgeMgmt    *mgmt = (EdgeMgmt*)vmgmt;

    if(!mgmt) return -1;

    sess = push_mgmt_sess_get(mgmt,sessid);
    if(sess){
        ikcp_send(sess->kcp, buf, buflen);
    } 

    return 0;
}


int push_sess_response(void *vsess, uint8 *buf, int buflen)
{
    EdgeMgmt   *mgmt = NULL;
    PushSess   *sess = (EdgeMgmt *)vsess;
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
    
	printf("SendTo %s:%d %d bytes\n", inet_ntoa(sock.sin_addr), ntohs(sock.sin_port), ret);
	

	return 0;
}


int push_sess_input(void *vmgmt, uint8 *pbuf, int buflen, struct sockaddr_in * addr)
{
    EdgeMgmt   *mgmt = (EdgeMgmt*)vmgmt;
    PushSess   *sess = NULL;    
	struct in_addr peerip;
	uint16         peerport;
    uint8          baseaddr[64];
    uint64         sessid = 0;

    arr_t         *explist = NULL;
    int            len = 0;
    uint8          buf[16*1024];
    int            num = 0;
    void          *tmp = NULL;
    int            i = 0;

    peerip = addr->sin_addr;
    peerport = ntohs(addr->sin_port);
    
    memset(baseaddr,0,sizeof(addr));
    sprintf(baseaddr,"%s:%d",inet_ntoa(peerip),peerport);

    addr_mgmt_sess_get(mgmt,baseaddr,&sessid);

    sess = push_mgmt_sess_get(mgmt,sessid);
    if(sess){

        if (!explist) explist = arr_new(16);
        push_sess_get_list(sess,explist);
        
        EnterCriticalSection(&sess->kcpCS);
        ikcp_input(sess->kcp,pbuf,buflen);
        LeaveCriticalSection(&sess->kcpCS);

        while(1)
        {
            EnterCriticalSection(&sess->kcpCS);
            len = ikcp_recv(sess->kcp, buf, sizeof(buf)-1);
            LeaveCriticalSection(&sess->kcpCS);

            //printf("push_sess_input %c %c \n",buf[0],buf[4]);
            printOctet(stdout, buf, 0,16, 2);

            if(len > 0 && buf[0] == 'T' && buf[4] == 'I' ){
                memcpy(sess->mdinfo,buf,buflen);
                sess->mdinfo_len = buflen;
            }

            if(len > 0){
			    num = arr_num(explist);
			    for (i=0; i<num; i++) {
				    tmp = arr_value(explist,i);
				    if(tmp){
                        pull_mgmt_sess_get(mgmt,(ulong)tmp);

                        //send to pull
				    }
		    	}
		    }else{
                break;
		    }
        }
        
    }

    return 0;
}

int  push_sess_get_list(void *vsess,void * arr)
{
    PushSess   *sess = (PushSess *)vsess;
    arr_t      *explist = (arr_t *)arr;
	void       *tmp = NULL;
	int         num = 0;
	int         i  = 0;

	if(!sess) return -1;
	
	EnterCriticalSection(&sess->pullCS);
	num =arr_num(sess->pull_list);
	for(i=0;i<num;i++){
		tmp = arr_value(sess->pull_list,i);
		if(tmp && arr){
			arr_push(explist, tmp);
		}
	}
	LeaveCriticalSection(&sess->pullCS);	

	return 0;
}

int   push_sess_build_list(void *vsess)
{
    
    return 0;
}

int push_body_sendto   (const char *buf, int len, void *kcp, void *user)
{
    push_sess_response(user,buf,len);
    return 0;
}

