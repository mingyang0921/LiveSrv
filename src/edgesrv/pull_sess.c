
#include "adifall.ext"
#include "epump.h"

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
	
    return 0;
}

int pull_sess_free(void *vsess)
{
    PullSess * sess = (PullSess *)vsess;

	if(!sess) return -1;

	kfree(sess);

	return 0;
}


