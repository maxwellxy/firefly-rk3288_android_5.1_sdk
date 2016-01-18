///============================================================================
/// Copyright 2011 Broadcom Corporation
///
/// This program is free software; you can redistribute it and/or modify
/// it under the following terms: 
///
/// Redistribution and use in source and binary forms, with or without 
/// modification, are permitted provided that the following conditions are met:
///  Redistributions of source code must retain the above copyright notice, this
///  list of conditions and the following disclaimer.
///  Redistributions in binary form must reproduce the above copyright notice, 
///  this list of conditions and the following disclaimer in the documentation 
///  and/or other materials provided with the distribution.
///  Neither the name of Broadcom nor the names of its contributors may be used 
///  to endorse or promote products derived from this software without specific 
///  prior written permission.
///  
///  THIS SOFTWARE IS PROVIDED “AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, 
///  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
///  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
///  BROADCOM BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
///  EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, 
///  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;LOSS OF USE, DATA, OR PROFITS; 
///  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
///  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR 
///  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
///  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/// ---------------------------------------------------------------------------
/// \file types.h   Broadcom LBS Client Data Types.
///============================================================================

#ifndef _BRCM_LBS_TYPES_H_
#define _BRCM_LBS_TYPES_H_

#include "os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************/
/** Result error codes
*******************************************************************************/
typedef enum BrcmLbs_Result
{
    BRCM_LBS_DATA_END                =  1,
    BRCM_LBS_OK                      =  0,
    BRCM_LBS_ERROR_LBS_INVALID       = -1,
    BRCM_LBS_ERROR_PARAMETER_INVALID = -2,
    BRCM_LBS_ERROR_OUT_OF_MEMORY     = -3,
    BRCM_LBS_ERROR_ENCODE            = -4,
    BRCM_LBS_ERROR_DECODE            = -5,
    BRCM_LBS_ERROR_COUNT             = -6,
    BRCM_LBS_ERROR_FAILED            = -7,
    BRCM_LBS_ERROR_UNEXPECTED        = -8,
    BRCM_LBS_ERROR_INVALID_VERSION   = -9,
} BrcmLbs_Result;

/*******************************************************************************/
/** Reference clock current status information
*******************************************************************************/
typedef enum Brcm_RefClkStatus
{
    BRCM_LBS_REFCLKSTAT_NO_INFO,      /**< no info is available regarding the state of the ref. clk */
    BRCM_LBS_REFCLKSTAT_ADJUSTING,    /**< CNT_IN signal is being corrected with VCO adjustments */
    BRCM_LBS_REFCLKSTAT_SEARCHING,    /**< CNT_IN signal is currently unlocked or is otherwise unreliable */
    BRCM_LBS_REFCLKSTAT_HOLDING,      /**< CNT_IN signal is being held constant (no VCO adjustments) */
    BRCM_LBS_REFCLKSTAT_OFFSET_KNOWN  /**< The value of the ref. clk is known and must be used instead of CNTIN */
} Brcm_RefClkStatus;

/*******************************************************************************/
/** Reference clock frequency offset information
*******************************************************************************/
typedef struct BrcmLbs_RefClkInfo
{
    OsInt32             FreqOff;                /**< Reference clock frequency offset from nominal [ppb] */
    OsUint16            FreqOffUncPpb;          /**< Reference clock frequency  offset uncertainty  [ppb] */
    Brcm_RefClkStatus   FreqStat;               /**< Current reference clock status information */
    OsUint32            FreqChange;             /**< Reference clock status change bit field */
} BrcmLbs_RefClkInfo;


/*******************************************************************************/
/** UTC time
*******************************************************************************/
typedef struct BrcmLbs_UtcTime
{
    OsUint16    year;           /**< Year     [0..65535] */
    OsUint16    month;          /**< Month    [1.12] */
    OsUint16    day;            /**< Day      [1..31] */
    OsUint16    hour;           /**< Hour     [0..23] */
    OsUint16    min;            /**< Minutes  [0..59] */
    OsUint16    sec;            /**< Seconds  [0..60] */
    OsInt16     milliSec;       /**< Millisec [0..999] */
    OsUint16    microSec;       /**< Microsec [0..999] */
    OsUint32    precMicroSec;   /**< Time precision microsec [0..100 000 000] */
} BrcmLbs_UtcTime;

/********************************************************************************/
/**                                                                             */
/** GSM Cell.                                                                   */
/**                                                                             */
/********************************************************************************/

/********************************************************************************/
/** Network Measurement Reports (NMR).
* Describes Contents of the Current Network Measurement Reports. 
* Contains 1 to 15 NMR elements
********************************************************************************
* Reference: OMA SUPL TS (Section 7.1 NMR).\n 
\n ASN.1 specification:
\verbatim
NMRelement ::= SEQUENCE {
    aRFCN   INTEGER(0..1023),
    bSIC    INTEGER(0..63),
    rxLev   INTEGER(0..63),
    ...
}
\endverbatim
*********************************************************************************/
typedef struct BrcmLbs_NMRelement
{
   OsUint16 aRFCN;
   OsUint8  bSIC;
   OsUint8  rxLev;
} BrcmLbs_NMRelement;

/********************************************************************************/
/** Network Measurement Reports (NMR).
* This parameter describes Contents of the Current Network Measurement Reports. 
* Contains 1 to 15 NMR elements
********************************************************************************
* Reference: OMA SUPL TS (Section 7.1 NMR).\n 
\n ASN.1 specification:
\verbatim
NMR ::= SEQUENCE (SIZE (1..15)) OF NMRelement
\endverbatim
*********************************************************************************/
typedef struct BrcmLbs_NMR
{
   OsUint8 n; /**< number of elements in elem must be 1..15 */
   BrcmLbs_NMRelement elem[15];
} BrcmLbs_NMR;

/********************************************************************************/
/** GSM Cell Info.
* This parameter defines the parameter of a GSM radio cell.
********************************************************************************
* Reference: OMA SUPL TS (Section 7.11.1 GSM Cell Info).\n 
\n ASN.1 specification:
\verbatim
GsmCellInformation ::= SEQUENCE {
    refMCC  INTEGER(0..999),                -- Mobile Country Code
    refMNC  INTEGER(0..999),                -- Mobile Network Code
    refLAC  INTEGER(0..65535),              -- Location area code
    refCI   INTEGER(0..65535),              -- Cell identity
    nMR     NMR                 OPTIONAL,
    tA      INTEGER(0..255)     OPTIONAL,   -- Timing Advance
    ...
}
\endverbatim
*********************************************************************************/
typedef struct BrcmLbs_GsmCellInformation
{
    struct {
        unsigned aRFCNPresent : 1;
        unsigned bSICPresent  : 1;
        unsigned rxLevPresent : 1;
    } m;
    OsUint16    refMCC;
    OsUint16    refMNC;
    OsUint16    refLAC;
    OsUint16    refCI;
    OsUint16    aRFCN;
    OsUint8     bSIC;
    OsUint8     rxLev;
} BrcmLbs_GsmCellInformation;

/********************************************************************************/
/*                                                                              */
/*  WCDMA Cell                                                                  */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/** Frequency info FDD.
* The FDD Frequency parameter of a WCDMA radio cell.
* FDD Frequency info can be:
* uarfcn-UL, range: (0..16383)
* uarfcn-DL, range: (0..16383)
* In case of fdd, uarfcn-UL is optional while uarfcn-DL is mandatory. 
* If uarfcn-UL is not present, the default duplex distance defined for 
* the operating frequency band shall be used [3GPP RRC].
********************************************************************************
* Reference: OMA SUPL TS (Section 7.11.2 WCDMA Cell Info).\n 
\n ASN.1 specification:
\verbatim
FrequencyInfoFDD ::= SEQUENCE {
    uarfcn-UL UARFCN    OPTIONAL,
    uarfcn-DL UARFCN,
    ...
}
\endverbatim
*********************************************************************************/
typedef struct BrcmLbs_FrequencyInfoFDD
{
    struct {
        unsigned uarfcn_ULPresent : 1;
    } m;
    OsUint16  uarfcn_UL;
    OsUint16  uarfcn_DL;
} BrcmLbs_FrequencyInfoFDD;

/********************************************************************************/
/** Frequency info TDD.
* The TDD Frequency parameter of a WCDMA radio cell.
* TDD Frequency info can be:
* uarfcn-Nt, range: (0..16383)
********************************************************************************
* Reference: OMA SUPL TS (Section 7.11.2 WCDMA Cell Info).\n 
\n ASN.1 specification:
\verbatim
FrequencyInfoTDD ::= SEQUENCE {
    uarfcn-Nt UARFCN,
    ...
}
\endverbatim
*********************************************************************************/
typedef struct BrcmLbs_FrequencyInfoTDD
{
    OsUint16  uarfcn_Nt;
} BrcmLbs_FrequencyInfoTDD;

/********************************************************************************/
/** Frequency info Type.
* The Frequency info Type can be FDD or TDD.
*********************************************************************************
* Reference: OMA SUPL TS (Section 7.11.2 WCDMA Cell Info).
*********************************************************************************/
typedef enum BrcmLbs_FrequencyInfoType
{
    BRCM_LBS_FRQ_FDD = 1,            /**< FDD Frequency information */
    BRCM_LBS_FRQ_TDD = 2             /**< TDD Frequency information */
} BrcmLbs_FrequencyInfoType;

/********************************************************************************/
/** Frequency Info Mode.
* Mode Specific Info of Frequency Info.
********************************************************************************
* Reference: OMA SUPL TS (Section 7.11.2 WCDMA Cell Info).\n 
\n ASN.1 specification:
\verbatim
FrequencyInfo ::= SEQUENCE {
    modeSpecificInfo CHOICE {
        fdd FrequencyInfoFDD,
        tdd FrequencyInfoTDD,
        ...
    },
    ...
}
\endverbatim
*********************************************************************************/
typedef struct BrcmLbs_FrequencyInfo_modeSpecificInfo
{
    BrcmLbs_FrequencyInfoType t;
    union 
    {
        BrcmLbs_FrequencyInfoFDD fdd;
        BrcmLbs_FrequencyInfoTDD tdd;
    } u;
} BrcmLbs_FrequencyInfo_modeSpecificInfo;

/********************************************************************************/
/** Frequency info.
* The Frequency info of a WCDMA radio cell.
*********************************************************************************
* Reference: OMA SUPL TS (Section 7.11.2 WCDMA Cell Info).
*********************************************************************************/
typedef struct BrcmLbs_FrequencyInfo
{
    BrcmLbs_FrequencyInfo_modeSpecificInfo modeSpecificInfo;
} BrcmLbs_FrequencyInfo;

/********************************************************************************/
/** Time slot.
* The Time slot list for TDD Frequency of a WCDMA radio cell.
********************************************************************************
* Reference: OMA SUPL TS (Section 7.11.2 WCDMA Cell Info).\n 
\n ASN.1 specification:
\verbatim
maxTS INTEGER ::= 14
TimeslotISCP-List ::= SEQUENCE (SIZE (1..maxTS)) OF TimeslotISCP
\endverbatim
*********************************************************************************/
typedef struct BrcmLbs_TimeslotISCP_List 
{
    OsUint8 n;
    OsUint8 elem[14];
} BrcmLbs_TimeslotISCP_List;

/********************************************************************************/
/** TDD Specific Info.
* The TDD Frequency Specific Info of a WCDMA radio cell.
********************************************************************************
* Reference: OMA SUPL TS (Section 7.11.2 WCDMA Cell Info).\n 
\n ASN.1 specification:
\verbatim
        tdd SEQUENCE {
            cellParametersID    CellParametersID,
            proposedTGSN        TGSN OPTIONAL,
            primaryCCPCH-RSCP   PrimaryCCPCH-RSCP   OPTIONAL,
            pathloss            Pathloss            OPTIONAL,
            timeslotISCP-List   TimeslotISCP-List   OPTIONAL
        }
\endverbatim
*********************************************************************************/
typedef struct BrcmLbs_CellMeasuredResults_modeSpecificInfo_tdd 
{
    struct {
        unsigned proposedTGSNPresent : 1;
        unsigned primaryCCPCH_RSCPPresent : 1;
        unsigned pathlossPresent : 1;
        unsigned timeslotISCP_ListPresent : 1;
    } m;
    OsUint8 cellParametersID;
    OsUint8 proposedTGSN;
    OsUint8 primaryCCPCH_RSCP;
    OsUint8 pathloss;
    BrcmLbs_TimeslotISCP_List timeslotISCP_List;
} BrcmLbs_CellMeasuredResults_modeSpecificInfo_tdd;

/********************************************************************************/
/** TDD Specific Info.
* The Primary CCPCH.
********************************************************************************
* Reference: OMA SUPL TS (Section 7.11.2 WCDMA Cell Info).\n 
\n ASN.1 specification:
\verbatim
    PrimaryCCPCH-RSCP ::= INTEGER(0..127)
\endverbatim
*********************************************************************************/
typedef struct BrcmLbs_PrimaryCPICH_Info 
{
    OsUint16 primaryScramblingCode;
} BrcmLbs_PrimaryCPICH_Info;

/********************************************************************************/
/** FDD Specific Info.
* The FDD Frequency Specific Info of a WCDMA radio cell.
********************************************************************************
* Reference: OMA SUPL TS (Section 7.11.2 WCDMA Cell Info)\n
\n ASN.1 specification:
\verbatim
        fdd SEQUENCE {
            primaryCPICH-Info   PrimaryCPICH-Info,
            cpich-Ec-N0         CPICH-Ec-N0         OPTIONAL,
            cpich-RSCP          CPICH-RSCP          OPTIONAL,
            pathloss            Pathloss            OPTIONAL
        },
\endverbatim
*********************************************************************************/
typedef struct BrcmLbs_CellMeasuredResults_modeSpecificInfo_fdd 
{
    struct {
        unsigned cpich_Ec_N0Present : 1;
        unsigned cpich_RSCPPresent : 1;
        unsigned pathlossPresent : 1;
    } m;
    BrcmLbs_PrimaryCPICH_Info primaryCPICH_Info;
    OsUint8 cpich_Ec_N0;
    OsUint8 cpich_RSCP;
    OsUint8 pathloss;
} BrcmLbs_CellMeasuredResults_modeSpecificInfo_fdd;

/********************************************************************************/
/** Cell Measured Results.
* The Cell Measured Results of a WCDMA radio cell.
********************************************************************************
* Reference: OMA SUPL TS (Section 7.11.2 WCDMA Cell Info)\n
\n ASN.1 specification:
\verbatim
modeSpecificInfo :=
    CHOICE {
        fdd SEQUENCE {
            primaryCPICH-Info   PrimaryCPICH-Info,
            cpich-Ec-N0         CPICH-Ec-N0         OPTIONAL,
            cpich-RSCP          CPICH-RSCP          OPTIONAL,
            pathloss            Pathloss            OPTIONAL
        },
        tdd SEQUENCE {
            cellParametersID    CellParametersID,
            proposedTGSN        TGSN OPTIONAL,
            primaryCCPCH-RSCP   PrimaryCCPCH-RSCP   OPTIONAL,
            pathloss            Pathloss            OPTIONAL,
            timeslotISCP-List   TimeslotISCP-List   OPTIONAL
        }
    }
\endverbatim
*********************************************************************************/
typedef struct BrcmLbs_CellMeasuredResults_modeSpecificInfo 
{
    BrcmLbs_FrequencyInfoType t;
    union {
        /* t = 1 */
        BrcmLbs_CellMeasuredResults_modeSpecificInfo_fdd fdd;
        /* t = 2 */
        BrcmLbs_CellMeasuredResults_modeSpecificInfo_tdd tdd;
    } u;
} BrcmLbs_CellMeasuredResults_modeSpecificInfo;

/********************************************************************************/
/** Cell Measured Results.
* The Cell Measured Results of a WCDMA radio cell.
********************************************************************************
* Reference: OMA SUPL TS (Section 7.11.2 WCDMA Cell Info)\n
\n ASN.1 specification:
\verbatim
CellMeasuredResults ::= SEQUENCE {
    cellIdentity INTEGER(0..268435455) OPTIONAL,
    modeSpecificInfo
}
\endverbatim
*********************************************************************************/
typedef struct BrcmLbs_CellMeasuredResults 
{
   struct {
      unsigned cellIdentityPresent : 1;
   } m;
   OsUint32                                     cellIdentity;
   BrcmLbs_CellMeasuredResults_modeSpecificInfo modeSpecificInfo;
} BrcmLbs_CellMeasuredResults;

/********************************************************************************/
/** Cell Measured Results List.
* The FDD Frequency Specific Info of a WCDMA radio cell.
********************************************************************************
* Reference: OMA SUPL TS (Section 7.11.2 WCDMA Cell Info)\n
\n ASN.1 specification:
\verbatim
    maxCellMeas INTEGER ::= 32
    CellMeasuredResultsList ::= SEQUENCE (SIZE (1..maxCellMeas)) OF CellMeasuredResults
\endverbatim
*********************************************************************************/
typedef struct BrcmLbs_CellMeasuredResultsList 
{
   OsUint8 n;
   BrcmLbs_CellMeasuredResults elem[32];
} BrcmLbs_CellMeasuredResultsList;

/********************************************************************************/
/** Cell Measured Results.
* The FDD Measured Results of a WCDMA radio cell.
********************************************************************************
* Reference: OMA SUPL TS (Section 7.11.2 WCDMA Cell Info)\n
\n ASN.1 specification:
\verbatim
MeasuredResults ::= SEQUENCE {
    frequencyInfo           FrequencyInfo           OPTIONAL,
    utra-CarrierRSSI        UTRA-CarrierRSSI        OPTIONAL,
    cellMeasuredResultsList CellMeasuredResultsList OPTIONAL
}
\endverbatim
*********************************************************************************/
typedef struct BrcmLbs_MeasuredResults
{
   struct {
      unsigned frequencyInfoPresent : 1;
      unsigned utra_CarrierRSSIPresent : 1;
      unsigned cellMeasuredResultsListPresent : 1;
   } m;
   BrcmLbs_FrequencyInfo            frequencyInfo;
   OsUint8                          utra_CarrierRSSI;
   BrcmLbs_CellMeasuredResultsList  cellMeasuredResultsList;
} BrcmLbs_MeasuredResults;

/********************************************************************************/
/** Measured Results List.
* Network Measurement Report for WCDMA comprising both intra- and/or 
* inter-frequency cell measurements (as per 3GPP TS 25.331).
********************************************************************************
* Reference: OMA SUPL TS (Section 7.11.2 WCDMA Cell Info)\n
\n ASN.1 specification:
\verbatim
    maxFreq INTEGER ::= 8
    MeasuredResultsList ::= SEQUENCE (SIZE (1..maxFreq)) OF MeasuredResults
\endverbatim
*********************************************************************************/
typedef struct BrcmLbs_MeasuredResultsList 
{
   OsUint8 n;
   BrcmLbs_MeasuredResults elem[8];
} BrcmLbs_MeasuredResultsList;

/********************************************************************************/
/** WCDMA Cell Information.
* This parameter defines the parameter of a WCDMA radio cell.
*********************************************************************************/
typedef struct BrcmLbs_WcdmaCellInformation
{
    struct {
        unsigned frequencyInfoPresent : 1;
        unsigned cellMeasuredResultPresent : 1;
    } m;
    OsUint16    refMCC;     /**< INTEGER(0..999),            -- Mobile Country Code  */
    OsUint16    refMNC;     /**< INTEGER(0..999),            -- Mobile Network Code  */
    OsUint32    refUC;      /**< INTEGER(0..268435455),      -- Cell identity        */
    BrcmLbs_FrequencyInfo                           frequencyInfo;
    BrcmLbs_CellMeasuredResults_modeSpecificInfo    cellMeasuredResult;
} BrcmLbs_WcdmaCellInformation;

/********************************************************************************/
/** Cell Info Type.
* The following cell IDs are supported:
* - GSM Cell Info
* - WCDMA Cell Info
* - CDMA Cell Info
*********************************************************************************
* Reference: OMA SUPL TS (Section 7.11 Location ID)
*********************************************************************************/
typedef enum BrcmLbs_CellInfoType
{
    BRCM_LBS_CELL_INFO_GSM     = 1,    /**< GSM Cell information   */
    BRCM_LBS_CELL_INFO_WCDMA   = 2     /**< WCDMA Cell information */
} BrcmLbs_CellInfoType;

/********************************************************************************/
/** Cell Info.
* The following cell IDs are supported:
* - GSM Cell Info
* - WCDMA Cell Info
*********************************************************************************/
typedef struct BrcmLbs_CellInfo
{
    BrcmLbs_CellInfoType  network_type;   /**< Cell Information type */
    union
    {
        BrcmLbs_GsmCellInformation    gsmCell;
        BrcmLbs_WcdmaCellInformation  wcdmaCell;
    } u;
} BrcmLbs_CellInfo;

/********************************************************************************/
/** GPS Time
*********************************************************************************/
typedef struct BrcmLbsGps_Time
{
    OsUint16    gps_week;   /**< Current GPS week [0..1023] */
    OsUint32    week_ms;    /**< Milisecond of current GPS week */
} BrcmLbsGps_Time;

/********************************************************************************/
/** Location information
*********************************************************************************/
typedef struct BrcmLbs_Position
{
    struct 
    {
        unsigned altitudePresent : 1;
        unsigned pos_errorPresent : 1;
    } m;
    double      latitude;   /**< Lattitude [degrees] */
    double      longitude;  /**< Longitude [degrees] */
    OsInt16     altitude;   /**< Altitude [meters] */
    OsUint16    pos_error;  /**< Estimated position error [meters] */
} BrcmLbs_Position;

typedef enum BrcmLbs_PositionSource
{
    BRCM_LBS_POS_UNKNOWN          = 0,
    BRCM_LBS_POS_UE_ASSISTED_AGPS = 1,
    BRCM_LBS_POS_UE_BASED_AGPS    = 2,
    BRCM_LBS_POS_AUTONOMOUS       = 3
} BrcmLbs_PositionSource;

typedef struct BrcmLbs_Location
{
    BrcmLbs_PositionSource      source;     /**< Position source from */
    BrcmLbsGps_Time             gpsTime;    /**< Current GPS time */
    BrcmLbs_Position            pos;        /**< Lattitude, Longitude, Altitude */
} BrcmLbs_Location;

/*******************************************************************************/
/** Cell geotag.
********************************************************************************/
typedef struct BrcmLbs_CellGeotag
{
    BrcmLbs_CellInfo  cellInfo;
    BrcmLbs_Location  location;
} BrcmLbs_CellGeotag;


/*******************************************************************************/
/** Navigation Model (Ephemeris)
********************************************************************************/
typedef struct BrcmLbs_EphemerisSubframe1Reserved
{
   OsUint32     reserved1;
   OsUint32     reserved2;
   OsUint32     reserved3;
   OsUint16     reserved4;
} BrcmLbs_EphemerisSubframe1Reserved;

/*******************************************************************************/
/** GPS Uncompressed Ephemeris.
********************************************************************************/
typedef struct BrcmLbs_UncompressedEphemeris
{
   OsUint8      ephemCodeOnL2;                  /**< codes on L2 channel */
   OsUint8      ephemURA;                       /**< URA Index */
   OsUint8      ephemSVhealth;                  /**< Satellite health */
   OsUint16     ephemIODC;                      /**< Issue of data, clock */
   OsUint8      ephemL2Pflag;                   /**< L2 P data flag */
   BrcmLbs_EphemerisSubframe1Reserved ephemSF1Rsvd; /**< reserved bits in ICD-200, sbfrm1 */
   char         ephemTgd;                       /**< group delay (seconds) */
   OsUint16     ephemToc;                       /**< time of clock (seconds) */
   char         ephemAF2;                       /**< SV clock drift rate (sec/sec2) */
   OsInt16      ephemAF1;                       /**< SV clock drift (sec/sec) */
   OsInt32      ephemAF0;                       /**< SV clock bias (seconds) */
   OsInt16      ephemCrs;                       /**< Amplitude of sine harmonic correction (meters) */
   OsInt16      ephemDeltaN;                    /**< Mean motion difference from computed (rad/s) */
   OsInt32      ephemM0;                        /**< Mean anomaly at reference time (rad) */
   OsInt16      ephemCuc;                       /**< Amplitude of cosine harmonic correction (rad) */
   OsUint32     ephemE;                         /**< Eccentricity (dimensionless) */
   OsInt16      ephemCus;                       /**< Amplitude of sine harmonic correction (rad) */
   OsUint32     ephemAPowerHalf;                /**< Square root of semi-major axis (meters^1/2) */
   OsUint16     ephemToe;                       /**< Reference time of ephemeris (seconds) */
   OsUint8      ephemFitFlag;                   /**< Fit Interval Flag */
   OsUint8      ephemAODA;                      /**< Age of data offset */
   OsInt16      ephemCic;                       /**< Amplitude of sine harmonic correction (rad) */
   OsInt32      ephemOmegaA0;                   /**< Longitude of ascending node (rad) */
   OsInt16      ephemCis;                       /**< Amplitude of sine harmonic correction (rad) */
   OsInt32      ephemI0;                        /**< Inclination angle at reference time (rad) */
   OsInt16      ephemCrc;                       /**< Amplitude of sine harmonic correction (meters) */
   OsInt32      ephemW;                         /**< Argument of perigee (radians) */
   OsInt32      ephemOmegaADot;                 /**< Rate of right ascension (radians/sec) */
   OsInt16      ephemIDot;                      /**< Rate of inclination angle (radians/sec) */
} BrcmLbs_UncompressedEphemeris;

typedef enum BrcmLbs_SatStatusType
{
    BRCM_LBS_SatStatus_newSatelliteAndModelUC = 1,
    BRCM_LBS_SatStatus_oldSatelliteAndModel   = 2,
    BRCM_LBS_SatStatus_newNaviModelUC         = 3,
    BRCM_LBS_SatStatus_extElem1               = 4
} BrcmLbs_SatStatusType;

/*******************************************************************************/
/** GPS Satellite Status.
********************************************************************************/
typedef struct BrcmLbs_SatStatus
{
   BrcmLbs_SatStatusType t;
   union {
      /* t = 1 */
      BrcmLbs_UncompressedEphemeris newSatelliteAndModelUC;
      /* t = 2 */
      /* t = 3 */
      BrcmLbs_UncompressedEphemeris newNaviModelUC;
      /* t = 4 */
   } u;
} BrcmLbs_SatStatus;

/*******************************************************************************/
/** Navigation Model Element.
********************************************************************************/
typedef struct BrcmLbs_NavModelElement
{
   OsUint8              satelliteID;
   BrcmLbs_SatStatus    satStatus;
} BrcmLbs_NavModelElement;

/**< DGPS Corrections */
typedef struct BrcmLbs_DGPSCorrections
{
} BrcmLbs_DGPSCorrections;

/**< Ionospheric Model */
typedef struct BrcmLbs_IonosphericModel
{
} BrcmLbs_IonosphericModel;

/**< UTC Model */
typedef struct BrcmLbs_UTCModel
{
} BrcmLbs_UTCModel;

/**< Almanac */
typedef struct BrcmLbs_Almanac
{
} BrcmLbs_Almanac;

/**< Acquisition Assistance */
typedef struct BrcmLbs_AcquisAssist
{
} BrcmLbs_AcquisAssist;

/**< Real-Time Integrity */
typedef struct BrcmLbs_BadSatellites
{
} BrcmLbs_BadSatellites;


/*******************************************************************************/
/** GPS Measurement Information Element
*  - see GSM 04.31 version 7.2.0 Release 1998, Table A.5
*  - see GSM 04.31 version 7.2.0 Release 1998, Table A.8
********************************************************************************/
#ifdef CONFIG_LCS_INCLUDE_ALL_CONSTELLATIONS
#define  BRCM_LBS_MAX_MSMT_SIZE 32
#else
#define  BRCM_LBS_MAX_MSMT_SIZE 12
#endif

/** Satellite Pseudo-Range Measurements */
typedef struct BrcmLbsGps_SatPrMeas
{
    OsUint32                satID;
    OsUint32                c2N0;                   /**< carrier to noise ratio */
    OsInt32                 sDopler;
    OsUint32                sWholeChip;
    OsUint32                sFracChip;
    OsUint32                cMultipathIndicator;
    OsUint32                cPrRmsError;            /**< pseudorange RMS error */
    OsUint32                cDopRmsError;           /**< Dopler RMS error */
} BrcmLbsGps_SatPrMeas;

/** Satellite Measurements */
typedef struct BrcmLbsGps_ResMeas
{
    OsInt32                 lRangeTimeUncMs;    /**< Uncertainty of the value below  */
    OsUint32                gpsTow;             /**< [0 ..  14399999] ms */
    OsUint32                gpsTowFull;         /**< [0 .. 604799999] ms */
    /** Rough estimated accuracy that a position would have when computed by a MSA server (0xFFFF when unknown)*/
    OsUint16                usEstAccM;
    OsUint32                numSats;
    BrcmLbsGps_SatPrMeas    arraySat[BRCM_LBS_MAX_MSMT_SIZE];
} BrcmLbsGps_ResMeas;

/** Per SV information */
typedef struct BrcmLbs_SvInfo 
{
    OsInt8                  bUsed;                  /**< Satellite was used for position fix */
    OsInt8                  bDetected;              /**< Satellite was detected */
    OsInt16                 sSvId;                  /**< SV id */
    OsInt16                 sElev;                  /**< SV elevation [degrees] */
    OsInt16                 sAz;                    /**< SV azimuth   [degrees] */
    OsInt16                 sCNo;                   /**< C/No [dBHz] */
    OsInt16                 sSigStrength;           /**< Signal strength estimation [dBm] */
    OsInt16                 sInstantSigStrength;    /**< un-smoothed Signal strength estimation [dBm] */
    OsInt16                 sCNoFT;                 /**< C/No [1/10 dBHz] for factory test only*/
    OsInt16                 sSigStrengthFT;         /**< Signal strength [1/10 dBm] for Factory test only*/
    OsInt8                  bHasEph;                /**< There is epehemeris available for that satellited*/
} BrcmLbs_SvInfo;


/*******************************************************************************/
/** Generic GPS Structures
********************************************************************************/

/** Parameter in periodic position request */
typedef struct BrcmLbsGps_LocReportPeriod
{
    OsUint32               ulPeriodMs;          /**< Requested report period in msec   */
    OsInt32                lMaxTotalFixCount;   /**< Requested Max Total Fix Count     */
    OsInt32                lMaxValidFixCount;   /**< Requested Max Valid Fix Count     */
    OsInt32                lTimeoutSec;         /**< Requested position timeout in sec */
} BrcmLbsGps_LocReportPeriod;

/*******************************************************************************/
/** Location information.
********************************************************************************/
typedef struct BrcmLbs_PosLocation
{
    struct 
    {
        OsUint8 altitudePresent    : 1;
        OsUint8 horAccuracyPresent : 1;
        OsUint8 verAccuracyPresent : 1;
        OsUint8 hdopPresent        : 1;
        OsUint8 speedPresent       : 1;
        OsUint8 bearingPresent     : 1;
    } m;
    OsBool              posValid;       /**< TRUE, if position is valid. */
    OsInt16             satUsed;        /**< number of used satelites */
    BrcmLbs_UtcTime     utcTime;        /**< UTC time of location */
    BrcmLbsGps_Time     gpsTime;        /**< GPS time of location */
    double              latitude;       /**< lattitude [degrees] */
    double              longitude;      /**< longitude [degrees] */
    OsInt16             altitude;       /**< altitude  [meters]  */
    OsUint32            horAccuracy;    /**< estimated horizontal position error [meters] */
    OsUint16            verAccuracy;    /**< estimated vertical position error [meters] */
    double              dHDOP;          /**< KF Horizontal Dilution Of Precision */
    double              speed;          /**< speed     [meters/second] */
    double              bearing;        /**< bearing   [degrees] */
    BrcmLbs_PositionSource source;      /**< Position source */
} BrcmLbs_PosLocation;

/** Structure used for position Info notification */
typedef struct BrcmLbs_PosInfo
{
    BrcmLbs_PosLocation location;
    OsInt8          svNum;
    BrcmLbs_SvInfo  svInfo[BRCM_LBS_MAX_MSMT_SIZE];
} BrcmLbs_PosInfo;

/*******************************************************************************/
/** Location Request Type */
typedef enum BrcmLbs_RequestLocationCode
{
    BRCM_LBS_REQ_NONE,               /**< No request */
    BRCM_LBS_REQ_PERIODIC,           /**< Request for periodic location */
    BRCM_LBS_REQ_SINGLE_SHOT         /**< Request for a single location */
} BrcmLbs_RequestLocationCode;

/** GPS Assistance Data Request Type */
typedef enum BrcmLbsGps_AdCode
{
    BRCM_LBS_GPS_AD_REFTIME            = 1 << 0,   /**< Reference Time */
    BRCM_LBS_GPS_AD_REFLOCATION        = 1 << 1,   /**< Reference Location */
    BRCM_LBS_GPS_AD_NAVMODEL           = 1 << 2,   /**< Navigation Model */
    BRCM_LBS_GPS_AD_DGPSCORRECTIONS    = 1 << 3,   /**< DGPS Corrections */
    BRCM_LBS_GPS_AD_IONOMODEL          = 1 << 4,   /**< Ionospheric Model */
    BRCM_LBS_GPS_AD_UTCMODEL           = 1 << 5,   /**< UTC Model */
    BRCM_LBS_GPS_AD_ALMANAC            = 1 << 6,   /**< Almanac */
    BRCM_LBS_GPS_AD_ACQUISASSIST       = 1 << 7,   /**< Acquisition Assistance */
    BRCM_LBS_GPS_AD_RTIMEINTEGRITY     = 1 << 8    /**< Real-Time Integrity */
} BrcmLbsGps_AdCode;

/** GPS Assistance Data Respond */
typedef struct BrcmLbsGps_AssistanceData
{
    BrcmLbsGps_AdCode               data_type;    /**< Data Type. */
    union
    { 
        BrcmLbsGps_Time             refTime;      /**< Reference Time */
        BrcmLbs_Position            refLoc;       /**< Reference Location */
        BrcmLbs_NavModelElement     navModel;     /**< Navigation Model */
        BrcmLbs_DGPSCorrections     dgpsc;        /**< DGPS Corrections */
        BrcmLbs_IonosphericModel    ionoModel;    /**< Ionospheric Model */
        BrcmLbs_UTCModel            utcModel;     /**< UTC Model */
        BrcmLbs_Almanac             almanac;      /**< Almanac */
        BrcmLbs_AcquisAssist        acquisAssist; /**< Acquisition Assistance */
        BrcmLbs_BadSatellites       rti;          /**< Real-Time Integrity */
    } u;
} BrcmLbsGps_AssistanceData;

/** SUPL Notification types */
typedef enum BrcmLbsSupl_NotificationType
{
   BRCM_LBS_noNotificationNoVerification = 0,
   BRCM_LBS_notificationOnly = 1,
   BRCM_LBS_notificationAndVerficationAllowedNA = 2,
   BRCM_LBS_notificationAndVerficationDeniedNA = 3,
   BRCM_LBS_privacyOverride = 4
} BrcmLbsSupl_NotificationType;

typedef enum BrcmLbsSupl_EncodingType 
{
    BRCMLBSSUPL_ENC_UCS2,
    BRCMLBSSUPL_ENC_GSMDEFAULT,
    BRCMLBSSUPL_ENC_UTF8,
    BRCMLBSSUPL_ENC_UNKNOWN
} BrcmLbsSupl_EncodingType;

typedef enum BrcmLbsSupl_FormatIndicator
{
    BRCMLBSSUPL_LOGICALNAME,
    BRCMLBSSUPL_E_MAILADDRESS,
    BRCMLBSSUPL_MSISDN,
    BRCMLBSSUPL_URL,
    BRCMLBSSUPL_SIPURL,
    BRCMLBSSUPL_MIN,
    BRCMLBSSUPL_MDN,
    BRCMLBSSUPL_IMSPUBLICINDENTITY,
    BRCMLBSSUPL_FORMAT_UNKOWN
} BrcmLbsSupl_FormatIndicator;

typedef struct BrcmLbsSupl_Notification_RequestorID {
    uint16_t    len;
    unsigned char data[50];
} BrcmLbsSupl_Notification_RequestorID;

typedef struct BrcmLbsSupl_Notification_clientName {
    uint16_t    len;
    unsigned char data[50];
} BrcmLbsSupl_Notification_clientName;

typedef struct BrcmLbsSupl_Notification
{
    BrcmLbsSupl_NotificationType notificationType;
    BrcmLbsSupl_EncodingType encodingType;
    BrcmLbsSupl_Notification_RequestorID requestorId;
    BrcmLbsSupl_FormatIndicator requestorIdType;
    BrcmLbsSupl_Notification_clientName clientName;
    BrcmLbsSupl_FormatIndicator clientNameType;
} BrcmLbsSupl_Notification;

/********************************************************************************/
/** SET positioning technologies
* SET capabilities (not mutually exclusive) in terms of supported positioning 
* technologies
*********************************************************************************/
typedef struct BRCM_LBS_PosTechnology
{
   OsBool agpsSETassisted;
   OsBool agpsSETBased;
   OsBool autonomousGPS;
   OsBool aFLT;
   OsBool eCID;
   OsBool eOTD;
   OsBool oTDOA;
} BRCM_LBS_PosTechnology;

/********************************************************************************/
/** SET Capabilities
*********************************************************************************/
typedef struct BRCM_LBS_SETCapabilities
{
   BRCM_LBS_PosTechnology posTechnology;
} BRCM_LBS_SETCapabilities;

/********************************************************************************/
/** SET ID Type
* SET Session ID type
*********************************************************************************/
typedef enum BrcmLbsSupl_SetIDType
{
    BRCM_LBS_ID_MSISDN,
    BRCM_LBS_ID_MDN,
    BRCM_LBS_ID_MIN,
    BRCM_LBS_ID_IMSI,
    BRCM_LBS_ID_IPv4,
    BRCM_LBS_ID_IPv6
} BrcmLbsSupl_SetIDType;

/********************************************************************************/
/** SET ID
*********************************************************************************/
typedef struct BrcmLbsSupl_SetID
{
    BrcmLbsSupl_SetIDType     eType;         /**< ID Type. */
    union
    { 
        OsOctet   msisdn[8];    /**< Mobile Subscriber ISDN Number (BCD format)     */
        OsOctet   mdn[8];       /**< MSDN Number                                    */
        OsOctet   min[5];       /**< Mobile ID number (first 34 bits are used)      */
        OsOctet   imsi[8];      /**< International Mobile Subscriber Identity (IMSI = MCC + MNC + MSIN) */
        OsOctet   ipv4[4];      /**< IP address v4 */
        OsOctet   ipv6[16];     /**< IP address v6 */
    } u;
} BrcmLbsSupl_SetID;
/********************************************************************************/
/** Connection type 
* TLS(Transport Layer security) protocol enable type
*********************************************************************************/
typedef enum BrcmLbsSupl_ConnectionType
{ 
    BRCM_LBS_NO_TLS,
    BRCM_LBS_TLS
} BrcmLbsSupl_ConnectionType;

/********************************************************************************/
/** Cell Info Status
*********************************************************************************/
typedef enum BrcmLbs_CellStatus 
{
   BRCM_LBS_STATUS_STALE   = 0,
   BRCM_LBS_STATUS_CURRENT = 1,
   BRCM_LBS_STATUS_UNKNOWN = 2
} BrcmLbs_CellStatus;

/********************************************************************************/
/** Location ID.
* Describes the globally unique cell identification of the most current serving cell.
*********************************************************************************/
typedef struct BrcmLbsSupl_LocationId 
{
   BrcmLbs_CellInfo cellInfo;
   BrcmLbs_CellStatus status;
} BrcmLbsSupl_LocationId;


/** LBS TCP connections */
typedef enum BrcmLbs_ConnType
{
    BRCM_LBS_CONN,
    BRCM_LBS_SUPL
} BrcmLbs_ConnType;

/** LBS files */
typedef enum BrcmLbs_FileType
{
    BRCM_LBS_FILE_LTO,
    BRCM_LBS_FILE_LBS,
    BRCM_LBS_FILE_NMR,
    BRCM_LBS_FILE_WLAN,
    BRCM_LBS_FILE_WNMR
} BrcmLbs_FileType;

/*******************************************************************************/
/** User data pointer
*******************************************************************************/
typedef OsHandle BrcmLbs_UserData;

/*******************************************************************************/
/** Timer handle
*******************************************************************************/
typedef OsHandle BrcmLbs_TimerHandle;


typedef enum BrcmLbs_FactTestMode{
    BRCM_LBS_FACT_TEST_ONCE = 0,
    BRCM_LBS_FACT_TEST_CONT
}BrcmLbs_FactTestMode;


typedef enum BrcmLbs_FactTestItem{
    BRCM_LBS_FACT_TEST_CW = 0,
    BRCM_LBS_FACT_TEST_CN0,
    BRCM_LBS_FACT_TEST_FRQ,
    BRCM_LBS_FACT_TEST_WER,
    BRCM_LBS_FACT_TEST_ACQ,
}BrcmLbs_FactTestItem;

typedef enum BrcmLbs_CntinStatus {
    BRCM_LBS_CNTIN_NOT_USED=0,
    BRCM_LBS_CNTIN_OK,
    BRCM_LBS_CNTIN_NOK,
    BRCM_LBS_CNTIN_NO_INPUT,
    BRCM_LBS_CNTIN_USER_DEAD,
    BRCM_LBS_CNTIN_USER_CANCEL,
}BrcmLbs_CntinStatus;

typedef struct BrcmLbs_FactTestInfo{
    BrcmLbs_FactTestItem TestItem;
    int SVid;
    int EnergyFound;
    int AvgValid;
    double LastSSdBm;
    double AvgSSdBm;

    double LastSNRdbHZ;
    double AvgSNRdBHZ;

    double NoiseFigure;
    int FrequencyMeasured;

    double LastFreqPpu;
    double LastFreqUncPpu;

    int WerMeasured;
    unsigned int GoodWordCnt;
    unsigned int TotalWordCnt;

    BrcmLbs_CntinStatus CntInStatus;
    int CntinMeasured;
    double CntinOffsetPpu;

    int RtcMeasured;
    double RtcOffsetPpu;

    int RfAgc;

    unsigned int SignalDropCount;

    int ValidCW;
    double DopplerPpuCW;
    double DopplerUncPpuCW;
    double SSdBmCW;

} BrcmLbs_FactTestInfo;


typedef struct {
    int ref_locationid;
    size_t len;
    unsigned char refloc[0];
} BrcmLbs_RefLocation_h;

typedef struct {
    int ref_locationid;
    unsigned int len;
} BrcmLbs_GsmCellId;

#ifdef __cplusplus
}
#endif

#endif  /* _BRCM_LBS_TYPES_H_ */
