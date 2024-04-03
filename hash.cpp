#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cmath>
#include "hash.h"
#include <bitset>
#include "utils.h"
#include <cassert>
#include <set>
using namespace std;



// get the last n digits of binary form and turn into integer
int getHashKey(int key, int global_depth)
{
    

    int hash_key = 0, temp = 1;

    for (int i = 0; i < global_depth; i++)
    {
        hash_key += (key % 2) * temp;
        temp *= 2;
        key /= 2;
    }
    
    return hash_key;


}
hash_entry::hash_entry(int key, int value){
    this->key = key;
    this->value = value;
    this->next = next;
}

hash_bucket::hash_bucket(int hash_key, int depth){
    // initialization
    this->local_depth = depth;
    this->first = nullptr;
    this->hash_key = hash_key;
    this->num_entries = 0;
}

/* Free the memory alocated to this->first
*/
void hash_bucket::clear(){
  
    hash_entry* current = first;
    while (current != nullptr)
    {
        hash_entry* next_node = current->next;
        delete current;
        current = next_node;
    }

    // 
    first = nullptr;
    num_entries = 0;
}

hash_table::hash_table(int table_size, int bucket_size, int num_rows, vector<int> key, vector<int> value){
    
    // initialization
    this->table_size = table_size;
    this->bucket_size = bucket_size;
    this->global_depth = 1;
    this->bucket_table.resize(table_size);  

    
    for (int i = 0; i < table_size; i++)
    {
        bucket_table[i] = new hash_bucket(i, global_depth);
    }
   
    for (int i = 0; i < num_rows; i++)
    {
        insert(key[i], value[i]);
    }
   
}

/* When insert collide happened, it needs to do rehash and distribute the entries in the bucket.
** Furthermore, if the global depth equals to the local depth, you need to extend the table size.
*/
void hash_table::extend(hash_bucket *bucket){

    //
    int original_table_size = table_size;
    table_size *= 2;
    global_depth ++;
    bucket_table.resize(table_size);
   
    for (int i = 0; i < original_table_size; i++)
    {
        int new_idx = i + original_table_size;
        bucket_table[new_idx] = bucket_table[i];
    }
 

}

void hash_table::split(hash_bucket* bucket)
{
    int local_depth = bucket -> local_depth;
    hash_bucket* new_bucket = new hash_bucket(bucket -> hash_key + (int)pow(2, local_depth), local_depth + 1);
    if(local_depth == global_depth) extend(new_bucket);
    int hash_key = bucket -> hash_key;
    for (int i = 0, g = (int)pow(2, local_depth); i < bucket_table.size(); i += g) {
            if (((i) & (1 << local_depth))) {
                bucket_table[i + hash_key] = new_bucket;
            } else {
                bucket_table[i + hash_key] = bucket;
            }
    }
    hash_entry* current = bucket -> first;
    bucket -> first = nullptr;
    bucket -> local_depth ++;
    bucket -> num_entries = 0;
    while(current != nullptr){
        hash_entry* temp = new hash_entry(current -> key, current -> value);
        if(getHashKey(current -> key, local_depth + 1) == bucket -> hash_key){
            bucket -> num_entries ++;
            temp -> next = bucket -> first;
            bucket -> first = temp;
        }else{
            new_bucket -> num_entries ++;
            temp -> next = new_bucket -> first;
            new_bucket -> first = temp;
        }
        current = current -> next;
    }
}

/* When construct hash_table you can call insert() in the for loop for each key-value pair.
*/
void hash_table::insert(int key, int value){
    
    // 
    int hash_index = getHashKey(key, global_depth);
    hash_entry* current = bucket_table[hash_index] -> first;
    while(current != nullptr){
        if(current -> key == key){
            current -> value = value;
            return;
        }
        current = current -> next;
    }
        if (bucket_table[hash_index] -> num_entries < bucket_size)
        {
            hash_entry* new_entry = new hash_entry(key, value);
            new_entry -> next = bucket_table[hash_index] -> first;
            bucket_table[hash_index]->first = new_entry; 
            bucket_table[hash_index]->num_entries++;

        }
        else
        {
           
            split(bucket_table[hash_index]);
            insert(key, value);
        }
}

/* The function might be called when shrink happened.
** Check whether the table necessary need the current size of table, or half the size of table
*/
void hash_table::half_table(){
  
    for(int i = 0; i < table_size / 2 ;i++){
        if(bucket_table[i] -> local_depth == global_depth){
            return;
        }
    }
    int new_table_size = table_size / 2;
    vector<hash_bucket*> new_bucket_table(new_table_size, nullptr);
    for (int i = 0; i < bucket_table.size(); i++) {
        if (bucket_table[i] != nullptr) {
            
            int new_bucket_index = i % new_table_size;

            if (getHashKey(i, bucket_table[i] -> local_depth) == new_bucket_index) {
                new_bucket_table[new_bucket_index] = bucket_table[i];
            }
        }
    }

    // Set the local depth of each bucket in the new bucket_table
    for (int i = 0; i < new_bucket_table.size(); i++) {
        if (new_bucket_table[i] != nullptr) {
            new_bucket_table[i]->local_depth = bucket_table[i]->local_depth;
        }
    }

    // update the new hash table
    table_size = new_table_size;
    bucket_table = new_bucket_table;
    global_depth--;

    //
    half_table();
}

/* If a bucket with no entries, it need to check whether the pair hash index bucket 
** is in the same local depth. If true, then merge the two bucket and reassign all the 
** related hash index. Or, keep the bucket in the same local depth and wait until the bucket 
** with pair hash index comes to the same local depth.
*/
void hash_table::shrink(hash_bucket *bucket){
    
    if(bucket -> local_depth == 1){
        return;
    }
    int bucket_index = -1;
    int hash_key = getHashKey(bucket -> hash_key, bucket -> local_depth - 1);
    for (int i = 0, g = (int)pow(2, bucket -> local_depth - 1); i + hash_key < bucket_table.size(); i += g) 
    {
        if (bucket_table[i + hash_key] -> local_depth == bucket -> local_depth && bucket_table[i + hash_key] -> hash_key != bucket -> hash_key && ((bucket_table[i + hash_key] -> num_entries == 0) || (bucket -> num_entries == 0)))
        {
            bucket_index = i + hash_key;
            break;
        }
    }
   
    if(bucket_index < 0){
        return;
    }
    if(bucket -> num_entries != 0){
        swap(bucket_table[bucket_index], bucket);
    }
    bucket_table[bucket_index] -> local_depth--;
    bucket_table[bucket_index] -> hash_key = getHashKey(bucket_table[bucket_index] -> hash_key, bucket_table[bucket_index] -> local_depth);
    hash_key = bucket_table[bucket_index] -> hash_key;
    for (int i = 0, g = (int)pow(2, bucket -> local_depth - 1); i + hash_key < bucket_table.size(); i += g) {
        if(bucket_table[i + hash_key] == bucket){
            bucket_table[i + hash_key] = bucket_table[bucket_index];
        }
    }
    half_table();
    assert(bucket_table[bucket_index] -> num_entries > 0);
    shrink(bucket_table[bucket_index]);
  
}



/* When executing remove_query you can call remove() in the for loop for each key.
*/
void hash_table::remove(int key) {
    int idx = getHashKey(key, global_depth);
    hash_bucket* bucket = bucket_table[idx];

    hash_entry* prev = nullptr;
    hash_entry* entry = bucket->first;
  
    while (entry != nullptr && entry -> key != key)
    {
        prev = entry;
        entry = entry->next;
    }

    // if found, remove it and concatenate other nodes back to the list.
    if (entry != nullptr)
    {
        if(prev != nullptr)
            prev-> next = entry -> next;
        else
            bucket -> first = entry -> next;
        delete entry;
        bucket -> num_entries--;
    }
    if (bucket -> num_entries == 0) shrink(bucket);
}

void hash_table::key_query(vector<int> query_keys, string file_name){
    ofstream  file;
    file.open(file_name);
    
    for (int i = 0; i < query_keys.size(); i++)
    {
        int key = query_keys[i];
        int idx = getHashKey(key, global_depth);
        hash_entry* traverse_node = bucket_table[idx]->first;
 
        bool find = 0;
        while (traverse_node != nullptr)
        {

            if (traverse_node->key == key)
            {
                file << traverse_node -> value << ",";
                file << bucket_table[idx] -> local_depth << "\n";
                find = 1;
                break;
            }
            else 
            {
                traverse_node = traverse_node->next;
            }
            
        }
        if (find) continue;

        file << "-1," << bucket_table[idx] -> local_depth << "\n";
    }
    file.close();
}

void hash_table::remove_query(vector<int> query_keys){
    for (int i = 0; i < query_keys.size(); i++)
    {
        remove(query_keys[i]);
    }
}

/* Free the memory that you have allocated in this program
*/
void hash_table::clear(){
    set<hash_bucket*>temp;
    for (int i = 0; i < table_size; i++)
    {   if(temp.find(bucket_table[i]) != temp.end()){
        continue;
    }
        bucket_table[i] -> clear();
        temp.insert(bucket_table[i]);
        delete bucket_table[i];
    }
    bucket_table.clear();
}