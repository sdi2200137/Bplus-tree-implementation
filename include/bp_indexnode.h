#ifndef BP_INDEX_NODE_H
#define BP_INDEX_NODE_H
#include <record.h>
#include <bf.h>
#include <bp_file.h>
#include "bp_datanode.h"


typedef struct
{int node_depth;
 unsigned char key_number; // Ο τρέχων αριθμός κλειδιών του block
 BPLUS_DATA_NODE ** index_pointer; // Ένας μονοδιάστατος πίνακας από pointers
 int * keys; // Ένας μονοδιάστατος πίνακας απο κλειδιά

} BPLUS_INDEX_NODE;

 typedef struct 
{int node_depth;
 unsigned char key_number; // Ο τρέχων αριθμός κλειδιών του block
 void ** index_pointer;// Ένας μονοδιάστατος πίνακας από pointers
 int * keys; // Ένας μονοδιάστατος πίνακας απο κλειδιά

} BPLUS_INNER_INDEX_NODE;



BPLUS_INDEX_NODE * bplus_index_init(BPLUS_INFO *bp_info);

#endif

 //struct BPLUS_INNER_INDEX_NODE ** index_pointer;