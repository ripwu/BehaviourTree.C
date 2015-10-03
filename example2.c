
#include "bt.h"
#include <stdio.h>

enum DIR
{
    DIR_S = 2,
    DIR_W = 4,
    DIR_E = 6,
    DIR_N = 8,
};

const char *DIR_NAME[9] = { [DIR_S]="SOUTH", [DIR_W]="WEST", [DIR_E]="EAST", [DIR_N]="NORTH" };

void looking_enter(void *node, void *object, struct node_operations *ops)
{
    printf("looking_enter %p\n", node);
}

void looking_exit(void *node, void *object, struct node_operations *ops)
{
    printf("looking_exit %p\n", node);
}

void looking_tick(void *node, void *object, struct node_operations *ops)
{
    printf("looking_tick %p\n", node);
    ops->success(node_control(node));
}

void running_enter(void *node, void *object, struct node_operations *ops)
{
    int dir = *(int *) node_blackboard(node);
    printf("running_enter %p %s\n", node, DIR_NAME[dir]);
}

void running_exit(void *node, void *object, struct node_operations *ops)
{
    int dir = *(int *) node_blackboard(node);
    printf("running_exit %p %s\n", node, DIR_NAME[dir]);
}

void running_tick(void *node, void *object, struct node_operations *ops)
{
    int dir = *(int *) node_blackboard(node);
    printf("running_tick %p %s\n", node, DIR_NAME[dir]);
    ops->success(node_control(node));
}

void standing_enter(void *node, void *object, struct node_operations *ops)
{
    printf("standing_enter %p\n", node);
}

void standing_exit(void *node, void *object, struct node_operations *ops)
{
    printf("standing_exit %p\n", node);
}

void standing_tick(void *node, void *object, struct node_operations *ops)
{
    int *standing_count = node_blackboard(node);
    *standing_count = *standing_count+1;
    printf("standing_tick %p, stanging_count %d\n", node, *standing_count);
    if (*standing_count > 4)
        ops->success(node_control(node));
    else if (*standing_count >= 3 && *standing_count <= 4)
        ops->fail(node_control(node));
    else
        ops->running(node_control(node));
}

int main(int argc, char *argv[])
{
    behaviourtree_init();

    void *priority_checking_node = 0;
    void *random_running_node = 0;

    {
        void *standing_node = 0;
        void *looking_node = 0;

        {
            struct node_callbacks standing_cbs = {
                .enter = standing_enter,
                .tick = standing_tick,
                .exit = standing_exit
            };

            int standing_count = 0;
            standing_node = node_create(&standing_cbs, &standing_count);
        }

        {
            struct node_callbacks looking_cbs = {
                .enter = looking_enter,
                .tick = looking_tick,
                .exit = looking_exit
            };

            looking_node = node_create(&looking_cbs, 0);
        }

        void *node_list[2] = { standing_node, looking_node };
        priority_checking_node = branch_create(BRANCH_TYPE_PRIORITY, 2, node_list);
    }

    {
        struct node_callbacks running_cbs = {
            .enter = running_enter,
            .tick = running_tick,
            .exit = running_exit
        };

        int running_dir[4] = { DIR_S, DIR_W, DIR_E, DIR_N };
        void *running_node[4];
        int i = 0;
        for (; i < 4; ++i)
        {
            running_node[i] = node_create(&running_cbs, &running_dir[i]);
        }

        random_running_node = branch_create(BRANCH_TYPE_RANDOM, 4, running_node);
    }

    void *node_list[2] = { priority_checking_node, random_running_node };
    void *branch = branch_create(BRANCH_TYPE_SEQUENCE, 2, node_list);
    void *tree = behaviourtree_create(branch);

    int ctx = 1;
    int tick = 0;
    for (tick = 0; tick < 10; ++tick)
    {
        printf(">>> behaviourtree_tick %d\n", tick);
        behaviourtree_tick(tree, (void *)&ctx);
    }

    printf("\n");
    behaviourtree_tick(tree, (void *)&ctx);

    return 0;
}

