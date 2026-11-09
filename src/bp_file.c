#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "bp_file.h"
#include "record.h"
#include <bp_datanode.h>
#include <stdbool.h>
#define bplus_ERROR -1
#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      return bplus_ERROR;     \
    }                         \
  }

// Στοιχεία ομάδας στο readme

// Εισαγωγή σε ένα μπλοκ δεδομένων
// Επιστρέφει NULL αν υπάρχει εγγραφή με το ίδιο id ή δεν χρειάζεται να σπάσει το μπλοκ
// Στην περίπτωση που σπάσει το μπλοκ επιστρέφει έναν δείκτη στο data_node που δημιουργείται
BPLUS_DATA_NODE * data_node_insert(BPLUS_DATA_NODE * node, Record record, int file_desc, BPLUS_INFO * bplus_info){
     for (int i = 0; i < node->records_num; i++ ){
      if (record.id == node->records[i].id){ //Δεν επιτρέπεται να υπάρχουν δύο εγγραφές με το ίδιο id στο δέντρο
        printf("ID Already exists\n");
        return NULL;

      }

    }
    // Περίπτωση που δεν χρειάζεται να σπάσει το μπλοκ δεδομένων
    if (node->records_num < 8){
      int pos;
      for (int i = 0; i < node->records_num; i++){ 
        // Άν βρεθεί εγγραφή μέσα στο block δεδομένων η οποία έχει μεγαλύτερο id από αυτήν που θέλουμε να εισάγουμε 
        // εισάγουμε τη νέα εγγραφή στη θέση αυτής με το μεγαλύτερο id και μετακινούμε από αυτή τη θέση και μετά τις εγγραφές
        // μια θέση δεξιά
        pos = i+1;
        if (record.id > node->records[i].id){
          continue;
        }
        else{
          for (int j = node->records_num - 1; j >= i; j--){
            Record temp = node->records[j];
            node->records[j+1] = temp;
          }
          pos = i; 
          break;
        }  
      }
      // Στην περίπτωση που έχει μεγαλύτερο id από τις εγγραφές του block εισάγεται στην τελευταία θέση
    
      node->records[pos] = record;
      node->records_num++;
      return NULL;
    }
    // Περίπτωση που θα σπάσει το block δεδομένων
    else{
      BF_Block *block2;
      void *data2;
      BF_Block_Init(&block2);
      CALL_BF(BF_AllocateBlock(file_desc, block2));
      data2 = BF_Block_GetData(block2);
      BPLUS_DATA_NODE * node2 = data2;
      node2->records = malloc(bplus_info->data_block_records * sizeof(Record));
      // Μοιράζουμε τις εγγραφές στα δύο block
      for (int i = 4; i < node->records_num; i++ ){
        node2->records[i-4] = node->records[i];      
      }
      node->records_num = 4;
      node2->records_num = 4;
      // Το νέο μπλοκ τοποθετείται μετά το παλιό και άρα θα δείχνει στο επόμενο του παλιού μπλοκ
      node2->next = node->next;
      node->next = (BPLUS_PTR)node2; // Το παλιό μπλοκ δείχνει στο νέο
      BF_Block_SetDirty(block2);
      CALL_BF(BF_UnpinBlock(block2));
      return node2;
    } 
}  
// Περίπτωση που σπάει ένα index node, επιστρέφει δείκτη στο νέο index node  
BPLUS_INDEX_NODE * index_node_split(int num_of_keys, BPLUS_INDEX_NODE * node, int file_desc, BPLUS_INFO *bplus_info){
  BF_Block *block3;

  void *data3;
  BF_Block_Init(&block3);
  CALL_BF(BF_AllocateBlock(file_desc, block3));
  data3 = BF_Block_GetData(block3);

  BPLUS_INDEX_NODE * node3 = data3;
  node3->index_pointer = malloc((num_of_keys + 1) * sizeof(BPLUS_DATA_NODE *));
  node3->keys = malloc(num_of_keys * sizeof(int *));
  // Μοιράζουμε τα κλειδιά και τους δείκτες 
  for (int i = num_of_keys/2 + 1; i < node->key_number; i++ ){
    node3->keys[i-16] = node->keys[i];
    node3->index_pointer[i-16] = node->index_pointer[i];
            
  }
  // Ο τελευταίος δείκτης 
  node3->index_pointer[node->key_number - 16] = node->index_pointer[node->key_number];
  node->key_number = node->key_number - num_of_keys/2; 
  node3->key_number = node->key_number - 1;
  return node3;
}

int BP_CreateFile(char *fileName)
{
  BPLUS_INFO *bp_info;
  int fd1;
  BF_Block *block;
  void *data;
  int blocks_num;
  BF_Block_Init(&block);
  CALL_BF(BF_CreateFile(fileName));
  CALL_BF(BF_OpenFile(fileName, &fd1));
  CALL_BF(BF_AllocateBlock(fd1, block));
  data = BF_Block_GetData(block);
  bp_info = data;
  
  bp_info->depth = 0;
  // Η μεταβλητή index_block_keys χωρίζεται σε δύο index_block_keys/2 + 1 για τον πίνακα των δεικτών και index_block_keys / 2 - 1 για τον πίνακα των κλειδιών
  bp_info->index_block_keys = (BF_BLOCK_SIZE - sizeof(int) - sizeof(unsigned char) - sizeof(BPLUS_DATA_NODE*))/(sizeof(int *)); // Πρώτο sizeof: node_depth Δεύτερο sizeof: key_number Τρίτο: index_pointer Τέταρτο: Πίνακας keys
  bp_info->data_block_records = (BF_BLOCK_SIZE - sizeof(unsigned char) - sizeof(struct BPLUS_DATA_NODE *)) / sizeof(Record); // Πρώτο sizeof: records_num Δεύτερο: next Τρίτο: Πίνακας records
  bp_info->root = NULL; 
  BF_GetBlockCounter(fd1, &blocks_num);
  
  BF_Block_SetDirty(block);
  // CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);
  //CALL_BF(BF_CloseFile(fd1)); 
  return 0;

}

BPLUS_INFO* BP_OpenFile(char *fileName, int *file_desc)
{
    BPLUS_INFO* bpInfo;    
    int fd1;
    void * data1;
    BF_Block *block;
    // Παίρνουμε τα μεταδεδομένα από το Block 0
    BF_Block_Init(&block);
    BF_OpenFile(fileName, &fd1);
    file_desc = &fd1;
    BF_GetBlock(fd1, 0, block);
    data1 = BF_Block_GetData(block);
    bpInfo = data1;
    BF_Block_Destroy(&block);
    return bpInfo;
}

int BP_CloseFile(int file_desc,BPLUS_INFO* info)
{
//   BF_Block *block, *last_block;
//   // Τα Block που δεν έχουν γίνει Unpin είναι το πρώτο και το τελευταίο

//   BF_Block_Init(&block); // Πρώτο Block
//   CALL_BF(BF_GetBlock(file_desc, 0, block));
//   CALL_BF(BF_UnpinBlock(block));
//   BF_Block_Destroy(&block);
//   BF_Block_Init(&last_block); //  Τελευταίο Block
//   int last_block_id;
//   BF_GetBlockCounter(file_desc, &last_block_id);
//   CALL_BF(BF_GetBlock(file_desc, last_block_id-1, last_block));
//   CALL_BF(BF_UnpinBlock(last_block));
//   BF_Block_Destroy(&last_block);

//  // CALL_BF(BF_CloseFile(file_desc));
 

  return 0;
}

int BP_InsertEntry(int file_desc,BPLUS_INFO *bplus_info, Record record)
{ 

  // Δημιουργείται το πρώτο μπλοκ δεδομένων
  if (bplus_info->depth == 0) {
    BF_Block *block;
    int blocks_num;
    void *data;
    BF_Block_Init(&block);
    CALL_BF(BF_AllocateBlock(file_desc, block));
    data = BF_Block_GetData(block);
    BPLUS_DATA_NODE * node = data;
    
    bplus_info->depth = 1;
    bplus_info->root = node;
    node->records = malloc(bplus_info->data_block_records * sizeof(Record));
    node->records[0] = record;
    node->records_num = 1;
    CALL_BF(BF_GetBlockCounter(file_desc, &blocks_num));
    BF_Block_SetDirty(block);
    return blocks_num; 

  }

  // Περίπτωση που δεν έχει σπάσει το πρώτο μπλοκ δεδομένων
  else if (bplus_info->depth == 1){
    
    BF_Block *block;
    void *data;
    int blocks_num1;
    BF_Block_Init(&block);
    BF_GetBlock(file_desc, 1, block);
    BF_GetBlockCounter(file_desc, &blocks_num1);
    data = BF_Block_GetData(block);
    BPLUS_DATA_NODE * node = data;
    for (int i = 0; i < node->records_num; i++ ){
      if (record.id == node->records[i].id){
        printf("ID Already exists\n");
        return -1;

      }

    }
    
    BPLUS_DATA_NODE * node2=data_node_insert(node, record, file_desc, bplus_info);
    BF_Block_SetDirty(block);

    // Αν το node2 != NULL τότε έσπασε το πρώτο μπλοκ δεδομένων 
    if (node2 != NULL){
      bplus_info->depth = 2;
      BF_Block *block3;
      void *data3;
      BF_Block_Init(&block3);
      CALL_BF(BF_AllocateBlock(file_desc, block3));
      data3 = BF_Block_GetData(block3);
      BPLUS_INDEX_NODE * node3 = data3; // Δημιουργία του πρώτου index μπλοκ
      int num_of_keys = bplus_info->index_block_keys / 2 - 1;
      node3->index_pointer = malloc((num_of_keys + 1) * sizeof(BPLUS_DATA_NODE *));
      node3->keys = malloc(num_of_keys * sizeof(int *));
      node3->index_pointer[0] = node;
      node3->index_pointer[1] = node2;
      node3->keys[0] = node2->records[0].id; // Εισάγουμε την μικρότερη τιμή κλειδιού του δεξιού μπλοκ στο γονέα
      node3->node_depth = 1;
      node3->key_number = 1;
      bplus_info->root = node3;
      BF_Block_SetDirty(block);
     // BF_Block_SetDirty(block2);
      BF_Block_SetDirty(block3);
     // BF_Block_Destroy(&block);
      //BF_Block_Destroy(&block2);
     // BF_Block_Destroy(&block3);

    }    
      return -1;
  }
    
      
    if (bplus_info->depth >= 2){
      int current_depth = 0;  // Θεωρούμε ότι το depth αρχίζει απο 1 αλλά για λόγους αρίθμησης πίνακα θα ξεκινήσουμε από το 0
      BPLUS_INNER_INDEX_NODE ** inner_nodes = NULL; // ¨Ένας πίνακας με τους γονείς του node για depth > 2
      BPLUS_INDEX_NODE * node = NULL; // Μπλοκ ευρετηρίου που δείχνει στο μπλοκ δεδομένων που θα κάνουμε την εισαγωγή
      if (bplus_info->depth == 2) {
        node = bplus_info->root;
      }
      else {
        BPLUS_INNER_INDEX_NODE * inner_node = bplus_info->root;
        inner_nodes = malloc((bplus_info->depth - 2) * sizeof(BPLUS_INNER_INDEX_NODE *));
        
        inner_nodes[0] = inner_node; // Αρχίζουμε από τη ρίζα και κατεβαίνουμε προς το node
        while (current_depth != bplus_info->depth - 2) { 
          
          BPLUS_INNER_INDEX_NODE * current_node = inner_nodes[current_depth];
          int start=0;
          int end=0;
          for(int i = 0; i < current_node->key_number; i++){
            
            if (record.id == current_node->keys[i]){
              printf("ID Already Exists\n");
              return -1;

            }
            else if (record.id < current_node->keys[i]){
              start = i;
              end = i+1;
              break;

            }
          }
          
          if (current_depth != bplus_info->depth-3) {
            if (end != 0) {
              inner_nodes[current_depth+1] = current_node->index_pointer[start];
            
            }
            else{
              inner_nodes[current_depth+1] = current_node->index_pointer[current_node->key_number];
            }

          }
          else {
            // Έχουμε φτάσει στο γονέα του node 
            if (end != 0) {
              node = current_node->index_pointer[start]; 
            }
            else{
              node = current_node->index_pointer[current_node->key_number];
            }
          } 
          current_depth++;
        }
      } 
      int start=0;
      int end=0;
      // Ψάχνουμε σε ποιό μπλοκ δεδομένων πρέπει να γίνει η εισαγωγή
      for(int i = 0; i < node->key_number; i++){
        if (record.id == node->keys[i]){
          printf("ID Already exists \n");
          return -1;
        }
        else if (record.id < node->keys[i]){
          start = i;
          end = i+1;
          break;
        }
      }  
        BPLUS_DATA_NODE * node2 = NULL;
        if (end != 0){
          BPLUS_DATA_NODE * data_node = node->index_pointer[start];          
          node2=data_node_insert(data_node, record, file_desc, bplus_info);
        }
        else {
          BPLUS_DATA_NODE * data_node = node->index_pointer[node->key_number];
          node2=data_node_insert(data_node, record, file_desc, bplus_info);     
        }
        if (node2 != NULL){
          // Περίπτωση που σπάει το μπλόκ δεδομένων και χωράει το κλειδί του νέου μπλοκ στο μπλοκ ευρετηρίου
          if (node->key_number < bplus_info->index_block_keys/2-2){
            if (end != 0){
              // Το νέο κλειδί δεν εισάγεται στην τελευταία θέση και μετακινούμε τα παλιά κλειδιά και τους δείκτες μία θέση προς τα δεξιά
              // από αυτή που θα εισαχθεί το νέο κλειδί 
              for (int k = node->key_number-1; k >= start; k--){
                int temp = node->keys[k];
                node->keys[k+1] = temp;        
              }
              for (int k = node->key_number; k >= start; k--){   
                BPLUS_DATA_NODE * temp_data = node->index_pointer[k];
                node->index_pointer[k+1] = temp_data;
              }
              node->keys[start] = node2->records[0].id;
              node->index_pointer[start+1] = node2;
              node->key_number++;
            }
            else {
              // Το νέο κλείδί θα εισαχθεί στο τέλος του μπλοκ ευρετηρίου 
              node->index_pointer[node->key_number+1] = node2;
              node->keys[node->key_number] = node2->records[0].id;
              node->key_number++;
            }
          }    
          else{
            // Περίπτωση που σπάει το μπλοκ ευρετηρίου 
            int num_of_keys = bplus_info->index_block_keys / 2 - 1;
            if (bplus_info->depth == 2){
              // Περίπτωση που θα σπάσει το πρώτο μπλοκ ευρετηρίου
              BPLUS_INDEX_NODE * node3 = index_node_split(num_of_keys, node, file_desc, bplus_info);         
              BF_Block *block4;
              void *data4;
              BF_Block_Init(&block4);
              CALL_BF(BF_AllocateBlock(file_desc, block4));
              data4 = BF_Block_GetData(block4);
              BPLUS_INNER_INDEX_NODE *new_node = data4;
              new_node->index_pointer = malloc((num_of_keys + 1) * sizeof(void*));
              new_node->keys = malloc(num_of_keys * sizeof(int *));
              new_node->index_pointer[0] = node;
              new_node->index_pointer[1] = node3;
              new_node->keys[0] = node3->keys[0];
              bplus_info->root = new_node;
              new_node->key_number = 1;
              bplus_info->depth = 3;
            }

            else{
              // Περίπτωση που έχει σπάσει το πρώτο μπλοκ ευρετηρίου και για μεγάλο depth υπάρχει περίπτωση 
              // να σπάσουν πολλά μπλοκ ευρετηρίου οπότε ανεβαίνουμε προς τη ρίζα
              BPLUS_INDEX_NODE * node3 = index_node_split(num_of_keys, node, file_desc, bplus_info);
              current_depth--;
              while (current_depth > 0) {
                BPLUS_INNER_INDEX_NODE * parent = inner_nodes[current_depth];
                int start=0;
                int end=0;   
                for(int i = 0; i < parent->key_number; i++){
                  if (node3->keys[0] < parent->keys[i]){
                    start = i;
                    end = i+1;
                    break;

                  }
                }      
                if (parent->key_number < bplus_info->index_block_keys/2-2){
                  // Περίπτωση που χωράει εισαγωγή στο γονέα 
                  if (end != 0) {
                    // Εισαγωγή πριν την τελευταία θέση και ταξινόμηση
                    for (int k = parent->key_number; k > start; k--){
                      int temp = parent->keys[k];
                      parent->keys[k+1] = temp;
                    }
                    for (int k = parent->key_number; k > start + 1; k--){
                      BPLUS_DATA_NODE * temp_data = parent->index_pointer[k];
                      parent->index_pointer[k+1] = temp_data;
                    }
                    parent->keys[start] = node3->keys[0];
                    parent->index_pointer[start+1] = node3;
                    parent->key_number++;
                  }
                  else {
                    // Εισαγωγή στην τελευταία θέση
                    parent->keys[parent->key_number] = node3->keys[0];
                    parent->index_pointer[parent->key_number+1] = node3;
                    parent->key_number++;
                  }
                  break;
                }
                else {
                  // Περίπτωση που δε χωράει ο γονέας την εισαγωγή και πρέπει να σπάσει 
                  BPLUS_INDEX_NODE * split_node = index_node_split(num_of_keys, parent, file_desc, bplus_info);
                  if (current_depth == 0){   
                    // Περίπτωση που δεν υπάρχει το από πάνω επίπεδο και πρέπει να δημιουργηθεί    
                    BF_Block *block5;
                    void *data5;
                    BF_Block_Init(&block5);
                    CALL_BF(BF_AllocateBlock(file_desc, block5));
                    data5 = BF_Block_GetData(block5);
                    BPLUS_INNER_INDEX_NODE *new_node = data5;
                    new_node->index_pointer = malloc((num_of_keys + 1) * sizeof(void*));
                    new_node->keys = malloc(num_of_keys * sizeof(int *));
                    new_node->index_pointer[0] = parent;
                    new_node->index_pointer[1] = split_node;
                    new_node->keys[0] = split_node->keys[0];
                    new_node->key_number = 1;
                    bplus_info->depth++;
                    bplus_info->root = new_node;
                    break;
                  }
                  else{
                    // Περίπτωση που υπάρχει το από πάνω επίπεδο και συνεχίζεται η επανάληψη για να γίνει εισαγωγή σε αυτό 
                    node3 = split_node;
                  }
                }
                current_depth--;
              }
            }


          }
        }     
    }
 
  return 0;
}

int BP_GetEntry(int file_desc,BPLUS_INFO *bplus_info, int value,Record** record)
{  // Περίπτωση που υπάρχει μόνο ένα μπλοκ δεδομένων 
   if (bplus_info->depth == 1) {
    BPLUS_DATA_NODE * root = bplus_info->root;
    for (int i = 0; i < root->records_num; i++){
      if (root->records[i].id == value){
        *record = &root->records[i];
        return 0;
      }
    }

   }
   // Περίπτωση που υπάρχει ένα μπλοκ ευρετηρίου 
   else if (bplus_info->depth == 2) {
    BPLUS_INDEX_NODE * root = bplus_info->root;
    for (int i = 0; i < root->key_number; i++){
      if (value < root->keys[i]){
        BPLUS_DATA_NODE * data_node = root->index_pointer[i];
        for (int j = 0; j < data_node->records_num; j++){
          if (data_node->records[j].id == value){
            *record = &data_node->records[j];
            return 0;
          }
        }
        *record = NULL;
        return -1;
      }
    }
    BPLUS_DATA_NODE * data_node = root->index_pointer[root->key_number];
    for (int j = 0; j < data_node->records_num; j++){
      if (data_node->records[j].id == value){
        *record = &data_node->records[j];
        return 0;
      }
    }
    *record = NULL;
    return -1;
       

   }
   // Περίπτωση depth >= 3
  //  else {
  //   BPLUS_INNER_INDEX_NODE * root = bplus_info->root;
  //   int current_depth = 0;
  //   int start = 0;
  //   int end = 0;
  //   while(bplus_info->depth - current_depth != 2){  
  //     for (int i = 0; i < root->key_number; i++){
  //       if (value < root->keys[i]){
  //         start = i;
  //         end = i + 1;
  //       }
  //     }  
  //       if (bplus_info->depth - current_depth == 3){
  //         BPLUS_INDEX_NODE * index_node = NULL;
  //         if (end != 0) {
  //           BPLUS_INDEX_NODE * index_node = root->index_pointer[start];
  //         }
  //         else{
  //           BPLUS_INDEX_NODE * index_node = root->index_pointer[root->key_number];
  //         }
  //         for (int j = 0; j < index_node->key_number; j++){
  //           if (value < index_node->keys[j]){
  //             BPLUS_DATA_NODE * data_node = index_node->index_pointer[j];
  //             for (int k = 0; k < data_node->records_num; k++){
  //               if (data_node->records[k].id == value){
  //                 *record = &data_node->records[k];
  //                 return 0;
  //               }
  //             }
  //             *record = NULL;
  //             return -1;
  //           }
  //         }
  //         BPLUS_DATA_NODE * data_node = index_node->index_pointer[index_node->key_number];
  //         for (int j = 0; j < data_node->records_num; j++){
  //           if (data_node->records[j].id == value){
  //             *record = &data_node->records[j];
  //             return 0;
  //           }
  //         }
  //         *record = NULL;
  //         return -1;
  //       }
  //       else {
  //          BPLUS_INNER_INDEX_NODE * inner_node = NULL;
  //         if (end != 0) {
  //           BPLUS_INNER_INDEX_NODE * inner_node = root->index_pointer[start];
  //         }
  //         else{
  //           BPLUS_INNER_INDEX_NODE * inner_node = root->index_pointer[root->key_number];
  //         }
  //         root = inner_node;
  //         current_depth++;
  //       }
  //   }
  //  }
  *record=NULL;
  return -1;
}











