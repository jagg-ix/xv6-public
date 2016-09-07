3350 struct buf {
3351   int flags;
3352   uint dev;
3353   uint blockno;
3354   struct buf *prev; // LRU cache list
3355   struct buf *next;
3356   struct buf *qnext; // disk queue
3357   uchar data[BSIZE];
3358 };
3359 #define B_BUSY  0x1  // buffer is locked by some process
3360 #define B_VALID 0x2  // buffer has been read from disk
3361 #define B_DIRTY 0x4  // buffer needs to be written to disk
3362 
3363 
3364 
3365 
3366 
3367 
3368 
3369 
3370 
3371 
3372 
3373 
3374 
3375 
3376 
3377 
3378 
3379 
3380 
3381 
3382 
3383 
3384 
3385 
3386 
3387 
3388 
3389 
3390 
3391 
3392 
3393 
3394 
3395 
3396 
3397 
3398 
3399 
