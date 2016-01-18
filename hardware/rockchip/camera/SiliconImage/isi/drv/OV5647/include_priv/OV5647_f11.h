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
 * @defgroup ov5630_F11   Illumination Profile F11
 * @{
 *
 */
#ifndef __OV5647_F11_H__
#define __OV5647_F11_H__

#ifdef __cplusplus
extern "C"
{
#endif


#define AWB_COLORMATRIX_ARRAY_SIZE_CIE_F11  2
#define AWB_LSCMATRIX_ARRAY_SIZE_CIE_F11    2

#define AWB_SATURATION_ARRAY_SIZE_CIE_F11   4
#define AWB_VIGNETTING_ARRAY_SIZE_CIE_F11   2

#define CC_OFFSET_SCALING_F11               14.0f

/*****************************************************************************/
/*!
 * CIE F11:
 *  TL84, Ultralume 40, SP41
 */
/*****************************************************************************/

// crosstalk matrix
const Isi3x3FloatMatrix_t  OV5647_XTalkCoeff_F11 =
{
    {
        1.75206f,  -0.48534f,  -0.26671f, 
       -0.28299f,   1.26655f,   0.01644f, 
        0.02763f,  -0.55967f,   1.53204f  
    }
};

// crosstalk offset matrix
const IsiXTalkFloatOffset_t OV5647_XTalkOffset_F11 =
{
    .fCtOffsetRed   = (-204.3125f / CC_OFFSET_SCALING_F11),
    .fCtOffsetGreen = (-204.0000f / CC_OFFSET_SCALING_F11),
    .fCtOffsetBlue  = (-205.8750f / CC_OFFSET_SCALING_F11)
};

// gain matrix
const IsiComponentGain_t OV5647_CompGain_F11 =
{
    .fRed      = 1.35402f,    
    .fGreenR   = 1.00000f,
    .fGreenB   = 1.00000f,
    .fBlue     = 1.72586f 
};

// mean value of gaussian mixture model
const Isi2x1FloatMatrix_t OV5647_GaussMeanValue_F11 =
{
    {
        -0.03684f,  0.04770f 
    }
};

// inverse covariance matrix
const Isi2x2FloatMatrix_t OV5647_CovarianceMatrix_F11 =
{
    {
        745.60506f,  -605.93573f, 
       -605.93573f,  1583.60503f 
    }
};

// factor in gaussian mixture model
const IsiGaussFactor_t OV5647_GaussFactor_F11 =
{
    .fGaussFactor =  143.55615f  
};

// thresholds for switching between MAP classification and interpolation
const Isi2x1FloatMatrix_t OV5647_Threshold_F11 =
{
    {
        0.90000f,  1.00000f
    }
};

// saturation curve
float afSaturationSensorGain_F11[AWB_SATURATION_ARRAY_SIZE_CIE_F11] =
{
    1.0f, 2.0f, 4.0f, 8.0f
};

float afSaturation_F11[AWB_SATURATION_ARRAY_SIZE_CIE_F11] =
{
    100.0f, 100.0f, 90.0f, 74.0f
};

const IsiSaturationCurve_t OV5647_SaturationCurve_F11 =
{
    .ArraySize      = AWB_SATURATION_ARRAY_SIZE_CIE_F11,
    .pSensorGain    = &afSaturationSensorGain_F11[0],
    .pSaturation    = &afSaturation_F11[0]
};

// saturation depended color conversion matrices
IsiSatCcMatrix_t OV5647_SatCcMatrix_F11[AWB_COLORMATRIX_ARRAY_SIZE_CIE_F11] =
{
    {
        .fSaturation    = 74.0f,
        .XTalkCoeff     =
        {
            {
                1.54924f,  -0.31872f,  -0.23052f, 
                0.08983f,   0.94041f,  -0.03025f, 
                0.31303f,  -0.38459f,   1.07156f  
            }
        }
    },
    {
        .fSaturation    = 100.0f,
        .XTalkCoeff     =
        {
            {
               1.75206f,  -0.48534f,  -0.26671f, 
              -0.28299f,   1.26655f,   0.01644f, 
               0.02763f,  -0.55967f,   1.53204f  
            }
        }
    }
};

const IsiCcMatrixTable_t OV5647_CcMatrixTable_F11 =
{
    .ArraySize          = AWB_COLORMATRIX_ARRAY_SIZE_CIE_F11,
    .pIsiSatCcMatrix    = &OV5647_SatCcMatrix_F11[0]
};

// saturation depended color conversion offset vectors
IsiSatCcOffset_t OV5647_SatCcOffset_F11[AWB_COLORMATRIX_ARRAY_SIZE_CIE_F11] =
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
            .fCtOffsetRed   = (-204.3125f / CC_OFFSET_SCALING_F11),
            .fCtOffsetGreen = (-204.0000f / CC_OFFSET_SCALING_F11),
            .fCtOffsetBlue  = (-205.8750f / CC_OFFSET_SCALING_F11)
        }
    }
};

const IsiCcOffsetTable_t OV5647_CcOffsetTable_F11=
{
    .ArraySize          = AWB_COLORMATRIX_ARRAY_SIZE_CIE_F11,
    .pIsiSatCcOffset    = &OV5647_SatCcOffset_F11[0]
};

// vignetting curve
float afVignettingSensorGain_F11[AWB_VIGNETTING_ARRAY_SIZE_CIE_F11] =
{
    1.0f, 8.0f
};

float afVignetting_F11[AWB_VIGNETTING_ARRAY_SIZE_CIE_F11] =
{
    100.0f, 100.0f
};

const IsiVignettingCurve_t OV5647_VignettingCurve_F11 =
{
    .ArraySize      = AWB_VIGNETTING_ARRAY_SIZE_CIE_F11,
    .pSensorGain    = &afVignettingSensorGain_F11[0],
    .pVignetting    = &afVignetting_F11[0]
};

// vignetting dependend lsc matrices
IsiVignLscMatrix_t OV5647_VignLscMatrix_CIE_F11_1920x1080[AWB_LSCMATRIX_ARRAY_SIZE_CIE_F11] = 
{
    // array item 0
    {
       .fVignetting    = 100.0f,
       .LscMatrix      =
       {
           // ISI_COLOR_COMPONENT_RED
           {
               {
                    1373U,  1319U,  1259U,  1210U,  1172U,  1128U,  1113U,  1106U,  1093U,  1102U,  1117U,  1142U,  1179U,  1226U,  1267U,  1337U,  1383U, 
                    1365U,  1299U,  1244U,  1199U,  1156U,  1124U,  1099U,  1086U,  1080U,  1084U,  1100U,  1131U,  1162U,  1205U,  1261U,  1309U,  1375U,
                    1350U,  1288U,  1230U,  1178U,  1143U,  1104U,  1084U,  1070U,  1064U,  1070U,  1087U,  1113U,  1148U,  1191U,  1241U,  1298U,  1357U,
                    1330U,  1273U,  1215U,  1171U,  1127U,  1093U,  1074U,  1055U,  1053U,  1061U,  1076U,  1101U,  1132U,  1179U,  1222U,  1285U,  1336U,
                    1320U,  1263U,  1207U,  1156U,  1118U,  1087U,  1061U,  1047U,  1043U,  1048U,  1063U,  1093U,  1126U,  1164U,  1218U,  1269U,  1334U,
                    1313U,  1253U,  1196U,  1150U,  1113U,  1075U,  1055U,  1038U,  1033U,  1042U,  1055U,  1080U,  1117U,  1160U,  1205U,  1260U,  1322U,
                    1302U,  1247U,  1192U,  1147U,  1105U,  1075U,  1049U,  1033U,  1032U,  1037U,  1052U,  1077U,  1110U,  1155U,  1202U,  1255U,  1315U,
                    1301U,  1242U,  1184U,  1139U,  1103U,  1067U,  1045U,  1032U,  1024U,  1030U,  1051U,  1074U,  1110U,  1148U,  1198U,  1257U,  1308U,
                    1298U,  1243U,  1189U,  1141U,  1104U,  1069U,  1045U,  1029U,  1024U,  1031U,  1046U,  1074U,  1111U,  1151U,  1192U,  1246U,  1310U,
                    1306U,  1248U,  1189U,  1145U,  1107U,  1072U,  1049U,  1034U,  1029U,  1034U,  1050U,  1075U,  1114U,  1152U,  1201U,  1258U,  1310U,
                    1308U,  1252U,  1195U,  1153U,  1111U,  1080U,  1052U,  1039U,  1034U,  1038U,  1059U,  1083U,  1112U,  1155U,  1204U,  1254U,  1312U,
                    1323U,  1267U,  1210U,  1163U,  1121U,  1084U,  1064U,  1045U,  1043U,  1048U,  1065U,  1092U,  1128U,  1164U,  1212U,  1268U,  1331U,
                    1339U,  1277U,  1218U,  1172U,  1132U,  1098U,  1074U,  1057U,  1053U,  1060U,  1076U,  1103U,  1134U,  1180U,  1223U,  1279U,  1338U,
                    1355U,  1294U,  1239U,  1183U,  1152U,  1114U,  1086U,  1073U,  1065U,  1073U,  1092U,  1116U,  1150U,  1193U,  1245U,  1293U,  1351U,
                    1366U,  1311U,  1257U,  1206U,  1163U,  1130U,  1108U,  1092U,  1086U,  1088U,  1109U,  1134U,  1162U,  1210U,  1260U,  1313U,  1377U,
                    1390U,  1333U,  1276U,  1225U,  1183U,  1148U,  1124U,  1105U,  1103U,  1106U,  1123U,  1147U,  1185U,  1229U,  1281U,  1335U,  1387U,
                    1409U,  1355U,  1294U,  1248U,  1208U,  1169U,  1140U,  1131U,  1126U,  1131U,  1142U,  1177U,  1208U,  1254U,  1302U,  1365U,  1409U 
               }
           }, 
    
           // ISI_COLOR_COMPONENT_GREENR
           {
               {
                    1330U,  1281U,  1236U,  1197U,  1162U,  1136U,  1112U,  1105U,  1097U,  1106U,  1122U,  1146U,  1179U,  1215U,  1260U,  1306U,  1336U, 
                    1308U,  1261U,  1220U,  1181U,  1149U,  1118U,  1099U,  1090U,  1087U,  1096U,  1105U,  1135U,  1165U,  1201U,  1242U,  1285U,  1334U,
                    1298U,  1249U,  1206U,  1166U,  1132U,  1105U,  1086U,  1074U,  1069U,  1077U,  1094U,  1121U,  1150U,  1183U,  1228U,  1268U,  1318U,
                    1282U,  1236U,  1193U,  1150U,  1124U,  1094U,  1073U,  1059U,  1060U,  1067U,  1081U,  1104U,  1135U,  1174U,  1214U,  1255U,  1303U,
                    1275U,  1226U,  1181U,  1143U,  1107U,  1084U,  1064U,  1048U,  1047U,  1055U,  1069U,  1096U,  1126U,  1160U,  1203U,  1246U,  1291U,
                    1267U,  1218U,  1174U,  1133U,  1103U,  1072U,  1053U,  1042U,  1037U,  1047U,  1058U,  1085U,  1117U,  1152U,  1195U,  1237U,  1281U,
                    1261U,  1210U,  1165U,  1127U,  1097U,  1065U,  1048U,  1032U,  1030U,  1037U,  1056U,  1083U,  1110U,  1146U,  1187U,  1232U,  1277U,
                    1251U,  1206U,  1161U,  1124U,  1087U,  1064U,  1040U,  1030U,  1027U,  1035U,  1052U,  1075U,  1110U,  1145U,  1185U,  1227U,  1272U,
                    1254U,  1208U,  1162U,  1124U,  1091U,  1060U,  1043U,  1028U,  1024U,  1035U,  1050U,  1075U,  1106U,  1141U,  1183U,  1228U,  1271U,
                    1255U,  1206U,  1164U,  1125U,  1090U,  1063U,  1044U,  1031U,  1029U,  1035U,  1053U,  1078U,  1107U,  1144U,  1185U,  1228U,  1278U,
                    1256U,  1217U,  1168U,  1131U,  1098U,  1068U,  1049U,  1036U,  1033U,  1044U,  1057U,  1081U,  1113U,  1151U,  1189U,  1233U,  1279U,
                    1270U,  1219U,  1176U,  1136U,  1103U,  1077U,  1057U,  1044U,  1044U,  1050U,  1065U,  1090U,  1120U,  1154U,  1198U,  1240U,  1283U,
                    1273U,  1231U,  1185U,  1147U,  1114U,  1086U,  1070U,  1054U,  1049U,  1061U,  1072U,  1100U,  1127U,  1167U,  1207U,  1245U,  1302U,
                    1292U,  1243U,  1198U,  1159U,  1125U,  1097U,  1081U,  1066U,  1065U,  1073U,  1089U,  1111U,  1139U,  1180U,  1214U,  1264U,  1302U,
                    1303U,  1258U,  1209U,  1171U,  1138U,  1111U,  1091U,  1081U,  1076U,  1088U,  1100U,  1125U,  1154U,  1193U,  1232U,  1277U,  1323U,
                    1321U,  1267U,  1226U,  1186U,  1153U,  1124U,  1108U,  1094U,  1092U,  1104U,  1116U,  1143U,  1172U,  1214U,  1251U,  1295U,  1340U,
                    1334U,  1293U,  1246U,  1206U,  1171U,  1145U,  1123U,  1118U,  1116U,  1121U,  1136U,  1162U,  1193U,  1229U,  1269U,  1313U,  1356U 
               },
           },
    
           // ISI_COLOR_COMPONENT_GREENB
           {
               {
                    1330U,  1279U,  1236U,  1197U,  1162U,  1133U,  1111U,  1101U,  1099U,  1106U,  1120U,  1145U,  1179U,  1214U,  1261U,  1302U,  1341U,
                    1307U,  1263U,  1218U,  1178U,  1148U,  1116U,  1099U,  1089U,  1084U,  1093U,  1105U,  1133U,  1161U,  1200U,  1239U,  1287U,  1332U,
                    1300U,  1249U,  1205U,  1166U,  1131U,  1105U,  1084U,  1072U,  1069U,  1076U,  1093U,  1119U,  1149U,  1184U,  1229U,  1267U,  1319U,
                    1283U,  1236U,  1194U,  1150U,  1122U,  1093U,  1071U,  1058U,  1058U,  1065U,  1079U,  1105U,  1134U,  1172U,  1214U,  1256U,  1305U,
                    1279U,  1225U,  1182U,  1143U,  1106U,  1084U,  1062U,  1049U,  1047U,  1052U,  1068U,  1096U,  1125U,  1160U,  1203U,  1248U,  1290U,
                    1268U,  1218U,  1176U,  1133U,  1102U,  1070U,  1053U,  1041U,  1035U,  1045U,  1056U,  1084U,  1115U,  1152U,  1195U,  1239U,  1283U,
                    1260U,  1211U,  1164U,  1126U,  1095U,  1065U,  1046U,  1033U,  1029U,  1037U,  1055U,  1082U,  1108U,  1148U,  1187U,  1232U,  1279U,
                    1253U,  1207U,  1161U,  1124U,  1087U,  1063U,  1039U,  1031U,  1025U,  1034U,  1051U,  1074U,  1109U,  1144U,  1185U,  1227U,  1272U,
                    1254U,  1210U,  1164U,  1124U,  1091U,  1058U,  1042U,  1026U,  1024U,  1036U,  1049U,  1074U,  1105U,  1141U,  1183U,  1229U,  1274U,
                    1259U,  1205U,  1165U,  1126U,  1089U,  1064U,  1042U,  1029U,  1028U,  1034U,  1051U,  1076U,  1108U,  1143U,  1186U,  1228U,  1278U,
                    1257U,  1219U,  1168U,  1130U,  1096U,  1066U,  1049U,  1034U,  1033U,  1041U,  1058U,  1080U,  1112U,  1151U,  1189U,  1234U,  1281U,
                    1269U,  1219U,  1177U,  1135U,  1104U,  1076U,  1056U,  1043U,  1041U,  1050U,  1063U,  1090U,  1120U,  1154U,  1195U,  1241U,  1286U,
                    1275U,  1230U,  1185U,  1146U,  1113U,  1085U,  1068U,  1053U,  1049U,  1060U,  1071U,  1097U,  1126U,  1165U,  1208U,  1246U,  1300U,
                    1292U,  1244U,  1198U,  1159U,  1126U,  1096U,  1080U,  1065U,  1062U,  1071U,  1088U,  1110U,  1138U,  1179U,  1214U,  1264U,  1303U,
                    1304U,  1257U,  1208U,  1170U,  1137U,  1110U,  1091U,  1078U,  1073U,  1086U,  1098U,  1123U,  1153U,  1191U,  1231U,  1276U,  1324U,
                    1320U,  1269U,  1227U,  1186U,  1152U,  1123U,  1105U,  1092U,  1091U,  1100U,  1114U,  1142U,  1169U,  1213U,  1251U,  1294U,  1337U,
                    1336U,  1292U,  1245U,  1203U,  1171U,  1145U,  1123U,  1115U,  1114U,  1118U,  1136U,  1158U,  1195U,  1228U,  1270U,  1311U,  1359U  
               },
           },
    
           // ISI_COLOR_COMPONENT_BLUE
           {
               {
                    1279U,  1240U,  1198U,  1170U,  1129U,  1111U,  1088U,  1081U,  1070U,  1083U,  1086U,  1109U,  1129U,  1154U,  1195U,  1230U,  1267U,
                    1267U,  1233U,  1192U,  1152U,  1130U,  1096U,  1080U,  1065U,  1062U,  1059U,  1075U,  1099U,  1116U,  1149U,  1183U,  1215U,  1252U,
                    1271U,  1216U,  1179U,  1143U,  1113U,  1090U,  1070U,  1057U,  1052U,  1057U,  1068U,  1084U,  1109U,  1140U,  1170U,  1206U,  1245U,
                    1254U,  1209U,  1169U,  1136U,  1103U,  1079U,  1061U,  1050U,  1040U,  1047U,  1058U,  1080U,  1099U,  1129U,  1166U,  1198U,  1237U,
                    1243U,  1206U,  1162U,  1131U,  1101U,  1073U,  1054U,  1040U,  1042U,  1043U,  1051U,  1075U,  1095U,  1124U,  1158U,  1194U,  1233U,
                    1248U,  1200U,  1158U,  1124U,  1094U,  1068U,  1048U,  1036U,  1032U,  1036U,  1044U,  1071U,  1088U,  1118U,  1151U,  1186U,  1227U,
                    1234U,  1196U,  1159U,  1117U,  1093U,  1067U,  1045U,  1033U,  1031U,  1033U,  1047U,  1058U,  1087U,  1118U,  1148U,  1186U,  1225U,
                    1234U,  1193U,  1153U,  1122U,  1089U,  1059U,  1043U,  1030U,  1024U,  1030U,  1040U,  1063U,  1080U,  1111U,  1146U,  1185U,  1219U,
                    1237U,  1197U,  1156U,  1125U,  1092U,  1063U,  1046U,  1030U,  1026U,  1028U,  1043U,  1063U,  1086U,  1114U,  1143U,  1184U,  1216U,
                    1235U,  1200U,  1156U,  1123U,  1096U,  1066U,  1047U,  1032U,  1029U,  1031U,  1046U,  1063U,  1086U,  1111U,  1146U,  1182U,  1228U,
                    1246U,  1198U,  1162U,  1128U,  1101U,  1072U,  1053U,  1040U,  1032U,  1040U,  1049U,  1072U,  1094U,  1122U,  1155U,  1191U,  1223U,
                    1248U,  1219U,  1172U,  1134U,  1109U,  1080U,  1061U,  1053U,  1041U,  1047U,  1062U,  1079U,  1099U,  1125U,  1163U,  1195U,  1236U,
                    1265U,  1222U,  1181U,  1146U,  1116U,  1096U,  1071U,  1061U,  1055U,  1059U,  1070U,  1093U,  1114U,  1141U,  1168U,  1212U,  1247U,
                    1273U,  1232U,  1192U,  1155U,  1134U,  1099U,  1084U,  1072U,  1066U,  1068U,  1081U,  1105U,  1124U,  1156U,  1185U,  1223U,  1258U,
                    1285U,  1246U,  1205U,  1169U,  1141U,  1115U,  1094U,  1081U,  1082U,  1089U,  1096U,  1119U,  1141U,  1166U,  1197U,  1236U,  1276U,
                    1300U,  1252U,  1218U,  1181U,  1152U,  1127U,  1107U,  1103U,  1092U,  1099U,  1117U,  1127U,  1158U,  1185U,  1220U,  1257U,  1291U,
                    1309U,  1270U,  1225U,  1201U,  1160U,  1145U,  1122U,  1113U,  1111U,  1113U,  1126U,  1153U,  1171U,  1207U,  1236U,  1275U,  1308U 
               },
           },
       },
    },
};

IsiLscMatrixTable_t OV5647_LscMatrixTable_CIE_F11_1920x1080 = 
{
    .ArraySize          = AWB_LSCMATRIX_ARRAY_SIZE_CIE_F11,
    .psIsiVignLscMatrix = &OV5647_VignLscMatrix_CIE_F11_1920x1080[0],
    .LscXGradTbl        = { 324U,  301U,  293U,  273U,  264U,  254U,  246U,  248U },
    .LscYGradTbl        = { 520U,  504U,  496U,  482U,  475U,  482U,  468U,  462U },
    .LscXSizeTbl        = { 101U,  109U,  112U,  120U,  124U,  129U,  133U,  132U },
    .LscYSizeTbl        = {  63U,   65U,   66U,   68U,   69U,   68U,   70U,   71U } 
};



#ifdef __cplusplus
}
#endif

/* @} ov5630_F11 */

#endif /* __OV5647_F11_H__ */

