#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "bp_file.h"
#include "record.h"
#include "bp_datanode.h"


BPLUS_DATA_NODE * bplus_data_init(BPLUS_INFO *bp_info, BPLUS_DATA_NODE * data_node) {
    data_node = malloc(sizeof(BPLUS_DATA_NODE));
    data_node->records_num = 0;
    data_node->records = malloc(bp_info->data_block_records * sizeof(Record));
    data_node->next = NULL;
    return data_node;
}