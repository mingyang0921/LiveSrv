
#ifndef _EDGE_SRV_H_
#define _EDGE_SRV_H_


#ifdef _cplusplus
extern "C" {
#endif

#define AVKCP_CONV (('A' << 0) | ('V' << 8) | ('K' << 16) | ('C' << 24))

typedef struct edge_mgmt {

	uint16             nodeid;
	int                udp_port;
	void			 * listendev_udp;

    CRITICAL_SECTION   logfpCS;
    FILE             * logfp;
    ulong              logdate;
    uint8              logfile[256];

	void             * hconf;
	uint8            * logpath;
	uint8              logstatus;

    CRITICAL_SECTION   pduseqidCS;
    uint32             pduseqid;

    CRITICAL_SECTION   addrCS;
    hashtab_t        * addr_table;

    CRITICAL_SECTION   pushCS;
    arr_t            * push_list;
    hashtab_t        * push_table;
    hashtab_t        * pushaddr_table;

    CRITICAL_SECTION   pullCS;
    arr_t            * pull_list;
    hashtab_t        * pull_table;
    
    bpool_t          * push_pool;
    bpool_t          * pull_pool;
	bpool_t          * frame_pool;
    
    void             * pcore;
	void             * nodemgmt;
	void             * http_mgmt;

} EdgeMgmt;


void * edge_mgmt_init ( void * hconf, void *pcore, void * http_mgmt);
int    edge_mgmt_clean(void * vmgmt);

int    edge_udp_socket_start(void * vmgmt);
int    edge_udp_socket_stop (void * vmgmt);


void  *push_mgmt_sess_get(void * vmgmt, ulong sessid);
int    push_mgmt_sess_add(void * vmgmt, void *vsess);
void  *push_mgmt_sess_del(void * vmgmt, ulong sessid);

void  *pull_mgmt_sess_get(void * vmgmt, ulong sessid);
int    pull_mgmt_sess_add(void * vmgmt, void *vsess);
void  *pull_mgmt_sess_del(void * vmgmt, ulong sessid);

int    pull_mgmt_get_list(void *vmgmt, void * arr);


int    addr_sess_cmp_sessid (void * a, void * b);
int    addr_mgmt_sess_add(void *vmgmt, uint8 *addr, ulong sessid);
int    addr_mgmt_sess_get(void *vmgmt, uint8 *addr, ulong *sessid);
int    addr_mgmt_sess_del(void *vmgmt, uint8 *addr);


int    edge_body_pump (void * vbody, void * vobj, int event, int fdtype);

int32  iclock();

#ifdef _cplusplus
}
#endif

#endif

