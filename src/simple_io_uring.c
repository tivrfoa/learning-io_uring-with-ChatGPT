#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <liburing.h>

#define QUEUE_DEPTH 1
#define BLOCK_SIZE  4096

int main(int argc, char *argv[]) {
    struct io_uring ring;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;
    int fd;
    void *buf;
    int ret;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    // Open the file for reading
    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    // Initialize the io_uring instance
    ret = io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
    if (ret < 0) {
        fprintf(stderr, "io_uring_queue_init: %s\n", strerror(-ret));
        close(fd);
        return 1;
    }

    // Allocate a buffer for reading
    buf = malloc(BLOCK_SIZE);
    if (!buf) {
        perror("malloc");
        io_uring_queue_exit(&ring);
        close(fd);
        return 1;
    }

    // Get an SQE (Submission Queue Entry)
    sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        fprintf(stderr, "io_uring_get_sqe: submission queue full\n");
        free(buf);
        io_uring_queue_exit(&ring);
        close(fd);
        return 1;
    }

    // Prepare a read request
    io_uring_prep_read(sqe, fd, buf, BLOCK_SIZE, 0);

    // Submit the request to the ring
    ret = io_uring_submit(&ring);
    if (ret < 0) {
        fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));
        free(buf);
        io_uring_queue_exit(&ring);
        close(fd);
        return 1;
    }

    // Wait for the completion of the request
    ret = io_uring_wait_cqe(&ring, &cqe);
    if (ret < 0) {
        fprintf(stderr, "io_uring_wait_cqe: %s\n", strerror(-ret));
        free(buf);
        io_uring_queue_exit(&ring);
        close(fd);
        return 1;
    }

    // Check if the read was successful
    if (cqe->res < 0) {
        fprintf(stderr, "I/O error: %s\n", strerror(-cqe->res));
    } else {
        printf("Read %d bytes from file: %s\n", cqe->res, argv[1]);
        printf("Data: %.*s\n", cqe->res, (char*)buf);
    }

    // Mark the CQE as seen
    io_uring_cqe_seen(&ring, cqe);

    // Clean up
    free(buf);
    io_uring_queue_exit(&ring);
    close(fd);

    return 0;
}

