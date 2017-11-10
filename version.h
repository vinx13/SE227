#ifndef VERSION_H
#define VERSION_H

#include <vector>
#include "inode_manager.h"

class inode_manager;
class block_manager;

struct log_entry {
    blockid_t id;
    char old_data[BLOCK_SIZE];
    char new_data[BLOCK_SIZE];

    log_entry (blockid_t id, const char *old_data, const char *new_data) {
        this->id = id;
        memcpy(this->old_data, old_data, BLOCK_SIZE);
        memcpy(this->new_data, new_data, BLOCK_SIZE);
    }
};


typedef std::vector<log_entry> actions;

class version_control {
public:
    version_control(block_manager *);

    void commit();
    void rollback();
    void redo();

    void add_entry(const log_entry &&);
private: 
    actions &current_version_();
    void do_actions_(const actions &);
    void undo_actions_(const actions &);

    std::vector<actions> versions_;
    int current_;
    block_manager * bm_;
    void new_version_();

    void print_logs_();
};


#endif

