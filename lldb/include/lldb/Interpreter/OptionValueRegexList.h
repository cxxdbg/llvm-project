//===-- OptionValueRegex.h --------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_OptionValueRegexList_h_
#define liblldb_OptionValueRegexList_h_

#include <mutex>

#include "lldb/Interpreter/OptionValue.h"
#include "lldb/Utility/RegularExpression.h"

namespace lldb_private {

class OptionValueRegexList : public Cloneable<OptionValueRegexList, OptionValue> {
public:
  OptionValueRegexList(const std::vector<RegularExpression> & list = {})
      : m_regex_list(list) {}

  OptionValueRegexList(const OptionValueRegexList & other)
      : m_regex_list(other.m_regex_list) {}

  ~OptionValueRegexList() override = default;

  // Virtual subclass pure virtual overrides

  OptionValue::Type GetType() const override { return eTypeRegexList; }

  void DumpValue(const ExecutionContext *exe_ctx, Stream &strm,
                 uint32_t dump_mask) override;

  Status
  SetValueFromString(llvm::StringRef value,
                     VarSetOperationType op = eVarSetOperationAssign) override;
  Status
  SetValueFromString(const char *,
                     VarSetOperationType = eVarSetOperationAssign) = delete;

  void Clear() override {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_regex_list.clear();
  }

  lldb::OptionValueSP DeepCopy(const lldb::OptionValueSP &new_parent) const override;

  bool IsAggregateValue() const override { return true; }

  // Subclass specific functions

  const std::vector<RegularExpression> *GetCurrentValue() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return &m_regex_list;
  }

  void SetCurrentValue(const std::vector<RegularExpression> &value) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_regex_list = value;
  }

  void AppendCurrentValue(const RegularExpression &value) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_regex_list.push_back(value);
  }

  bool IsValid() const { return true; }

protected:
  mutable std::recursive_mutex m_mutex;
  std::vector<RegularExpression> m_regex_list;
};

} // namespace lldb_private

#endif // liblldb_OptionValueRegexList_h_
