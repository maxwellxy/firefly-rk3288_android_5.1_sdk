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
 * @defgroup ov5630_d50   Illumination Profile D50
 * @{
 *
 */
#ifndef __OV5647_D50_H__
#define __OV5647_D50_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define AWB_COLORMATRIX_ARRAY_SIZE_CIE_D50  2
#define AWB_LSCMATRIX_ARRAY_SIZE_CIE_D50    1

#define AWB_SATURATION_ARRAY_SIZE_CIE_D50   4
#define AWB_VIGNETTING_ARRAY_SIZE_CIE_D50   2

/*****************************************************************************/
/*!
 * Sunny Outdoor Profile of sunny daylight
 *  color temperature was ~5000 Kelvin during measurement
 */
/*****************************************************************************/

// crosstalk matrix
const Isi3x3FloatMatrix_t  OV5647_XTalkCoeff_D50 =
{
    {
         1.73618f,  -0.45240f,  -0.28378f, 
        -0.24785f,   1.40716f,  -0.15931f, 
         0.02490f,  -0.61683f,   1.59193f 
    }
};

// crosstalk offset matrix
const IsiXTalkFloatOffset_t OV5647_XTalkOffset_D50 =
{
    .fCtOffsetRed      = (-127.0625f / CC_OFFSET_SCALING),
    .fCtOffsetGreen    = (-126.6875f / CC_OFFSET_SCALING),
    .fCtOffsetBlue     = (-121.5625f / CC_OFFSET_SCALING)
};

// gain matrix
const IsiComponentGain_t OV5647_CompGain_D50 =
{
    .fRed      = 1.77781f,  
    .fGreenR   = 1.00000f,
    .fGreenB   = 1.00000f,
    .fBlue     = 1.31855f
};

// mean value of gaussian mixture model
const Isi2x1FloatMatrix_t OV5647_GaussMeanValue_D50 =
{
    {
        0.07044f,  0.06202f 
    }
};

// inverse covariance matrix
const Isi2x2FloatMatrix_t OV5647_CovarianceMatrix_D50 =
{
    {
        512.05323f,  -330.54996f, 
       -330.54996f,  1817.50973f
    }
};

// factor in gaussian mixture model
const IsiGaussFactor_t OV5647_GaussFactor_D50 =
{
    .fGaussFactor = 144.24377f 
};

// thresholds for switching between MAP classification and interpolation
const Isi2x1FloatMatrix_t OV5647_Threshold_D50 =
{
    {
        1.0f, 1.0f // 1 = disabled
    }
};

// saturation curve
float afSaturationSensorGain_D50[AWB_SATURATION_ARRAY_SIZE_CIE_D50] =
{
    1.0f, 2.0f, 4.0f, 8.0f
};

float afSaturation_D50[AWB_SATURATION_ARRAY_SIZE_CIE_D50] =
{
    100.0f, 100.0f, 90.0f, 74.0f
};

const IsiSaturationCurve_t OV5647_SaturationCurve_D50 =
{
    .ArraySize      = AWB_SATURATION_ARRAY_SIZE_CIE_D50,
    .pSensorGain    = &afSaturationSensorGain_D50[0],
    .pSaturation    = &afSaturation_D50[0]
};

// saturation depended color conversion matrices
IsiSatCcMatrix_t OV5647_SatCcMatrix_D50[AWB_COLORMATRIX_ARRAY_SIZE_CIE_D50] =
{
    {
        .fSaturation    = 74.0f,
        .XTalkCoeff     =
        {
            {
                1.54704f,  -0.29736f,  -0.24968f, 
                0.11426f,   1.04855f,  -0.16281f, 
                0.31047f,  -0.42725f,   1.11678f    
            }
        }
    },
    {
        .fSaturation    = 100.0f,
        .XTalkCoeff     =
        {
            {
                1.73618f,  -0.45240f,  -0.28378f, 
               -0.24785f,   1.40716f,  -0.15931f, 
                0.02490f,  -0.61683f,   1.59193f  
            }
        }
    }
};

const IsiCcMatrixTable_t OV5647_CcMatrixTable_D50 =
{
    .ArraySize          = AWB_COLORMATRIX_ARRAY_SIZE_CIE_D50,
    .pIsiSatCcMatrix    = &OV5647_SatCcMatrix_D50[0]
};

// saturation depended color conversion offset vectors
IsiSatCcOffset_t OV5647_SatCcOffset_D50[AWB_COLORMATRIX_ARRAY_SIZE_CIE_D50] =
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
            .fCtOffsetRed      = (-127.0625f / CC_OFFSET_SCALING),
            .fCtOffsetGreen    = (-126.6875f / CC_OFFSET_SCALING),
            .fCtOffsetBlue     = (-121.5625f / CC_OFFSET_SCALING)
        }
    }
};

const IsiCcOffsetTable_t OV5647_CcOffsetTable_D50=
{
    .ArraySize          = AWB_COLORMATRIX_ARRAY_SIZE_CIE_D50,
    .pIsiSatCcOffset    = &OV5647_SatCcOffset_D50[0]
};

// vignetting curve
float afVignettingSensorGain_D50[AWB_VIGNETTING_ARRAY_SIZE_CIE_D50] =
{
    1.0f, 8.0f
};

float afVignetting_D50[AWB_VIGNETTING_ARRAY_SIZE_CIE_D50] =
{
    100.0f, 100.0f
};

const IsiVignettingCurve_t OV5647_VignettingCurve_D50 =
{
    .ArraySize      = AWB_VIGNETTING_ARRAY_SIZE_CIE_D50,
    .pSensorGain    = &afVignettingSensorGain_D50[0],
    .pVignetting    = &afVignetting_D50[0]
};

// vignetting dependend lsc matrices
IsiVignLscMatrix_t OV5647_VignLscMatrix_CIE_D50_1920x1080[AWB_LSCMATRIX_ARRAY_SIZE_CIE_D50] = 
{
    // array item 0
    {
       .fVignetting    = 100.0f,
       .LscMatrix      =
       {
           // ISI_COLOR_COMPONENT_RED
           {
               {
                    1437U,  1367U,  1308U,  1247U,  1209U,  1154U,  1136U,  1123U,  1118U,  1127U,  1146U,  1172U,  1216U,  1269U,  1328U,  1388U,  1444U,
                    1412U,  1351U,  1284U,  1230U,  1186U,  1148U,  1120U,  1105U,  1100U,  1103U,  1126U,  1163U,  1197U,  1251U,  1308U,  1365U,  1428U,
                    1394U,  1329U,  1266U,  1209U,  1166U,  1122U,  1100U,  1081U,  1078U,  1088U,  1110U,  1135U,  1180U,  1229U,  1289U,  1352U,  1411U,
                    1387U,  1311U,  1249U,  1199U,  1148U,  1112U,  1083U,  1065U,  1066U,  1072U,  1089U,  1128U,  1166U,  1216U,  1271U,  1332U,  1399U,
                    1364U,  1302U,  1236U,  1176U,  1132U,  1097U,  1073U,  1053U,  1050U,  1059U,  1086U,  1112U,  1150U,  1201U,  1256U,  1321U,  1384U,
                    1354U,  1285U,  1227U,  1170U,  1124U,  1089U,  1057U,  1044U,  1037U,  1045U,  1068U,  1104U,  1141U,  1190U,  1250U,  1307U,  1375U,
                    1343U,  1280U,  1211U,  1164U,  1113U,  1079U,  1054U,  1033U,  1037U,  1043U,  1059U,  1094U,  1137U,  1188U,  1239U,  1300U,  1371U,
                    1339U,  1274U,  1213U,  1156U,  1109U,  1073U,  1043U,  1035U,  1024U,  1039U,  1057U,  1089U,  1134U,  1177U,  1234U,  1299U,  1354U,
                    1335U,  1275U,  1210U,  1158U,  1111U,  1069U,  1048U,  1030U,  1026U,  1037U,  1057U,  1093U,  1127U,  1180U,  1231U,  1294U,  1363U,
                    1348U,  1276U,  1214U,  1153U,  1114U,  1076U,  1050U,  1031U,  1033U,  1038U,  1063U,  1089U,  1135U,  1182U,  1239U,  1302U,  1359U,
                    1351U,  1278U,  1213U,  1161U,  1116U,  1081U,  1053U,  1042U,  1037U,  1045U,  1066U,  1094U,  1137U,  1184U,  1242U,  1302U,  1373U,
                    1356U,  1297U,  1232U,  1176U,  1130U,  1090U,  1066U,  1050U,  1045U,  1056U,  1074U,  1108U,  1149U,  1196U,  1254U,  1315U,  1386U,
                    1376U,  1309U,  1238U,  1189U,  1138U,  1104U,  1077U,  1064U,  1060U,  1067U,  1090U,  1121U,  1162U,  1212U,  1262U,  1332U,  1392U,
                    1392U,  1323U,  1263U,  1199U,  1161U,  1124U,  1097U,  1076U,  1074U,  1081U,  1107U,  1137U,  1174U,  1232U,  1283U,  1349U,  1412U,
                    1414U,  1347U,  1282U,  1230U,  1177U,  1145U,  1114U,  1104U,  1097U,  1106U,  1126U,  1159U,  1194U,  1246U,  1304U,  1365U,  1438U,
                    1442U,  1362U,  1306U,  1244U,  1203U,  1162U,  1138U,  1121U,  1113U,  1126U,  1146U,  1179U,  1221U,  1270U,  1327U,  1388U,  1450U,
                    1463U,  1390U,  1324U,  1275U,  1225U,  1187U,  1158U,  1148U,  1143U,  1148U,  1168U,  1201U,  1243U,  1292U,  1347U,  1422U,  1475U 
               }
           }, 
    
           // ISI_COLOR_COMPONENT_GREENR
           {
               {
                    1315U,  1275U,  1226U,  1188U,  1158U,  1135U,  1109U,  1101U,  1092U,  1106U,  1119U,  1148U,  1179U,  1214U,  1256U,  1298U,  1336U,
                    1298U,  1256U,  1213U,  1174U,  1143U,  1112U,  1094U,  1084U,  1083U,  1092U,  1103U,  1129U,  1159U,  1200U,  1239U,  1283U,  1333U,
                    1289U,  1243U,  1200U,  1156U,  1129U,  1100U,  1082U,  1070U,  1068U,  1074U,  1090U,  1115U,  1148U,  1183U,  1228U,  1271U,  1313U,
                    1275U,  1227U,  1185U,  1148U,  1114U,  1089U,  1071U,  1056U,  1056U,  1064U,  1078U,  1106U,  1135U,  1171U,  1213U,  1257U,  1299U,
                    1264U,  1221U,  1173U,  1134U,  1104U,  1078U,  1060U,  1047U,  1045U,  1053U,  1067U,  1093U,  1123U,  1157U,  1202U,  1245U,  1288U,
                    1258U,  1211U,  1167U,  1129U,  1093U,  1069U,  1050U,  1037U,  1036U,  1044U,  1061U,  1082U,  1116U,  1151U,  1194U,  1241U,  1280U,
                    1253U,  1202U,  1156U,  1117U,  1089U,  1063U,  1044U,  1031U,  1028U,  1036U,  1052U,  1079U,  1109U,  1145U,  1188U,  1232U,  1273U,
                    1241U,  1198U,  1155U,  1116U,  1080U,  1057U,  1037U,  1027U,  1024U,  1033U,  1049U,  1071U,  1107U,  1142U,  1185U,  1228U,  1273U,
                    1245U,  1200U,  1151U,  1115U,  1083U,  1054U,  1038U,  1027U,  1024U,  1031U,  1051U,  1072U,  1103U,  1139U,  1180U,  1228U,  1266U,
                    1249U,  1197U,  1155U,  1118U,  1083U,  1058U,  1040U,  1028U,  1026U,  1035U,  1050U,  1076U,  1104U,  1143U,  1186U,  1228U,  1277U,
                    1245U,  1207U,  1162U,  1119U,  1088U,  1059U,  1046U,  1032U,  1035U,  1038U,  1054U,  1083U,  1110U,  1146U,  1186U,  1233U,  1279U,
                    1260U,  1210U,  1161U,  1127U,  1096U,  1071U,  1053U,  1042U,  1038U,  1050U,  1066U,  1086U,  1115U,  1156U,  1197U,  1237U,  1290U,
                    1267U,  1220U,  1176U,  1136U,  1104U,  1079U,  1061U,  1050U,  1050U,  1058U,  1069U,  1095U,  1125U,  1163U,  1203U,  1246U,  1293U,
                    1282U,  1235U,  1190U,  1147U,  1118U,  1089U,  1072U,  1063U,  1061U,  1068U,  1085U,  1110U,  1138U,  1179U,  1213U,  1263U,  1301U,
                    1292U,  1246U,  1201U,  1162U,  1131U,  1105U,  1086U,  1077U,  1074U,  1085U,  1097U,  1124U,  1151U,  1192U,  1231U,  1275U,  1316U,
                    1309U,  1263U,  1217U,  1177U,  1144U,  1119U,  1099U,  1091U,  1093U,  1100U,  1115U,  1143U,  1169U,  1212U,  1246U,  1289U,  1340U,
                    1324U,  1276U,  1229U,  1197U,  1164U,  1136U,  1119U,  1107U,  1110U,  1116U,  1132U,  1156U,  1190U,  1226U,  1267U,  1313U,  1348U 
               },
           },
    
           // ISI_COLOR_COMPONENT_GREENB
           {
               {
                    1320U,  1276U,  1227U,  1189U,  1159U,  1136U,  1110U,  1101U,  1096U,  1107U,  1122U,  1148U,  1179U,  1217U,  1257U,  1297U,  1340U,
                    1297U,  1257U,  1216U,  1174U,  1144U,  1113U,  1094U,  1087U,  1083U,  1091U,  1104U,  1130U,  1160U,  1201U,  1239U,  1287U,  1333U,
                    1290U,  1244U,  1201U,  1159U,  1129U,  1102U,  1083U,  1071U,  1068U,  1074U,  1091U,  1116U,  1150U,  1183U,  1229U,  1273U,  1314U,
                    1280U,  1228U,  1187U,  1148U,  1115U,  1089U,  1070U,  1058U,  1057U,  1065U,  1078U,  1107U,  1136U,  1173U,  1212U,  1258U,  1300U,
                    1263U,  1222U,  1173U,  1134U,  1103U,  1079U,  1059U,  1047U,  1045U,  1052U,  1068U,  1093U,  1123U,  1157U,  1204U,  1244U,  1291U,
                    1260U,  1212U,  1168U,  1129U,  1095U,  1070U,  1050U,  1037U,  1037U,  1045U,  1060U,  1085U,  1116U,  1150U,  1196U,  1240U,  1282U,
                    1255U,  1203U,  1158U,  1119U,  1089U,  1063U,  1043U,  1033U,  1029U,  1038U,  1052U,  1080U,  1110U,  1146U,  1187U,  1232U,  1276U,
                    1243U,  1199U,  1156U,  1118U,  1079U,  1058U,  1037U,  1028U,  1024U,  1033U,  1050U,  1072U,  1106U,  1142U,  1185U,  1228U,  1272U,
                    1245U,  1201U,  1153U,  1114U,  1085U,  1053U,  1039U,  1027U,  1024U,  1033U,  1051U,  1071U,  1104U,  1139U,  1181U,  1229U,  1271U,
                    1251U,  1193U,  1157U,  1117U,  1084U,  1059U,  1039U,  1028U,  1028U,  1035U,  1050U,  1077U,  1104U,  1142U,  1186U,  1228U,  1276U,
                    1247U,  1208U,  1163U,  1119U,  1087U,  1058U,  1045U,  1032U,  1033U,  1038U,  1054U,  1081U,  1110U,  1147U,  1186U,  1234U,  1277U,
                    1256U,  1212U,  1161U,  1128U,  1097U,  1071U,  1053U,  1040U,  1039U,  1049U,  1065U,  1087U,  1116U,  1156U,  1197U,  1238U,  1290U,
                    1265U,  1218U,  1176U,  1135U,  1103U,  1079U,  1060U,  1052U,  1050U,  1055U,  1068U,  1094U,  1125U,  1163U,  1201U,  1245U,  1293U,
                    1278U,  1237U,  1188U,  1147U,  1116U,  1091U,  1073U,  1061U,  1060U,  1068U,  1084U,  1110U,  1138U,  1178U,  1214U,  1262U,  1302U,
                    1293U,  1244U,  1203U,  1161U,  1129U,  1103U,  1085U,  1077U,  1075U,  1082U,  1098U,  1121U,  1152U,  1192U,  1230U,  1275U,  1316U,
                    1307U,  1261U,  1215U,  1175U,  1144U,  1119U,  1099U,  1091U,  1091U,  1098U,  1113U,  1142U,  1170U,  1209U,  1246U,  1289U,  1340U,
                    1324U,  1278U,  1231U,  1195U,  1163U,  1137U,  1118U,  1106U,  1110U,  1114U,  1133U,  1156U,  1191U,  1225U,  1269U,  1312U,  1349U 
               },
           },
    
           // ISI_COLOR_COMPONENT_BLUE
           {
               {
                    1289U,  1248U,  1208U,  1177U,  1141U,  1117U,  1092U,  1083U,  1083U,  1083U,  1094U,  1113U,  1141U,  1167U,  1201U,  1245U,  1280U,
                    1281U,  1238U,  1197U,  1160U,  1128U,  1100U,  1085U,  1069U,  1065U,  1067U,  1085U,  1099U,  1124U,  1158U,  1191U,  1224U,  1265U,
                    1269U,  1226U,  1185U,  1148U,  1115U,  1094U,  1070U,  1063U,  1058U,  1062U,  1070U,  1089U,  1116U,  1143U,  1178U,  1216U,  1252U,
                    1263U,  1217U,  1182U,  1136U,  1114U,  1084U,  1065U,  1050U,  1045U,  1049U,  1064U,  1084U,  1103U,  1136U,  1172U,  1208U,  1252U,
                    1251U,  1213U,  1166U,  1135U,  1098U,  1075U,  1057U,  1044U,  1042U,  1047U,  1056U,  1073U,  1100U,  1131U,  1162U,  1199U,  1243U,
                    1251U,  1205U,  1163U,  1126U,  1097U,  1069U,  1046U,  1037U,  1031U,  1035U,  1049U,  1068U,  1092U,  1118U,  1160U,  1193U,  1238U,
                    1239U,  1197U,  1159U,  1116U,  1093U,  1063U,  1046U,  1032U,  1027U,  1031U,  1047U,  1062U,  1090U,  1120U,  1156U,  1195U,  1230U,
                    1240U,  1197U,  1156U,  1121U,  1081U,  1060U,  1044U,  1027U,  1026U,  1029U,  1040U,  1065U,  1081U,  1115U,  1149U,  1187U,  1227U,
                    1244U,  1195U,  1157U,  1118U,  1090U,  1061U,  1039U,  1031U,  1024U,  1027U,  1041U,  1060U,  1089U,  1111U,  1148U,  1186U,  1231U,
                    1238U,  1201U,  1159U,  1122U,  1090U,  1068U,  1044U,  1033U,  1027U,  1032U,  1048U,  1060U,  1091U,  1116U,  1149U,  1195U,  1221U,
                    1248U,  1201U,  1162U,  1127U,  1098U,  1071U,  1049U,  1040U,  1030U,  1041U,  1047U,  1072U,  1092U,  1120U,  1158U,  1195U,  1234U,
                    1255U,  1211U,  1167U,  1130U,  1103U,  1073U,  1059U,  1050U,  1040U,  1047U,  1061U,  1078U,  1101U,  1128U,  1165U,  1199U,  1238U,
                    1259U,  1225U,  1178U,  1144U,  1111U,  1089U,  1069U,  1057U,  1054U,  1060U,  1069U,  1091U,  1112U,  1142U,  1171U,  1218U,  1248U,
                    1278U,  1228U,  1190U,  1151U,  1128U,  1097U,  1077U,  1068U,  1063U,  1069U,  1082U,  1101U,  1120U,  1157U,  1188U,  1221U,  1260U,
                    1290U,  1242U,  1203U,  1166U,  1136U,  1106U,  1091U,  1079U,  1080U,  1081U,  1095U,  1114U,  1137U,  1163U,  1199U,  1239U,  1280U,
                    1296U,  1254U,  1211U,  1177U,  1151U,  1125U,  1105U,  1094U,  1092U,  1094U,  1112U,  1126U,  1155U,  1187U,  1218U,  1256U,  1295U,
                    1313U,  1274U,  1224U,  1198U,  1157U,  1134U,  1118U,  1109U,  1107U,  1114U,  1130U,  1144U,  1171U,  1204U,  1248U,  1278U,  1316U 
               },
           },
       },
    },
};

IsiLscMatrixTable_t OV5647_LscMatrixTable_CIE_D50_1920x1080 = 
{
    .ArraySize          = AWB_LSCMATRIX_ARRAY_SIZE_CIE_D50,
    .psIsiVignLscMatrix = &OV5647_VignLscMatrix_CIE_D50_1920x1080[0],
    .LscXGradTbl        = { 324U,  301U,  293U,  273U,  264U,  254U,  246U,  248U },
    .LscYGradTbl        = { 520U,  504U,  496U,  482U,  475U,  482U,  468U,  462U },
    .LscXSizeTbl        = { 101U,  109U,  112U,  120U,  124U,  129U,  133U,  132U },
    .LscYSizeTbl        = {  63U,   65U,   66U,   68U,   69U,   68U,   70U,   71U } 
};



#ifdef __cplusplus
}
#endif

/* @} ov5630_d50 */

#endif /* __OV5647_D50_H__ */

