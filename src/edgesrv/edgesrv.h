
#ifndef _EDGE_SRV_H_
#define _EDGE_SRV_H_


#ifdef _cplusplus
extern "C" {
#endif


void * edge_mgmt_init (void * hconf,void * http_mgmt);
int    edge_mgmt_clean(void * vmgmt);

#ifdef _cplusplus
}
#endif

#endif

