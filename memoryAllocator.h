#ifndef MEM_ALLOC
#define MEM_ALLOC

void *malloc(size_t size);
void *get_free_block(size_t size);
void free(void *block);
void *calloc(size_t num, size_t nsize);
void *realloc(void *block, size_t size);

#endif
