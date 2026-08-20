#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "memoryAllocator.h"

typedef char ALIGN[16];

union header {
    struct {
        size_t size;
        unsigned is_free;
        union header *next;
    }s;

    ALIGN stub;
};

typedef union header header_t;

header_t *head, *tail;

pthread_mutex_t lock;

void *malloc(size_t size) {
    size_t total_size;
    void *block;
    header_t *header;

    if (!size) {
        return NULL;
    }
    pthread_mutex_lock(&lock);

    header = get_free_block(size);
    if (header) {
        header->s.is_free = 0;
        pthread_mutex_unlock(&lock);
        return (void *)header + 1;
    }

    total_size = sizeof(header_t) + size;
    block = sbrk(total_size);
    if (block == (void *)-1) {
        pthread_mutex_unlock(&lock);
        return NULL;
    }

    header = block;
    header->s.size = size;
    header->s.next = NULL;
    header->s.is_free = 0;

    if (!head) {
        head = header;
    }

    if (tail) {
        tail->s.next = header; 
    }

    tail = header;
    return (void *)header + 1;
}

void *get_free_block(size_t size) {
    header_t *curr = head;

    while (curr) {
        if (curr->s.is_free == 0 && curr->s.size >= size) {
            return curr;

        }
        curr = curr->s.next;
    }
    return NULL;
}

void free(void *block) {
    header_t *header, *tmp;
    void *programbreak;

    if (!block) {
        return;
    }
    pthread_mutex_lock(&lock);
    header = (header_t *)header - 1;

    programbreak = sbrk(0);
    if ((char *)block + header->s.size == programbreak) { // checks if we are at the end of the heap
        if (head == tail) {
            head = tail = NULL;
        }
        else {
            tmp = head;
            while (tmp) {
                if (tmp->s.next == tail) {
                    tmp->s.next = NULL;
                    tail = tmp;                 
                }
                tmp = tmp->s.next;
            }
        }
        sbrk(0 - sizeof(header_t) - header->s.size);
        pthread_mutex_unlock(&lock);
    }
    header->s.is_free = 1;
    pthread_mutex_unlock(&lock);
}

void *calloc(size_t num, size_t nsize) {
    void *block;
    size_t size;
    if (!num || !nsize) {
        return NULL;
    }

    size = num * nsize;

    if (nsize != size / num) {
        return NULL;
    }
    block = malloc(size);

    if (!block) {
        return NULL;
    }

    memset(block, 0, size);
    return block;
    
}

void *realloc(void *block, size_t size) {
    header_t *header;
    void *ret;
    
    if (!block || !size) {
        return malloc(size);
    }

    header = (header_t *)block - 1;
    if (header->s.size >= size) {
        return block;
    }
    ret = malloc(size);

    if (ret) {
        memcpy(ret, block, header->s.size);
        free(block);
    }
    return ret;
}


