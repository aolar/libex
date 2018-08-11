/**
 * @file html.h
 * @brief libex html functions
 * @author brian_boru
 * @mainpage HTML Functions
 */
#ifndef __LIBEX_HTML_H__
#define __LIBEX_HTML_H__

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "str.h"
#include "list.h"

/** tag not found */
#define HTML_NOT_FOUND -1
/** tag found */
#define HTML_FOUND 0
/** tag found and it's the end */
#define HTML_FOUND_FIN 1

/** callback function for tag finding */
typedef list_item_t* (*on_tag_h) (list_item_t*, void*);

/** @brief html tag */
typedef struct html_tag {
    /** tag start */
    str_t *tag_s;
    /** tag text */
    str_t *tag_t;
    /** end of tag */
    str_t *tag_e;
    /** children tags */
    list_t *tags;
    /** parent tag */
    struct html_tag *parent;
} html_tag_t;

/** html */
typedef struct {
    /* head tag */
    html_tag_t *head;
} html_t;

/** @brief free all tags
 * @param html
 */
void html_clear (html_t *html);

/** @brief free tag
 * @param tag
 */
void html_free_tag (html_tag_t *tag);

/** @brief free tag list
 * @param tags
 */
void html_clear_tags (list_t *tags);

/** @brief parse html content
 * @param content
 * @param content_length
 * @param html result html structure
 */
int html_parse (char *content, size_t content_length, html_t *html);

/** @brief add tag
 * @param parent tag for appending
 * @param tag_s tag start
 * @param tag_s_len \b tag_s length
 * @param tag_t tag text
 * @param tag_t_len \b tag_t length
 * @param tag_e end of tag
 * @param tag_e_len \b tag_e length
 * @param add_to_head
 * @retval int 0 if success, -1 if error
 */
int html_add_tag (html_tag_t *parent,
                  const char *tag_s, size_t tag_s_len,
                  const char *tag_t, size_t tag_t_len,
                  const char *tag_e, size_t tag_e_len,
                  int add_to_head);
/** @brief find tag
 * @param tags tag list
 * @param item from item
 * @param on_tag callback function for each tags
 * @param recurse recursive searching
 * @param found_item result item
 * @param userdata user data for callback function
 * @retval some from that
 * <ul>
 *      <li> #HTML_NOT_FOUND
 *      <li> #HTML_FOUND
 *      <li> #HTML_FOUND_FIN
 * </ul>
 */
int html_find_tag (list_t *tags, list_item_t *item, on_tag_h on_tag, int recurse, list_item_t **found_item, void *userdata);

/** @brief get tag attribute
 * @param tag
 * @param idx
 * @param val result string pointer
 * @retval 0 id success, -1 tag not exists
 */
int html_get_tag_attr (str_t *tag, int idx, strptr_t *val);

/** @brief make content from html structure
 * @param html it's source
 * @param start_len initial string start length
 * @param chunk_size chunk size of string
 * @retval string
 */
str_t *html_mkcontent (html_t *html, size_t start_len, size_t chunk_size);

#endif // __LIBEX_HTML_H__
