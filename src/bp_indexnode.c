#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "bp_file.h"
#include "record.h"
#include "bp_indexnode.h"



BPLUS_INDEX_NODE * bplus_index_init(BPLUS_INFO * bp_info){
    int num_of_keys = bp_info->index_block_keys / 2 - 1;
    BPLUS_INDEX_NODE * index_node = malloc(sizeof(BPLUS_INDEX_NODE));
    index_node->node_depth = 0;
    index_node->key_number = 0;
    index_node->index_pointer = malloc((num_of_keys + 1) * sizeof(BF_Block**));
    index_node->keys = malloc(num_of_keys * sizeof(int *));
    return index_node;



}





