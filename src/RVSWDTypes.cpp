#include <cassert>

#include <algorithm>
#include <iterator>

#include <AnalyzerChannelData.h>
#include <AnalyzerHelpers.h>

#include "RVSWDAnalyzer.h"
#include "RVSWDTypes.h"
#include "RVSWDUtils.h"

const int TRAN_REQ_AND_ACK = 8 + 1 + 3;             // request/turnaround/ACK
const int TRAN_READ_LENGTH = TRAN_REQ_AND_ACK + 33; // previous + 32bit data + parity
const int TRAN_WRITE_LENGTH = TRAN_READ_LENGTH + 1; // previous + one bit for turnaround

S64 RVSWDBit::GetMinStartEnd() const
{
    S64 s = ( rising - low_start ) / 2;
    S64 e = ( low_end - falling ) / 2;
    return s < e ? s : e;
}

S64 RVSWDBit::GetStartSample() const
{
    return rising - GetMinStartEnd() + 1;
}

S64 RVSWDBit::GetEndSample() const
{
    return falling + GetMinStartEnd() - 1;
}

Frame RVSWDBit::MakeFrame()
{
    Frame f;

    f.mType = RVSWDFT_Bit;
    f.mFlags = 0;
    f.mStartingSampleInclusive = GetStartSample();
    f.mEndingSampleInclusive = GetEndSample();

    f.mData1 = state_rising == BIT_HIGH ? 1 : 0;
    f.mData2 = 0;

    return f;
}

// ********************************************************************************

std::string RVSWDRequestFrame::GetRegisterName() const
{
    return ::GetRegisterName( GetRegister() );
}

// ********************************************************************************

void RVSWDOperation::Clear()
{
    RnW = APnDP = parity_read = data_parity_ok = false;
    addr = parity_read = request_byte = ACK = data_parity = data = 0;
    reg = RVSWDR_undefined;

    bits.clear();
}

void RVSWDOperation::AddFrames( RVSWDAnalyzerResults* pResults )
{
    Frame f;

    assert( bits.size() >= TRAN_REQ_AND_ACK );

    // request
    RVSWDRequestFrame req;
    req.mStartingSampleInclusive = bits[ 0 ].GetStartSample();
    req.mEndingSampleInclusive = bits[ 7 ].GetEndSample();
    req.mFlags = ( IsRead() ? RVSWDRequestFrame::IS_READ : 0 ) | ( APnDP ? RVSWDRequestFrame::IS_ACCESS_PORT : 0 );
    req.SetRequestByte( request_byte );
    req.SetRegister( reg );
    req.mType = RVSWDFT_Request;
    pResults->AddFrame( req );

    // turnaround
    f = bits[ 8 ].MakeFrame();
    f.mType = RVSWDFT_Turnaround;
    pResults->AddFrame( f );

    // ack
    f.mStartingSampleInclusive = bits[ 9 ].GetStartSample();
    f.mEndingSampleInclusive = bits[ 11 ].GetEndSample();
    f.mType = RVSWDFT_ACK;
    f.mData1 = ACK;
    pResults->AddFrame( f );

    if( bits.size() < TRAN_READ_LENGTH )
        return;

    // turnaround
    std::vector<RVSWDBit>::iterator bi( bits.begin() + 12 );
    if( !IsRead() )
    {
        f = bits[ 12 ].MakeFrame();
        f.mType = RVSWDFT_Turnaround;
        pResults->AddFrame( f );
        bi++;
    }

    // data
    f = bi->MakeFrame();
    f.mEndingSampleInclusive = bi[ 31 ].GetEndSample();
    f.mType = RVSWDFT_WData;
    f.mData1 = data;
    f.mData2 = reg;
    pResults->AddFrame( f );

    // data parity
    f = bi[ 32 ].MakeFrame();
    f.mType = RVSWDFT_DataParity;
    f.mData1 = data_parity;
    f.mData2 = data_parity_ok ? 1 : 0;
    pResults->AddFrame( f );

    bi += 33;

    // do we have trailing bits?
    if( bi < bits.end() )
    {
        f.mStartingSampleInclusive = bi->GetStartSample();
        f.mEndingSampleInclusive = bits.back().GetEndSample();
        f.mType = RVSWDFT_TrailingBits;

        f.mFlags = 0;
        f.mData1 = 0;
        f.mData2 = 0;

        pResults->AddFrame( f );
    }
}

void RVSWDOperation::AddMarkers( RVSWDAnalyzerResults* pResults )
{
    for( std::vector<RVSWDBit>::iterator bi( bits.begin() ); bi != bits.end(); bi++ )
    {
        int ndx = bi - bits.begin();

        // turnaround
        if( ndx == 8 || ndx == 12 && !IsRead() )
            pResults->AddMarker( ( bi->falling + bi->rising ) / 2, AnalyzerResults::X, pResults->GetSettings()->mCLK );

        // write
        else if( ndx < 8 || ndx > 12 && !IsRead() )
            pResults->AddMarker( bi->falling, bi->state_falling == BIT_HIGH ? AnalyzerResults::One : AnalyzerResults::Zero,
                                 pResults->GetSettings()->mCLK );
        // read
        else
            pResults->AddMarker( bi->rising, bi->state_rising == BIT_HIGH ? AnalyzerResults::One : AnalyzerResults::Zero,
                                 pResults->GetSettings()->mCLK );
    }
}

void RVSWDOperation::SetRegister( U32 select_reg )
{
    if( APnDP ) // AccessPort or DebugPort?
    {
        U8 apbanksel = U8( select_reg & 0xf0 );
        U8 apreg = apbanksel | addr;

        switch( apreg )
        {
        case 0x00:
            reg = RVSWDR_AP_CSW;
            break;
        case 0x04:
            reg = RVSWDR_AP_TAR;
            break;
        case 0x0C:
            reg = RVSWDR_AP_DRW;
            break;
        case 0x10:
            reg = RVSWDR_AP_BD0;
            break;
        case 0x14:
            reg = RVSWDR_AP_BD1;
            break;
        case 0x18:
            reg = RVSWDR_AP_BD2;
            break;
        case 0x1C:
            reg = RVSWDR_AP_BD3;
            break;
        case 0xF4:
            reg = RVSWDR_AP_CFG;
            break;
        case 0xF8:
            reg = RVSWDR_AP_BASE;
            break;
        case 0xFC:
            reg = RVSWDR_AP_IDR;
            break;
        default:
            reg = RVSWDR_AP_RAZ_WI;
            break;
        }
    }
    else
    {
        switch( addr )
        {
        case 0x0:
            reg = RnW ? RVSWDR_DP_IDCODE : RVSWDR_DP_ABORT;
            break;
        case 0x4:
            reg = ( select_reg & 1 ) != 0 ? RVSWDR_DP_WCR : RVSWDR_DP_CTRL_STAT;
            break;
        case 0x8:
            reg = RnW ? RVSWDR_DP_RESEND : RVSWDR_DP_SELECT;
            break;
        case 0xC:
            reg = RnW ? RVSWDR_DP_RDBUFF : RVSWDR_DP_ROUTESEL;
            break;
        }
    }
}

// ********************************************************************************

void RVSWDLineReset::AddFrames( AnalyzerResults* pResults )
{
    Frame f;

    // line reset
    f.mStartingSampleInclusive = bits.front().GetStartSample();
    f.mEndingSampleInclusive = bits.back().GetEndSample();
    f.mType = RVSWDFT_LineReset;
    f.mData1 = bits.size();
    pResults->AddFrame( f );
}

// ********************************************************************************

RVSWDParser::RVSWDParser() : mDIO( 0 ), mCLK( 0 )
{
}

void RVSWDParser::Setup( AnalyzerChannelData* pDIO, AnalyzerChannelData* pCLK, RVSWDAnalyzer* pAnalyzer )
{
    mDIO = pDIO;
    mCLK = pCLK;

    mAnalyzer = pAnalyzer;

    // skip the CLK high
    if( mCLK->GetBitState() == BIT_HIGH )
    {
        mCLK->AdvanceToNextEdge();
        mDIO->AdvanceToAbsPosition( mCLK->GetSampleNumber() );
    }
}

RVSWDBit RVSWDParser::ParseBit()
{
    RVSWDBit rbit;

    assert( mCLK->GetBitState() == BIT_LOW );

    rbit.low_start = mCLK->GetSampleNumber();

    // sample the rising edge 1 sample before the the actual
    mCLK->AdvanceToAbsPosition( mCLK->GetSampleOfNextEdge() - 1 );
    mDIO->AdvanceToAbsPosition( mCLK->GetSampleNumber() );
    rbit.rising = mCLK->GetSampleNumber();
    rbit.state_rising = mDIO->GetBitState();
    mCLK->AdvanceToNextEdge();
    mDIO->AdvanceToAbsPosition( mCLK->GetSampleNumber() );

    /*
    // go to the rising egde
    mCLK->AdvanceToNextEdge();
    mDIO->AdvanceToAbsPosition(mCLK->GetSampleNumber());

    rbit.rising = mCLK->GetSampleNumber();
    rbit.state_rising = mDIO->GetBitState();
    */

    // go to the falling edge
    mCLK->AdvanceToNextEdge();
    mDIO->AdvanceToAbsPosition( mCLK->GetSampleNumber() );

    rbit.falling = mCLK->GetSampleNumber();
    rbit.state_falling = mDIO->GetBitState();

    rbit.low_end = mCLK->GetSampleOfNextEdge();

    return rbit;
}

void RVSWDParser::BufferBits( size_t num_bits )
{
    while( mBitsBuffer.size() < num_bits )
        mBitsBuffer.push_back( ParseBit() );
}

RVSWDBit RVSWDParser::PopFrontBit()
{
    assert( !mBitsBuffer.empty() );

    RVSWDBit ret_val( mBitsBuffer.front() );

    // shift the elements by 1
    std::copy( mBitsBuffer.begin() + 1, mBitsBuffer.end(), mBitsBuffer.begin() );
    mBitsBuffer.pop_back();

    return ret_val;
}

bool RVSWDParser::IsOperation( RVSWDOperation& tran )
{
    tran.Clear();

    // read enough bits so that we don't have to worry of subscripts out of range
    BufferBits( TRAN_REQ_AND_ACK );

    // turn the bits into a byte
    tran.request_byte = 0;
    for( size_t cnt = 0; cnt < 8; ++cnt )
    {
        tran.request_byte >>= 1;
        tran.request_byte |= ( mBitsBuffer[ cnt ].IsHigh() ? 0x80 : 0 );
    }

    // are the request's constant bits (start, stop & park) wrong?
    if( ( tran.request_byte & 0xC1 ) != 0x81 )
        return false;

    // get the indivitual bits
    tran.APnDP = ( tran.request_byte & 0x02 ) != 0;                   //(mBitsBuffer[1].state_falling == BIT_HIGH);
    tran.RnW = ( tran.request_byte & 0x04 ) != 0;                     //(mBitsBuffer[2].state_falling == BIT_HIGH);
    tran.addr = ( tran.request_byte & 0x18 ) >> 1;                    //(mBitsBuffer[3].state_falling == BIT_HIGH ? (1<<2) : 0)
                                                                      //| (mBitsBuffer[4].state_falling == BIT_HIGH ? (1<<3) : 0);
    tran.parity_read = ( ( tran.request_byte & 0x20 ) != 0 ? 1 : 0 ); //(mBitsBuffer[5].state_falling == BIT_HIGH ? 1 : 0);

    // check parity
    int check = ( mBitsBuffer[ 1 ].state_rising == BIT_HIGH ? 1 : 0 ) + ( mBitsBuffer[ 2 ].state_rising == BIT_HIGH ? 1 : 0 ) +
                ( mBitsBuffer[ 3 ].state_rising == BIT_HIGH ? 1 : 0 ) + ( mBitsBuffer[ 4 ].state_rising == BIT_HIGH ? 1 : 0 );

    if( tran.parity_read != ( check & 1 ) )
        return false;

    // Set the actual register in this operation based on the data from the request
    // and the previous select register state.
    tran.SetRegister( mSelectRegister );

    // get the ACK value
    tran.ACK = ( mBitsBuffer[ 9 ].state_rising == BIT_HIGH ? 1 : 0 ) + ( mBitsBuffer[ 10 ].state_rising == BIT_HIGH ? 2 : 0 ) +
               ( mBitsBuffer[ 11 ].state_rising == BIT_HIGH ? 4 : 0 );

    // we're only handling OK, WAIT and FAULT responses
    if( tran.ACK == ACK_WAIT || tran.ACK == ACK_FAULT )
    {
        // copy this operation's bits
        tran.bits.clear();
        std::copy( mBitsBuffer.begin(), mBitsBuffer.begin() + TRAN_REQ_AND_ACK, std::back_inserter( tran.bits ) );

        // consume this operation's bits
        mBitsBuffer.erase( mBitsBuffer.begin(), mBitsBuffer.begin() + TRAN_REQ_AND_ACK );

        return true;
    }

    if( tran.ACK != ACK_OK )
        return false;

    BufferBits( TRAN_READ_LENGTH );

    // turnaround if write operation
    bool read_rising = true;
    std::vector<RVSWDBit>::iterator bi( mBitsBuffer.begin() + 12 );
    if( !tran.IsRead() )
    {
        BufferBits( TRAN_WRITE_LENGTH );
        ++bi;
        // !!! read_rising = false;
    }

    // read the data
    tran.data = 0;
    check = 0;
    size_t ndx;
    for( ndx = 0; ndx < 32; ndx++ )
    {
        tran.data >>= 1;

        if( bi[ ndx ].IsHigh( read_rising ) )
        {
            tran.data |= 0x80000000;
            ++check;
        }
    }

    // data parity
    tran.data_parity = bi[ ndx ].IsHigh( read_rising ) ? 1 : 0;

    tran.data_parity_ok = ( tran.data_parity == ( check & 1 ) );

    if( !tran.data_parity_ok )
        return false;

    // if this is a SELECT register write, remember the value
    if( tran.reg == RVSWDR_DP_SELECT && !tran.RnW )
        mSelectRegister = tran.data;

    // buffered trailing zeros
    ++ndx;
    bool all_zeros = true;
    while( bi + ndx < mBitsBuffer.end() )
    {
        if( bi[ ndx ].IsHigh( read_rising ) )
        {
            all_zeros = false;
            break;
        }

        ++ndx;
    }

    // if we haven't seen a high bit carry on until we do
    if( all_zeros )
    {
        // read the remaining zero bits
        RVSWDBit bit;
        while( 1 )
        {
            bit = ParseBit();
            if( bit.IsHigh( read_rising ) )
                break;

            mBitsBuffer.push_back( bit );
        }

        // give the bits to the tran object
        tran.bits = mBitsBuffer;
        mBitsBuffer.clear();

        // keep the high bit because that one is probably next operation's start bit
        mBitsBuffer.push_back( bit );
    }
    else
    {
        // copy this operation's bits
        tran.bits.clear();
        std::copy( mBitsBuffer.begin(), mBitsBuffer.begin() + ndx, std::back_inserter( tran.bits ) );

        // remove this operation's bits from the buffer
        mBitsBuffer.erase( mBitsBuffer.begin(), mBitsBuffer.begin() + ndx );
    }

    return true;
}

bool RVSWDParser::IsLineReset( RVSWDLineReset& reset )
{
    reset.Clear();

    // we need at least 50 bits with a value of 1
    for( size_t cnt = 0; cnt < 50; cnt++ )
    {
        if( cnt >= mBitsBuffer.size() )
            mBitsBuffer.push_back( ParseBit() );

        // we can't have a low bit
        if( !mBitsBuffer[ cnt ].IsHigh() )
            return false;
    }

    RVSWDBit bit;
    while( true )
    {
        bit = ParseBit();
        if( !bit.IsHigh() )
            break;

        mBitsBuffer.push_back( bit );
    }

    // give the bits to the reset object
    reset.bits = mBitsBuffer;
    mBitsBuffer.clear();

    // keep the high bit because that one is probably next operation's start bit
    mBitsBuffer.push_back( bit );

    return true;
}
