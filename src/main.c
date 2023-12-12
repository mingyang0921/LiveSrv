
#include "adifall.ext"
#include <signal.h>
#include "epump.h"
#include "ejet.h"
#include "edgesrv.h"

#ifdef UNIX
#define INSTALL_DIR "."
#define LOCK_FILE "./edgesrv.lck"
#endif


void      * g_quitsys_event = NULL;
void      * g_pcore = NULL;
void      * g_httpmgmt = NULL;

#ifdef _WIN32
CHAR      * g_ServiceName = "edgesrv";
#endif


int EntryPoint (int argc, char ** argv);


int system_shutdown ()
{
    epcore_stop_epump(g_pcore);
    epcore_stop_worker(g_pcore);
 
    SLEEP(1000);
 
    event_set(g_quitsys_event, -10);
 
    return 0;
}

static void signal_handler(int sig)
{
    switch(sig) {
    case SIGHUP:
        tolog(1, "hangup signal catched\n");
        break;
    case SIGTERM:
    case SIGKILL:
    case SIGINT:
        tolog(1, "terminate signal catched, now exiting...\n");
        system_shutdown();
        break;
    }
}

int main (int argc, char ** argv)
{
    return EntryPoint(argc, argv);
}

int EntryPoint (int argc, char ** argv)
{
    void     * pcore = NULL;
    char       opt;
    int        cpunum = 0;
    int        epumpnum = 0;
    int        workernum = 0;

    char     * confile = "edgesrv.conf";
    //char     * arglist[4];

    void     * httpmgmt = NULL;
    char     * jsonconf = "ejet.conf";
    void     * hlog = NULL;

    void     * body = NULL;

    uint8      daemon = 0;
    char     * plockfile   = "edgesrv.lck",
             * pinstalldir = ".";
    int        lock_fd = -1;
 
#ifdef UNIX
    struct sigaction sa;
    char     * glocksrv = "/var/lock/subsys/edgesrv";
    void     * glock = NULL;
#endif

    /*if (argc < 2) {
        fprintf(stderr, "Usage: %s [-j json conf] [-L exclusive file] [-d] "
                        "[-l lockfile.lck] [-i install-dir]\n", argv[0]);
        return 0;
    }*/
 
    while (argc > 1 && (opt = getopt(argc, argv, "l:i:j:d")) != -1) {
        switch (opt) {
        case 'j':
            jsonconf = optarg;
            break;
        case 'L':
            glocksrv = optarg;
            break;
        case 'd':
            daemon = 1;
            break;
        case 'i':
            pinstalldir = optarg;
            break;
        case 'l':
            glocksrv = optarg;
            break;
        default:
            fprintf(stderr, "Unknown option -%c\n", opt);
            break;
        }
    }

    if (g_quitsys_event != NULL) return 0;
    g_quitsys_event = event_create();

#ifdef _WIN32
    const char * strMutexName = "eJet-gatesrv-Demo-1122";
    HANDLE hMutex = NULL;
 
    //main_entry(argc, argv);
 
    hMutex = CreateMutex(NULL, TRUE, strMutexName);
    if (ERROR_ALREADY_EXISTS == GetLastError() || NULL == hMutex)
        return 0;
#endif

#ifdef UNIX
    signal(SIGPIPE, SIG_IGN);
 
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigemptyset(&sa.sa_mask) == -1 || sigaction(SIGPIPE, &sa, 0) == -1) {
        perror("failed to ignore SIGPIPE in sigaction");
        exit(0);
    }
 
    if (!file_exist(glocksrv)) {
        file_dir_create(glocksrv, 1);
    }
    if (file_mutex_locked(glocksrv) != 0) {
        exit(0);
    }
 
    if (daemon) {
        lock_fd = daemonize (plockfile, pinstalldir);
		if(lock_fd){}
    }
    
    signal(SIGCHLD, SIG_IGN); /* ignore child */
    signal(SIGTSTP, SIG_IGN); /* ignore tty signals */
    signal(SIGTTOU, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGHUP,  signal_handler); /* catch hangup signal */
    signal(SIGTERM, signal_handler); /* catch kill signal */
    signal(SIGINT, signal_handler); /* catch SIGINT signal */
 
    glock = file_mutex_init(glocksrv);
    file_mutex_lock(glock);
#endif

    hlog = trlog_init("webapp.log", 0);
 
    pcore = epcore_new(65535, 1);
    g_pcore = pcore;
 
    httpmgmt = http_mgmt_alloc(pcore, jsonconf, 0, 0);
    http_mgmt_init(httpmgmt);
 
    //arglist[0] = argv[0];
    argv[1] = confile;

    body = edge_mgmt_init("edgesrv.conf", httpmgmt);

   /* now startup the system, epump as the engine will be erected */
 
    cpunum = get_cpu_num();
    epumpnum = cpunum * 0.2;
    if (epumpnum < 3) epumpnum = 3;
    workernum = cpunum - epumpnum;
    if (workernum < 3) workernum = 3;
 
    /* start worker threads */
    epcore_start_worker(pcore, workernum);
 
    /* start epump threads */
    epcore_start_epump(pcore, epumpnum - 1);
 
 
#ifdef _DEBUG
    /* create new epump thread executing the epump_main_proc */
    epump_main_start(pcore, 1);
 
    while(event_wait(g_quitsys_event, 1000) != -10) {
        continue;
    }
    event_destroy(g_quitsys_event);
#else
    /* main thread executing the epump_main_proc as an epump thread */
    epump_main_start(pcore, 0);
#endif
 
    edge_mgmt_clean(body);

    http_mgmt_cleanup(httpmgmt);
    epcore_clean(pcore);

#ifdef UNIX
    file_mutex_unlock(glock);
    file_mutex_destroy(glock);

#ifndef _DEBUG 
    if (lock_fd >= 0) close(lock_fd);
#endif
#endif

#ifdef WINDOWS
    CloseHandle(hMutex);
#endif

    trlog_clean(hlog);
	
#ifdef _DEBUG
printf("\nMAIN Thread exited successfully...\n");
#endif
    return 0;
}

