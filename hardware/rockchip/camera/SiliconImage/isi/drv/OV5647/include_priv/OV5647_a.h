/******************************************************************************
 *
 * Copyright 2010, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
/**
 * @file isi_iss.h
 *
 * @brief Interface description for image sensor specific implementation (iss).
 *
 *****************************************************************************/
/**
 * @page module_name_page Module Name
 * Describe here what this module does.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref module_name
 *
 * @defgroup ov5630_a   Illumination Profile A
 * @{
 *
 */
#ifndef __OV5647_A_H__
#define __OV5647_A_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define AWB_COLORMATRIX_ARRAY_SIZE_CIE_A    2
#define AWB_LSCMATRIX_ARRAY_SIZE_CIE_A      1

#define AWB_SATURATION_ARRAY_SIZE_CIE_A     4
#define AWB_VIGNETTING_ARRAY_SIZE_CIE_A     2

#define CC_OFFSET_SCALING_A                 4.0f

/*****************************************************************************/
/**
 * CIE PROFILE A:
 *
 *  incandescent/tungsten, 2856K
 */
/*****************************************************************************/
// crosstalk matrix
const Isi3x3FloatMatrix_t  OV5647_XTalkCoeff_CIE_A =
{
    {
          1.75526f,  -0.58101f,  -0.17425f, 
         -0.39342f,   1.31770f,   0.07572f, 
         -0.11799f,  -0.14063f,   1.25862f 
    }
};

// crosstalk offset matrix
const IsiXTalkFloatOffset_t OV5647_XTalkOffset_CIE_A =
{
    .fCtOffsetRed   = (-129.5f      / CC_OFFSET_SCALING_A),
    .fCtOffsetGreen = (-119.5f      / CC_OFFSET_SCALING_A),
    .fCtOffsetBlue  = (-124.4375f   / CC_OFFSET_SCALING_A)
};

// gain matrix
const IsiComponentGain_t OV5647_CompGain_CIE_A =
{
    .fRed      = 1.09016f,  
    .fGreenR   = 1.00000f,  
    .fGreenB   = 1.00000f, 
    .fBlue     = 2.59572f
};

// mean value of gaussian mixture model
const Isi2x1FloatMatrix_t OV5647_GaussMeanValue_CIE_A =
{
    {
       -0.15107f,  0.03529f
    }
};

// inverse covariance matrix
const Isi2x2FloatMatrix_t OV5647_CovarianceMatrix_CIE_A =
{
    {
        1382.35513f,  -1400.44794f, 
       -1400.44794f,  2490.11422f
    }
};

// factor in gaussian mixture model
const IsiGaussFactor_t OV5647_GaussFactor_CIE_A =
 {
    .fGaussFactor = 193.68364f
 };

// thresholds for switching between MAP classification and interpolation
const Isi2x1FloatMatrix_t OV5647_Threshold_CIE_A =
{
    {
        1.00000f,  1.00000f
    }
};

// saturation curve for F11 profile
float afSaturationSensorGain_CIE_A[AWB_SATURATION_ARRAY_SIZE_CIE_A] =
{
    1.0f, 2.0f, 4.0f, 8.0f
};

float afSaturation_CIE_A[AWB_SATURATION_ARRAY_SIZE_CIE_A] =
{
    100.0f, 100.0f, 90.0f, 74.0f
};

const IsiSaturationCurve_t OV5647_SaturationCurve_CIE_A =
{
    .ArraySize      = AWB_SATURATION_ARRAY_SIZE_CIE_A,
    .pSensorGain    = &afSaturationSensorGain_CIE_A[0],
    .pSaturation    = &afSaturation_CIE_A[0]
};

// saturation depended color conversion matrices
IsiSatCcMatrix_t OV5647_SatCcMatrix_CIE_A[AWB_COLORMATRIX_ARRAY_SIZE_CIE_A] =
{
    {
        .fSaturation    = 74.0f,
        .XTalkCoeff     =
        {
            {
                 1.50764f,  -0.31796f,  -0.18968f, 
                -0.00812f,   1.02221f,  -0.01409f, 
                 0.18594f,  -0.01869f,   0.83274f  
            }
        }
    },
    {
        .fSaturation    = 100.0f,
        .XTalkCoeff     =
        {
            {
                 1.75526f,  -0.58101f,  -0.17425f, 
                -0.39342f,   1.31770f,   0.07572f, 
                -0.11799f,  -0.14063f,   1.25862f  
            }
        }
    }
};

const IsiCcMatrixTable_t OV5647_CcMatrixTable_CIE_A =
{
    .ArraySize          = AWB_COLORMATRIX_ARRAY_SIZE_CIE_A,
    .pIsiSatCcMatrix    = &OV5647_SatCcMatrix_CIE_A[0]
};

// saturation depended color conversion offset vectors
IsiSatCcOffset_t OV5647_SatCcOffset_CIE_A[AWB_COLORMATRIX_ARRAY_SIZE_CIE_A] =
{
    {
        .fSaturation    = 74.0f,
        .CcOffset       =
        {
            .fCtOffsetRed   = 0.0f,
            .fCtOffsetGreen = 0.0f,
            .fCtOffsetBlue  = 0.0f
        }
    },
    {
        .fSaturation    = 100.0f,
        .CcOffset       =
        {
            .fCtOffsetRed   = (-129.5f      / CC_OFFSET_SCALING_A),
            .fCtOffsetGreen = (-119.5f      / CC_OFFSET_SCALING_A),
            .fCtOffsetBlue  = (-124.4375f   / CC_OFFSET_SCALING_A)
        }
    }
};

const IsiCcOffsetTable_t OV5647_CcOffsetTable_CIE_A =
{
    .ArraySize          = AWB_COLORMATRIX_ARRAY_SIZE_CIE_A,
    .pIsiSatCcOffset    = &OV5647_SatCcOffset_CIE_A[0]
};

// vignetting curve
float afVignettingSensorGain_CIE_A[AWB_VIGNETTING_ARRAY_SIZE_CIE_A] =
{
    1.0f, 8.0f
};

float afVignetting_CIE_A[AWB_VIGNETTING_ARRAY_SIZE_CIE_A] =
{
    100.0f, 100.0f
};

const IsiVignettingCurve_t OV5647_VignettingCurve_CIE_A =
{
    .ArraySize      = AWB_VIGNETTING_ARRAY_SIZE_CIE_A,
    .pSensorGain    = &afVignettingSensorGain_CIE_A[0],
    .pVignetting    = &afVignetting_CIE_A[0]
};



// vignetting dependend lsc matrices ( 1080pXX  1920x1080 )
IsiVignLscMatrix_t OV5647_VignLscMatrix_CIE_A_1920x1080[AWB_LSCMATRIX_ARRAY_SIZE_CIE_A] = 
{
    // array item 0
    {
       .fVignetting    = 100.0f,
       .LscMatrix      =
       {
           // ISI_COLOR_COMPONENT_RED
           {
               {
                    1494U,  1418U,  1350U,  1286U,  1228U,  1184U,  1163U,  1145U,  1135U,  1149U,  1166U,  1203U,  1255U,  1311U,  1376U,  1442U,  1525U,
                    1467U,  1392U,  1321U,  1260U,  1212U,  1166U,  1135U,  1120U,  1115U,  1121U,  1149U,  1187U,  1225U,  1287U,  1355U,  1422U,  1497U,
                    1442U,  1373U,  1299U,  1237U,  1184U,  1140U,  1112U,  1094U,  1088U,  1101U,  1121U,  1161U,  1204U,  1265U,  1327U,  1401U,  1474U,
                    1429U,  1346U,  1281U,  1218U,  1164U,  1123U,  1093U,  1073U,  1071U,  1078U,  1105U,  1139U,  1188U,  1246U,  1309U,  1382U,  1451U,
                    1402U,  1340U,  1261U,  1200U,  1149U,  1107U,  1079U,  1057U,  1055U,  1062U,  1091U,  1127U,  1172U,  1227U,  1289U,  1369U,  1439U,
                    1398U,  1318U,  1249U,  1186U,  1134U,  1091U,  1063U,  1045U,  1036U,  1051U,  1072U,  1113U,  1157U,  1216U,  1280U,  1347U,  1424U,
                    1383U,  1308U,  1236U,  1174U,  1125U,  1081U,  1051U,  1032U,  1035U,  1039U,  1064U,  1102U,  1150U,  1208U,  1272U,  1339U,  1425U,
                    1371U,  1300U,  1230U,  1168U,  1116U,  1071U,  1044U,  1030U,  1024U,  1034U,  1060U,  1094U,  1143U,  1201U,  1261U,  1339U,  1408U,
                    1373U,  1301U,  1230U,  1168U,  1115U,  1071U,  1044U,  1026U,  1024U,  1035U,  1057U,  1094U,  1144U,  1197U,  1262U,  1329U,  1409U,
                    1380U,  1298U,  1230U,  1167U,  1116U,  1073U,  1044U,  1031U,  1027U,  1034U,  1061U,  1094U,  1142U,  1202U,  1265U,  1340U,  1405U,
                    1384U,  1306U,  1232U,  1173U,  1121U,  1082U,  1050U,  1037U,  1032U,  1040U,  1065U,  1102U,  1150U,  1204U,  1270U,  1337U,  1422U,
                    1392U,  1321U,  1248U,  1187U,  1136U,  1089U,  1061U,  1043U,  1045U,  1052U,  1078U,  1116U,  1164U,  1218U,  1279U,  1355U,  1427U,
                    1413U,  1337U,  1258U,  1201U,  1142U,  1108U,  1076U,  1062U,  1056U,  1066U,  1091U,  1124U,  1174U,  1231U,  1298U,  1366U,  1440U,
                    1431U,  1353U,  1286U,  1217U,  1170U,  1127U,  1097U,  1079U,  1075U,  1085U,  1111U,  1147U,  1192U,  1254U,  1312U,  1386U,  1462U,
                    1455U,  1379U,  1304U,  1247U,  1187U,  1150U,  1118U,  1104U,  1097U,  1106U,  1131U,  1168U,  1208U,  1272U,  1339U,  1411U,  1486U,
                    1471U,  1401U,  1334U,  1265U,  1215U,  1171U,  1146U,  1123U,  1122U,  1130U,  1158U,  1191U,  1242U,  1295U,  1362U,  1435U,  1505U,
                    1491U,  1428U,  1356U,  1292U,  1242U,  1200U,  1167U,  1161U,  1152U,  1158U,  1184U,  1220U,  1265U,  1329U,  1394U,  1459U,  1541U 
               },
           }, 
    
           // ISI_COLOR_COMPONENT_GREENR
           {
               {
                    1323U,  1279U,  1230U,  1194U,  1160U,  1140U,  1118U,  1102U,  1105U,  1107U,  1130U,  1151U,  1183U,  1221U,  1265U,  1311U,  1350U, 
                    1302U,  1261U,  1216U,  1178U,  1145U,  1116U,  1102U,  1090U,  1088U,  1096U,  1110U,  1137U,  1168U,  1211U,  1248U,  1292U,  1344U,
                    1288U,  1247U,  1203U,  1161U,  1131U,  1104U,  1086U,  1073U,  1070U,  1078U,  1095U,  1123U,  1156U,  1190U,  1232U,  1280U,  1322U,
                    1281U,  1230U,  1188U,  1150U,  1116U,  1091U,  1070U,  1058U,  1059U,  1067U,  1083U,  1108U,  1140U,  1181U,  1221U,  1265U,  1309U,
                    1262U,  1221U,  1177U,  1137U,  1105U,  1078U,  1062U,  1046U,  1048U,  1052U,  1070U,  1099U,  1130U,  1165U,  1207U,  1252U,  1300U,
                    1266U,  1213U,  1168U,  1128U,  1097U,  1070U,  1049U,  1037U,  1036U,  1044U,  1063U,  1086U,  1123U,  1157U,  1201U,  1248U,  1288U,
                    1255U,  1206U,  1160U,  1121U,  1091U,  1062U,  1045U,  1032U,  1028U,  1040U,  1054U,  1086U,  1115U,  1153U,  1196U,  1239U,  1291U,
                    1245U,  1199U,  1157U,  1119U,  1081U,  1056U,  1039U,  1025U,  1028U,  1034U,  1050U,  1077U,  1113U,  1149U,  1193U,  1233U,  1285U,
                    1252U,  1198U,  1158U,  1114U,  1085U,  1057U,  1037U,  1026U,  1024U,  1035U,  1051U,  1077U,  1109U,  1145U,  1189U,  1237U,  1277U,
                    1250U,  1201U,  1155U,  1119U,  1085U,  1058U,  1040U,  1028U,  1025U,  1035U,  1053U,  1080U,  1109U,  1153U,  1193U,  1241U,  1286U,
                    1252U,  1206U,  1162U,  1122U,  1089U,  1064U,  1045U,  1034U,  1035U,  1040U,  1056U,  1085U,  1116U,  1153U,  1195U,  1241U,  1292U,
                    1263U,  1211U,  1166U,  1130U,  1097U,  1068U,  1052U,  1043U,  1038U,  1050U,  1066U,  1091U,  1122U,  1159U,  1204U,  1251U,  1291U,
                    1265U,  1226U,  1177U,  1139U,  1106U,  1082U,  1062U,  1053U,  1051U,  1058U,  1075U,  1104U,  1132U,  1173U,  1210U,  1256U,  1305U,
                    1279U,  1234U,  1189U,  1150U,  1119U,  1094U,  1076U,  1064U,  1065U,  1074U,  1087U,  1114U,  1142U,  1184U,  1221U,  1270U,  1312U,
                    1295U,  1250U,  1205U,  1164U,  1134U,  1108U,  1089U,  1081U,  1078U,  1088U,  1103U,  1132U,  1159U,  1200U,  1241U,  1286U,  1331U,
                    1311U,  1264U,  1216U,  1181U,  1145U,  1126U,  1106U,  1097U,  1097U,  1103U,  1121U,  1148U,  1177U,  1215U,  1255U,  1299U,  1342U,
                    1326U,  1283U,  1236U,  1200U,  1169U,  1140U,  1126U,  1117U,  1118U,  1125U,  1141U,  1162U,  1201U,  1237U,  1276U,  1320U,  1365U 
               }
           },
    
           // ISI_COLOR_COMPONENT_GREENB
           {
               {
                    1329U,  1280U,  1230U,  1194U,  1163U,  1139U,  1120U,  1101U,  1104U,  1107U,  1130U,  1154U,  1183U,  1222U,  1268U,  1311U,  1353U,
                    1304U,  1264U,  1219U,  1180U,  1144U,  1115U,  1100U,  1090U,  1084U,  1094U,  1110U,  1135U,  1168U,  1210U,  1249U,  1295U,  1343U,
                    1293U,  1250U,  1203U,  1162U,  1133U,  1103U,  1085U,  1073U,  1070U,  1077U,  1094U,  1123U,  1156U,  1191U,  1235U,  1283U,  1327U,
                    1284U,  1232U,  1191U,  1150U,  1116U,  1091U,  1069U,  1059U,  1056U,  1066U,  1082U,  1109U,  1141U,  1181U,  1222U,  1266U,  1317U,
                    1271U,  1225U,  1180U,  1139U,  1108U,  1081U,  1063U,  1046U,  1047U,  1054U,  1068U,  1098U,  1132U,  1168U,  1212U,  1256U,  1302U,
                    1266U,  1216U,  1173U,  1130U,  1100U,  1070U,  1050U,  1038U,  1035U,  1046U,  1063U,  1088U,  1123U,  1159U,  1205U,  1251U,  1293U,
                    1260U,  1209U,  1161U,  1125U,  1091U,  1064U,  1044U,  1033U,  1028U,  1039U,  1053U,  1085U,  1117U,  1154U,  1198U,  1242U,  1294U,
                    1251U,  1205U,  1161U,  1120U,  1083U,  1056U,  1038U,  1026U,  1025U,  1034U,  1052U,  1077U,  1114U,  1151U,  1195U,  1238U,  1288U,
                    1254U,  1202U,  1161U,  1118U,  1086U,  1058U,  1039U,  1027U,  1024U,  1034U,  1051U,  1077U,  1111U,  1147U,  1192U,  1240U,  1282U,
                    1258U,  1203U,  1160U,  1122U,  1087U,  1060U,  1040U,  1028U,  1026U,  1035U,  1053U,  1081U,  1112U,  1154U,  1195U,  1245U,  1289U,
                    1256U,  1212U,  1166U,  1124U,  1092U,  1064U,  1047U,  1034U,  1034U,  1039U,  1058U,  1086U,  1119U,  1154U,  1197U,  1246U,  1295U,
                    1265U,  1215U,  1169U,  1131U,  1097U,  1071U,  1052U,  1042U,  1038U,  1049U,  1066U,  1091U,  1122U,  1162U,  1206U,  1254U,  1295U,
                    1269U,  1229U,  1180U,  1141U,  1108U,  1081U,  1063U,  1050U,  1051U,  1057U,  1075U,  1104U,  1134U,  1174U,  1211U,  1257U,  1312U,
                    1281U,  1239U,  1193U,  1153U,  1122U,  1097U,  1078U,  1065U,  1063U,  1073U,  1087U,  1114U,  1144U,  1185U,  1225U,  1273U,  1316U,
                    1300U,  1253U,  1205U,  1166U,  1135U,  1109U,  1090U,  1080U,  1078U,  1089U,  1104U,  1132U,  1160U,  1200U,  1242U,  1288U,  1334U,
                    1317U,  1265U,  1221U,  1182U,  1147U,  1125U,  1105U,  1096U,  1096U,  1102U,  1120U,  1145U,  1179U,  1215U,  1258U,  1302U,  1346U,
                    1327U,  1286U,  1235U,  1201U,  1169U,  1142U,  1128U,  1119U,  1116U,  1125U,  1141U,  1164U,  1201U,  1238U,  1274U,  1324U,  1366U 
               }
           },
    
           // ISI_COLOR_COMPONENT_BLUE
           {
               {
                    1259U,  1215U,  1190U,  1155U,  1125U,  1102U,  1084U,  1074U,  1080U,  1077U,  1091U,  1109U,  1128U,  1160U,  1196U,  1230U,  1256U,
                    1243U,  1210U,  1171U,  1141U,  1116U,  1093U,  1079U,  1066U,  1059U,  1066U,  1083U,  1103U,  1126U,  1155U,  1184U,  1218U,  1256U,
                    1238U,  1196U,  1161U,  1129U,  1107U,  1081U,  1065U,  1056U,  1051U,  1057U,  1070U,  1088U,  1112U,  1149U,  1178U,  1207U,  1245U,
                    1229U,  1199U,  1160U,  1122U,  1095U,  1076U,  1058U,  1050U,  1042U,  1045U,  1064U,  1084U,  1107U,  1138U,  1171U,  1202U,  1240U,
                    1224U,  1193U,  1149U,  1123U,  1091U,  1068U,  1049U,  1041U,  1038U,  1045U,  1056U,  1082U,  1103U,  1132U,  1164U,  1197U,  1239U,
                    1220U,  1185U,  1148U,  1115U,  1086U,  1066U,  1046U,  1035U,  1027U,  1036U,  1052U,  1074U,  1100U,  1126U,  1161U,  1193U,  1227U,
                    1221U,  1179U,  1148U,  1105U,  1088U,  1059U,  1039U,  1030U,  1024U,  1033U,  1048U,  1069U,  1100U,  1128U,  1160U,  1196U,  1230U,
                    1220U,  1180U,  1145U,  1113U,  1079U,  1056U,  1041U,  1029U,  1028U,  1029U,  1048U,  1073U,  1094U,  1125U,  1159U,  1193U,  1226U,
                    1218U,  1180U,  1150U,  1116U,  1089U,  1055U,  1039U,  1033U,  1027U,  1033U,  1047U,  1069U,  1101U,  1123U,  1158U,  1193U,  1230U,
                    1218U,  1187U,  1148U,  1114U,  1089U,  1064U,  1044U,  1033U,  1032U,  1037U,  1054U,  1072U,  1099U,  1129U,  1160U,  1197U,  1228U,
                    1229U,  1188U,  1151U,  1119U,  1094U,  1068U,  1053U,  1038U,  1036U,  1040U,  1055U,  1081U,  1107U,  1129U,  1167U,  1203U,  1234U,
                    1229U,  1199U,  1162U,  1127U,  1105U,  1074U,  1065U,  1053U,  1047U,  1051U,  1073U,  1089U,  1114U,  1147U,  1171U,  1206U,  1243U,
                    1245U,  1209U,  1167U,  1140U,  1111U,  1093U,  1073U,  1063U,  1057U,  1070U,  1078U,  1107U,  1129U,  1155U,  1184U,  1218U,  1258U,
                    1252U,  1215U,  1185U,  1145U,  1130U,  1098U,  1087U,  1077U,  1074U,  1082U,  1094U,  1113U,  1144U,  1166U,  1199U,  1233U,  1269U,
                    1264U,  1228U,  1194U,  1166U,  1133U,  1116U,  1100U,  1094U,  1084U,  1101U,  1110U,  1133U,  1151U,  1188U,  1213U,  1250U,  1282U,
                    1277U,  1240U,  1201U,  1173U,  1148U,  1128U,  1115U,  1105U,  1107U,  1110U,  1127U,  1147U,  1173U,  1197U,  1229U,  1264U,  1304U,
                    1301U,  1252U,  1214U,  1189U,  1158U,  1140U,  1129U,  1125U,  1110U,  1128U,  1141U,  1157U,  1188U,  1214U,  1256U,  1278U,  1325U 
               },
           },
       },
    },
};

IsiLscMatrixTable_t OV5647_LscMatrixTable_CIE_A_1920x1080 = 
{
    .ArraySize          = AWB_LSCMATRIX_ARRAY_SIZE_CIE_A,
    .psIsiVignLscMatrix = &OV5647_VignLscMatrix_CIE_A_1920x1080[0],
    .LscXGradTbl        = { 324U,  301U,  293U,  273U,  264U,  254U,  246U,  248U },
    .LscYGradTbl        = { 520U,  504U,  496U,  482U,  475U,  482U,  468U,  462U },
    .LscXSizeTbl        = { 101U,  109U,  112U,  120U,  124U,  129U,  133U,  132U },
    .LscYSizeTbl        = {  63U,   65U,   66U,   68U,   69U,   68U,   70U,   71U }
};



#ifdef __cplusplus
}
#endif

/* @} ov5630_a */

#endif /* __OV5647_A_H__ */

