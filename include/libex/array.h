#ifndef __LIBEX_ARRAY_H__
#define __LIBEX_ARRAY_H__

#include <stdlib.h>
#include <string.h>

#define DEFINE_ARRAY(_array_type_, _data_type_) \
    static inline size_t data_size_##_array_type_ () { return sizeof( _data_type_ ); }; \
    typedef void (*free_item_##_array_type_##_h) (_data_type_); \
    typedef struct _array_##_array_type_##_ _array_type_; \
    struct _array_##_array_type_##_ { \
        size_t len, bufsize, chunk_size, data_size; \
        free_item_##_array_type_##_h on_free; \
        _data_type_ ptr [0]; \
    }

#define INIT_ARRAY(_array_type_, _array_, _start_len_, _chunk_size_, _on_free_) { \
    size_t _##_array_type_##_bufsize = (_start_len_ / _chunk_size_) * _chunk_size_; \
    _array_ = malloc(data_size_##_array_type_() * _##_array_type_##_bufsize + 4 * sizeof(size_t) + sizeof(void*)); \
    if (_array_) { \
        (_array_)->bufsize = _##_array_type_##_bufsize; \
        (_array_)->len = 0; \
        (_array_)->data_size = data_size_##_array_type_(); \
        (_array_)->chunk_size = _chunk_size_; \
        (_array_)->on_free = _on_free_; \
    } \
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
    static inline size_t data_size_##_array_type_ () { return sizeof( _data_type_ ); }; \
    typedef void (*free_item_##_array_type_##_h) (_data_type_); \
    typedef int (*compare_items_##_array_type_##_h) (_data_type_*, _data_type_*); \
    typedef struct _array_##_array_type_##_ _array_type_; \
    struct _array_##_array_type_##_ { \
        size_t len, bufsize, chunk_size, data_size; \
        free_item_##_array_type_##_h on_free; \
        compare_items_##_array_type_##_h on_compare; \
        _data_type_ ptr [0]; \
    }

#define INIT_SORTED_ARRAY(_array_type_, _array_, _start_len_, _chunk_size_, _on_free_, _on_compare_) { \
    size_t _##_array_type_##_bufsize = (_start_len_ / _chunk_size_) * _chunk_size_; \
    _array_ = malloc( data_size_##_array_type_() * _##_array_type_##_bufsize + 4 * sizeof(size_t) + sizeof(void*) * 2); \
    if (_array_) { \
        (_array_)->bufsize = _##_array_type_##_bufsize; \
        (_array_)->len = 0; \
        (_array_)->data_size = data_size_##_array_type_(); \
        (_array_)->chunk_size = _chunk_size_; \
        (_array_)->on_free = _on_free_; \
        (_array_)->on_compare = _on_compare_; \
    } \
}

#define SORTED_ARRAY_FIND(_array_, _data_, _found_idx_, _is_found_) \
    _is_found_ = 0; _found_idx_ = 0; \
    { \
        int __is_found__ = -1; \
        signed long long int _l_ = 0, _r_ = _array_->len - 1; \
        while (_l_ <= _r_) { \
            _found_idx_ = (_l_ + _r_) / 2LL; \
            __is_found__ = _array_->on_compare(&(_data_), &_array_->ptr[_found_idx_]); \
            if (__is_found__ > 0) _l_ = _found_idx_ + 1; \
            else if (__is_found__ < 0) _r_ = _found_idx_ - 1; \
            else break; \
        } \
        if (__is_found__ == 0) _is_found_ = 1; \
    }

#define SORTED_ARRAY_ADD(_array_, _data_, _idx_) { \
    signed long long _found_idx_ = 0; \
    { \
        int _is_found_ = 0; \
        signed long long int _l_ = 0, _r_ = _array_->len - 1; \
        while (_l_ <= _r_) { \
            _found_idx_ = (_l_ + _r_) / 2LL; \
            _is_found_ = _array_->on_compare(&_data_, &_array_->ptr[_found_idx_]); \
            if (_is_found_ > 0) _l_ = _found_idx_ + 1; \
            else if (_is_found_ < 0) _r_ = _found_idx_ - 1; \
            else break; \
        } \
        if (_array_->len > 0 && _is_found_ > 0) ++_found_idx_; \
        if (_found_idx_ < 0) _found_idx_ = 0; \
    } \
    if (_array_->bufsize == _array_->len) { \
        size_t nbufsize = _array_->bufsize + (_array_)->chunk_size; \
        if ((_array_ = realloc(_array_, _array_->data_size * nbufsize + 4 * sizeof(size_t) + sizeof(void*) * 2))) { \
            (_array_)->bufsize = nbufsize; \
        } \
    } \
    if (_found_idx_ < (_array_)->len) { \
        memmove(&_array_->ptr[_found_idx_ + 1], &_array_->ptr[_found_idx_], (_array_->len - _found_idx_) * _array_->data_size); \
        _array_->ptr[_found_idx_] = _data_; \
        ++_array_->len; \
    } else { \
        (_array_)->ptr[(_array_)->len] = _data_; \
        ++(_array_)->len; \
    }\
    _idx_ = _found_idx_; \
}

#endif // __LIBEX_ARRAY_H__
