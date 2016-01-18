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
 * @defgroup ov5630_F2   Illumination Profile F2
 * @{
 *
 */
#ifndef __OV5647_F2_H__
#define __OV5647_F2_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define AWB_COLORMATRIX_ARRAY_SIZE_CIE_F2   2
#define AWB_LSCMATRIX_ARRAY_SIZE_CIE_F2     1

#define AWB_SATURATION_ARRAY_SIZE_CIE_F2    4
#define AWB_VIGNETTING_ARRAY_SIZE_CIE_F2    2

#define CC_OFFSET_SCALING_F2                10.0f

/*****************************************************************************/
/*!
 * CIE F2:
 *  cool white flourescent CWF
 */
/*****************************************************************************/

// crosstalk matrix
const Isi3x3FloatMatrix_t  OV5647_XTalkCoeff_F2 =
{
    {
         2.03335f,  -0.74538f,  -0.28798f, 
        -0.29720f,   1.21625f,   0.08094f, 
        -0.04853f,  -0.47316f,   1.52170f  
    }
};

// crosstalk offset matrix
const IsiXTalkFloatOffset_t OV5647_XTalkOffset_F2 =
{
    .fCtOffsetRed   = (-195.8125f / CC_OFFSET_SCALING_F2),
    .fCtOffsetGreen = (-191.125f  / CC_OFFSET_SCALING_F2),
    .fCtOffsetBlue  = (-188.5625f / CC_OFFSET_SCALING_F2)
};

// gain matrix
const IsiComponentGain_t OV5647_CompGain_F2 =
{
    .fRed      = 1.73827f,  
    .fGreenR   = 1.00000f,
    .fGreenB   = 1.00000f,
    .fBlue     = 1.95747f 
};

// mean value of gaussian mixture model
const Isi2x1FloatMatrix_t OV5647_GaussMeanValue_F2 =
{
    {
       -0.01853f,  0.10955f 
    }
};

// inverse covariance matrix
const Isi2x2FloatMatrix_t OV5647_CovarianceMatrix_F2 =
{
    {
        589.96448f,  -250.10927f, 
       -250.10927f,  1676.57680f  
    }
};

// factor in gaussian mixture model
const IsiGaussFactor_t OV5647_GaussFactor_F2 =
{
    .fGaussFactor =  153.19985f 
};

// thresholds for switching between MAP classification and interpolation
const Isi2x1FloatMatrix_t OV5647_Threshold_F2 =
{
    {
        0.60000f,  0.90000f 
    }
};

// saturation curve
float afSaturationSensorGain_F2[AWB_SATURATION_ARRAY_SIZE_CIE_F2] =
{
    1.0f, 2.0f, 4.0f, 8.0f
};

float afSaturation_F2[AWB_SATURATION_ARRAY_SIZE_CIE_F2] =
{
    100.0f, 100.0f, 90.0f, 74.0f
};

const IsiSaturationCurve_t OV5647_SaturationCurve_F2 =
{
    .ArraySize      = AWB_SATURATION_ARRAY_SIZE_CIE_F2,
    .pSensorGain    = &afSaturationSensorGain_F2[0],
    .pSaturation    = &afSaturation_F2[0]
};

// saturation depended color conversion matrices
IsiSatCcMatrix_t OV5647_SatCcMatrix_F2[AWB_COLORMATRIX_ARRAY_SIZE_CIE_F2] =
{
    {
        .fSaturation    = 74.0f,
        .XTalkCoeff     =
        {
            {
                1.77884f,  -0.52285f,  -0.25599f, 
                0.10290f,   0.89232f,   0.00478f, 
                0.28040f,  -0.33406f,   1.05366f   
            }
        }
    },
    {
        .fSaturation    = 100.0f,
        .XTalkCoeff     =
        {
            {
                2.03335f,  -0.74538f,  -0.28798f, 
               -0.29720f,   1.21625f,   0.08094f, 
               -0.04853f,  -0.47316f,   1.52170f              
            }
        }
    }
 };

const IsiCcMatrixTable_t OV5647_CcMatrixTable_F2 =
{
    .ArraySize          = AWB_COLORMATRIX_ARRAY_SIZE_CIE_F2,
    .pIsiSatCcMatrix    = &OV5647_SatCcMatrix_F2[0]
};

// saturation depended color conversion offset vectors
IsiSatCcOffset_t OV5647_SatCcOffset_F2[AWB_COLORMATRIX_ARRAY_SIZE_CIE_F2] =
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
            .fCtOffsetRed   = (-195.8125f / CC_OFFSET_SCALING_F2),
            .fCtOffsetGreen = (-191.125f  / CC_OFFSET_SCALING_F2),
            .fCtOffsetBlue  = (-188.5625f / CC_OFFSET_SCALING_F2)
        }
    }
};

const IsiCcOffsetTable_t OV5647_CcOffsetTable_F2=
{
    .ArraySize          = AWB_COLORMATRIX_ARRAY_SIZE_CIE_F2,
    .pIsiSatCcOffset    = &OV5647_SatCcOffset_F2[0]
};

// vignetting curve
float afVignettingSensorGain_F2[AWB_VIGNETTING_ARRAY_SIZE_CIE_F2] =
{
    1.0f, 8.0f
};

float afVignetting_F2[AWB_VIGNETTING_ARRAY_SIZE_CIE_F2] =
{
    100.0f, 100.0f
};

const IsiVignettingCurve_t OV5647_VignettingCurve_F2 =
{
    .ArraySize      = AWB_VIGNETTING_ARRAY_SIZE_CIE_F2,
    .pSensorGain    = &afVignettingSensorGain_F2[0],
    .pVignetting    = &afVignetting_F2[0]
};

// vignetting dependend lsc matrices
IsiVignLscMatrix_t OV5647_VignLscMatrix_CIE_F2_1920x1080[AWB_LSCMATRIX_ARRAY_SIZE_CIE_F2] = 
{
    // array item 0
    {
       .fVignetting    = 100.0f,
       .LscMatrix      =
       {
           // ISI_COLOR_COMPONENT_RED
           {
               {
                    1379U,  1317U,  1273U,  1217U,  1179U,  1140U,  1126U,  1106U,  1101U,  1109U,  1127U,  1154U,  1187U,  1227U,  1268U,  1326U,  1378U,
                    1362U,  1302U,  1251U,  1202U,  1163U,  1130U,  1104U,  1093U,  1088U,  1091U,  1110U,  1140U,  1168U,  1211U,  1260U,  1306U,  1360U,
                    1339U,  1289U,  1230U,  1187U,  1144U,  1112U,  1087U,  1073U,  1070U,  1081U,  1094U,  1117U,  1153U,  1198U,  1243U,  1294U,  1351U,
                    1336U,  1272U,  1224U,  1172U,  1134U,  1098U,  1076U,  1061U,  1057U,  1060U,  1081U,  1110U,  1140U,  1180U,  1229U,  1281U,  1331U,
                    1317U,  1271U,  1209U,  1162U,  1121U,  1087U,  1063U,  1049U,  1049U,  1052U,  1069U,  1099U,  1130U,  1172U,  1216U,  1271U,  1324U,
                    1313U,  1250U,  1201U,  1152U,  1112U,  1081U,  1058U,  1040U,  1035U,  1042U,  1062U,  1087U,  1121U,  1164U,  1209U,  1260U,  1313U,
                    1304U,  1252U,  1197U,  1147U,  1108U,  1073U,  1049U,  1036U,  1033U,  1040U,  1052U,  1083U,  1116U,  1157U,  1206U,  1252U,  1310U,
                    1294U,  1241U,  1187U,  1145U,  1102U,  1066U,  1042U,  1030U,  1024U,  1032U,  1051U,  1075U,  1111U,  1149U,  1197U,  1252U,  1301U,
                    1304U,  1243U,  1193U,  1145U,  1104U,  1067U,  1045U,  1027U,  1026U,  1034U,  1051U,  1078U,  1111U,  1154U,  1197U,  1246U,  1311U,
                    1304U,  1248U,  1191U,  1143U,  1104U,  1068U,  1049U,  1031U,  1029U,  1034U,  1052U,  1074U,  1115U,  1152U,  1203U,  1251U,  1302U,
                    1314U,  1252U,  1196U,  1147U,  1107U,  1079U,  1051U,  1039U,  1033U,  1040U,  1057U,  1085U,  1116U,  1158U,  1204U,  1254U,  1310U,
                    1318U,  1262U,  1208U,  1164U,  1121U,  1084U,  1064U,  1046U,  1043U,  1048U,  1068U,  1096U,  1130U,  1170U,  1213U,  1266U,  1321U,
                    1337U,  1274U,  1218U,  1172U,  1129U,  1098U,  1071U,  1061U,  1050U,  1061U,  1075U,  1107U,  1137U,  1182U,  1226U,  1278U,  1332U,
                    1341U,  1293U,  1240U,  1184U,  1151U,  1111U,  1091U,  1073U,  1069U,  1071U,  1093U,  1120U,  1151U,  1198U,  1241U,  1290U,  1346U,
                    1369U,  1305U,  1254U,  1202U,  1163U,  1131U,  1104U,  1095U,  1082U,  1092U,  1112U,  1137U,  1166U,  1210U,  1257U,  1310U,  1361U,
                    1390U,  1325U,  1273U,  1219U,  1182U,  1147U,  1126U,  1108U,  1105U,  1111U,  1130U,  1152U,  1190U,  1231U,  1276U,  1329U,  1382U,
                    1394U,  1351U,  1288U,  1246U,  1201U,  1174U,  1141U,  1132U,  1127U,  1134U,  1145U,  1181U,  1208U,  1253U,  1302U,  1354U,  1396U 
               }
           }, 
    
           // ISI_COLOR_COMPONENT_GREENR
           {
               {
                    1321U,  1275U,  1230U,  1190U,  1159U,  1134U,  1112U,  1102U,  1098U,  1104U,  1122U,  1147U,  1175U,  1214U,  1255U,  1297U,  1335U,
                    1305U,  1255U,  1218U,  1179U,  1143U,  1116U,  1097U,  1087U,  1083U,  1091U,  1104U,  1128U,  1158U,  1193U,  1234U,  1279U,  1322U,
                    1292U,  1246U,  1200U,  1160U,  1129U,  1104U,  1083U,  1072U,  1069U,  1075U,  1089U,  1114U,  1145U,  1179U,  1222U,  1264U,  1307U,
                    1278U,  1232U,  1189U,  1149U,  1118U,  1090U,  1072U,  1056U,  1056U,  1062U,  1079U,  1102U,  1134U,  1166U,  1210U,  1247U,  1296U,
                    1271U,  1220U,  1175U,  1138U,  1105U,  1080U,  1063U,  1049U,  1044U,  1051U,  1067U,  1093U,  1121U,  1157U,  1195U,  1242U,  1288U,
                    1257U,  1215U,  1170U,  1128U,  1099U,  1069U,  1052U,  1037U,  1035U,  1043U,  1058U,  1081U,  1115U,  1145U,  1189U,  1232U,  1276U,
                    1255U,  1206U,  1160U,  1124U,  1090U,  1067U,  1046U,  1032U,  1030U,  1037U,  1051U,  1077U,  1105U,  1141U,  1184U,  1225U,  1272U,
                    1247U,  1204U,  1158U,  1117U,  1086U,  1058U,  1040U,  1027U,  1024U,  1033U,  1049U,  1070U,  1106U,  1138U,  1181U,  1221U,  1270U,
                    1252U,  1202U,  1159U,  1119U,  1088U,  1060U,  1042U,  1026U,  1027U,  1032U,  1048U,  1072U,  1101U,  1136U,  1180U,  1224U,  1263U,
                    1253U,  1204U,  1158U,  1122U,  1088U,  1060U,  1042U,  1030U,  1025U,  1034U,  1052U,  1073U,  1103U,  1139U,  1180U,  1224U,  1267U,
                    1257U,  1209U,  1168U,  1125U,  1094U,  1064U,  1048U,  1034U,  1031U,  1040U,  1053U,  1080U,  1108U,  1142U,  1185U,  1225U,  1272U,
                    1261U,  1216U,  1171U,  1133U,  1100U,  1073U,  1054U,  1040U,  1039U,  1047U,  1064U,  1082U,  1114U,  1148U,  1189U,  1236U,  1278U,
                    1272U,  1226U,  1178U,  1140U,  1109U,  1081U,  1065U,  1053U,  1048U,  1055U,  1068U,  1096U,  1122U,  1163U,  1195U,  1239U,  1288U,
                    1284U,  1239U,  1193U,  1155U,  1120U,  1094U,  1076U,  1061U,  1064U,  1068U,  1080U,  1109U,  1134U,  1171U,  1209U,  1257U,  1295U,
                    1296U,  1251U,  1204U,  1167U,  1135U,  1107U,  1086U,  1079U,  1073U,  1084U,  1099U,  1121U,  1147U,  1188U,  1225U,  1270U,  1311U,
                    1313U,  1262U,  1218U,  1180U,  1148U,  1121U,  1102U,  1092U,  1091U,  1096U,  1111U,  1138U,  1167U,  1203U,  1241U,  1285U,  1326U,
                    1327U,  1288U,  1239U,  1199U,  1166U,  1139U,  1122U,  1111U,  1109U,  1116U,  1131U,  1152U,  1187U,  1222U,  1260U,  1305U,  1345U 
               },
           },
    
           // ISI_COLOR_COMPONENT_GREENB
           {
               {
                    1324U,  1275U,  1232U,  1191U,  1160U,  1136U,  1113U,  1103U,  1101U,  1107U,  1122U,  1147U,  1177U,  1215U,  1255U,  1297U,  1335U,
                    1306U,  1258U,  1218U,  1179U,  1143U,  1115U,  1097U,  1086U,  1083U,  1090U,  1103U,  1127U,  1155U,  1194U,  1233U,  1279U,  1322U,
                    1294U,  1246U,  1201U,  1161U,  1131U,  1103U,  1082U,  1072U,  1069U,  1074U,  1088U,  1114U,  1146U,  1180U,  1223U,  1264U,  1312U,
                    1279U,  1234U,  1190U,  1149U,  1118U,  1092U,  1071U,  1057U,  1056U,  1062U,  1078U,  1103U,  1134U,  1167U,  1209U,  1249U,  1295U,
                    1273U,  1221U,  1177U,  1138U,  1106U,  1081U,  1064U,  1049U,  1044U,  1052U,  1067U,  1093U,  1121U,  1157U,  1195U,  1243U,  1287U,
                    1258U,  1214U,  1172U,  1129U,  1099U,  1069U,  1053U,  1038U,  1033U,  1044U,  1057U,  1082U,  1112U,  1147U,  1189U,  1233U,  1277U,
                    1256U,  1210U,  1161U,  1125U,  1091U,  1067U,  1045U,  1034U,  1030U,  1037U,  1051U,  1077U,  1107U,  1142U,  1185U,  1226U,  1272U,
                    1249U,  1205U,  1158U,  1120U,  1087U,  1058U,  1042U,  1026U,  1024U,  1032U,  1050U,  1071U,  1105U,  1137U,  1181U,  1223U,  1266U,
                    1252U,  1204U,  1160U,  1120U,  1088U,  1059U,  1040U,  1027U,  1025U,  1034U,  1048U,  1072U,  1102U,  1138U,  1179U,  1223U,  1266U,
                    1253U,  1205U,  1161U,  1123U,  1088U,  1062U,  1041U,  1030U,  1026U,  1033U,  1051U,  1073U,  1104U,  1139U,  1180U,  1225U,  1270U,
                    1258U,  1211U,  1168U,  1125U,  1093U,  1064U,  1048U,  1033U,  1031U,  1038U,  1053U,  1079U,  1108U,  1141U,  1186U,  1226U,  1270U,
                    1262U,  1217U,  1173U,  1134U,  1102U,  1074U,  1054U,  1040U,  1039U,  1044U,  1063U,  1082U,  1114U,  1150U,  1187U,  1237U,  1280U,
                    1272U,  1227U,  1180U,  1141U,  1108U,  1083U,  1062U,  1053U,  1048U,  1054U,  1068U,  1095U,  1123U,  1160U,  1196U,  1239U,  1288U,
                    1287U,  1240U,  1193U,  1154U,  1123U,  1094U,  1076U,  1061U,  1062U,  1066U,  1080U,  1107U,  1135U,  1172U,  1211U,  1257U,  1299U,
                    1297U,  1253U,  1204U,  1166U,  1135U,  1105U,  1087U,  1078U,  1072U,  1084U,  1098U,  1120U,  1147U,  1188U,  1224U,  1269U,  1311U,
                    1313U,  1264U,  1218U,  1180U,  1147U,  1122U,  1100U,  1092U,  1090U,  1095U,  1111U,  1137U,  1167U,  1202U,  1242U,  1286U,  1331U,
                    1330U,  1289U,  1239U,  1199U,  1168U,  1139U,  1123U,  1112U,  1109U,  1114U,  1131U,  1151U,  1190U,  1222U,  1263U,  1308U,  1344U 
               },
           },
    
           // ISI_COLOR_COMPONENT_BLUE
           {
               {
                    1279U,  1238U,  1194U,  1165U,  1129U,  1106U,  1086U,  1079U,  1068U,  1076U,  1089U,  1103U,  1130U,  1161U,  1189U,  1226U,  1265U,
                    1261U,  1226U,  1187U,  1150U,  1123U,  1097U,  1077U,  1064U,  1064U,  1065U,  1075U,  1094U,  1119U,  1145U,  1179U,  1210U,  1256U,
                    1256U,  1216U,  1172U,  1144U,  1109U,  1088U,  1066U,  1055U,  1054U,  1056U,  1064U,  1089U,  1105U,  1138U,  1168U,  1199U,  1238U,
                    1251U,  1210U,  1172U,  1132U,  1104U,  1080U,  1060U,  1050U,  1041U,  1046U,  1058U,  1075U,  1099U,  1126U,  1166U,  1200U,  1234U,
                    1242U,  1200U,  1159U,  1129U,  1097U,  1076U,  1050U,  1044U,  1041U,  1040U,  1051U,  1074U,  1095U,  1124U,  1155U,  1186U,  1229U,
                    1235U,  1201U,  1158U,  1129U,  1095U,  1068U,  1049U,  1036U,  1033U,  1036U,  1047U,  1067U,  1091U,  1114U,  1151U,  1185U,  1224U,
                    1239U,  1189U,  1151U,  1116U,  1093U,  1069U,  1046U,  1031U,  1029U,  1030U,  1047U,  1062U,  1089U,  1114U,  1149U,  1187U,  1219U,
                    1231U,  1191U,  1154U,  1120U,  1088U,  1062U,  1042U,  1031U,  1026U,  1028U,  1040U,  1064U,  1083U,  1114U,  1146U,  1180U,  1218U,
                    1244U,  1192U,  1155U,  1123U,  1092U,  1062U,  1045U,  1031U,  1024U,  1031U,  1044U,  1059U,  1089U,  1111U,  1145U,  1178U,  1220U,
                    1232U,  1198U,  1158U,  1124U,  1094U,  1069U,  1047U,  1034U,  1028U,  1035U,  1048U,  1066U,  1085U,  1119U,  1144U,  1186U,  1221U,
                    1246U,  1196U,  1160U,  1126U,  1104U,  1069U,  1058U,  1039U,  1034U,  1039U,  1048U,  1074U,  1090U,  1121U,  1151U,  1189U,  1223U,
                    1254U,  1214U,  1171U,  1136U,  1111U,  1080U,  1061U,  1054U,  1042U,  1051U,  1063U,  1081U,  1099U,  1132U,  1160U,  1197U,  1235U,
                    1268U,  1218U,  1180U,  1147U,  1122U,  1093U,  1077U,  1060U,  1059U,  1060U,  1075U,  1092U,  1117U,  1137U,  1168U,  1211U,  1242U,
                    1269U,  1234U,  1194U,  1157U,  1135U,  1103U,  1084U,  1075U,  1065U,  1071U,  1086U,  1103U,  1126U,  1157U,  1188U,  1216U,  1262U,
                    1283U,  1244U,  1204U,  1169U,  1143U,  1112U,  1096U,  1089U,  1079U,  1094U,  1095U,  1123U,  1141U,  1166U,  1198U,  1238U,  1268U,
                    1297U,  1253U,  1209U,  1177U,  1152U,  1127U,  1110U,  1103U,  1094U,  1101U,  1118U,  1134U,  1157U,  1186U,  1216U,  1255U,  1289U,
                    1306U,  1268U,  1227U,  1199U,  1167U,  1145U,  1120U,  1116U,  1107U,  1115U,  1126U,  1147U,  1172U,  1211U,  1237U,  1275U,  1307U 
               },
           },
       },
    },
};

IsiLscMatrixTable_t OV5647_LscMatrixTable_CIE_F2_1920x1080 = 
{
    .ArraySize          = AWB_LSCMATRIX_ARRAY_SIZE_CIE_F2,
    .psIsiVignLscMatrix = &OV5647_VignLscMatrix_CIE_F2_1920x1080[0],
    .LscXGradTbl        = { 324U,  301U,  293U,  273U,  264U,  254U,  246U,  248U },
    .LscYGradTbl        = { 520U,  504U,  496U,  482U,  475U,  482U,  468U,  462U },
    .LscXSizeTbl        = { 101U,  109U,  112U,  120U,  124U,  129U,  133U,  132U },
    .LscYSizeTbl        = {  63U,   65U,   66U,   68U,   69U,   68U,   70U,   71U } 
};



#ifdef __cplusplus
}
#endif

/* @} ov5630_F2 */

#endif /* __OV5647_F2_H__ */

