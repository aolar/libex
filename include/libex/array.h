#ifndef __LIBEX_ARRAY_H__
#define __LIBEX_ARRAY_H__

#include <stdlib.h>
#include <string.h>

#define DEFINE_ARRAY(_array_type_, _data_type_) \
    typedef void (*free_item_##_array_type_##_h) (_data_type_); \
    typedef struct _array_##_array_type_##_ _array_type_; \
    struct _array_##_array_type_##_ { \
        size_t len, bufsize, chunk_size, data_size; \
        free_item_##_array_type_##_h on_free; \
        _data_type_ ptr [0]; \
    }

#define INIT_ARRAY(_data_type_, _array_, _start_len_, _chunk_size_, _on_free_) \
    size_t _##_array_##_bufsize = (_start_len_ / _chunk_size_) * _chunk_size_; \
    _array_ = malloc(sizeof(_data_type_) * _##_array_##_bufsize + 4 * sizeof(size_t) + sizeof(void*)); \
    if (_array_) { \
        (_array_)->bufsize = _##_array_##_bufsize; \
        (_array_)->len = 0; \
        (_array_)->data_size = sizeof(_data_type_); \
        (_array_)->chunk_size = _chunk_size_; \
        (_array_)->on_free = _on_free_; \
    }

#define ARRAY_ADD(_array_, _data_) \
    if (_array_->bufsize == _array_->len) { \
        size_t nbufsize = _array_->bufsize + (_array_)->chunk_size; \
        if ((_array_ = realloc(_array_, _array_->data_size * nbufsize + 4 * sizeof(size_t) + sizeof(void*)))) { \
            (_array_)->bufsize = nbufsize; \
        } \
    } \
    (_array_)->ptr[(_array_)->len] = _data_; \
    ++(_array_)->len;

#define ARRAY_INS(_array_, _data_, _idx_) \
    if (_array_->bufsize == _array_->len) { \
        size_t nbufsize = _array_->bufsize + (_array_)->chunk_size; \
        if ((_array_ = realloc(_array_, _array_->data_size * nbufsize + 4 * sizeof(size_t) + sizeof(void*)))) { \
            (_array_)->bufsize = nbufsize; \
        } \
    } \
    if (_idx_ < (_array_)->len) { \
        memmove(&_array_->ptr[_idx_ + 1], &_array_->ptr[_idx_], (_array_->len - _idx_) * _array_->data_size); \
        _array_->ptr[_idx_] = _data_; \
        ++_array_->len; \
    } else { \
        (_array_)->ptr[(_array_)->len] = _data_; \
        ++(_array_)->len; \
    }

#define ARRAY_DEL(_array_, _idx_, _count_) \
    if (_idx_ >= 0 && _idx_ < _array_->len && _count_ > 0) { \
        if (_idx_ == _array_->len - 1) \
            --_array_->len; \
        else { \
            size_t count; \
            if (_idx_ + _count_ < _array_->len) \
                count = _count_; \
            else \
                count = _array_->len - _idx_; \
            memmove(&_array_->ptr[_idx_], &_array_->ptr[_idx_ + count], (count + 1) * _array_->data_size); \
            _array_->len -= count; \
        } \
    }

#define ARRAY_FREE(_array_) \
    if (_array_->on_free) \
        for (size_t i = 0; i < _array_->len; ++i) \
            _array_->on_free(_array_->ptr[i]); \
    free(_array_)

#define DEFINE_SORTED_ARRAY(_array_type_, _data_type_) \
    typedef void (*free_item_##_array_type_##_h) (_data_type_); \
    typedef int (*compare_items_##_array_type_##_h) (_data_type_, _data_type_); \
    typedef struct _array_##_array_type_##_ _array_type_; \
    struct _array_##_array_type_##_ { \
        size_t len, bufsize, chunk_size, data_size; \
        free_item_##_array_type_##_h on_free; \
        compare_items_##_array_type_##_h on_compare; \
        _data_type_ ptr [0]; \
    }

#define INIT_SORTED_ARRAY(_data_type_, _array_, _start_len_, _chunk_size_, _on_compare, _on_free_) \
    size_t _##_array_##_bufsize = (_start_len_ / _chunk_size_) * _chunk_size_; \
    _array_ = malloc(sizeof(_data_type_) * _##_array_##_bufsize + 4 * sizeof(size_t) + sizeof(void*)); \
    if (_array_) { \
        (_array_)->bufsize = _##_array_##_bufsize; \
        (_array_)->len = 0; \
        (_array_)->data_size = sizeof(_data_type_); \
        (_array_)->chunk_size = _chunk_size_; \
        (_array_)->on_free = _on_free_; \
        (_array_)->on_compare = _on_compare_; \
    }

#endif // __LIBEX_ARRAY_H__
