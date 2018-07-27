#include "../include/libex/qdb.h"

#define QDB_TRUNC 0x00000001
#define QDB_STATIC 0x00000002

static int write_default_header (int fd, size_t default_size) {
    int rc;
    qdb_t hdr = { .head = sizeof(qdb_t), .tail = sizeof(qdb_t), .size = default_size };
    if (0 == (rc = flock(fd, LOCK_EX))) {
        rc = sizeof(qdb_t) == write(fd, &hdr, sizeof(qdb_t)) ? 0 : -1;
        flock(fd, LOCK_UN);
    }
    return rc;
}

int qdb_open (const char *fname) {
    return qdb_nopen(fname, 0);
}

static int load_header (int fd, qdb_t *hdr, int flags) {
    off_t fsize, fcur;
    if (-1 == (fsize = lseek(fd, 0, SEEK_END))) return -1;
    if (-1 == (fcur = lseek(fd, 0, SEEK_SET))) return -1;
    if (sizeof(qdb_t) != read(fd, hdr, sizeof(qdb_t))) return -1;
    if ((flags & QDB_TRUNC) && hdr->head == hdr->tail && (fcur + sizeof(qdb_t) < fsize)) {
        if (-1 == lseek(fd, 0, SEEK_SET)) return -1;
        hdr->head = hdr->tail = sizeof(qdb_t);
        if (sizeof(qdb_t) != write(fd, hdr, sizeof(qdb_t))) return -1;
        if (-1 == ftruncate(fd, sizeof(qdb_t))) return -1;
    }
    return 0;
}

static int update_header (int fd, qdb_t *hdr) {
    if (-1 == lseek(fd, 0, SEEK_SET)) return -1;
    return sizeof(qdb_t) == write(fd, hdr, sizeof(qdb_t)) ? 0 : -1;
}

int qdb_put (int fd, void *data, serialize_h fn) {
    int rc = -1;
    qdb_t hdr;
    ssize_t wrote;
    if (-1 == flock(fd, LOCK_EX)) return -1;
    if (-1 == load_header(fd, &hdr, 1)) goto done;
    if (-1 == lseek(fd, hdr.tail, SEEK_SET)) goto done;
    if (-1 == (wrote = fn(fd, data))) goto done;
    hdr.tail += wrote;
    if (-1 == update_header(fd, &hdr)) goto done;
    rc = 0;
done:
    flock(fd, LOCK_UN);
    return rc;
}

int qdb_get (int fd, void **data, deserialize_h fn) {
    int rc = -1;
    qdb_t hdr;
    ssize_t readed;
    if (-1 == flock(fd, LOCK_EX)) return -1;
    if (-1 == load_header(fd, &hdr, 0)) goto done;
    if (EAGAIN == (errno = hdr.head < hdr.tail ? 0 : EAGAIN)) goto done;
    if (-1 == lseek(fd, hdr.head, SEEK_SET)) goto done;
    if (-1 == (rc = -1 == (readed = fn(fd, data) ? -1 : 0))) goto done;
    hdr.head += readed;
    if (-1 == (rc = update_header(fd, &hdr))) goto done;
done:
    flock(fd, LOCK_UN);
    return rc;
}

int qdb_nopen (const char *fname, size_t default_size) {
    struct stat st;
    if (-1 != stat(fname, &st) && S_IFREG == (st.st_mode & S_IFMT))
        return open(fname, O_RDWR);
    if (ENOENT == errno) {
        int fd = open(fname, O_CREAT | O_TRUNC | O_RDWR, 0644);
        if (-1 == fd) return -1;
        if (-1 == write_default_header(fd, default_size)) {
            close(fd);
            return -1;
        }
        return fd;
    }
    return -1;
}

int qdb_nput (int fd, void *data) {
    int rc = -1;
    qdb_t hdr;
    if (-1 == flock(fd, LOCK_EX)) return -1;
    if (-1 == load_header(fd, &hdr, 1)) goto done;
    if (-1 == lseek(fd, hdr.tail, SEEK_SET)) goto done;
    if (hdr.size != write(fd, data, hdr.size)) goto done;
    hdr.tail += hdr.size;
    if (-1 == update_header(fd, &hdr)) goto done;
    rc = 0;
done:
    flock(fd, LOCK_UN);
    return rc;
}

int qdb_nget (int fd, void *data, int flags) {
    int rc = -1;
    qdb_t hdr;
    if (-1 == flock(fd, LOCK_EX)) return -1;
    if (-1 == load_header(fd, &hdr, 0)) goto done;
    if (EAGAIN == (errno = hdr.head < hdr.tail ? 0 : EAGAIN)) goto done;
    if (-1 == (rc = lseek(fd, hdr.head, SEEK_SET))) goto done;
    if (data) {
        if (-1 == (rc = hdr.size == read(fd, data, hdr.size) ? 0 : -1)) goto done;
    }
    if (QDB_PEEK != flags) {
        hdr.head += hdr.size;
        if (-1 == (rc = update_header(fd, &hdr))) goto done;
    }
done:
    flock(fd, LOCK_UN);
    return rc;
}

int qdb_stat (const char *fname, qdb_t *hdr) {
    int fd, rc;
    if (-1 == (fd = qdb_open(fname))) return -1;
    if (-1 == (rc = flock(fd, LOCK_EX))) goto done;
    rc = load_header(fd, hdr, 0);
    flock(fd, LOCK_UN);
done:
    close(fd);
    return rc;
}
