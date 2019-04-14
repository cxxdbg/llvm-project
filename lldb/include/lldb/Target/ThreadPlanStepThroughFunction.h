
#ifndef liblldb_ThreadPlanStepThroughFunction_h_
#define liblldb_ThreadPlanStepThroughFunction_h_


#include "lldb/Target/ThreadPlanStepRange.h"


namespace lldb_private {

class ThreadPlanStepThroughFunction: public ThreadPlanStepRange {
public:
    void GetDescription(Stream *s, lldb::DescriptionLevel level) override;
    bool ShouldStop(Event *event_ptr) override;
    bool StopOthers() override;
    bool MischiefManaged() override;
    void DidPush() override;

protected:
    ThreadPlanStepThroughFunction(Thread &thread,
                                  const SymbolContext &addr_context,
                                  lldb::RunMode stop_others);

    bool DoPlanExplainsStop(Event *event_ptr) override;
    bool DoWillResume(lldb::StateType resume_state, bool current_plan) override;
    lldb::StateType GetPlanRunState() override;

private:
    ThreadPlanStepThroughFunction(const ThreadPlanStepThroughFunction &) = delete;

    friend lldb::ThreadPlanSP
    Thread::QueueThreadPlanForStepThroughFunction(const SymbolContext &addr_context,
                                                  lldb::RunMode stop_others,
                                                  bool abort_other_plans);
};

} // namespace lldb_private

#endif // liblldb_ThreadPlanStepThroughFunction_h_
