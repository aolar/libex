/**
 * @file hash.h
 * @brief hash functions
 * @mainpage Hash functions
 */
#ifndef __LIBEX_HASH_H__
#define __LIBEX_HASH_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include "list.h"
#include "str.h"

/**
 * threshold for hash increasing
 */
#define INC_THRESHOLD 0.7

/**
 * threshold for hash decreasing
 */
#define DEC_THRESHOLD 0.3

/**
 * #MIN_HASH_SIZE minimal hash size
 */
#define MIN_HASH_SIZE 1024
/**
 * #MAX_HASH_SIZE maximal hash size
 */
#define MAX_HASH_SIZE INT_MAX

/**
 * @brief hash type
 */
typedef enum {
    /**
     * hash don't change the size
     */
    HASH_CONSTSIZE,
    /**
     * hash can change size
     */
    HASH_VARSIZE
} hash_type_t;

#if __x86_64 || __ppc64__
/**
 * maximum hash value
 */
#define MAX_HASH_KEY UINT_MAX
/**
 * hash value type
 */
typedef uint64_t hash_key_t;
#else
/**
 * maximum hash value
 */
#define MAX_HASH_KEY ULONG_MAX
/**
 * hash value type
 */
typedef uint32_t hash_key_t;
#endif

/**
 * @brief returns string hash value, string must be finished by 0. Uses as callback in \b hash_alloc function
 * @param s source string
 * @param dummy don't used
 * @retval hash value
 */
hash_key_t hash_str (const char *s, size_t dummy);

/**
 * @brief returns string hash value, string. Uses as callback in \b hash_alloc function
 * @param s source string
 * @param len source string length
 * @retval hash value
 */
hash_key_t hash_nstr (const char *s, size_t len);

/**
 * A hash structure
 */
typedef struct hash hash_t;
/**
 * A hash item structure
 */
typedef struct {
    /**
     * key
     */
    void *key;
    /**
     * value
     */
    void *value;
    /**
     * key length
     */
    size_t key_len;
    /**
     * hash index
     */
    hash_key_t idx;
    /**
     * node item in hash bucket
     */
    list_item_t *h_node;
    /**
     * node item in history list
     */
    list_item_t *b_node;
} hash_item_t;

/**
 * @brief callback hash calculation function
 * @param 1-st source data
 * @param 2-nd size of data
 * @retval hash value
 */
typedef hash_key_t (*calc_h) (void*, size_t);

/**
 * @brief callback function for \b hash_enum.
 * @param 1-st current hash item
 * @param 2-nd userdata
 */
typedef int (*hash_item_h) (hash_item_t*, void*);

/**
 * @brief A hash structure
 */
struct hash {
    /**
     * hash size
     */
    hash_key_t size;
    /**
     * maximum hash size
     */
    hash_key_t max_size;
    /**
     * user size
     */
    hash_key_t used_size;
    /**
     * increment size
     */
    int32_t inc_size;
    /**
     * decrement size
     */
    int32_t dec_size;
    /**
     * type of hash
     */
    hash_type_t type;
    /**
     * hash
     */
    list_t **ptr;
    /**
     * history hash
     */
    list_t *hist;
    /**
     * key length
     */
    hash_key_t len;
    /**
     * callback for calculate hash
     */
    calc_h on_hash;
    /**
     * callback for compare keys
     */
    compare_h on_compare;
    /**
     * callback for copy key
     */
    copy_h on_copy;
    /**
     * callback for free element
     */
    free_h on_free;
};

/**
 * @brief create hash
 * @param hash_buf_size starting value of hash size
 * @param type type of hash. It can be constant or variable size.
 * @param on_hash callback for calculating hash value
 * @param on_compare callback for compare hash item keys
 * @param on_copy callback for copying key, can be NULL
 * @param on_free callback for free item
 * @retval hash if success, NULL if error
 */
hash_t *hash_alloc (hash_key_t hash_buf_size, hash_type_t type, calc_h on_hash, compare_h on_compare, copy_h on_copy, free_h on_free);

/**
 * @brief find hash item by key value
 * @param hash
 * @param key
 * @param key_len
 * @retval hash item or NULL if not found
 */
hash_item_t *hash_get (hash_t *hash, void *key, size_t key_len);

/**
 * @brief returns pointer to inserted or existing hash item. If item is already exists then errno == EEXIST
 * @param hash
 * @param key
 * @param key_len
 * @retval hash item
 */
hash_item_t *hash_add (hash_t *hash, void *key, size_t key_len);

/**
 * @brief delete hash item from hash
 * @param hash
 * @param key
 * @param key_len
 */
void hash_del (hash_t *hash, void *key, size_t key_len);

/**
 * @brief enumerate all hash items
 * @param hash
 * @param on_item callback that executed on every item
 * @param userdata it is used as parameter for callback \b on_item
 * @param flags can be assigned the next values
 * <ul>
 *  <li> 0 enumerate all items
    <li> \b ENUM_STOP_IF_BREAK. Enumerate will be stopped if callback returns \b ENUM_BREAK
 * </ul>
 */
void hash_enum (hash_t *hash, hash_item_h on_item, void *userdata, int flags);

/**
 * @brief function is resized a hash size
 * @param hash
 * @param grow hash size will be increased or decreased on \b grow elements
 */
void hash_resize (hash_t *hash, int32_t grow);

/**
 * @brief frees a hash
 * @param hash
 */
void hash_free (hash_t *hash);

/**
 * @brief set new maximal hash size, if current size greater then new maximal size then it don't changes
 * @param hash
 * @param max_size
 */
void hash_set_max_size (hash_t *hash, hash_key_t max_size);

/**
 * @brief returns filling coefficient of hash
 * @param hash
 * @retval filling coefficient
 */
double hash_param_filling (hash_t *hash);

/**
 * @brief returns expected conflict parameter
 * @param hash
 * @retval expected conflict parameter
 */
double hash_param_conflict (hash_t *hash);

#endif // __LIBEX_HASH_H__
