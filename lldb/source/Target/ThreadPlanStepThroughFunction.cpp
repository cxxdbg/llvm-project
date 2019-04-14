
#include "lldb/Target/ThreadPlanStepThroughFunction.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/Log.h"
#include "lldb/Utility/Stream.h"
#include "lldb/Symbol/Function.h"
#include "lldb/Symbol/Symbol.h"
#include "lldb/Target/RegisterContext.h"


using namespace lldb;
using namespace lldb_private;


void ThreadPlanStepThroughFunction::GetDescription(Stream *s, lldb::DescriptionLevel level)
{
    if (level == lldb::eDescriptionLevelBrief) {
        s->Printf ("Step through function");
        return;
    }

    s->PutCString ("Stepping through function");
    // TODO
}


bool ThreadPlanStepThroughFunction::ShouldStop(Event *event_ptr)
{
    Log *log = GetLog(LLDBLog::Step);

    if (log)
    {
        StreamString s;
        DumpAddress(
            s.AsRawOstream(), GetThread().GetRegisterContext()->GetPC(),
            GetThread().CalculateTarget()->GetArchitecture().GetAddressByteSize());
        LLDB_LOGF(log, "ThreadPlanStepThroughFunction reached %s.", s.GetData());
    }

    if (IsPlanComplete())
        return true;

    // if we are not in function we are stepping through then we should stop
    // and plan execution is complete
    if (!InRange()) {
        SetPlanComplete();
        return true;
    }

    // adding breakpoint at the next branch instruction and continue execution
    SetNextBranchBreakpoint();
    return false;
}


bool ThreadPlanStepThroughFunction::StopOthers()
{
    // ThreadPlanStepRange::StopOthers() returns parameter passed to constructor
    return ThreadPlanStepRange::StopOthers();
}


bool ThreadPlanStepThroughFunction::MischiefManaged()
{
    // If we reached here then ShouldStop returned true. After stop the plan
    // is always completed and should be removed from plan stack.
    // Default ThreadPlan::MischiefManaged always sets complete flag and returns true.
    // We need override it because ThreadPlanStepRange performs things we don't need.
    return ThreadPlan::MischiefManaged();
}


void ThreadPlanStepThroughFunction::DidPush()
{
    // ThreadPlanStepRange::DidPush sets breakpoint to the next branch instruction
    ThreadPlanStepRange::DidPush();
}



// Returns range of all instructions in function
AddressRange all_function_range(const SymbolContext &ctx)
{
    if (ctx.function != nullptr) {
        return ctx.function->GetAddressRange();
    } else if (ctx.symbol != nullptr) {
        return AddressRange(ctx.symbol->GetAddressRef(), ctx.symbol->GetByteSize());
    }

    return AddressRange();
}


ThreadPlanStepThroughFunction::ThreadPlanStepThroughFunction(Thread &thread,
                                                             const SymbolContext &addr_context,
                                                             lldb::RunMode stop_others):
ThreadPlanStepRange(ThreadPlan::eKindStepThroughFunction,
                    "Step through function",
                    thread,
                    all_function_range(addr_context),
                    addr_context,
                    stop_others)
{
}


bool ThreadPlanStepThroughFunction::DoPlanExplainsStop(Event *event_ptr)
{
    StopInfoSP stop_info = GetPrivateStopInfo();

    // TODO: can this happen?
    if (!stop_info)
        return false;

    StopReason reason = stop_info->GetStopReason();
    if (reason == eStopReasonTrace) {
        // we performed single step at branch instruction
        return true;
    } else if (reason == eStopReasonBreakpoint) {
        if (NextRangeBreakpointExplainsStop(stop_info)) {
            // stop at breakpoint at next branch instruction
            return true;
        } else {
            // stop at other breakpoint
            return false;
        }
    }

    // unknown stop reason
    return false;
}


bool ThreadPlanStepThroughFunction::DoWillResume(lldb::StateType resume_state, bool current_plan)
{
    // we always should resume
    return true;
}


lldb::StateType ThreadPlanStepThroughFunction::GetPlanRunState()
{
    // ThreadPlanStepRange::GetPlanRunState() return state dependning on was
    // breakpoint at the next branch instruction added or not
    return ThreadPlanStepRange::GetPlanRunState();
}
