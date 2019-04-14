//===-- OptionValueRegexList.cpp ------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "lldb/Interpreter/OptionValueRegexList.h"

#include "lldb/Utility/Args.h"
#include "lldb/Utility/Stream.h"

using namespace lldb;
using namespace lldb_private;

void OptionValueRegexList::DumpValue(const ExecutionContext *exe_ctx, Stream &strm,
                                     uint32_t dump_mask) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (dump_mask & eDumpOptionType)
    strm.Printf("(%s)", GetTypeAsCString());
  if (dump_mask & eDumpOptionValue) {
    const bool one_line = dump_mask & eDumpOptionCommand;
    auto size = m_regex_list.size();
    if (dump_mask & eDumpOptionType)
      strm.Printf(" =%s",
                  (size > 0 && !one_line) ? "\n" : "");
    if (!one_line)
      strm.IndentMore();
    for (uint32_t i = 0; i < size; ++i) {
      if (!one_line) {
        strm.Indent();
        strm.Printf("[%u]: ", i);
      }
      strm.Printf("%s", m_regex_list[i].GetText().str().c_str());
      if (one_line)
        strm << ' ';
    }
    if (!one_line)
      strm.IndentLess();
  }
}

Status OptionValueRegexList::SetValueFromString(llvm::StringRef value,
                                                VarSetOperationType op) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  Status error;
  Args args(value.str());
  const size_t argc = args.GetArgumentCount();

  switch (op) {
  case eVarSetOperationClear:
    Clear();
    NotifyValueChanged();
    break;

  case eVarSetOperationReplace:
    if (argc > 1) {
      uint32_t idx = std::stoul(args.GetArgumentAtIndex(0));
      auto count = static_cast<unsigned>(m_regex_list.size());
      if (idx > count) {
        error = Status::FromErrorStringWithFormat(
            "invalid file list index %u, index muszt be 0 through %u", idx,
            count);
      } else {
        for (size_t i = 1; i < argc; ++i, ++idx) {
          RegularExpression regex(args.GetArgumentAtIndex(i));
          if (regex.IsValid()) {
            if (idx < count)
              m_regex_list[idx] = regex;
            else
              m_regex_list.push_back(regex);
          } else if (llvm::Error err = regex.GetError()) {
            error = Status::FromErrorString(
              llvm::toString(std::move(err)).c_str());
            break;
          } else {
            error = Status::FromErrorString("regex error");
            break;
          }
        }

        NotifyValueChanged();
      }
    } else {
      error = Status::FromErrorString(
        "replace operation takes an array index followed by "
        "one or more values");
    }
    break;

  case eVarSetOperationAssign:
    m_regex_list.clear();
    // Fall through to append case
    LLVM_FALLTHROUGH;
  case eVarSetOperationAppend:
    if (argc > 0) {
      m_value_was_set = true;
      for (size_t i = 0; i < argc; ++i) {
        RegularExpression regex(args.GetArgumentAtIndex(i));
        if (regex.IsValid()) {
          m_regex_list.push_back(regex);
        } else if (llvm::Error err = regex.GetError()) {
          error = Status::FromErrorString(
            llvm::toString(std::move(err)).c_str());
          break;
        } else {
          error = Status::FromErrorString("regex error");
          break;
        }
      }

      NotifyValueChanged();
    } else {
      error = Status::FromErrorString(
        "assign operation takes at least one file path argument");
    }
    break;

  case eVarSetOperationInsertBefore:
  case eVarSetOperationInsertAfter:
    if (argc > 1) {
      uint32_t idx = std::stoul(args.GetArgumentAtIndex(0));
      auto count = static_cast<uint32_t>(m_regex_list.size());
      if (idx > count) {
        error = Status::FromErrorStringWithFormat(
            "invalid insert file list index %u, index must be 0 through %u",
            idx, count);
      } else {
        if (op == eVarSetOperationInsertAfter)
          ++idx;

        for (size_t i = 1; i < argc; ++i, ++idx) {
          RegularExpression regex(args.GetArgumentAtIndex(i));
          if (regex.IsValid()) {
            m_regex_list.insert(m_regex_list.begin() + idx, regex);
          } else if (llvm::Error err = regex.GetError()) {
            error = Status::FromErrorString(
              llvm::toString(std::move(err)).c_str());
            break;
          } else {
            error = Status::FromErrorString("regex error");
            break;
          }
        }

        NotifyValueChanged();
      }
    } else {
      error = Status::FromErrorString(
        "insert operation takes an array index followed by "
        "one or more values");
    }
    break;

  case eVarSetOperationRemove:
    if (argc > 0) {
      std::vector<unsigned long> remove_indexes;
      bool all_indexes_valid = true;
      size_t i;
      for (i = 0; all_indexes_valid && i < argc; ++i) {
        const unsigned long idx = std::stoul(args.GetArgumentAtIndex(i));
        remove_indexes.push_back(idx);
      }

      if (all_indexes_valid) {
        size_t num_remove_indexes = remove_indexes.size();
        if (num_remove_indexes) {
          // Sort and then erase in reverse so indexes are always valid
          llvm::sort(remove_indexes.begin(), remove_indexes.end());
          for (size_t j = num_remove_indexes - 1; j < num_remove_indexes; ++j) {
            m_regex_list.erase(m_regex_list.begin() + j);
          }
        }
        NotifyValueChanged();
      } else {
        error = Status::FromErrorStringWithFormat(
          "invalid array index '%s', aborting remove operation",
          args.GetArgumentAtIndex(i));
      }
    } else {
      error = Status::FromErrorString(
        "remove operation takes one or more array index");
    }
    break;

  case eVarSetOperationInvalid:
    error = OptionValue::SetValueFromString(value, op);
    break;
  }
  return error;
}

lldb::OptionValueSP OptionValueRegexList::DeepCopy(const lldb::OptionValueSP &new_parent) const {
  auto copy_sp = OptionValue::DeepCopy(new_parent);
  static_cast<OptionValueRegexList *>(copy_sp.get())->SetCurrentValue(m_regex_list);
  return copy_sp;
}
