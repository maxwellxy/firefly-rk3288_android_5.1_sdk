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
 * @defgroup ov5630_D65   Illumination Profile D65
 * @{
 *
 */
#ifndef __OV5647_D65_H__
#define __OV5647_D65_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define AWB_COLORMATRIX_ARRAY_SIZE_CIE_D65  2
#define AWB_LSCMATRIX_ARRAY_SIZE_CIE_D65    1

#define AWB_SATURATION_ARRAY_SIZE_CIE_D65   4
#define AWB_VIGNETTING_ARRAY_SIZE_CIE_D65   2

#define CC_OFFSET_SCALING_D65               12.0f

/*****************************************************************************/
/*!
 * CIE D65 Indoor Profile
 *  noon daylight, 6504K
 * This illumination is not tuned for this sensor correctly! This color profile
 * might not yield satisfying results.
 */
/*****************************************************************************/

// crosstalk matrix
const Isi3x3FloatMatrix_t  OV5647_XTalkCoeff_D65 =
{
    {
         1.74172f,  -0.47289f,  -0.26882f, 
        -0.21875f,   1.48801f,  -0.26926f, 
         0.04133f,  -0.60102f,   1.55969f   
    }
};

// crosstalk offset matrix
const IsiXTalkFloatOffset_t OV5647_XTalkOffset_D65 =
{
    .fCtOffsetRed      = (-345.4375f / CC_OFFSET_SCALING_D65),
    .fCtOffsetGreen    = (-341.8750f / CC_OFFSET_SCALING_D65),
    .fCtOffsetBlue     = (-344.9375f / CC_OFFSET_SCALING_D65)
};

// gain matrix
const IsiComponentGain_t OV5647_CompGain_D65 =
{
    .fRed      = 1.95780f,  
    .fGreenR   = 1.00000f,
    .fGreenB   = 1.00000f,
    .fBlue     = 1.03839f 
};

// mean value of gaussian mixture model
const Isi2x1FloatMatrix_t OV5647_GaussMeanValue_D65 =
{
    {
        0.14386f,  0.03936f 
    }
};

// inverse covariance matrix
const Isi2x2FloatMatrix_t OV5647_CovarianceMatrix_D65 =
{
    {
        450.88272f,  -85.00798f, 
        -85.00798f,  1818.81931f 
    }
};

// factor in gaussian mixture model
const IsiGaussFactor_t OV5647_GaussFactor_D65 =
{
    .fGaussFactor = 143.49103f 
};

// thresholds for switching between MAP classification and interpolation
const Isi2x1FloatMatrix_t OV5647_Threshold_D65 =
{
    {
        1.0f, 1.0f // 1 = disabled
    }
};

// saturation curve
float afSaturationSensorGain_D65[AWB_SATURATION_ARRAY_SIZE_CIE_D65] =
{
    1.0f, 2.0f, 4.0f, 8.0f
};

float afSaturation_D65[AWB_SATURATION_ARRAY_SIZE_CIE_D65] =
{
    100.0f, 100.0f, 90.0f, 74.0f
};

const IsiSaturationCurve_t OV5647_SaturationCurve_D65 =
{
    .ArraySize      = AWB_SATURATION_ARRAY_SIZE_CIE_D65,
    .pSensorGain    = &afSaturationSensorGain_D65[0],
    .pSaturation    = &afSaturation_D65[0]
};

// saturation depended color conversion matrices
IsiSatCcMatrix_t OV5647_SatCcMatrix_D65[AWB_COLORMATRIX_ARRAY_SIZE_CIE_D65] =
{
    {
        .fSaturation    = 74.0f,
        .XTalkCoeff     =
        {
            {
                1.55544f,  -0.31765f,  -0.23778f, 
                0.13584f,   1.10459f,  -0.24043f, 
                0.32310f,  -0.42160f,   1.09850f
            }
        }
    },
    {
        .fSaturation    = 100.0f,
        .XTalkCoeff     =
        {
            {
                 1.74172f,  -0.47289f,  -0.26882f, 
                -0.21875f,   1.48801f,  -0.26926f, 
                 0.04133f,  -0.60102f,   1.55969f   
            }
        }
    }
 };

const IsiCcMatrixTable_t OV5647_CcMatrixTable_D65 =
{
    .ArraySize          = AWB_COLORMATRIX_ARRAY_SIZE_CIE_D65,
    .pIsiSatCcMatrix    = &OV5647_SatCcMatrix_D65[0]
};

// saturation depended color conversion offset vectors
IsiSatCcOffset_t OV5647_SatCcOffset_D65[AWB_COLORMATRIX_ARRAY_SIZE_CIE_D65] =
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
            .fCtOffsetRed      = (-345.4375f / CC_OFFSET_SCALING_D65),
            .fCtOffsetGreen    = (-341.8750f / CC_OFFSET_SCALING_D65),
            .fCtOffsetBlue     = (-344.9375f / CC_OFFSET_SCALING_D65)
        }
    }
};

const IsiCcOffsetTable_t OV5647_CcOffsetTable_D65=
{
    .ArraySize          = AWB_COLORMATRIX_ARRAY_SIZE_CIE_D65,
    .pIsiSatCcOffset    = &OV5647_SatCcOffset_D65[0]
};

// vignetting curve
float afVignettingSensorGain_D65[AWB_VIGNETTING_ARRAY_SIZE_CIE_D65] =
{
    1.0f, 8.0f
};

float afVignetting_D65[AWB_VIGNETTING_ARRAY_SIZE_CIE_D65] =
{
    100.0f, 100.0f
};

const IsiVignettingCurve_t OV5647_VignettingCurve_D65 =
{
    .ArraySize      = AWB_VIGNETTING_ARRAY_SIZE_CIE_D65,
    .pSensorGain    = &afVignettingSensorGain_D65[0],
    .pVignetting    = &afVignetting_D65[0]
};

// vignetting dependend lsc matrices
IsiVignLscMatrix_t OV5647_VignLscMatrix_CIE_D65_1920x1080[AWB_LSCMATRIX_ARRAY_SIZE_CIE_D65] = 
{
    // array item 0
    {
       .fVignetting    = 100.0f,
       .LscMatrix      =
       {
           // ISI_COLOR_COMPONENT_RED
           {
               {
                    1427U,  1368U,  1304U,  1236U,  1198U,  1155U,  1131U,  1119U,  1113U,  1123U,  1141U,  1172U,  1208U,  1258U,  1318U,  1374U,  1442U,
                    1404U,  1339U,  1279U,  1228U,  1178U,  1140U,  1115U,  1100U,  1092U,  1099U,  1118U,  1158U,  1189U,  1240U,  1293U,  1353U,  1418U,
                    1381U,  1323U,  1262U,  1204U,  1161U,  1120U,  1092U,  1078U,  1071U,  1085U,  1102U,  1132U,  1173U,  1221U,  1275U,  1339U,  1399U,
                    1368U,  1308U,  1244U,  1191U,  1144U,  1111U,  1078U,  1057U,  1060U,  1063U,  1090U,  1116U,  1159U,  1208U,  1262U,  1324U,  1385U,
                    1359U,  1298U,  1237U,  1178U,  1132U,  1092U,  1066U,  1050U,  1046U,  1053U,  1079U,  1105U,  1148U,  1192U,  1250U,  1311U,  1373U,
                    1352U,  1281U,  1218U,  1168U,  1121U,  1086U,  1056U,  1038U,  1034U,  1040U,  1062U,  1098U,  1136U,  1188U,  1238U,  1297U,  1363U,
                    1342U,  1276U,  1212U,  1162U,  1116U,  1077U,  1048U,  1033U,  1030U,  1036U,  1054U,  1091U,  1129U,  1186U,  1230U,  1295U,  1356U,
                    1337U,  1274U,  1208U,  1157U,  1111U,  1071U,  1045U,  1026U,  1025U,  1031U,  1054U,  1086U,  1125U,  1172U,  1231U,  1285U,  1357U,
                    1341U,  1275U,  1212U,  1164U,  1116U,  1074U,  1048U,  1029U,  1024U,  1032U,  1055U,  1086U,  1126U,  1179U,  1225U,  1289U,  1347U,
                    1346U,  1280U,  1212U,  1159U,  1114U,  1073U,  1050U,  1028U,  1025U,  1034U,  1051U,  1087U,  1129U,  1175U,  1229U,  1291U,  1354U,
                    1351U,  1279U,  1220U,  1167U,  1120U,  1081U,  1052U,  1040U,  1031U,  1036U,  1066U,  1092U,  1133U,  1183U,  1231U,  1299U,  1355U,
                    1368U,  1298U,  1235U,  1180U,  1132U,  1094U,  1067U,  1047U,  1042U,  1050U,  1075U,  1108U,  1143U,  1193U,  1250U,  1309U,  1376U,
                    1375U,  1310U,  1241U,  1191U,  1142U,  1105U,  1078U,  1059U,  1055U,  1064U,  1083U,  1119U,  1160U,  1209U,  1259U,  1319U,  1385U,
                    1395U,  1326U,  1263U,  1211U,  1166U,  1127U,  1096U,  1081U,  1070U,  1082U,  1103U,  1139U,  1172U,  1225U,  1280U,  1339U,  1403U,
                    1416U,  1349U,  1283U,  1227U,  1185U,  1144U,  1120U,  1101U,  1092U,  1104U,  1122U,  1157U,  1192U,  1247U,  1298U,  1361U,  1423U,
                    1441U,  1370U,  1310U,  1252U,  1207U,  1167U,  1139U,  1121U,  1116U,  1128U,  1147U,  1175U,  1220U,  1263U,  1323U,  1386U,  1440U,
                    1448U,  1390U,  1325U,  1273U,  1227U,  1190U,  1161U,  1153U,  1144U,  1148U,  1168U,  1205U,  1244U,  1294U,  1342U,  1408U,  1470U 
               }
           }, 
    
           // ISI_COLOR_COMPONENT_GREENR
           {
               {
                    1323U,  1280U,  1230U,  1196U,  1159U,  1138U,  1117U,  1102U,  1103U,  1105U,  1122U,  1145U,  1178U,  1219U,  1254U,  1301U,  1341U,
                    1304U,  1262U,  1218U,  1179U,  1151U,  1116U,  1100U,  1089U,  1087U,  1095U,  1108U,  1131U,  1166U,  1199U,  1243U,  1287U,  1330U,
                    1297U,  1251U,  1204U,  1168U,  1130U,  1106U,  1085U,  1073U,  1071U,  1077U,  1094U,  1119U,  1149U,  1188U,  1230U,  1273U,  1315U,
                    1278U,  1233U,  1193U,  1149U,  1121U,  1093U,  1072U,  1058U,  1061U,  1062U,  1082U,  1105U,  1136U,  1175U,  1211U,  1259U,  1305U,
                    1274U,  1228U,  1181U,  1145U,  1111U,  1082U,  1064U,  1051U,  1048U,  1058U,  1070U,  1098U,  1129U,  1161U,  1206U,  1246U,  1298U,
                    1260U,  1221U,  1174U,  1133U,  1100U,  1077U,  1053U,  1042U,  1040U,  1045U,  1061U,  1086U,  1119U,  1154U,  1195U,  1244U,  1281U,
                    1263U,  1209U,  1164U,  1127U,  1094U,  1066U,  1050U,  1031U,  1031U,  1039U,  1055U,  1080U,  1114U,  1146U,  1188U,  1232U,  1282U,
                    1249U,  1207U,  1160U,  1123U,  1090U,  1060U,  1043U,  1032U,  1028U,  1037U,  1050U,  1077U,  1109U,  1145U,  1186U,  1228U,  1277U,
                    1256U,  1207U,  1163U,  1122U,  1089U,  1063U,  1042U,  1029U,  1024U,  1037U,  1051U,  1076U,  1104U,  1144U,  1184U,  1227U,  1274U,
                    1251U,  1211U,  1163U,  1126U,  1090U,  1063U,  1047U,  1032U,  1029U,  1037U,  1054U,  1080U,  1107U,  1145U,  1190U,  1231U,  1278U,
                    1260U,  1215U,  1170U,  1126U,  1098U,  1069U,  1052U,  1034U,  1034U,  1042U,  1059U,  1083U,  1114U,  1149U,  1189U,  1234U,  1281U,
                    1266U,  1219U,  1175U,  1136U,  1103U,  1076U,  1055U,  1045U,  1043U,  1051U,  1066U,  1089U,  1120U,  1158U,  1197U,  1241U,  1289U,
                    1275U,  1231U,  1186U,  1146U,  1112U,  1087U,  1070U,  1055U,  1051U,  1062U,  1074U,  1101U,  1131U,  1166U,  1209U,  1250U,  1297U,
                    1291U,  1244U,  1197U,  1160U,  1125U,  1099U,  1081U,  1065U,  1065U,  1071U,  1089U,  1114U,  1141U,  1183U,  1215U,  1265U,  1316U,
                    1302U,  1255U,  1211U,  1169U,  1139U,  1111U,  1090U,  1084U,  1076U,  1089U,  1101U,  1130U,  1156U,  1196U,  1235U,  1280U,  1318U,
                    1319U,  1273U,  1226U,  1188U,  1154U,  1126U,  1107U,  1097U,  1099U,  1104U,  1120U,  1147U,  1173U,  1213U,  1250U,  1294U,  1341U,
                    1336U,  1287U,  1246U,  1200U,  1176U,  1140U,  1128U,  1113U,  1112U,  1124U,  1136U,  1159U,  1192U,  1232U,  1272U,  1316U,  1356U 
               },
           },
    
           // ISI_COLOR_COMPONENT_GREENB
           {
               {
                    1325U,  1282U,  1234U,  1198U,  1163U,  1143U,  1118U,  1104U,  1103U,  1110U,  1125U,  1151U,  1180U,  1220U,  1257U,  1303U,  1342U, 
                    1304U,  1263U,  1218U,  1181U,  1150U,  1118U,  1100U,  1091U,  1087U,  1095U,  1110U,  1131U,  1166U,  1202U,  1243U,  1290U,  1331U,
                    1294U,  1252U,  1204U,  1169U,  1132U,  1106U,  1087U,  1075U,  1073U,  1079U,  1095U,  1120U,  1152U,  1187U,  1231U,  1274U,  1319U,
                    1280U,  1235U,  1194U,  1152U,  1121U,  1095U,  1072U,  1060U,  1063U,  1063U,  1085U,  1106U,  1138U,  1177U,  1212U,  1260U,  1307U,
                    1275U,  1227U,  1183U,  1145U,  1113U,  1084U,  1066U,  1053U,  1050U,  1059U,  1070U,  1100U,  1129U,  1164U,  1207U,  1249U,  1298U,
                    1263U,  1219U,  1174U,  1133U,  1099U,  1077U,  1053U,  1042U,  1039U,  1046U,  1063U,  1087U,  1120U,  1154U,  1197U,  1244U,  1281U,
                    1260U,  1210U,  1164U,  1127U,  1095U,  1066U,  1050U,  1033U,  1031U,  1040U,  1054U,  1082U,  1113U,  1145U,  1189U,  1232U,  1285U,
                    1247U,  1210U,  1160U,  1123U,  1089U,  1062U,  1043U,  1031U,  1028U,  1036U,  1053U,  1076U,  1111U,  1146U,  1187U,  1231U,  1273U,
                    1255U,  1206U,  1163U,  1121U,  1090U,  1062U,  1044U,  1029U,  1024U,  1038U,  1051U,  1077U,  1104U,  1145U,  1184U,  1228U,  1275U,
                    1253U,  1208U,  1165U,  1128U,  1092U,  1064U,  1046U,  1032U,  1029U,  1036U,  1054U,  1080U,  1109U,  1145U,  1188U,  1230U,  1280U,
                    1261U,  1213U,  1168U,  1126U,  1098U,  1068U,  1051U,  1034U,  1034U,  1043U,  1060U,  1082U,  1115U,  1147U,  1190U,  1236U,  1281U,
                    1268U,  1218U,  1176U,  1137U,  1106U,  1076U,  1057U,  1044U,  1044U,  1051U,  1066U,  1090U,  1121U,  1158U,  1197U,  1243U,  1290U,
                    1276U,  1229U,  1184U,  1147U,  1111U,  1088U,  1070U,  1056U,  1052U,  1061U,  1075U,  1100U,  1133U,  1167U,  1211U,  1251U,  1295U,
                    1288U,  1243U,  1197U,  1158U,  1127U,  1098U,  1080U,  1066U,  1065U,  1071U,  1087U,  1114U,  1140U,  1181U,  1216U,  1263U,  1314U,
                    1298U,  1255U,  1208U,  1171U,  1137U,  1112U,  1091U,  1084U,  1075U,  1090U,  1102U,  1128U,  1156U,  1195U,  1234U,  1280U,  1318U,
                    1318U,  1270U,  1226U,  1183U,  1155U,  1124U,  1107U,  1095U,  1098U,  1103U,  1118U,  1145U,  1174U,  1213U,  1250U,  1294U,  1339U,
                    1334U,  1288U,  1245U,  1205U,  1173U,  1144U,  1127U,  1117U,  1113U,  1125U,  1139U,  1161U,  1192U,  1234U,  1273U,  1317U,  1355U 
               },
           },
    
           // ISI_COLOR_COMPONENT_BLUE
           {
               {
                    1319U,  1265U,  1226U,  1192U,  1151U,  1126U,  1104U,  1091U,  1093U,  1086U,  1102U,  1121U,  1145U,  1183U,  1208U,  1255U,  1290U,
                    1297U,  1256U,  1211U,  1173U,  1142U,  1112U,  1092U,  1075U,  1070U,  1077U,  1089U,  1106U,  1133U,  1160U,  1201U,  1235U,  1274U,
                    1289U,  1241U,  1200U,  1164U,  1124U,  1104U,  1076U,  1065U,  1064U,  1066U,  1074U,  1101U,  1122U,  1153U,  1188U,  1225U,  1266U,
                    1277U,  1235U,  1194U,  1149U,  1123U,  1092U,  1069U,  1056U,  1049U,  1055U,  1066U,  1086U,  1113U,  1141U,  1175U,  1220U,  1249U,
                    1269U,  1226U,  1182U,  1143U,  1108U,  1083U,  1060U,  1047U,  1045U,  1046U,  1059U,  1081U,  1102U,  1136U,  1171U,  1206U,  1249U,
                    1265U,  1221U,  1175U,  1137U,  1107U,  1076U,  1056U,  1039U,  1033U,  1042U,  1049U,  1072U,  1100U,  1124U,  1165U,  1200U,  1246U,
                    1256U,  1212U,  1172U,  1129U,  1102U,  1071U,  1047U,  1037U,  1031U,  1034U,  1049U,  1068U,  1091U,  1123U,  1160U,  1200U,  1238U,
                    1260U,  1213U,  1166U,  1137U,  1092U,  1064U,  1048U,  1026U,  1031U,  1031U,  1041U,  1067U,  1088U,  1120U,  1155U,  1196U,  1234U,
                    1255U,  1215U,  1168U,  1130U,  1098U,  1068U,  1047U,  1033U,  1024U,  1031U,  1045U,  1060U,  1093U,  1118U,  1155U,  1191U,  1231U,
                    1263U,  1214U,  1171U,  1133U,  1101U,  1072U,  1050U,  1033U,  1032U,  1036U,  1047U,  1067U,  1093U,  1119U,  1155U,  1199U,  1233U,
                    1263U,  1218U,  1172U,  1138U,  1108U,  1076U,  1056U,  1041U,  1031U,  1039U,  1049U,  1076U,  1095U,  1127U,  1165U,  1201U,  1238U,
                    1271U,  1230U,  1183U,  1146U,  1116U,  1081U,  1064U,  1054U,  1044U,  1053U,  1062U,  1081U,  1106U,  1131U,  1172U,  1206U,  1248U,
                    1279U,  1238U,  1191U,  1152U,  1122U,  1098U,  1071U,  1059U,  1058U,  1059U,  1071U,  1091U,  1114U,  1147U,  1178U,  1217U,  1255U,
                    1294U,  1245U,  1206U,  1169U,  1135U,  1105U,  1086U,  1073U,  1063U,  1073U,  1082U,  1107U,  1126U,  1159U,  1191U,  1232U,  1268U,
                    1298U,  1260U,  1215U,  1173U,  1147U,  1115U,  1098U,  1085U,  1083U,  1086U,  1098U,  1119U,  1141U,  1172U,  1205U,  1240U,  1285U,
                    1314U,  1271U,  1223U,  1193U,  1161U,  1134U,  1113U,  1103U,  1093U,  1099U,  1117U,  1132U,  1158U,  1188U,  1229U,  1263U,  1302U,
                    1326U,  1287U,  1241U,  1206U,  1173U,  1143U,  1125U,  1113U,  1114U,  1115U,  1127U,  1150U,  1178U,  1212U,  1245U,  1286U,  1320U 
               },
           },
       },
    },
};

IsiLscMatrixTable_t OV5647_LscMatrixTable_CIE_D65_1920x1080 = 
{
    .ArraySize          = AWB_LSCMATRIX_ARRAY_SIZE_CIE_D65,
    .psIsiVignLscMatrix = &OV5647_VignLscMatrix_CIE_D65_1920x1080[0],
    .LscXGradTbl        = { 324U,  301U,  293U,  273U,  264U,  254U,  246U,  248U },
    .LscYGradTbl        = { 520U,  504U,  496U,  482U,  475U,  482U,  468U,  462U },
    .LscXSizeTbl        = { 101U,  109U,  112U,  120U,  124U,  129U,  133U,  132U },
    .LscYSizeTbl        = {  63U,   65U,   66U,   68U,   69U,   68U,   70U,   71U } 
};



#ifdef __cplusplus
}
#endif

/* @} ov5630_D65 */

#endif /* __OV5647_D65_H__ */

