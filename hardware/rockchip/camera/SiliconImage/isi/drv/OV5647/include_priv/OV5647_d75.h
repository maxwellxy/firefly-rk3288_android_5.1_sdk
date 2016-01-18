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
 * @defgroup ov5630_D75   Illumination Profile D65
 * @{
 *
 */
#ifndef __OV5647_D75_H__
#define __OV5647_D75_H__

#ifdef __cplusplus
extern "C"
{
#endif


#define AWB_COLORMATRIX_ARRAY_SIZE_CIE_D75  2
#define AWB_LSCMATRIX_ARRAY_SIZE_CIE_D75    1

#define AWB_SATURATION_ARRAY_SIZE_CIE_D75   4
#define AWB_VIGNETTING_ARRAY_SIZE_CIE_D75   2


/*****************************************************************************/
/*!
 * CIE D75:
 *  overcast daylight, 7500K
 * This illumination is not tuned for this sensor correctly! This color profile
 * might not yield satisfying results.
 */
/*****************************************************************************/

// crosstalk matrix
const Isi3x3FloatMatrix_t  OV5647_XTalkCoeff_D75 =
{
    {
        1.72406f,  -0.44927f,  -0.27478f, 
       -0.19818f,   1.51700f,  -0.31882f, 
        0.04300f,  -0.59480f,   1.55180f  
    }
};

// crosstalk offset matrix
const IsiXTalkFloatOffset_t OV5647_XTalkOffset_D75 =
{
    .fCtOffsetRed      = (-132.6875f / CC_OFFSET_SCALING),
    .fCtOffsetGreen    = (-133.1250f / CC_OFFSET_SCALING),
    .fCtOffsetBlue     = (-146.6875f / CC_OFFSET_SCALING)
};

// gain matrix
const IsiComponentGain_t OV5647_CompGain_D75 =
{
    .fRed      = 2.15769f,  
    .fGreenR   = 1.03435f,  
    .fGreenB   = 1.03435f,  
    .fBlue     = 1.00000f 
};

// mean value of gaussian mixture model
const Isi2x1FloatMatrix_t OV5647_GaussMeanValue_D75 =
{
    {
       0.17154f,  0.03529f 
    }
};

// inverse covariance matrix
const Isi2x2FloatMatrix_t OV5647_CovarianceMatrix_D75 =
{
    {
        450.89459f,  17.44826f, 
        17.44826f,  1856.43362f
    }
};

// factor in gaussian mixture model
const IsiGaussFactor_t OV5647_GaussFactor_D75 =
{
    .fGaussFactor = 145.58558f 
};

// thresholds for switching between MAP classification and interpolation
const Isi2x1FloatMatrix_t OV5647_Threshold_D75 =
{
    {
        1.00000f,  1.00000f
    }
};

// saturation curve
float afSaturationSensorGain_D75[AWB_SATURATION_ARRAY_SIZE_CIE_D75] =
{
    1.0f, 2.0f, 4.0f, 8.0f
};

float afSaturation_D75[AWB_SATURATION_ARRAY_SIZE_CIE_D75] =
{
    100.0f, 100.0f, 90.0f, 74.0f
};

const IsiSaturationCurve_t OV5647_SaturationCurve_D75 =
{
    .ArraySize      = AWB_SATURATION_ARRAY_SIZE_CIE_D75,
    .pSensorGain    = &afSaturationSensorGain_D75[0],
    .pSaturation    = &afSaturation_D75[0]
};

// saturation depended color conversion matrices
IsiSatCcMatrix_t OV5647_SatCcMatrix_D75[AWB_COLORMATRIX_ARRAY_SIZE_CIE_D75] =
{
    {
        .fSaturation    = 74.0f,
        .XTalkCoeff     =
        {
            {
                1.53764f,  -0.29385f,  -0.24379f, 
                0.14570f,   1.13199f,  -0.27769f, 
                0.31902f,  -0.41018f,   1.09116f  
            }
        }
    },
    {
        .fSaturation    = 100.0f,
        .XTalkCoeff     =
        {
            {
                 1.72406f,  -0.44927f,  -0.27478f, 
                -0.19818f,   1.51700f,  -0.31882f, 
                 0.04300f,  -0.59480f,   1.55180f   
            }
        }
    }
 };

const IsiCcMatrixTable_t OV5647_CcMatrixTable_D75 =
{
    .ArraySize          = AWB_COLORMATRIX_ARRAY_SIZE_CIE_D75,
    .pIsiSatCcMatrix    = &OV5647_SatCcMatrix_D75[0]
};

// saturation depended color conversion offset vectors
IsiSatCcOffset_t OV5647_SatCcOffset_D75[AWB_COLORMATRIX_ARRAY_SIZE_CIE_D75] =
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
            .fCtOffsetRed      = (-132.6875f / CC_OFFSET_SCALING),
            .fCtOffsetGreen    = (-133.1250f / CC_OFFSET_SCALING),
            .fCtOffsetBlue     = (-146.6875f / CC_OFFSET_SCALING)
        }
    }
};

const IsiCcOffsetTable_t OV5647_CcOffsetTable_D75=
{
    .ArraySize          = AWB_COLORMATRIX_ARRAY_SIZE_CIE_D75,
    .pIsiSatCcOffset    = &OV5647_SatCcOffset_D75[0]
};

// vignetting curve
float afVignettingSensorGain_D75[AWB_VIGNETTING_ARRAY_SIZE_CIE_D75] =
{
    1.0f, 8.0f
};

float afVignetting_D75[AWB_VIGNETTING_ARRAY_SIZE_CIE_D75] =
{
    100.0f, 100.0f
};

const IsiVignettingCurve_t OV5647_VignettingCurve_D75 =
{
    .ArraySize      = AWB_VIGNETTING_ARRAY_SIZE_CIE_D75,
    .pSensorGain    = &afVignettingSensorGain_D75[0],
    .pVignetting    = &afVignetting_D75[0]
};

// vignetting dependend lsc matrices
IsiVignLscMatrix_t OV5647_VignLscMatrix_CIE_D75_1920x1080[AWB_LSCMATRIX_ARRAY_SIZE_CIE_D75] = 
{
    // array item 0
    {
       .fVignetting    = 100.0f,
       .LscMatrix      =
       {
           // ISI_COLOR_COMPONENT_RED
           {
               {
                    1414U,  1350U,  1293U,  1237U,  1191U,  1149U,  1131U,  1111U,  1115U,  1114U,  1139U,  1166U,  1207U,  1258U,  1313U,  1370U,  1432U,
                    1390U,  1326U,  1270U,  1212U,  1174U,  1133U,  1108U,  1094U,  1088U,  1097U,  1115U,  1158U,  1185U,  1238U,  1295U,  1350U,  1412U,
                    1368U,  1313U,  1251U,  1195U,  1153U,  1119U,  1092U,  1078U,  1071U,  1079U,  1102U,  1130U,  1165U,  1223U,  1271U,  1334U,  1401U,
                    1361U,  1292U,  1239U,  1182U,  1139U,  1102U,  1074U,  1064U,  1057U,  1067U,  1083U,  1121U,  1158U,  1200U,  1258U,  1321U,  1375U,
                    1347U,  1286U,  1224U,  1167U,  1125U,  1092U,  1064U,  1049U,  1048U,  1052U,  1076U,  1108U,  1145U,  1195U,  1246U,  1313U,  1369U,
                    1345U,  1268U,  1210U,  1159U,  1116U,  1078U,  1059U,  1036U,  1034U,  1044U,  1063U,  1099U,  1135U,  1180U,  1237U,  1294U,  1362U,
                    1325U,  1263U,  1202U,  1151U,  1110U,  1073U,  1047U,  1032U,  1031U,  1037U,  1056U,  1090U,  1130U,  1178U,  1229U,  1289U,  1355U,
                    1318U,  1258U,  1200U,  1149U,  1101U,  1064U,  1044U,  1028U,  1024U,  1034U,  1054U,  1083U,  1126U,  1169U,  1228U,  1287U,  1341U,
                    1328U,  1263U,  1200U,  1151U,  1105U,  1066U,  1042U,  1026U,  1024U,  1029U,  1056U,  1089U,  1122U,  1177U,  1220U,  1283U,  1348U,
                    1331U,  1262U,  1205U,  1144U,  1107U,  1069U,  1045U,  1033U,  1024U,  1038U,  1055U,  1089U,  1130U,  1174U,  1231U,  1290U,  1353U,
                    1332U,  1271U,  1204U,  1154U,  1112U,  1073U,  1050U,  1037U,  1034U,  1040U,  1066U,  1092U,  1129U,  1184U,  1230U,  1295U,  1354U,
                    1349U,  1278U,  1221U,  1164U,  1125U,  1085U,  1058U,  1048U,  1043U,  1053U,  1073U,  1109U,  1141U,  1193U,  1244U,  1305U,  1375U,
                    1364U,  1296U,  1226U,  1180U,  1133U,  1098U,  1074U,  1060U,  1057U,  1062U,  1085U,  1117U,  1153U,  1202U,  1258U,  1315U,  1381U,
                    1380U,  1311U,  1253U,  1192U,  1161U,  1116U,  1093U,  1078U,  1071U,  1081U,  1103U,  1135U,  1173U,  1225U,  1275U,  1332U,  1400U,
                    1398U,  1330U,  1279U,  1219U,  1174U,  1134U,  1111U,  1099U,  1094U,  1102U,  1124U,  1154U,  1190U,  1245U,  1293U,  1353U,  1426U,
                    1423U,  1356U,  1292U,  1238U,  1195U,  1155U,  1135U,  1115U,  1112U,  1123U,  1141U,  1173U,  1217U,  1261U,  1323U,  1383U,  1440U,
                    1444U,  1378U,  1314U,  1260U,  1219U,  1184U,  1150U,  1147U,  1140U,  1146U,  1167U,  1200U,  1237U,  1291U,  1338U,  1403U,  1458U 
               }
           }, 
    
           // ISI_COLOR_COMPONENT_GREENR
           {
               {
                    1305U,  1268U,  1221U,  1186U,  1154U,  1131U,  1110U,  1096U,  1095U,  1100U,  1117U,  1146U,  1171U,  1211U,  1252U,  1295U,  1335U,
                    1298U,  1250U,  1210U,  1169U,  1140U,  1111U,  1093U,  1083U,  1081U,  1091U,  1103U,  1128U,  1159U,  1196U,  1236U,  1283U,  1325U,
                    1280U,  1239U,  1196U,  1157U,  1125U,  1098U,  1080U,  1069U,  1066U,  1073U,  1090U,  1112U,  1144U,  1182U,  1225U,  1266U,  1313U,
                    1275U,  1222U,  1181U,  1146U,  1111U,  1087U,  1067U,  1054U,  1055U,  1063U,  1075U,  1102U,  1133U,  1169U,  1210U,  1255U,  1297U,
                    1261U,  1218U,  1171U,  1133U,  1103U,  1075U,  1059U,  1045U,  1045U,  1053U,  1066U,  1095U,  1124U,  1158U,  1199U,  1240U,  1290U,
                    1253U,  1207U,  1163U,  1123U,  1091U,  1068U,  1046U,  1034U,  1032U,  1042U,  1059U,  1079U,  1115U,  1149U,  1191U,  1237U,  1275U,
                    1248U,  1200U,  1155U,  1117U,  1086U,  1060U,  1045U,  1030U,  1028U,  1036U,  1052U,  1080U,  1107U,  1143U,  1186U,  1227U,  1272U,
                    1239U,  1196U,  1149U,  1114U,  1080U,  1054U,  1036U,  1026U,  1024U,  1034U,  1049U,  1069U,  1106U,  1140U,  1182U,  1222U,  1275U,
                    1246U,  1194U,  1150U,  1112U,  1083U,  1053U,  1039U,  1026U,  1025U,  1034U,  1049U,  1074U,  1103U,  1140U,  1182U,  1224U,  1266U,
                    1243U,  1194U,  1150U,  1117U,  1081U,  1056U,  1037U,  1027U,  1027U,  1032U,  1050U,  1074U,  1106U,  1141U,  1182U,  1225U,  1275U,
                    1247U,  1202U,  1159U,  1118U,  1086U,  1061U,  1044U,  1033U,  1031U,  1039U,  1055U,  1079U,  1109U,  1146U,  1185U,  1231U,  1277U,
                    1252U,  1205U,  1160U,  1127U,  1094U,  1070U,  1048U,  1039U,  1041U,  1045U,  1063U,  1085U,  1115U,  1151U,  1192U,  1239U,  1281U,
                    1262U,  1216U,  1172U,  1134U,  1101U,  1078U,  1063U,  1051U,  1046U,  1058U,  1070U,  1096U,  1125U,  1161U,  1202U,  1245U,  1295U,
                    1275U,  1230U,  1182U,  1147U,  1114U,  1088U,  1071U,  1062U,  1060U,  1067U,  1082U,  1108U,  1138U,  1178U,  1213U,  1259U,  1300U,
                    1290U,  1243U,  1198U,  1162U,  1127U,  1104U,  1083U,  1076U,  1070U,  1083U,  1098U,  1122U,  1153U,  1189U,  1228U,  1271U,  1320U,
                    1303U,  1256U,  1213U,  1172U,  1143U,  1118U,  1098U,  1092U,  1089U,  1100U,  1112U,  1141U,  1166U,  1207U,  1246U,  1288U,  1328U,
                    1326U,  1276U,  1228U,  1192U,  1161U,  1136U,  1118U,  1110U,  1108U,  1119U,  1133U,  1155U,  1187U,  1228U,  1264U,  1307U,  1355U 
               },
           },
    
           // ISI_COLOR_COMPONENT_GREENB
           {
               {
                    1310U,  1269U,  1223U,  1189U,  1157U,  1134U,  1112U,  1098U,  1099U,  1105U,  1123U,  1147U,  1172U,  1215U,  1257U,  1296U,  1332U, 
                    1295U,  1251U,  1212U,  1170U,  1142U,  1111U,  1095U,  1085U,  1084U,  1092U,  1106U,  1129U,  1161U,  1198U,  1237U,  1284U,  1326U,
                    1283U,  1241U,  1197U,  1159U,  1127U,  1099U,  1082U,  1071U,  1067U,  1076U,  1092U,  1115U,  1146U,  1184U,  1226U,  1266U,  1317U,
                    1275U,  1224U,  1182U,  1146U,  1112U,  1087U,  1069U,  1055U,  1057U,  1064U,  1076U,  1105U,  1134U,  1170U,  1211U,  1257U,  1298U,
                    1261U,  1216U,  1173U,  1133U,  1105U,  1077U,  1060U,  1045U,  1047U,  1054U,  1069U,  1095U,  1125U,  1159U,  1198U,  1242U,  1289U,
                    1252U,  1206U,  1163U,  1125U,  1092U,  1068U,  1049U,  1037U,  1034U,  1044U,  1060U,  1082U,  1116U,  1149U,  1194U,  1238U,  1276U,
                    1247U,  1199U,  1154U,  1118U,  1087U,  1061U,  1045U,  1032U,  1029U,  1038U,  1052U,  1082U,  1109U,  1143U,  1187U,  1228U,  1274U,
                    1238U,  1196U,  1153U,  1114U,  1082U,  1056U,  1039U,  1026U,  1025U,  1035U,  1050U,  1071U,  1108U,  1142U,  1182U,  1224U,  1275U,
                    1240U,  1196U,  1148U,  1113U,  1083U,  1052U,  1040U,  1025U,  1024U,  1035U,  1050U,  1073U,  1103U,  1140U,  1182U,  1224U,  1266U,
                    1244U,  1193U,  1151U,  1118U,  1082U,  1058U,  1037U,  1029U,  1029U,  1033U,  1049U,  1076U,  1105U,  1142U,  1180U,  1228U,  1270U,
                    1247U,  1200U,  1159U,  1118U,  1086U,  1059U,  1046U,  1033U,  1033U,  1040U,  1058U,  1079U,  1112U,  1144U,  1186U,  1232U,  1277U,
                    1250U,  1204U,  1160U,  1125U,  1095U,  1070U,  1048U,  1038U,  1040U,  1045U,  1062U,  1088U,  1115U,  1151U,  1192U,  1238U,  1282U,
                    1260U,  1214U,  1173U,  1133U,  1100U,  1078U,  1063U,  1052U,  1046U,  1057U,  1070U,  1095U,  1125U,  1163U,  1204U,  1245U,  1294U,
                    1271U,  1228U,  1181U,  1144U,  1115U,  1089U,  1071U,  1063U,  1061U,  1067U,  1081U,  1108U,  1138U,  1178U,  1212U,  1259U,  1299U,
                    1289U,  1242U,  1196U,  1161U,  1126U,  1105U,  1083U,  1075U,  1070U,  1083U,  1099U,  1119U,  1152U,  1187U,  1228U,  1270U,  1320U,
                    1300U,  1254U,  1212U,  1172U,  1143U,  1118U,  1098U,  1093U,  1090U,  1098U,  1112U,  1141U,  1165U,  1207U,  1246U,  1286U,  1330U,
                    1324U,  1275U,  1229U,  1191U,  1163U,  1138U,  1120U,  1110U,  1110U,  1119U,  1135U,  1155U,  1192U,  1226U,  1268U,  1310U,  1354U 
               },
           },
    
           // ISI_COLOR_COMPONENT_BLUE
           {
               {
                    1310U,  1260U,  1221U,  1187U,  1149U,  1126U,  1097U,  1088U,  1084U,  1090U,  1099U,  1119U,  1144U,  1177U,  1215U,  1252U,  1282U,
                    1292U,  1253U,  1206U,  1171U,  1134U,  1108U,  1094U,  1074U,  1071U,  1073U,  1087U,  1107U,  1134U,  1165U,  1198U,  1235U,  1275U,
                    1282U,  1235U,  1195U,  1157U,  1128U,  1098U,  1078U,  1061U,  1061U,  1064U,  1075U,  1097U,  1120U,  1148U,  1187U,  1227U,  1260U,
                    1271U,  1228U,  1189U,  1144U,  1116U,  1085U,  1066U,  1055U,  1051U,  1053U,  1065U,  1084U,  1110U,  1142U,  1179U,  1216U,  1252U,
                    1261U,  1220U,  1177U,  1142U,  1104U,  1082U,  1059U,  1045U,  1043U,  1045U,  1058U,  1079U,  1101U,  1133U,  1169U,  1206U,  1251U,
                    1262U,  1215U,  1169U,  1133U,  1098U,  1074U,  1050U,  1040U,  1032U,  1043U,  1049U,  1071U,  1095U,  1126U,  1161U,  1199U,  1243U,
                    1254U,  1205U,  1165U,  1123U,  1096U,  1067U,  1046U,  1033U,  1031U,  1031U,  1048U,  1064U,  1093U,  1122U,  1159U,  1199U,  1240U,
                    1246U,  1204U,  1158U,  1125U,  1089U,  1064U,  1045U,  1028U,  1024U,  1031U,  1044U,  1062U,  1085U,  1116U,  1155U,  1194U,  1232U,
                    1248U,  1204U,  1161U,  1126U,  1089U,  1062U,  1043U,  1028U,  1025U,  1030U,  1044U,  1062U,  1088U,  1114U,  1151U,  1189U,  1230U,
                    1247U,  1207U,  1161U,  1126U,  1095U,  1067U,  1049U,  1033U,  1027U,  1033U,  1049U,  1063U,  1089U,  1114U,  1154U,  1198U,  1227U,
                    1254U,  1206U,  1165U,  1127U,  1100U,  1069U,  1051U,  1037U,  1034U,  1036U,  1046U,  1070U,  1094U,  1122U,  1158U,  1198U,  1235U,
                    1261U,  1217U,  1174U,  1133U,  1108U,  1077U,  1060U,  1049U,  1040U,  1048U,  1058U,  1079U,  1103U,  1133U,  1166U,  1207U,  1245U,
                    1270U,  1227U,  1180U,  1146U,  1110U,  1090U,  1067U,  1057U,  1052U,  1058U,  1070U,  1089U,  1112U,  1142U,  1173U,  1218U,  1253U,
                    1284U,  1233U,  1195U,  1158U,  1129U,  1095U,  1080U,  1067U,  1062U,  1070U,  1080U,  1100U,  1124U,  1155U,  1187U,  1227U,  1269U,
                    1289U,  1250U,  1203U,  1166U,  1138U,  1108U,  1092U,  1078U,  1076U,  1084U,  1091U,  1112U,  1136U,  1170U,  1199U,  1239U,  1278U,
                    1306U,  1262U,  1219U,  1178U,  1152U,  1124U,  1104U,  1095U,  1091U,  1094U,  1110U,  1128U,  1153U,  1182U,  1220U,  1259U,  1301U,
                    1310U,  1277U,  1228U,  1203U,  1160U,  1139U,  1118U,  1105U,  1104U,  1116U,  1123U,  1144U,  1173U,  1206U,  1240U,  1285U,  1309U 
               },
           },
       },
    },
};

IsiLscMatrixTable_t OV5647_LscMatrixTable_CIE_D75_1920x1080 = 
{
    .ArraySize          = AWB_LSCMATRIX_ARRAY_SIZE_CIE_D75,
    .psIsiVignLscMatrix = &OV5647_VignLscMatrix_CIE_D75_1920x1080[0],
    .LscXGradTbl        = { 324U,  301U,  293U,  273U,  264U,  254U,  246U,  248U },
    .LscYGradTbl        = { 520U,  504U,  496U,  482U,  475U,  482U,  468U,  462U },
    .LscXSizeTbl        = { 101U,  109U,  112U,  120U,  124U,  129U,  133U,  132U },
    .LscYSizeTbl        = {  63U,   65U,   66U,   68U,   69U,   68U,   70U,   71U } 
};



#ifdef __cplusplus
}
#endif

/* @} ov5630_D75 */

#endif /* __OV5647_D75_H__ */

