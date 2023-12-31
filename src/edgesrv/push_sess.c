
#include "adifall.ext"
#include "epump.h"

#include "ikcp.h"
#include "edgesrv.h"
#include "edge_io.h"
#include "pull_sess.h"
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
    ikcp_setoutput(sess->kcp,(int (*)(const char*, int, ikcpcb *, void* ))push_body_sendto);
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

int push_sess_para(uint8 *pbuf, int
    buflen, uint64 *sessid, uint64 *runid)
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
    //printf("kcp_sess_check:sess=%p \n",sess);
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


int push_sess_addr(void *vmgmt, uint64 sessid, uint8 *baseaddr)
{
    PushSess    *sess = NULL;
    EdgeMgmt    *mgmt = (EdgeMgmt*)vmgmt;

    if(!mgmt) return -1;

    sess = push_mgmt_sess_get(mgmt,sessid);
    if(sess){
        addr_mgmt_sess_add(mgmt,baseaddr,sessid,TRUE);
    }
    
    return 0;
}

int   push_sess_peer(void *vmgmt, uint64 sessid, struct sockaddr_in * addr)
{
    PushSess    *sess = NULL;
    EdgeMgmt    *mgmt = (EdgeMgmt*)vmgmt;

    if(!mgmt) return -1;

    sess = push_mgmt_sess_get(mgmt,sessid);
    if(sess){
        sess->peerip = addr->sin_addr;
        sess->peerport = ntohs(addr->sin_port);
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
        ikcp_send(sess->kcp, (const char*)buf, buflen);
    } 

    return 0;
}


int push_sess_response(void *vsess, uint8 *buf, int buflen)
{
    EdgeMgmt   *mgmt = NULL;
    PushSess   *sess = (PushSess *)vsess;
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
    if(ret<0) printf("error: %s\n", strerror(errno));
    
	return 0;
}

int   push_sess_savemd(void *vsess, uint8 *data, int datalen)
{
    PushSess   *sess = (PushSess *)vsess;

    uint8       head=0;
    uint32      ts=0;
    uint8       type=0;
    uint32      size=0;

    if(!sess) return 0;

    head_info_get(data,datalen,&head,&ts,&type,&size);

    if(head == 'T' && type == 'I' && (size == (datalen - 8))){
        if(datalen > 256) datalen = 256;
        
        memcpy(sess->mdinfo,data,datalen);
        sess->mdinfo_len = datalen;
        return 1;
    }

    return 0;
}


static int datacount = 0;

int push_sess_printf_data(uint8 *data, int datalen)
{
    uint32  first=0;
    uint32  second=0;

    uint8   H_TYPE=0;
    uint32  H_PTS=0;
    uint8   T_TYPE=0;
    uint32  T_LEN = 0;

    memcpy(&first,data,4);
    memcpy(&second,data+4,4);
    
    H_TYPE = first & 0xff;
    H_PTS  = first >> 8;
    
    T_TYPE = second & 0xff;
    T_LEN  = second >> 8;

    if(H_TYPE == 'T' && (T_TYPE == 'V' || T_TYPE == 'A' || T_TYPE == 'I'))
    {
        if(T_TYPE == 'I'){
            printf("[%s][%d]\n",data+8,datalen-8);
        }
        datacount = datalen-8;
        printf("[HEAD] %c %u %c %u %d %d\n",H_TYPE,H_PTS,T_TYPE,T_LEN,datalen,datacount);   
    }else{
        datacount += datalen;
        printf("[BODY] %d-%d\n",datalen,datacount);
    }
    
    return 0;
}


static int datacount1 = 0;

int push_sess_printf_data1(uint8 *data, int datalen)
{
    uint32  first=0;
    uint32  second=0;

    uint8   H_TYPE=0;
    uint32  H_PTS=0;
    uint8   T_TYPE=0;
    uint32  T_LEN = 0;

    memcpy(&first,data,4);
    memcpy(&second,data+4,4);
    
    H_TYPE = first & 0xff;
    H_PTS  = first >> 8;
    
    T_TYPE = second & 0xff;
    T_LEN  = second >> 8;

    if(H_TYPE == 'T' && (T_TYPE == 'V' || T_TYPE == 'A' || T_TYPE == 'I'))
    {
        if(T_TYPE == 'I'){
            printf("[%s][%d]\n",data+8,datalen-8);
        }
        datacount1 = datalen-8;
        printf("[HEAD0] %c %u %c %u %d %d\n",H_TYPE,H_PTS,T_TYPE,T_LEN,datalen,datacount1);   
    }else{
        datacount1 += datalen;
        printf("[BODY0] %d-%d\n",datalen,datacount1);
    }
    
    return 0;
}


int push_sess_input(void *vmgmt, uint64 sessid, uint8 *pbuf, int buflen)
{
    EdgeMgmt      *mgmt = (EdgeMgmt*)vmgmt;
    PushSess      *sess = NULL;    
    arr_t         *explist = NULL;
    int            len = 0;
    uint8          buf[16*1024];
    int            num = 0;
    void          *tmp = NULL;
    int            i = 0;
    uint8          ifinfo=0;

    sess = push_mgmt_sess_get(mgmt,sessid);
    if(sess){

        if (!explist) explist = arr_new(16);
        push_sess_get_list(sess,explist);
        
        EnterCriticalSection(&sess->kcpCS);
        ikcp_input(sess->kcp,(const char *)pbuf,(long)buflen);
        LeaveCriticalSection(&sess->kcpCS);

        //EnterCriticalSection(&mgmt->sendCS);
        
        while(1)
        {
            EnterCriticalSection(&sess->kcpCS);
            len = ikcp_recv(sess->kcp, (char*)buf, (int)(sizeof(buf)-1));
            if(len > 0 ){
                //push_sess_printf_data1(buf,len);
            }            
            LeaveCriticalSection(&sess->kcpCS);

            if(len > 0 ){
                //push_sess_printf_data(buf,len);
            }
            
            //printf("push_sess_input %c %c \n",buf[0],buf[4]);
            //printOctet(stdout, buf, 0,30, 2);

            ifinfo = FALSE;
            if(push_sess_savemd(sess,buf,len)){
                ifinfo = TRUE; 
            }

            //ifinfo = FALSE;
            //if(len > 0 && buf[0] == 'T' && buf[4] == 'I' ){
            //    memcpy(sess->mdinfo,buf,buflen);
            //    sess->mdinfo_len = buflen;
            //    ifinfo = TRUE;
            //}

            if(len > 0){
			    num = arr_num(explist);
			    for (i=0; i<1; i++) {
				    tmp = arr_value(explist,i);
				    if(tmp){
                        //pull_mgmt_sess_get(mgmt,(ulong)tmp);

                        if(ifinfo){
                            pull_mgmt_onlyinfo(mgmt,(ulong)tmp,buf,len);
                        }else{
                            pull_mgmt_stream(mgmt,(ulong)tmp,buf,len,sess->mdinfo,sess->mdinfo_len);
                            //send to pull
                        }
				    }
		    	}
		    }else{
                break;
		    }
        }
        //LeaveCriticalSection(&mgmt->sendCS);
    }

    return 0;
}

int   push_sess_add_list(void *vmgmt, ulong sessid, ulong pullid)
{
    EdgeMgmt    *mgmt = (EdgeMgmt*)vmgmt;
    PushSess    *sess = NULL;

    if(!mgmt) return -1;

    sess = push_mgmt_sess_get(mgmt,sessid);
    if(!sess) return -2;

	EnterCriticalSection(&sess->pullCS);
    arr_delete_ptr(sess->pull_list,(void*)pullid);
    arr_push(sess->pull_list, (void*)pullid);
	LeaveCriticalSection(&sess->pullCS);	

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

int   push_sess_del_list(void *vsess, uint64 pullid)
{
    
    return 0;
}

int push_body_sendto   (const char *buf, int len, void *kcp, void *user)
{
    push_sess_response(user,(uint8*)buf,len);
    return 0;
}
