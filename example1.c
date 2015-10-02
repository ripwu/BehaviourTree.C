
#include "bt.h"
#include <stdio.h>

void *node_list[2] = { 0 };

void example_enter(void *node, void *object, struct node_operations *ops)
{
    int *ctx = (int *)object;
    *ctx = *ctx + 1;
    printf("example_enter %p %d\n", node, *ctx);
}

void example_exit(void *node, void *object, struct node_operations *ops)
{
    int *ctx = (int *)object;
    *ctx = *ctx + 1;
    printf("example_exit %p %d\n", node, *ctx);
}

void example_tick(void *node, void *object, struct node_operations *ops)
{
    int *ctx = (int *)object;
    *ctx = *ctx + 1;
    printf("example_tick %p %d\n", node, *ctx);

    if (node == node_list[0])
    {
        ops->fail(node_control(node));
        return;
    }

    if (*ctx < 10) ops->running(node_control(node));
    else ops->success(node_control(node));
}

int main(int argc, char *argv[])
{
    behaviourtree_init();

    struct node_callbacks example_cbs = {
        .enter = example_enter,
        .tick = example_tick,
        .exit = example_exit
    };

    int begin_type = BRANCH_TYPE_BEGIN;
    int end_type = BRANCH_TYPE_COUNT;

    int running_type = begin_type;
    for (; running_type < end_type; ++running_type)
    {
        int i = 0;
        for (i = 0; i < 2; ++i)
        {
            node_list[i] = node_create(&example_cbs, 0);
            printf(">>> node_create %d %p\n", i, node_list[i]);
        }

        printf("\n");

        void *branch = branch_create(running_type, 2, node_list);
        void *tree = behaviourtree_create(branch);

        static const char *BRANCH_NAME[3] = { "SEQUENCE", "PRIORITY", "RANDOM" };
        printf(">>> branch_type %s\n", BRANCH_NAME[running_type]);

        int ctx = 1;
        int tick = 0;
        for (tick = 0; tick < 10; ++tick)
        {
            printf(">>> behaviourtree_tick %d\n", tick);
            behaviourtree_tick(tree, (void *)&ctx);
        }

        printf("\n");
        behaviourtree_release(tree);
    }

    return 0;
}

