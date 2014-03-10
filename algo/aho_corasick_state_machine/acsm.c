
#include <stdarg.h>
#include "acsm.h"


/*
* Copyright (C) Weibin Yao
* Adopt the design of Martin Roesch <roesch@sourcefire.com>
*
* DOCUMENT
*
* See wiki: http://en.wikipedia.org/wiki/Aho%E2%80%93Corasick_string_matching_algorithm
*
* This program is an implementation of the paper[1].
*
* [1]  Margaret J. Corasick (June 1975). "Efficient string matching: An aid to bibliographic 
* search". Communications of the ACM 18 (6): 333–340. doi:10.1145/360825.360855 
*
* */


#define DEBUG 0

static void
debug_printf(const char *fmt, ...)
{

#if (DEBUG)
    va_list ap;

    va_start(ap, fmt);
    (void) vprintf(fmt, ap);
    va_end(ap);
#endif

}


#define acsm_queue_init(q)                                                    \
    (q)->prev = q;                                                            \
    (q)->next = q


#define acsm_queue_empty(h)                                                   \
    (h == (h)->prev)


#define acsm_queue_insert_head(h, x)                                          \
    (x)->next = (h)->next;                                                    \
    (x)->next->prev = x;                                                      \
    (x)->prev = h;                                                            \
    (h)->next = x


#define acsm_queue_insert_tail(h, x)                                          \
    (x)->prev = (h)->prev;                                                    \
    (x)->prev->next = x;                                                      \
    (x)->next = h;                                                            \
    (h)->prev = x


#define acsm_queue_head(h)                                                    \
    (h)->next


#define acsm_queue_last(h)                                                    \
    (h)->prev


#define acsm_queue_sentinel(h)                                                \
    (h)


#define acsm_queue_next(q)                                                    \
    (q)->next


#define acsm_queue_prev(q)                                                    \
    (q)->prev


#define acsm_queue_remove(x)                                                  \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next


#define acsm_queue_data(q, type, link)                                        \
    (type *) ((u_char *) q - offsetof(type, link))


static acsm_state_queue_t *acsm_alloc_state_queue(acsm_context_t *ctx)
{
    acsm_queue_t       *q;
    acsm_state_queue_t *sq;

    if (acsm_queue_empty(&ctx->free_queue.queue)) {
        sq = malloc(sizeof(acsm_state_queue_t)); 
    }
    else {
        q = acsm_queue_last(&ctx->free_queue.queue);
        acsm_queue_remove(q);
        sq = acsm_queue_data(q, acsm_state_queue_t, queue);
    }

    return sq;
}


static void acsm_free_state_queue(acsm_context_t *ctx, acsm_state_queue_t *sq)
{
    acsm_queue_remove(&sq->queue);
    acsm_queue_insert_tail(&ctx->free_queue.queue, &sq->queue);

    sq->state = ACSM_FAIL_STATE;
}


static int acsm_add_state(acsm_context_t *ctx, int state)
{
    acsm_state_queue_t *sq;

    sq = acsm_alloc_state_queue(ctx);
    if (sq == NULL) {
        return -1;
    }

    sq->state = state;
    acsm_queue_insert_head(&ctx->work_queue.queue, &sq->queue);

    return 0;
}


static int acsm_next_state(acsm_context_t *ctx)
{
    int                 state;
    acsm_queue_t       *q;
    acsm_state_queue_t *sq;

    if (acsm_queue_empty(&ctx->work_queue.queue)) {
        return ACSM_FAIL_STATE;
    }

    q = acsm_queue_last(&ctx->work_queue.queue);
    acsm_queue_remove(q);
    sq = acsm_queue_data(q, acsm_state_queue_t, queue);

    state = sq->state;

    acsm_free_state_queue(ctx, sq);

    return state;
}


static void acsm_free_state(acsm_context_t *ctx)
{
    acsm_queue_t       *q;
    acsm_state_queue_t *sq;

    while ((q = acsm_queue_last(&ctx->free_queue.queue))) {
        if (q == acsm_queue_sentinel(&ctx->free_queue.queue)) {
            break;
        }

        acsm_queue_remove(q);
        sq = acsm_queue_data(q, acsm_state_queue_t, queue);

        free(sq);
    }
}


static int acsm_state_add_match_pattern(acsm_context_t *ctx, 
        int state, acsm_pattern_t *p)
{
    acsm_pattern_t *copy;

    copy = malloc(sizeof(acsm_pattern_t));
    if (copy == NULL) {
        return -1;
    }
    memcpy(copy, p, sizeof(acsm_pattern_t));

    copy->next = ctx->state_table[state].match_list; 
    ctx->state_table[state].match_list = copy; 

    return 0;
}


static int acsm_state_union_match_pattern(acsm_context_t *ctx, 
        int state, int fail_state)
{
    acsm_pattern_t *p;

    p = ctx->state_table[fail_state].match_list;

    while (p) {
        acsm_state_add_match_pattern(ctx, state, p);
        p = p->next;
    }

    return 0;
}


acsm_context_t *acsm_alloc(int flag)
{
    int no_case = 0;
    acsm_context_t *ctx;

    if (flag & NO_CASE) {
        no_case = 1;
    }

    ctx = calloc(1, sizeof(acsm_context_t));
    if (ctx == NULL) {
        return NULL;
    }

    ctx->no_case = no_case;
    ctx->max_state = 1;
    ctx->num_state = 0;

    acsm_queue_init(&ctx->work_queue.queue);
    acsm_queue_init(&ctx->free_queue.queue);

    return ctx;
}


void acsm_free(acsm_context_t *ctx)
{
    unsigned        i;
    acsm_pattern_t *p, *op;

    for (i = 0; i <= ctx->num_state; i++) {
        if (ctx->state_table[i].match_list) {
            p = ctx->state_table[i].match_list;
            
            while(p) {
                op = p;
                p = p->next;

                free(op);
            }
        }
    }
    
    p = ctx->patterns;
    while (p) {
        op = p;
        p = p->next;

        if (op->string) {
            free(op->string);
        }

        free(op);
    }

    free(ctx->state_table);
    
    free(ctx);
}


int acsm_add_pattern(acsm_context_t *ctx, u_char *string, size_t len) 
{
    u_char ch;
    size_t i;
    acsm_pattern_t *p;

    p = malloc(sizeof(acsm_pattern_t));
    if (p == NULL) {
        return -1;
    }

    p->len = len;

    if (len > 0) {
        p->string = malloc(len);
        if (p->string == NULL) {
            return -1;
        }

        for(i = 0; i < len; i++) {
            ch = string[i];
            p->string[i] = ctx->no_case ? acsm_tolower(ch) : ch;
        }
    }

    p->next = ctx->patterns;
    ctx->patterns = p;

    debug_printf("add_pattern: \"%.*s\"\n", p->len, string);

    ctx->max_state += len;

    return 0;
}


int acsm_compile(acsm_context_t *ctx)
{
    int state, new_state, r, s;
    u_char ch;
    unsigned int i, j, k;
    acsm_pattern_t *p;

    ctx->state_table = malloc(ctx->max_state * sizeof(acsm_state_node_t));
    if (ctx->state_table == NULL) {
        return -1;
    }

    for (i = 0; i < ctx->max_state; i++) {

        ctx->state_table[i].fail_state = 0;
        ctx->state_table[i].match_list = NULL;

        for (j = 0; j < ASCIITABLE_SIZE; j++) {
            ctx->state_table[i].next_state[j] = ACSM_FAIL_STATE;
        }
    }

    debug_printf("goto function\n");

    /* Construction of the goto function */
    new_state = 0;
    p = ctx->patterns;
    while (p) {
        state = 0;
        j = 0;

        while(j < p->len) {

            ch = p->string[j];
            if (ctx->state_table[state].next_state[ch] == ACSM_FAIL_STATE) {
                break;
            }

            state = ctx->state_table[state].next_state[ch];
            j++;
        }

        for (k = j; k < p->len; k++) {
            new_state = ++ctx->num_state;
            ch = p->string[k];

            debug_printf("add_match_pattern: state=%d, new_state=%d\n", state, new_state);
            ctx->state_table[state].next_state[ch] = new_state;
            state = new_state;
        }

        debug_printf("add_match_pattern: state=%d, s=%.*s\n", state, p->len, p->string);

        acsm_state_add_match_pattern(ctx, state, p);

        p = p->next;
    }

    for (j = 0; j < ASCIITABLE_SIZE; j++) {
        if (ctx->state_table[0].next_state[j] == ACSM_FAIL_STATE) {
            ctx->state_table[0].next_state[j] = 0;
        }
    }

    debug_printf("failure function\n");

    /* Construction of the failure function */
    for (j = 0; j < ASCIITABLE_SIZE; j++) {
        if (ctx->state_table[0].next_state[j] != 0) {
            s = ctx->state_table[0].next_state[j];

            if (acsm_add_state(ctx, s) != 0) {
                return -1;
            }

            ctx->state_table[s].fail_state = 0;
        }
    }

    while (!acsm_queue_empty(&ctx->work_queue.queue)) {

        r = acsm_next_state(ctx);
        debug_printf("next_state: r=%d\n", r);

        for (j = 0; j < ASCIITABLE_SIZE; j++) {
            if (ctx->state_table[r].next_state[j] != ACSM_FAIL_STATE) {
                s = ctx->state_table[r].next_state[j];

                acsm_add_state(ctx, s);

                state = ctx->state_table[r].fail_state;

                while(ctx->state_table[state].next_state[j] == ACSM_FAIL_STATE) {
                    state = ctx->state_table[state].fail_state;
                }

                ctx->state_table[s].fail_state = ctx->state_table[state].next_state[j];
                debug_printf("fail_state: f[%d] = %d\n", s, ctx->state_table[s].fail_state);

                acsm_state_union_match_pattern(ctx, s, ctx->state_table[s].fail_state);
            }
            else {
                state = ctx->state_table[r].fail_state;
                ctx->state_table[r].next_state[j] = ctx->state_table[state].next_state[j];
            }
        }
    }

    acsm_free_state(ctx);
    debug_printf("end of compile function\n");

    return 0;
}


int acsm_search(acsm_context_t *ctx, u_char *text, size_t len)
{
    int state = 0;
    u_char *p, *last, ch;

    p = text;
    last = text + len;

    while (p < last) {
        ch = ctx->no_case ? acsm_tolower((*p)) : (*p);

        while (ctx->state_table[state].next_state[ch] == ACSM_FAIL_STATE) {
            state = ctx->state_table[state].fail_state;
        }

        state = ctx->state_table[state].next_state[ch];

        if (ctx->state_table[state].match_list) {
            return 1;
        }

        p++;
    }

    return 0;
}

#if 1

char *test_patterns[] = {"hers", "his", "she", "he", NULL};
char *text = 
"In the beginning God created the heaven and the earth. \n" \
"And the earth was without form, and void; and darkness was upon the face of the deep. And the Spirit of God moved upon the face of the waters. \n" \
"And God said, Let there be light: and there was light.\n" \
"And God saw the light, that it was good: and God divided the light from the darkness.\n" \
"And God called the light Day, and the darkness he called Night. And the evening and the morning were the first day.\n";


int main() 
{
    u_char         **input;
    acsm_context_t  *ctx;

    ctx = acsm_alloc(NO_CASE);
    if (ctx == NULL) {
        fprintf(stderr, "acsm_alloc() error.\n");
        return -1;
    }

    input = (u_char**) test_patterns;
   
    while(*input) {
        if (acsm_add_pattern(ctx, *input, acsm_strlen(*input)) != 0) {
            fprintf(stderr, "acsm_add_pattern() with pattern \"%s\" error.\n", 
                    *input);
            return -1;
        }

        input++;
    }

    debug_printf("after add_pattern: max_state=%d\n", ctx->max_state);
    
    if (acsm_compile(ctx) != 0) {
        fprintf(stderr, "acsm_compile() error.\n");
        return -1;
    }

    if (acsm_search(ctx, (u_char *)text, acsm_strlen(text))) {
        printf("match!\n");
    }
    else {
        printf("not match!\n");
    }

    acsm_free(ctx);

    return 0;
}

#endif
