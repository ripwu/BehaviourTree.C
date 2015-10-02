#include "bt.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct node_vtable
{
    struct node_callbacks *cbs;
    struct node_operations *ops;
    NODE_OPERATION release;
};

struct node {
    struct node_vtable *vtable;
    struct node *control;
    void *object;
    void *blackboard;
};

struct branch {
    struct node node;
    char is_running;
    int  running_idx;
    int  child_num;
    struct node* child_list[0];
};

struct behaviourtree {
    struct node node;
    struct branch *root;
    char started;
};

static struct node_operations node_ops;
static struct node_callbacks branch_cbs[BRANCH_TYPE_COUNT];
static struct node_operations branch_ops[BRANCH_TYPE_COUNT];
static struct node_operations behaviourtree_ops;

#define RUN_CALLBACK(node, op, object) do { node->vtable->cbs->op(node, object, node->control->vtable->ops); } while(0) 
#define RUN_OPERATION(node, op) do { node->vtable->ops->op(node); } while(0) 

void node_release(struct node *this)
{
    this->vtable->release(this);
}

void *node_create(struct node_callbacks *cbs, void *blackboard)
{
    if (cbs == 0) return 0;
    if (cbs->enter == 0 || cbs->tick == 0 || cbs->exit == 0) return 0;

    struct node *this = malloc(sizeof(struct node) + sizeof(struct node_vtable));
    if (this == 0) return 0;

    memset(this, 0, sizeof(struct node));
    this->blackboard = blackboard;
    this->vtable = (struct node_vtable *)(this + 1);
    this->vtable->cbs = cbs;
    this->vtable->ops = &node_ops;
    this->vtable->release = free;

    return this;
}

void node_callrun(struct node *this, void *object)
{
    this->vtable->cbs->tick(this, object, this->vtable->ops);
}

void node_running(void *node)
{
    struct node* this = node;
    if (this->control)
    {
        RUN_OPERATION(this->control, running);
    }
}

void node_success(void *node)
{
    struct node* this = node;
    if (this->control)
    {
        RUN_OPERATION(this->control, success);
    }
}

void node_fail(void *node)
{
    struct node* this = node;
    if (this->control)
    {
        RUN_OPERATION(this->control, fail);
    }
}

void branch_release(void *node)
{
    struct branch *this = node;

    int i = 0;
    for (; i < this->child_num; ++i)
    {
        struct node *running_node = this->child_list[i];
        node_release(running_node);
    }

    free(this);
}

void *branch_create(int type, int num, void **node)
{
    if (type < BRANCH_TYPE_BEGIN || type >= BRANCH_TYPE_COUNT) return 0;

    int size = sizeof(struct branch) + num*sizeof(struct node*) + sizeof(struct node_vtable);

    struct branch *this = malloc(size);
    if (this == 0) return 0;

    memset(this, 0, size);
    this->node.vtable = (struct node_vtable *)((char *)this + sizeof(struct branch) + num*sizeof(struct node*));
    this->node.vtable->cbs = &branch_cbs[type];
    this->node.vtable->ops = &branch_ops[type];
    this->node.vtable->release = branch_release;
    this->child_num = num;

    int i = 0;
    for (i = 0; i < num; ++i)
    {
        this->child_list[i] = ((struct node **)node)[i];
        if (this->child_list[i] == 0) goto err;
        this->child_list[i]->control = (struct node *)this;
    }

    return this;

err:
    free(this);
    return 0;
}

void branch_run(struct branch *this, void *object)
{
    struct node *running_node = this->child_list[this->running_idx];
    if (!this->is_running)
    {
        RUN_CALLBACK(running_node, enter, object);
    }

    RUN_CALLBACK(running_node, tick, object);
}

void branch_enter(void *node, void *object, struct node_operations *ops)
{
    struct branch *this = node;
    if (!this->is_running)
    {
        this->node.object = object;
        this->running_idx = 0;
    }
}

void branch_exit(void *node, void *object, struct node_operations *ops)
{
}

void branch_tick(void *node, void *object, struct node_operations *ops)
{
    struct branch *this = node;
    if (this->running_idx < this->child_num)
    {
        branch_run(this, object);
    }
}

void branch_running(void *node)
{
    struct branch *this = node;
    this->is_running = 1;
    RUN_OPERATION(this->node.control, running);
}

void branch_success(void *node)
{
    struct branch *this = node;
    this->is_running = 0;

    struct node *running_node = this->child_list[this->running_idx];
    RUN_CALLBACK(running_node, exit, this->node.object);
}

void branch_fail(void *node)
{
    struct branch *this = node;
    this->is_running = 0;

    struct node *running_node = this->child_list[this->running_idx];
    RUN_CALLBACK(running_node, exit, this->node.object);
}

void sequence_success(void *node)
{
    struct branch *this = node;
    branch_success(this);
    this->running_idx++;
    if (this->running_idx < this->child_num)
    {
        branch_run(this, this->node.object);
    }
    else
    {
        RUN_OPERATION(this->node.control, success);
    }
}

void sequence_fail(void *node)
{
    struct branch *this = node;
    branch_fail(this);
    RUN_OPERATION(this->node.control, fail);
}

void random_enter(void *node, void *object, struct node_operations *ops)
{
    struct branch *this = node;
    branch_enter(this, object, ops);

    int rand = random();
    this->running_idx = rand % this->child_num;
#ifndef DEBUG
    printf(">>> random_enter running_idx %d\n", this->running_idx);
#endif
}

void random_success(void *node)
{
    struct branch *this = node;
    branch_success(this);
    RUN_OPERATION(this->node.control, success);
}

void random_fail(void *node)
{
    struct branch *this = node;
    branch_fail(this);
    RUN_OPERATION(this->node.control, fail);
}

void priority_success(void *node)
{
    struct branch *this = node;
    branch_success(this);
    RUN_OPERATION(this->node.control, success);
}

void priority_fail(void *node)
{
    struct branch *this = node;
    branch_fail(this);
    this->running_idx++;
    if (this->running_idx < this->child_num)
    {
        branch_run(this, this->node.object);
    }
    else
    {
        RUN_OPERATION(this->node.control, fail);
    }
}

void behaviourtree_release(void *bt)
{
    struct behaviourtree *this = bt;
    node_release((struct node *)this->root);
    free(this);
}

void *behaviourtree_create(void *branch)
{
    if (branch == 0) return 0;

    struct behaviourtree *this = malloc(sizeof(struct behaviourtree) + sizeof(struct node_vtable));
    if (this == 0) return 0;

    memset(this, 0, sizeof(struct behaviourtree));
    this->root = branch;
    this->root->node.control = (struct node *)this;
    this->node.vtable = (struct node_vtable *)(this + 1);
    //this->node.cbs = { NULL, NULL, NULL };
    this->node.vtable->ops = &behaviourtree_ops;
    this->node.vtable->release = behaviourtree_release;

    return this;
}

void behaviourtree_tick(void *bt, void *object)
{
    if (bt == 0) return;

    struct behaviourtree *this = (struct behaviourtree *)bt;
    if (this->started)
    {
        node_running(&this->node);
    }
    else
    {
        this->started = 1;
        this->node.object = object;
        RUN_CALLBACK((&this->root->node), enter, object);
        node_callrun(&this->root->node, object);
    }
}

void behaviourtree_running(void *bt)
{
    struct behaviourtree *this = (struct behaviourtree *)bt;
    node_running((struct node *)this);
    this->started = 0;
}

void behaviourtree_success(void *bt)
{
    struct behaviourtree *this = (struct behaviourtree *)bt;
    RUN_CALLBACK((&this->root->node), exit, this->node.object);
    this->started = 0;
    node_success((struct node *)this);
}

void behaviourtree_fail(void *bt)
{
    struct behaviourtree *this = (struct behaviourtree *)bt;
    RUN_CALLBACK((&this->root->node), exit, this->node.object);
    this->started = 0;
    node_fail((struct node *)this);
}

void behaviourtree_init()
{
    node_ops.running = node_running;
    node_ops.success = node_success;
    node_ops.fail = node_fail;

    branch_cbs[BRANCH_TYPE_SEQUENCE].enter = branch_enter;
    branch_cbs[BRANCH_TYPE_SEQUENCE].tick = branch_tick;
    branch_cbs[BRANCH_TYPE_SEQUENCE].exit = branch_exit;

    branch_cbs[BRANCH_TYPE_PRIORITY].enter = branch_enter;
    branch_cbs[BRANCH_TYPE_PRIORITY].tick = branch_tick;
    branch_cbs[BRANCH_TYPE_PRIORITY].exit = branch_exit;

    branch_cbs[BRANCH_TYPE_RANDOM].enter = random_enter;
    branch_cbs[BRANCH_TYPE_RANDOM].tick = branch_tick;
    branch_cbs[BRANCH_TYPE_RANDOM].exit = branch_exit;

    branch_ops[BRANCH_TYPE_SEQUENCE].running = branch_running;
    branch_ops[BRANCH_TYPE_SEQUENCE].success = sequence_success;
    branch_ops[BRANCH_TYPE_SEQUENCE].fail = sequence_fail;

    branch_ops[BRANCH_TYPE_PRIORITY].running = branch_running;
    branch_ops[BRANCH_TYPE_PRIORITY].success = priority_success;
    branch_ops[BRANCH_TYPE_PRIORITY].fail = priority_fail;

    branch_ops[BRANCH_TYPE_RANDOM] .running = branch_running;
    branch_ops[BRANCH_TYPE_RANDOM] .success = random_success;
    branch_ops[BRANCH_TYPE_RANDOM] .fail = random_fail;

    behaviourtree_ops.running = behaviourtree_running;
    behaviourtree_ops.success = behaviourtree_success;
    behaviourtree_ops.fail = behaviourtree_fail;
}

void *node_control(void *node)
{
    struct node *this = node;
    return this->control;
}

void *node_blackboard(void *node)
{
    struct node *this = node;
    return this->blackboard;
}

