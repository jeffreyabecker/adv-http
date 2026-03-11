#pragma once


namespace HttpServerAdvanced
{


    enum class PipelineHandleClientResult
    {
        Idle,
        Processing,
        Completed,
        ClientDisconnected,
        NoPipelineHandlerPresent,
        ErroredUnrecoverably,
        TimedOutUnrecoverably,
        Aborted
    };

    bool inline isPipelineHandleClientResultFinal(PipelineHandleClientResult result)
    {
        switch (result)
        {
        case PipelineHandleClientResult::Completed:
        case PipelineHandleClientResult::ClientDisconnected:
        case PipelineHandleClientResult::ErroredUnrecoverably:
        case PipelineHandleClientResult::TimedOutUnrecoverably:
        case PipelineHandleClientResult::Aborted:
        case PipelineHandleClientResult::NoPipelineHandlerPresent:
            return true;
        case PipelineHandleClientResult::Idle:
        case PipelineHandleClientResult::Processing:
        default:
            return false;
        }
    }
}
