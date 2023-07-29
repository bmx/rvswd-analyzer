#include <vector>
#include <algorithm>

#include <AnalyzerChannelData.h>

#include "RVSWDAnalyzer.h"
#include "RVSWDAnalyzerSettings.h"
#include "RVSWDUtils.h"

RVSWDAnalyzer::RVSWDAnalyzer() : mSimulationInitilized( false )
{
    SetAnalyzerSettings( &mSettings );
}

RVSWDAnalyzer::~RVSWDAnalyzer()
{
    KillThread();
}

void RVSWDAnalyzer::SetupResults()
{
    // reset the results
    mResults.reset( new RVSWDAnalyzerResults( this, &mSettings ) );
    SetAnalyzerResults( mResults.get() );

    // set which channels will carry bubbles
    mResults->AddChannelBubblesWillAppearOn( mSettings.mDIO );
    mResults->AddChannelBubblesWillAppearOn( mSettings.mCLK );
}

void RVSWDAnalyzer::WorkerThread()
{
    // SetupResults();
    // get the channel pointers
    mDIO = GetAnalyzerChannelData( mSettings.mDIO );
    mCLK = GetAnalyzerChannelData( mSettings.mCLK );

    mRVSWDParser.Setup( mDIO, mCLK, this );

    // these are our three objects that SWDParser will fill with data
    // on calls to IsOperation or IsLineReset
    RVSWDOperation tran;
    RVSWDLineReset reset;
    RVSWDBit error_bit;

    mRVSWDParser.Clear();

    // For every new bit the parser extracts from the stream,
    // ask if this can be a valid operation or line reset.
    // A valid operation will have the constant part of the request correctly set,
    // and also the parity bits will be correct.
    // A valid line reset has at least 50 high bits in succession.
    for( ;; )
    {
        if( mRVSWDParser.IsOperation( tran ) )
        {
            tran.AddFrames( mResults.get() );
            tran.AddMarkers( mResults.get() );

            mResults->CommitResults();
        }
        else if( mRVSWDParser.IsLineReset( reset ) )
        {
            reset.AddFrames( mResults.get() );

            mResults->CommitResults();
        }
        else
        {
            // This is neither a valid transaction nor a valid reset,
            // so remove the first bit and try again.
            // We're dropping the error bit into oblivion.
            error_bit = mRVSWDParser.PopFrontBit();
        }

        ReportProgress( mDIO->GetSampleNumber() );
    }
}

bool RVSWDAnalyzer::NeedsRerun()
{
    return false;
}

U32 RVSWDAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate,
                                         SimulationChannelDescriptor** simulation_channels )
{
    if( !mSimulationInitilized )
    {
        mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), &mSettings );
        mSimulationInitilized = true;
    }

    return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 RVSWDAnalyzer::GetMinimumSampleRateHz()
{
    // this 1MHz limit is a little arbitrary, since the specs don't say much about the
    // valid frequency range a SWD stream should be in.
    return 1000000;
}

const char* RVSWDAnalyzer::GetAnalyzerName() const
{
    return ::GetAnalyzerName();
}

const char* GetAnalyzerName()
{
    return "RVSWD";
}

Analyzer* CreateAnalyzer()
{
    return new RVSWDAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
    delete analyzer;
}
