//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// seq_scan_executor.cpp
//
// Identification: src/execution/seq_scan_executor.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/seq_scan_executor.h"

namespace bustub {

SeqScanExecutor::SeqScanExecutor(ExecutorContext *exec_ctx, const SeqScanPlanNode *plan)
    : AbstractExecutor(exec_ctx), plan_(plan) {
  txn_ = exec_ctx_->GetTransaction();
}

void SeqScanExecutor::Init() {
  table_oid_t table_oid = plan_->GetTableOid();
  auto info = exec_ctx_->GetCatalog()->GetTable(table_oid);
  auto tmp_it = info->table_->MakeIterator();
  it_.emplace(std::move(tmp_it));

  if (txn_ != nullptr) {
    if (exec_ctx_->IsDelete()) {
      if (!exec_ctx_->GetLockManager()->LockTable(txn_, LockManager::LockMode::INTENTION_EXCLUSIVE, table_oid)) {
        throw ExecutionException("table lock fails");
      }
    } else if (txn_->GetIsolationLevel() != IsolationLevel::READ_UNCOMMITTED &&
               !txn_->IsTableIntentionExclusiveLocked(table_oid) &&
               !exec_ctx_->GetLockManager()->LockTable(txn_, LockManager::LockMode::INTENTION_SHARED, table_oid)) {
      throw ExecutionException("table lock fails");
    }
  }
}

auto SeqScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  while (true) {
    auto oid = plan_->GetTableOid();
    if (it_->IsEnd()) {
      if (txn_ != nullptr && txn_->GetIsolationLevel() == IsolationLevel::READ_COMMITTED &&
          txn_->IsTableIntentionSharedLocked(oid)) {
        txn_->LockTxn();
        auto release_set = (*txn_->GetSharedRowLockSet())[oid];
        txn_->UnlockTxn();
        for (auto locked_rid : release_set) {
          exec_ctx_->GetLockManager()->UnlockRow(txn_, oid, locked_rid);
        }

        exec_ctx_->GetLockManager()->UnlockTable(txn_, oid);
      }
      return false;
    }

    auto cur_rid = it_->GetRID();
    if (txn_ != nullptr) {
      if (exec_ctx_->IsDelete()) {
        if (!exec_ctx_->GetLockManager()->LockRow(txn_, LockManager::LockMode::EXCLUSIVE, plan_->GetTableOid(),
                                                  cur_rid)) {
          throw ExecutionException("Grant X row lock fails");
        }
      } else {
        if (txn_->GetIsolationLevel() != IsolationLevel::READ_UNCOMMITTED) {
          if (!txn_->IsRowExclusiveLocked(oid, cur_rid) &&
              !exec_ctx_->GetLockManager()->LockRow(txn_, LockManager::LockMode::SHARED, plan_->GetTableOid(),
                                                    cur_rid)) {
            throw ExecutionException("Grant S row lock fails");
          }
        }
      }
    }

    auto tuple_pair = it_->GetTuple();
    ++(*it_);
    if (!tuple_pair.first.is_deleted_) {
      *tuple = tuple_pair.second;
      *rid = tuple_pair.second.GetRid();
      return true;
    }
  }
}

}  // namespace bustub