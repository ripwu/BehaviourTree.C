
typedef void (*NODE_OPERATION)(void *node);

struct node_operations 
{
    NODE_OPERATION running;
    NODE_OPERATION success;
    NODE_OPERATION fail;
};

typedef void (*NODE_CALLBACK)(void *node, void *object, struct node_operations *ops);

struct node_callbacks 
{
    NODE_CALLBACK enter;
    NODE_CALLBACK tick;
    NODE_CALLBACK exit;
};

enum BRANCH_TYPE
{
    BRANCH_TYPE_BEGIN = 0,
    BRANCH_TYPE_SEQUENCE = BRANCH_TYPE_BEGIN,
    BRANCH_TYPE_PRIORITY,
    BRANCH_TYPE_RANDOM,
    BRANCH_TYPE_COUNT,
};

void behaviourtree_init();
void *node_create(struct node_callbacks *cbs, void *blackboard);
void *node_control(void *node);
void *node_blackboard(void *node);
void *branch_create(int type, int num, void **nodes);
void *behaviourtree_create(void *branch);
void behaviourtree_release(void *bt);
void behaviourtree_tick(void *bt, void *object);

