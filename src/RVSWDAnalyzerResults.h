#ifndef RVSWD_ANALYZER_RESULTS_H
#define RVSWD_ANALYZER_RESULTS_H

#include <AnalyzerResults.h>

class RVSWDAnalyzer;
class RVSWDAnalyzerSettings;

class RVSWDAnalyzerResults : public AnalyzerResults
{
  public:
    RVSWDAnalyzerResults( RVSWDAnalyzer* analyzer, RVSWDAnalyzerSettings* settings );
    virtual ~RVSWDAnalyzerResults();

    virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
    virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

    virtual void GenerateFrameTabularText( U64 frame_index, DisplayBase display_base );
    virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
    virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

    double GetSampleTime( S64 sample ) const;
    std::string GetSampleTimeStr( S64 sample ) const;

    RVSWDAnalyzerSettings* GetSettings()
    {
        return mSettings;
    }

  protected: // functions
    void GetBubbleText( const Frame& f, DisplayBase display_base, std::vector<std::string>& results );

  protected: // vars
    RVSWDAnalyzerSettings* mSettings;
    RVSWDAnalyzer* mAnalyzer;
};

#endif // RVSWD_ANALYZER_RESULTS_H