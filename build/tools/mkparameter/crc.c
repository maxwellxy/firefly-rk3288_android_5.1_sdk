/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/******************************************************/
//�Զ���� �� CRC 32 ���,����ʽΪ 0X04c10db7
/******************************************************/
#include <stdio.h>
#include <stdlib.h>

unsigned long gTable_Crc32[256] =
{
	0x00000000, 0x04c10db7, 0x09821b6e, 0x0d4316d9,
		0x130436dc, 0x17c53b6b, 0x1a862db2, 0x1e472005,
		0x26086db8, 0x22c9600f, 0x2f8a76d6, 0x2b4b7b61,
		0x350c5b64, 0x31cd56d3, 0x3c8e400a, 0x384f4dbd,
		0x4c10db70, 0x48d1d6c7, 0x4592c01e, 0x4153cda9,
		0x5f14edac, 0x5bd5e01b, 0x5696f6c2, 0x5257fb75,
		0x6a18b6c8, 0x6ed9bb7f, 0x639aada6, 0x675ba011,
		0x791c8014, 0x7ddd8da3, 0x709e9b7a, 0x745f96cd,
		0x9821b6e0, 0x9ce0bb57, 0x91a3ad8e, 0x9562a039,
		0x8b25803c, 0x8fe48d8b, 0x82a79b52, 0x866696e5,
		0xbe29db58, 0xbae8d6ef, 0xb7abc036, 0xb36acd81,
		0xad2ded84, 0xa9ece033, 0xa4aff6ea, 0xa06efb5d,
		0xd4316d90, 0xd0f06027, 0xddb376fe, 0xd9727b49,
		0xc7355b4c, 0xc3f456fb, 0xceb74022, 0xca764d95,
		0xf2390028, 0xf6f80d9f, 0xfbbb1b46, 0xff7a16f1,
		0xe13d36f4, 0xe5fc3b43, 0xe8bf2d9a, 0xec7e202d,
		0x34826077, 0x30436dc0, 0x3d007b19, 0x39c176ae,
		0x278656ab, 0x23475b1c, 0x2e044dc5, 0x2ac54072,
		0x128a0dcf, 0x164b0078, 0x1b0816a1, 0x1fc91b16,
		0x018e3b13, 0x054f36a4, 0x080c207d, 0x0ccd2dca,
		0x7892bb07, 0x7c53b6b0, 0x7110a069, 0x75d1adde,
		0x6b968ddb, 0x6f57806c, 0x621496b5, 0x66d59b02,
		0x5e9ad6bf, 0x5a5bdb08, 0x5718cdd1, 0x53d9c066,
		0x4d9ee063, 0x495fedd4, 0x441cfb0d, 0x40ddf6ba,
		0xaca3d697, 0xa862db20, 0xa521cdf9, 0xa1e0c04e,
		0xbfa7e04b, 0xbb66edfc, 0xb625fb25, 0xb2e4f692,
		0x8aabbb2f, 0x8e6ab698, 0x8329a041, 0x87e8adf6,
		0x99af8df3, 0x9d6e8044, 0x902d969d, 0x94ec9b2a,
		0xe0b30de7, 0xe4720050, 0xe9311689, 0xedf01b3e,
		0xf3b73b3b, 0xf776368c, 0xfa352055, 0xfef42de2,
		0xc6bb605f, 0xc27a6de8, 0xcf397b31, 0xcbf87686,
		0xd5bf5683, 0xd17e5b34, 0xdc3d4ded, 0xd8fc405a,
		0x6904c0ee, 0x6dc5cd59, 0x6086db80, 0x6447d637,
		0x7a00f632, 0x7ec1fb85, 0x7382ed5c, 0x7743e0eb,
		0x4f0cad56, 0x4bcda0e1, 0x468eb638, 0x424fbb8f,
		0x5c089b8a, 0x58c9963d, 0x558a80e4, 0x514b8d53,
		0x25141b9e, 0x21d51629, 0x2c9600f0, 0x28570d47,
		0x36102d42, 0x32d120f5, 0x3f92362c, 0x3b533b9b,
		0x031c7626, 0x07dd7b91, 0x0a9e6d48, 0x0e5f60ff,
		0x101840fa, 0x14d94d4d, 0x199a5b94, 0x1d5b5623,
		0xf125760e, 0xf5e47bb9, 0xf8a76d60, 0xfc6660d7,
		0xe22140d2, 0xe6e04d65, 0xeba35bbc, 0xef62560b,
		0xd72d1bb6, 0xd3ec1601, 0xdeaf00d8, 0xda6e0d6f,
		0xc4292d6a, 0xc0e820dd, 0xcdab3604, 0xc96a3bb3,
		0xbd35ad7e, 0xb9f4a0c9, 0xb4b7b610, 0xb076bba7,
		0xae319ba2, 0xaaf09615, 0xa7b380cc, 0xa3728d7b,
		0x9b3dc0c6, 0x9ffccd71, 0x92bfdba8, 0x967ed61f,
		0x8839f61a, 0x8cf8fbad, 0x81bbed74, 0x857ae0c3,
		0x5d86a099, 0x5947ad2e, 0x5404bbf7, 0x50c5b640,
		0x4e829645, 0x4a439bf2, 0x47008d2b, 0x43c1809c,
		0x7b8ecd21, 0x7f4fc096, 0x720cd64f, 0x76cddbf8,
		0x688afbfd, 0x6c4bf64a, 0x6108e093, 0x65c9ed24,
		0x11967be9, 0x1557765e, 0x18146087, 0x1cd56d30,
		0x02924d35, 0x06534082, 0x0b10565b, 0x0fd15bec,
		0x379e1651, 0x335f1be6, 0x3e1c0d3f, 0x3add0088,
		0x249a208d, 0x205b2d3a, 0x2d183be3, 0x29d93654,
		0xc5a71679, 0xc1661bce, 0xcc250d17, 0xc8e400a0,
		0xd6a320a5, 0xd2622d12, 0xdf213bcb, 0xdbe0367c,
		0xe3af7bc1, 0xe76e7676, 0xea2d60af, 0xeeec6d18,
		0xf0ab4d1d, 0xf46a40aa, 0xf9295673, 0xfde85bc4,
		0x89b7cd09, 0x8d76c0be, 0x8035d667, 0x84f4dbd0,
		0x9ab3fbd5, 0x9e72f662, 0x9331e0bb, 0x97f0ed0c,
		0xafbfa0b1, 0xab7ead06, 0xa63dbbdf, 0xa2fcb668,
		0xbcbb966d, 0xb87a9bda, 0xb5398d03, 0xb1f880b4,
};
/******************************************************************************/
#define CRC16_CCITT         0x1021  // CRC ������ʽ.
#define CRC32_CRC32         0x04C10DB7
////////////////////////////////////////////////////////////////////////////////////////
//#include "crctable.c"
/******************************************************************************/
// ���� 16 λ CRC ��
void CRCBuildTable16(unsigned short aPoly , unsigned short *crcTable)
{
    unsigned short i, j;
    unsigned short nData;
    unsigned short nAccum;

    for (i = 0; i < 256; i++)
    {
        nData = (unsigned short)(i << 8);
        nAccum = 0;
        for (j = 0; j < 8; j++)
        {
            if ((nData ^ nAccum) & 0x8000)
                nAccum = (nAccum << 1) ^ aPoly;
            else
                nAccum <<= 1;
            nData <<= 1;
        }
        crcTable[i] = nAccum;
    }
}


// ���� 16 λ CRC ֵ��CRC-16 �� CRC-CCITT
unsigned short CRC_16(unsigned char * aData, unsigned long aSize)
{
    unsigned long i;
    unsigned short nAccum = 0;
    unsigned short crcTable[256];

#if 1
    CRCBuildTable16(CRC16_CCITT , crcTable);
    for (i = 0; i < aSize; i++)
        nAccum = (nAccum << 8) ^ crcTable[(nAccum >> 8) ^ *aData++];
#else
    for (i = 0; i < aSize; i++)
        nAccum = (nAccum << 8) ^(unsigned short)gCrcTable[(nAccum >> 8) ^ *aData++];
#endif
    return nAccum;
}

/******************************************************************************/
// ע�⣺�����λһ��Ϊ"1"������ȥ
//const INT16U cnCRC_16 = 0x8005;
// CRC-16 = X16 + X15 + X2 + X0
//const INT16U cnCRC_CCITT = 0x1021;
// CRC-CCITT = X16 + X12 + X5 + X0����˵��� 16 λ CRC ����ʽ����һ��Ҫ��

//const INT32U cnCRC_32 = 0x04C10DB7;
// CRC-32 = X32 + X26 + X23 + X22 + X16 + X11 + X10 + X8 + X7 + X5 + X4 + X2 + X1 + X0

//unsigned long Table_CRC[256]; // CRC ��

//extern unsigned long gTable_Crc32[256];

// ���� 32 λ CRC-32 ֵ
unsigned long CRC_32(unsigned char * aData, unsigned long aSize)
{
    unsigned long i;
    unsigned long nAccum = 0;
//    unsigned long crc32Table[256];

//    CRCBuildTable32( CRC32_CRC32 , crc32Table );
    for (i = 0; i < aSize; i++)
        nAccum = (nAccum << 8) ^ gTable_Crc32[(nAccum >> 24) ^ *aData++];
    return nAccum;
}
unsigned long CRC_32_File(char *szFile,unsigned long ulSize)
{
	FILE *pFile;
	pFile = fopen(szFile,"rb");
	if (!pFile)
		return 0;
	unsigned char buf[1024];
	unsigned long nAccum = 0;
	unsigned int usNeedRead=0;
	unsigned int i,uiRead;
	while ( ulSize>0 )
	{
		if ( ulSize<1024 )
		{
			usNeedRead = ulSize;
		}
		else
		{
			usNeedRead = 1024;
		}
		memset(buf,0,1024);
		uiRead = fread(buf,1,usNeedRead,pFile);
		if ( uiRead!=usNeedRead )
		{
			if ( ferror(pFile) )
			{
				fclose(pFile);
				return 0;
			}
		}
		ulSize -= uiRead;
		for (i = 0; i < uiRead; i++)
			nAccum = (nAccum << 8) ^ gTable_Crc32[(nAccum >> 24) ^ buf[i]];
	}
	fclose(pFile);
	return nAccum;
}

// ��� BUFFER CRC У���Ƿ� �д���.���� ��� 4 �� BYTE �� CRC32 ��У��ֵ.
#if 0
BOOL CRC_32CheckBuffer(unsigned char * aData, unsigned long aSize)
{
    unsigned long   *crc;
    if (aSize <= 4)
    {
        return FALSE;
    }
    aSize -= 4;
    crc = (unsigned long*)(aData + aSize);
    if (CRC_32(aData , aSize) == *crc)
        return TRUE;
    return FALSE;

}
#endif
#ifdef VERSION  // PC �������.

// ���� 32 λ CRC ��
void CRCBuildTable32(unsigned long aPoly , unsigned long *crcTable)
{
    unsigned long i, j;
    unsigned long nData;
    unsigned long nAccum;

    for (i = 0; i < 256; i++)
    {
        nData = (unsigned long)(i << 24);
        nAccum = 0;
        for (j = 0; j < 8; j++)
        {
            if ((nData ^ nAccum) & 0x80000000)
                nAccum = (nAccum << 1) ^ aPoly;
            else
                nAccum <<= 1;
            nData <<= 1;
        }
        crcTable[i] = nAccum;
    }
}


int CRCWriteTableFile(BOOLEAN crc16 , char* fileName)
{
    FILE    *file;
//    int     ii;
//    int     jj ;
    unsigned long crcTable[256]; //32 or 16

    file = fopen(fileName , "wb");
    if (file == NULL)
    {
        printf("Open File %s Failed!\n" , fileName);
        return 6;
    }

    if (crc16)
    {
        CRCBuildTable16(CRC16_CCITT , (unsigned short*)crcTable);   // or cnCRC_CCITT
        if (fwrite(crcTable , sizeof(unsigned short) , 256 , file) != 256)
        {
            printf("Write CRC16 File[%s] Error\n", fileName);
            fclose(file);
            return 1;
        }
    }
    else
    {
        CRCBuildTable32(CRC32_CRC32 , crcTable);   // or cnCRC_CCITT
        if (fwrite(crcTable , sizeof(unsigned long) , 256 , file) != 256)
        {
            printf("Write CRC32 File[%s] Error\n", fileName);
            fclose(file);
            return 2;
        }
    }

    fclose(file);
    return 0;
}


int CRCPrintCrcTable(BOOLEAN crc16)
{
    int     ii;
    int     jj ;
    unsigned long crcTable[256]; //32 or 16

    if (crc16)
    {
        CRCBuildTable16(CRC16_CCITT , (unsigned short*)crcTable);   // or cnCRC_CCITT
    }
    else
    {
        CRCBuildTable32(CRC32_CRC32 , crcTable);   // or cnCRC_CCITT
    }

    if (crc16)
    {
        unsigned short *pCrc = (unsigned short*)crcTable;
        printf("/******************************************************\n"
               "//�Զ���� �� CRC 16 ���,����ʽΪ 0X%04x\n"
               "/******************************************************/\n\n"
               , CRC16_CCITT);

        printf("unsigned long gTable_Crc16[256]={\n");
        for (ii = 0; ii < 32 ; ii ++)
        {
            for (jj = 0; jj < 8 ; jj ++)
            {
                printf("0x%04x," , pCrc[ii*8+jj]);
            }
            printf("\n");
        }
        printf("};\n\n");

    }
    else
    {
        printf("/******************************************************\n"
               "//�Զ���� �� CRC 32 ���,����ʽΪ 0X%08x\n"
               "/******************************************************/\n\n"
               , CRC32_CRC32);

        printf("unsigned long gTable_Crc32[256]={\n");
        for (ii = 0; ii < 64 ; ii ++)
        {
            for (jj = 0; jj < 4 ; jj ++)
            {
                printf("0x%08x," , crcTable[ii*4+jj]);
            }
            printf("\n");
        }
        printf("};\n\n");
    }

    return 0;
}

#endif

/////////////////////////////////////////////////////////////

