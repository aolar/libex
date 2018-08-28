/**
 * @file task.h
 * @brief process/task functions
 */
#ifndef __LIBEX_TASK_H__
#define __LIBEX_TASK_H__

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#ifndef __WIN32__
#include <sys/wait.h>
#include <signal.h>
#else
#include <windows.h>
#include <tchar.h>
#endif
#include "str.h"
#include "thread.h"
#include "list.h"

/** @brief process exist status */
typedef enum {
    RUN_EXITED,         /**< child process terminated normally */
    RUN_STOPPED,        /**< child process was stopped by a signal */
    RUN_SIGNALED        /**< child process was terminated by a signal */
} run_exit_t;

/** wait for state changes */
#define RUN_WAIT 1

/** @brief process structure */
typedef struct {
    #ifndef __WIN32__
    pid_t pid;                  /**< process identifier */
    int exit_code;              /**< exit code */
    #else
    PROCESS_INFORMATION pi;     /**< process information */
    DWORD exit_code;            /**< exit code */
    #endif
    strptr_t cmd;               /**< command string */
    run_exit_t run_exit;        /**< exit status */
} run_t;

/** @brief create command string array
 * @param cmdline command line
 * @param cmdline_len command line length
 * @return array of strings as argument for \b exec* function family
 */
char **mkcmdstr (const char *cmdline, size_t cmdline_len);

/** @brief free command string array
 * @param cmd
 */
void free_cmd (char **cmd);

/** @brief start process
 * @param cmd command line
 * @param cmd_len command line length
 * @param proc process structure
 * @param flags 0 or RUN_WAIT
 */
void run (const char *cmd, size_t cmd_len, run_t *proc, int flags);

/** @brief default slot count. It is equals cpu core count */
#define POOL_DEFAULT_SLOTS -1

/** Opaque structure for slot */
typedef struct slot slot_t;

/** @brief callback function for call message
 * <ul>
 *      <li>1-st parameter user data for slot,
 *      <li>2-nd parameter input data,
 *      <li>3-rd parameter pointer for output data
 * </ul>
 */
typedef void (*pool_msg_h) (void*, void*, void**);

/** @brief callback function for create slot
 * <ul>
 *      <li>1-st parameter current slot
 *      <li>2-nd parameter user data
 * </ul>
 */
typedef void (*pool_create_h) (slot_t*, void*);

/** @brief callback function for destroy slot and destroy message
 * parameter is user data or message data
 */
typedef void (*pool_destroy_h) (void*);

/** @brief task pool parameters */
typedef enum {
    POOL_MSG,                           /**< set \b pool_msg_h callback */
    POOL_FREEMSG,                       /**< set \b pool_destroy_h callback */
    POOL_MAXSLOTS,                      /**< set slot count */
    POOL_TIMEOUT,                       /**< set timneout between task executing */
    POOL_LIVINGTIME,                    /**< set livingtime for slot, if during this time message not received then slot is terminated */
    POOL_CREATESLOT,                    /**< callback for create slot */
    POOL_DESTROYSLOT                    /**< callback for destroy slot */
    //POOL_INITDATA
} pool_opt_t;

/** @brief message structure */
typedef struct {
    void *in_data;                      /**< input data */
    void **out_data;                    /**< output data */
    pool_msg_h on_msg;                  /**< callback handler */
    pthread_mutex_t *mutex;             /**< mutex */
    pthread_cond_t *cond;               /**< condition */
} msg_t;

/** @brief pool structure */
typedef struct {
    int is_alive;                       /**< pool is alive */
    pthread_mutex_t locker;             /**< locker */
    pthread_condattr_t cond_attr;       /**< condition attribute */
    pthread_cond_t cond;                /**< condition */
    list_t *queue;                      /**< queue list */
    long max_slots;                     /**< slot count */
    list_t *slots;                      /**< slots */
    pool_msg_h on_msg;                  /**< pool callback message handler */
    pool_destroy_h on_freemsg;          /**< callback for free message */
    pool_create_h on_create_slot;       /**< callback for create slot */
    pool_destroy_h on_destroy_slot;     /**< callback for destroy slot */
    pthread_t th;                       /**< thread */
    long timeout;                       /**< timeout */
    long livingtime;                    /**< living time */
} pool_t;

/** @brief slot structure */
struct slot {
    int is_alive;                       /**< slot is alove */
    pool_t *pool;                       /**< parent pool */
    pthread_t th;                       /**< thread */
    list_item_t *node;                  /**< item in queue */
    void *data;                         /**< user data */
};

/** @brief set integer option
 * @param pool
 * @param opt
 * @param arg
 * @return 0 if success, -1 if error
 */
int pool_setopt_int (pool_t *pool, pool_opt_t opt, long arg);

/** @brief set message callback option
 * @param pool
 * @param opt
 * @param arg
 * @return 0 if success, -1 if error
 */
int pool_setopt_msg (pool_t *pool, pool_opt_t opt, pool_msg_h arg);

/** @brief set creating option
 * @param pool
 * @param opt
 * @param arg
 * @return 0 if success, -1 if error
 */
int pool_setopt_create (pool_t *pool, pool_opt_t opt, pool_create_h arg);

/** @brief set destroying option
 * @param pool
 * @param opt
 * @param arg
 * @return 0 if success, -1 if error
 */
int pool_setopt_destroy (pool_t *pool, pool_opt_t opt, pool_destroy_h arg);

/** @brief set pool option */
#define pool_setopt(pool,opt,arg) \
    _Generic((arg), \
    pool_msg_h: pool_setopt_msg, \
    pool_create_h: pool_setopt_create, \
    pool_destroy_h: pool_setopt_destroy, \
    default: pool_setopt_int \
)(pool,opt,arg)

/** @brief create pool
 * @return pool structure
 */
pool_t *pool_create ();

/** @brief start pool
 * @param pool
 */
void pool_start (pool_t *pool);

/** @brief send message to pool
 * @param pool
 * @param on_msg message callback
 * @param init_data user data for creating slot if creating slot is needed
 * @param in_data input data
 * @param out_data output data
 * @param mutex mutex for waiting response
 * @param cond condition for waiting response
 * @return 0 if success, -1 if error
 */
int pool_call (pool_t *pool,
                pool_msg_h on_msg,
                void *init_data, void *in_data, void **out_data,
                pthread_mutex_t *mutex, pthread_cond_t *cond);

/** @brief pool destroying
 * @param pool
 */
void pool_destroy (pool_t *pool);

#endif // __LIBEX_TASK_h__
