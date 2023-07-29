#ifndef RVSWD_ANALYZER_H
#define RVSWD_ANALYZER_H

#include <Analyzer.h>

#include "RVSWDAnalyzerSettings.h"
#include "RVSWDAnalyzerResults.h"
#include "RVSWDSimulationDataGenerator.h"

#include "RVSWDTypes.h"

class RVSWDAnalyzer : public Analyzer2
{
  public:
    RVSWDAnalyzer();
    virtual ~RVSWDAnalyzer();
    virtual void SetupResults();
    virtual void WorkerThread();

    virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
    virtual U32 GetMinimumSampleRateHz();

    virtual const char* GetAnalyzerName() const;
    virtual bool NeedsRerun();

  protected: // vars
    RVSWDAnalyzerSettings mSettings;
    std::unique_ptr<RVSWDAnalyzerResults> mResults;

    AnalyzerChannelData* mDIO;
    AnalyzerChannelData* mCLK;

    RVSWDSimulationDataGenerator mSimulationDataGenerator;

    RVSWDParser mRVSWDParser;

    bool mSimulationInitilized;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif // RVSWD_ANALYZER_H
