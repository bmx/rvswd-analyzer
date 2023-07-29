#ifndef RVSWD_TYPES_H
#define RVSWD_TYPES_H

#include <LogicPublicTypes.h>

#include "RVSWDAnalyzerResults.h"

// the possible frame types
enum RVSWDFrameTypes
{
    RVSWDFT_Error,
    RVSWDFT_Bit,

    RVSWDFT_LineReset,

    RVSWDFT_Request,
    RVSWDFT_Turnaround,
    RVSWDFT_ACK,
    RVSWDFT_WData,
    RVSWDFT_DataParity,
    RVSWDFT_TrailingBits,
};

// the DebugPort and AccessPort registers as defined by SWD
enum RVSWDRegisters
{
    RVSWDR_undefined,

    // DP
    RVSWDR_DP_IDCODE,
    RVSWDR_DP_ABORT,
    RVSWDR_DP_CTRL_STAT,
    RVSWDR_DP_WCR,
    RVSWDR_DP_RESEND,
    RVSWDR_DP_SELECT,
    RVSWDR_DP_RDBUFF,
    RVSWDR_DP_ROUTESEL,

    // AP
    RVSWDR_AP_CSW,
    RVSWDR_AP_TAR,
    RVSWDR_AP_DRW,
    RVSWDR_AP_BD0,
    RVSWDR_AP_BD1,
    RVSWDR_AP_BD2,
    RVSWDR_AP_BD3,
    RVSWDR_AP_CFG,
    RVSWDR_AP_BASE,
    RVSWDR_AP_RAZ_WI,
    RVSWDR_AP_IDR,
};

// some ACK values
enum RVSWDACK
{
    ACK_OK = 1,
    ACK_WAIT = 2,
    ACK_FAULT = 4,
};

// this is the basic token of the analyzer
// objects of this type are buffered in SWDOperation
struct RVSWDBit
{
    BitState state_rising;
    BitState state_falling;

    S64 low_start;
    S64 rising;
    S64 falling;
    S64 low_end;

    bool IsHigh( bool is_rising = true ) const
    {
        return ( is_rising ? state_rising : state_falling ) == BIT_HIGH;
    }

    S64 GetMinStartEnd() const;
    S64 GetStartSample() const;
    S64 GetEndSample() const;

    Frame MakeFrame();
};

// this object contains data about one SWD operation as described in section 5.3
// of the ARM Debug Interface v5 Architecture Specification
struct RVSWDOperation
{
    // request
    bool APnDP;
    bool RnW;
    U8 addr; // A[2..3]

    U8 parity_read;

    U8 request_byte; // the entire request byte

    // acknowledge
    U8 ACK;

    // data
    U32 data;
    U8 data_parity;
    bool data_parity_ok;

    std::vector<RVSWDBit> bits;

    // DebugPort or AccessPort register that this operation is reading/writing
    RVSWDRegisters reg;

    void Clear();
    void AddFrames( RVSWDAnalyzerResults* pResults );
    void AddMarkers( RVSWDAnalyzerResults* pResults );
    void SetRegister( U32 select_reg );

    bool IsRead()
    {
        return RnW;
    }
};

struct RVSWDLineReset
{
    std::vector<RVSWDBit> bits;

    void Clear()
    {
        bits.clear();
    }

    void AddFrames( AnalyzerResults* pResults );
};

struct RVSWDRequestFrame : public Frame
{
    // mData1 contains addr, mData2 contains the register enum

    // mFlag
    enum
    {
        IS_READ = ( 1 << 0 ),
        IS_ACCESS_PORT = ( 1 << 1 ),
    };

    void SetRequestByte( U8 request_byte )
    {
        mData1 = request_byte;
    }

    U8 GetAddr() const
    {
        return ( U8 )( ( mData1 >> 1 ) & 0xc );
    }
    bool IsRead() const
    {
        return ( mFlags & IS_READ ) != 0;
    }
    bool IsAccessPort() const
    {
        return ( mFlags & IS_ACCESS_PORT ) != 0;
    }
    bool IsDebugPort() const
    {
        return !IsAccessPort();
    }

    void SetRegister( RVSWDRegisters reg )
    {
        mData2 = reg;
    }
    RVSWDRegisters GetRegister() const
    {
        return RVSWDRegisters( mData2 );
    }
    std::string GetRegisterName() const;
};

class RVSWDAnalyzer;

// This object parses and buffers the bits of the SWD stream.
// IsOperation and IsLineReset return true if the subsequent bits in
// the stream are a valid operation or line reset.
class RVSWDParser
{
  private:
    AnalyzerChannelData* mDIO;
    AnalyzerChannelData* mCLK;

    RVSWDAnalyzer* mAnalyzer;

    std::vector<RVSWDBit> mBitsBuffer;
    U32 mSelectRegister;

    RVSWDBit ParseBit();
    void BufferBits( size_t num_bits );

  public:
    RVSWDParser();

    void Setup( AnalyzerChannelData* pDIO, AnalyzerChannelData* pCLK, RVSWDAnalyzer* pAnalyzer );

    void Clear()
    {
        mBitsBuffer.clear();
        mSelectRegister = 0;
    }

    bool IsOperation( RVSWDOperation& tran );
    bool IsLineReset( RVSWDLineReset& reset );

    RVSWDBit PopFrontBit();
};

#endif // RVSWD_TYPES_H
