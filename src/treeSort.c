#include "sort.h"
#include <stdlib.h>

typedef struct TreeNode {
    int value;
    size_t occurrences;
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

static void freeTree(TreeNode *root) {
    while (root != NULL) {
        if (root->left != NULL) {
            TreeNode *left = root->left;
            root->left = left->right;
            left->right = root;
            root = left;
        } else {
            TreeNode *right = root->right;
            free(root);
            root = right;
        }
    }
}

void treeSort(int values[], int count, SortStats *stats) {
    if (stats != NULL) {
        *stats = (SortStats){0};
    }
    if (count < 2) {
        return;
    }

    TreeNode *root = NULL;
    size_t node_count = 0;
    for (int index = 0; index < count; index++) {
        TreeNode **link = &root;
        while (*link != NULL) {
            if (stats != NULL) {
                stats->comparisons++;
            }
            if (values[index] < (*link)->value) {
                link = &(*link)->left;
            } else if (values[index] > (*link)->value) {
                link = &(*link)->right;
            } else {
                (*link)->occurrences++;
                break;
            }
        }

        if (*link == NULL) {
            TreeNode *node = malloc(sizeof(*node));
            if (node == NULL) {
                freeTree(root);
                return;
            }
            *node = (TreeNode){values[index], 1, NULL, NULL};
            *link = node;
            node_count++;
        }
    }

    TreeNode **stack = malloc(node_count * sizeof(*stack));
    if (stack == NULL) {
        freeTree(root);
        return;
    }
    if (stats != NULL) {
        stats->auxiliary_bytes = node_count * sizeof(TreeNode) +
                                 node_count * sizeof(*stack);
    }

    size_t stack_size = 0;
    int output = 0;
    TreeNode *current = root;
    while (current != NULL || stack_size > 0) {
        while (current != NULL) {
            stack[stack_size++] = current;
            current = current->left;
        }

        current = stack[--stack_size];
        for (size_t occurrence = 0; occurrence < current->occurrences;
             occurrence++) {
            values[output++] = current->value;
            if (stats != NULL) {
                stats->moves++;
            }
        }
        current = current->right;
    }

    free(stack);
    freeTree(root);
}
