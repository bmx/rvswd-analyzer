#ifndef RVSWD_ANALYZER_SETTINGS_H
#define RVSWD_ANALYZER_SETTINGS_H

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

#include "RVSWDTypes.h"

class RVSWDAnalyzerSettings : public AnalyzerSettings
{
  public:
    RVSWDAnalyzerSettings();
    virtual ~RVSWDAnalyzerSettings();

    virtual bool SetSettingsFromInterfaces();
    virtual void LoadSettings( const char* settings );
    virtual const char* SaveSettings();

    void UpdateInterfacesFromSettings();

    Channel mDIO;
    Channel mCLK;

  protected:
    AnalyzerSettingInterfaceChannel mDIOInterface;
    AnalyzerSettingInterfaceChannel mCLKInterface;
};

#endif // RVSWD_ANALYZER_SETTINGS_H