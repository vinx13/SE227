#include <iostream>

#include "version.h"

version_control::version_control (block_manager *bm): bm_(bm), current_(0) {
    versions_.emplace_back();
}

void version_control::commit() {
    // print_logs_();
    std::cout << "vc: committing version " << current_ << std::endl;

    ++current_;
    if (current_ == versions_.size()) {
        versions_.emplace_back();
    } else {
        versions_[current_].clear();
    }
}

void version_control::rollback() {
    // print_logs_();

    std::cout << "vc: rollback version " << current_ << std::endl;

    if (current_ == 0) return;
    undo_actions_(current_version_());
    --current_;
}

void version_control::redo() {
    // print_logs_();

    if (current_ == versions_.size() - 1) {
        return;
    }

    ++current_;
    std::cout << "vc: redo version " << current_ << std::endl;

    do_actions_(current_version_());
}

actions &version_control::current_version_() {
    return versions_[current_];
}

void version_control::add_entry(const log_entry && entry) {
    std::cout << "vc: add entry on version " << current_ << ", block " << entry.id << std::endl;
    current_version_().emplace_back(entry);
}

void version_control::do_actions_(const actions &records) {
    for (const auto &record: records) {
       std::cout << "vc: redo on block " << record.id << std::endl;
       bm_->write_block_internal_(record.id, record.new_data);
    }
}
void version_control::undo_actions_(const actions &records) {
    for (auto it = records.rbegin(); it != records.rend(); it++) {
        std::cout << "vc: undo on block " << it->id << std::endl;
       bm_->write_block_internal_(it->id, it->old_data); 
    }
}

void version_control::print_logs_() {
    int i = 0;
    for (const auto &actions: versions_) {
        std::cout << "Version: " << i++ << std::endl;
        for (const auto &action: actions) {
            std::cout << action.id << std::endl;
        }
        std::cout << std::endl;
    }
}
