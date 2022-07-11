#include <stdio.h>
#include <stdlib.h>
#include "libavutil/fifo.h"

int main()
{
    /* create a FIFO buffer */
    AVFifoBuffer *fifo = av_fifo_alloc(13 * sizeof(int));
    int i, out, num, *p;

    /* fill data */
    for (i = 0; av_fifo_space(fifo) >= sizeof(int); i++) {
        av_fifo_generic_write(fifo, &i, sizeof(int), NULL);
    }
    /*
    * 此时fifo中内容为:
    * ------------------------------
    * |0|1|2|3|4|5|6|7|8|9|10|11|12|
    * ------------------------------
    */

    /* peek_at at FIFO */
    num = av_fifo_size(fifo) / sizeof(int);
    for (i = 0; i < num; i++) {
        av_fifo_generic_peek_at(fifo, &out, i * sizeof(int), sizeof(out), NULL);
        printf("%d: %d\n", i, out);
    }
    printf("\n");

    /* generic peek at FIFO */
    num = av_fifo_size(fifo);
    p = malloc(num);
    if (p == NULL) {
        fprintf(stderr, "failed to alloc memory.\n");
        av_fifo_freep(&p);
        exit(1);
    }

    (void)av_fifo_generic_peek(fifo, p, num, NULL);

    /* read data at p */
    num /= sizeof(int);
    for (i = 0; i < num; i++) {
        printf("%d: %d\n", i, p[i]);
    }
    printf("\n");

    /* test *ndx overflow */
    av_fifo_reset(fifo);
    fifo->rndx = fifo->wndx = ~(uint32_t)0 - 5;

    /* fill data */
    for (i = 0; av_fifo_space(fifo) >= sizeof(int); i++) {
        av_fifo_generic_write(fifo, &i, sizeof(int), NULL);
    }

    /* peek_at FIFO */
    num = av_fifo_size(fifo) / sizeof(int);
    for (i = 0; i < num; i++) {
        av_fifo_generic_peek_at(fifo, &out, i * sizeof(int), sizeof(out), NULL);
        printf("%d: %d\n", i, out);
    }
    printf("\n");

    /* test fifo_grow */
    (void)av_fifo_grow(fifo, 15 * sizeof(int));

    /* fill data */
    num = av_fifo_size(fifo) / sizeof(int);
    for (i = num; av_fifo_space(fifo) >= sizeof(int); i++) {
        av_fifo_generic_write(fifo, &i, sizeof(int), NULL);
    }
    /* peek_at at FIFO */
    num = av_fifo_size(fifo) / sizeof(int);
    for (i = 0; i < num; i++) {
        av_fifo_generic_peek_at(fifo, &out, i * sizeof(int), sizeof(out), NULL);
        printf("%d: %d\n", i, out);
    }

    av_fifo_free(fifo);
    free(p);

    return 0;
}