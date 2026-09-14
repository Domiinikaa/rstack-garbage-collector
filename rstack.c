/* Author: Dominika Szczecińska 469390
 *
 * SOLUTION OVERVIEW AND LOGIC:
 * 
 * 1. DATA STRUCTURE:
 * 
 *  The stack is implemented as a doubly-linked list (element_t). This allows 
 *  easy traversal from the 'bottom' (essential for maintaining order during file output in rstack_write).
 * 
 *  All rstack instances are tracked in a global doubly-linked list with a 
 * 'sentinel' node to facilitate global memory management.
 *
 * 2. MEMORY MANAGEMENT - Garbage Collection (Mark & Sweep):
 * 
 * rstack_delete: Instead of immediate deallocation, it marks a stack as 
 * no longer being a "root" and triggers a 'sweep'.
 * This ensures that stacks shared between multiple parents are not freed
 * as long as they are still reachable from any root.
 * 
 * Mark Phase: Starting from all active root stacks, the algorithm 
 * recursively marks all reachable stacks as 'visited'.
 * 
 * Sweep Phase: The global list is traversed, and any stack not marked as 
 * reachable is unlinked and its memory is freed.
 *
 * 3. RECURSION CONTROL (visit_id and Cycle Detection):
 * 
 * To handle potential cycles and prevent infinite loops in operations like 
 * 'empty', 'front', I implemented a 'visit_id' system.
 * 
 * Each global operation increments a 'current_visit_id'. If a function 
 * reaches a stack with the same ID, it knows it has already processed 
 * that node during the current pass.
 * 
 * In 'rstack_write', path_node_t represents the current recursion path 
 * - it stores all stacks visited along the way during rstack_write.
 * Each time the function goes deeper, it adds the current stack to this path.
 * 
 * The visited() function checks whether a stack is already in the current path.
 * If it is, a cycle is detected, and recursion stops to prevent an infinite loop.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

static uint64_t current_visit_id = 1;

typedef enum {
    VALUE,
    STACK
} element_type_t;

typedef struct element {
    struct element *next; 
    struct element *prev; 
    element_type_t type;

    union {
        struct rstack *stack;
        uint64_t number;
    } data;

} element_t;

typedef struct rstack {
    bool          is_reachable;
    bool          is_root;
    element_t     *top;   
    element_t     *bottom; 
    uint64_t      visit_id; 
    struct rstack *global_next;
    struct rstack *global_prev;
} rstack_t;

typedef struct {
    bool     flag;
    uint64_t value;
} result_t;

typedef struct path_node {
    struct path_node *prev;
    rstack_t         *rs;
} path_node_t;

static rstack_t  sentinel = {
    .top = NULL,
    .bottom = NULL,
    .global_next = &sentinel,
    .global_prev = &sentinel
};

static void      mark(rstack_t *rs) {
    if(!rs || rs->is_reachable) {
        return;
    }

    rs->is_reachable = true;
    element_t *e = rs->top;

    while(e) {
        if(e->type == STACK) {
            mark(e->data.stack);
        }
        e = e->next;
    }
}

static void      reset() {
    rstack_t *curr = sentinel.global_next;

    while(curr != &sentinel) {
        curr->is_reachable = false;
        curr = curr->global_next;
    }
}

static void      sweep() {
    reset();

    rstack_t *curr = sentinel.global_next;

    while(curr != &sentinel) {
        if(curr->is_root) {
            mark(curr);
        }
        curr = curr->global_next;
    }

    curr = sentinel.global_next;

    while(curr != &sentinel) {
        rstack_t *next = curr->global_next;

        if(!curr->is_reachable) {
            curr->global_prev->global_next = curr->global_next;
            curr->global_next->global_prev = curr->global_prev;
            element_t *e = curr->top;

            while(e) {
                element_t *to_free = e;
                e = e->next;
                free(to_free);
            }
            free(curr);
        }
        curr = next;
    }
}

rstack_t*        rstack_new() {
    rstack_t *new = (rstack_t *)malloc(sizeof(rstack_t));
    if(!new) {
        errno = ENOMEM;
        return NULL;
    }

    new->global_next = sentinel.global_next;
    new->global_prev = &sentinel;
    new->global_next->global_prev = new;
    sentinel.global_next = new;

    new->is_reachable = false;
    new->is_root = true;
    new->bottom = NULL;
    new->top = NULL;
    new->visit_id = 0;

    return new;
}

void             rstack_delete(rstack_t *rs) {
    if(!rs) return;
    
    rs->is_root = false;
    sweep();
}

int              rstack_push_value(rstack_t *rs, uint64_t value) {
    if(!rs) {
        errno = EINVAL;
        return -1;
    }

    element_t *new = (element_t *)malloc(sizeof(element_t));
    if(!new) {
        errno = ENOMEM;
        return -1;
    }

    new->data.number = value;
    new->type = VALUE;
    new->next = rs->top;
    new->prev = NULL;

    if(rs->top) 
        rs->top->prev = new;
    else 
        rs->bottom = new;
    
    rs->top = new;
    return 0;
}

int              rstack_push_rstack(rstack_t *rs1, rstack_t *rs2) {
    if(!rs1 || !rs2) {
        errno = EINVAL;
        return -1;
    }

    element_t *new = (element_t *)malloc(sizeof(element_t));
    if(!new) {
        errno = ENOMEM;
        return -1;
    }

    new->data.stack = rs2;
    new->type = STACK;
    new->next = rs1->top;
    new->prev = NULL;

    if(rs1->top)
        rs1->top->prev = new;
    else
        rs1->bottom = new;
    
    rs1->top = new;
    return 0;
}

void             rstack_pop(rstack_t *rs) {
    if(!rs || !rs->top)
        return;

    element_t *temp = rs->top;
    rs->top = rs->top->next;

    if (rs->top)
        rs->top->prev = NULL;
    else
        rs->bottom = NULL;

    free(temp);
}

static bool      rstack_empty_inner(rstack_t *rs, uint64_t visit_id) {
    if (visit_id == rs->visit_id)
        return true;
    if (!rs)
        return true;

    rs->visit_id = visit_id;
    element_t *e = rs->top;

    while(e) {
        if(e->type == VALUE)
            return false;
        if(e->type == STACK && !rstack_empty_inner(e->data.stack, visit_id))
            return false;
        
        e = e->next;
    }

    return true;
}

bool             rstack_empty(rstack_t *rs) {
    if (!rs)
        return true;

    current_visit_id++;
    return rstack_empty_inner(rs, current_visit_id);
}

static result_t  rstack_front_inner(rstack_t *rs, uint64_t visit_id) {
    result_t res = {.flag = false, .value = 0};

    if(!rs || rs->visit_id == visit_id) {
        return res;
    }

    rs->visit_id = visit_id;
    element_t *e = rs->top;

    while(e) {
        if(e->type == VALUE) {
            res.flag = true;
            res.value = e->data.number;
            return res;
        }

        if(e->type == STACK) {
            res = rstack_front_inner(e->data.stack, visit_id);

            if (res.flag) return res;
        }
        e = e->next;
    }
    return res;
}

result_t         rstack_front(rstack_t *rs) {
    if(!rs) {
        return (result_t){.flag = false, .value = 0};
    }

    current_visit_id++;
    return rstack_front_inner(rs, current_visit_id);
}

rstack_t         *rstack_read(char const *path) {
    if(!path) {
        errno = EINVAL;
        return NULL;
    }

    FILE *f = fopen(path, "r");
    if(!f) return NULL;
    
    rstack_t *stack = rstack_new();

    if(!stack) {
        fclose(f);
        return NULL;
    }

    char buffer[128];
    while(fscanf(f, "%127s", buffer) == 1) {
        if(buffer[0] == '-') {
            rstack_delete(stack);
            fclose(f);
            errno = EINVAL;
            return NULL;
        }

        char *endptr;
        errno = 0;
        unsigned long long val = strtoull(buffer, &endptr, 10);

        if(errno == ERANGE || *endptr != '\0' || endptr == buffer) {
            rstack_delete(stack);
            fclose(f);
            if(errno == 0)
                errno = EINVAL;
            return NULL;
        }

        if(rstack_push_value(stack, (uint64_t)val) != 0) {
            rstack_delete(stack);
            fclose(f);
            return NULL;
        }
    }

    if(ferror(f)) {
        rstack_delete(stack);
        fclose(f);
        errno = EIO;
        return NULL;
    }

    fclose(f);
    return stack;
}

static bool      visited(path_node_t *path, rstack_t *rs) {
    while(path != NULL) {
        if(path->rs == rs)
            return true;
        
        path = path->prev;
    }
    return false;
}

static bool      write_recursive(rstack_t *rs, path_node_t *path, FILE *f) {
    if(!rs) {
        return true;
    }
    if(visited(path, rs)) {
        return false;
    }

    path_node_t curr_visit = {.rs = rs, .prev = path};

    for(element_t *e = rs->bottom; e != NULL; e = e->prev) {
        if(e->type == VALUE) {
            if(fprintf(f, "%" PRIu64 "\n", e->data.number) < 0) {
                return false;
            }
        }
        else {
            if(!write_recursive(e->data.stack, &curr_visit, f)) {
                return false;
            }
        }
    }
    return true;
}

int              rstack_write(char const *path, rstack_t *rs) {
    if(!path || !rs) {
        errno = EINVAL;
        return -1;
    }

    FILE *f = fopen(path, "w");
    if (!f) {
        return -1;
    }

    write_recursive(rs, NULL, f);
    int result = (ferror(f) || fclose(f) != 0) ? -1 : 0;

    return result;
}
