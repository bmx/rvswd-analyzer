#include <AnalyzerHelpers.h>

#include "RVSWDAnalyzerSettings.h"
#include "RVSWDAnalyzerResults.h"
#include "RVSWDTypes.h"

RVSWDAnalyzerSettings::RVSWDAnalyzerSettings() : mDIO( UNDEFINED_CHANNEL ), mCLK( UNDEFINED_CHANNEL )
{
    // init the interface
    mDIOInterface.SetTitleAndTooltip( "DIO", "DIO" );
    mDIOInterface.SetChannel( mDIO );

    mCLKInterface.SetTitleAndTooltip( "CLK", "CLK" );
    mCLKInterface.SetChannel( mCLK );

    // add the interface
    AddInterface( &mDIOInterface );
    AddInterface( &mCLKInterface );

    // describe export
    AddExportOption( 0, "Export as text file" );
    AddExportExtension( 0, "text", "txt" );

    ClearChannels();

    AddChannel( mDIO, "DIO", false );
    AddChannel( mCLK, "CLK", false );
}

RVSWDAnalyzerSettings::~RVSWDAnalyzerSettings()
{
}

bool RVSWDAnalyzerSettings::SetSettingsFromInterfaces()
{
    if( mDIOInterface.GetChannel() == UNDEFINED_CHANNEL )
    {
        SetErrorText( "Please select an input for the channel 1." );
        return false;
    }

    if( mCLKInterface.GetChannel() == UNDEFINED_CHANNEL )
    {
        SetErrorText( "Please select an input for the channel 2." );
        return false;
    }

    mDIO = mDIOInterface.GetChannel();
    mCLK = mCLKInterface.GetChannel();

    if( mDIO == mCLK )
    {
        SetErrorText( "Please select different inputs for the channels." );
        return false;
    }

    ClearChannels();

    AddChannel( mDIO, "DIO", true );
    AddChannel( mCLK, "CLK", true );

    return true;
}

void RVSWDAnalyzerSettings::UpdateInterfacesFromSettings()
{
    mDIOInterface.SetChannel( mDIO );
    mCLKInterface.SetChannel( mCLK );
}

void RVSWDAnalyzerSettings::LoadSettings( const char* settings )
{
    SimpleArchive text_archive;
    text_archive.SetString( settings );

    text_archive >> mDIO;
    text_archive >> mCLK;

    ClearChannels();

    AddChannel( mDIO, "DIO", true );
    AddChannel( mCLK, "CLK", true );

    UpdateInterfacesFromSettings();
}

const char* RVSWDAnalyzerSettings::SaveSettings()
{
    SimpleArchive text_archive;

    text_archive << mDIO;
    text_archive << mCLK;

    return SetReturnString( text_archive.GetString() );
}
