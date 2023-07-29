#ifndef RVSWD_SIMULATION_DATA_GENERATOR_H
#define RVSWD_SIMULATION_DATA_GENERATOR_H

#include <AnalyzerHelpers.h>

#include "RVSWDTypes.h"

class RVSWDAnalyzerSettings;

class RVSWDSimulationDataGenerator
{
  public:
    RVSWDSimulationDataGenerator();
    ~RVSWDSimulationDataGenerator();

    void Initialize( U32 simulation_sample_rate, RVSWDAnalyzerSettings* settings );
    U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );

  protected:
    RVSWDAnalyzerSettings* mSettings;
    U32 mSimulationSampleRateHz;
    U32 mSimulCnt;

    void AdvanceAllBySec( double sec )
    {
        mRVSWDSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByTimeS( sec ) );
    }

    // read and write in this context is a bit read or written from the perspective of the host
    void OutputWriteBit( BitState state );
    void OutputReadBit( BitState first_half, BitState second_half );

    void OutputTurnaround( BitState state );
    bool OutputRequest( U8 req, U8 ack, BitState first_data_bit );
    void OutputData( U32 data, bool is_write );
    void OutputLineReset();

  protected:
    ClockGenerator mClockGenerator;

    SimulationChannelDescriptorGroup mRVSWDSimulationChannels;
    SimulationChannelDescriptor* mDIO;
    SimulationChannelDescriptor* mCLK;
};

#endif // RVSWD_SIMULATION_DATA_GENERATOR_H
