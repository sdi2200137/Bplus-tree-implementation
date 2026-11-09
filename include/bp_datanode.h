#ifndef BP_DATANODE_H
#define BP_DATANODE_H
#include <record.h>
//#include <record.h>
#include "bf.h"
#include "bp_file.h"
#include "bp_indexnode.h"

typedef struct BPLUS_DATA_NODE * BPLUS_PTR;

typedef struct {
    unsigned char records_num; // Ο τρέχων αριθμός των records που υπάρχουν στο block
    //int max_recs; // Μέγιστος αριθμός records που χωράνε σε ένα block δεδομένων
    BPLUS_PTR next; // Δείκτης που δείχνει στο επόμενο block δεδομένων
    Record * records; // Ένας μονοδιάστατος πίνακας που δείχνει στις εγγραφές

} BPLUS_DATA_NODE;




BPLUS_DATA_NODE * bplus_data_init(BPLUS_INFO *bp_info, BPLUS_DATA_NODE * data_node);

#endif 