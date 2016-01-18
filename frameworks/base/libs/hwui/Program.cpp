/*
 * Copyright (C) 2010 The Android Open Source Project
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

#define LOG_TAG "OpenGLRenderer"
#define ATRACE_TAG ATRACE_TAG_VIEW

#include <utils/Trace.h>

#include "Program.h"
#include "Vertex.h"
#include <utils/JenkinsHash.h>
#include <string.h>
#include <cutils/properties.h> 

#define MaxHasList    20
#ifdef USE_X86 
#define BuildShaderOPT      1
#endif
#define DEBUG_INFO_OPT      0
#define HASHFILE    "/data/hwuihas/hwuihas.bin"
#define DATAFILE    "/data/hwuihas/hwuishader.bin"

unsigned int ghashlist[MaxHasList] ={1533734761,1850271742,2045591190,3195415028,1141984664,
                                     2036775216,4152780369,2218586390,482269496,1110924158,
                                     0,0,0,0,0,
                                     0,0,0,0,0,};
  
typedef struct _HashManager
{
    unsigned int hash;
    unsigned int offset;
    unsigned int size;
}
HashManager;
                                     

namespace android {
namespace uirenderer {

///////////////////////////////////////////////////////////////////////////////
// Base program
///////////////////////////////////////////////////////////////////////////////
bool  searchHash(unsigned int hash,HashManager & HahMager)
{

    FILE* infile = fopen(HASHFILE, "rb");
    int ret = 0;
    HashManager *pHmager;
	if (!infile)
		return false;

    fseek(infile, 0, SEEK_END);
    
    int length =  (int)ftell(infile);

	if (length <=  0)
	{
		fclose(infile);
		return false;
	}

    void* pmem = (void*)malloc(length);
    if(!pmem)
    {
        ALOGW("malloc %d Byte failed",length);
		fclose(infile);        
        return false;
    }
    fseek(infile, 0, SEEK_SET);
    fread(pmem, length, 1, infile);
    fclose(infile);
    pHmager = ( HashManager *)pmem;
    //ALOGD("toal=%d,lencht=%d,hash=%u",length/sizeof(HashManager),length,hash);
    for(int i = 0;i < (length/sizeof(HashManager));i++)
    {
        if(pHmager[i].hash == hash)
        {
            free(pmem);
            HahMager.offset = pHmager[i].offset;
            HahMager.size =  pHmager[i].size;
            //ALOGD("have been searched [%u,%d,%d]",hash,HahMager.offset,HahMager.size);
            return true;
        }
    }
        
    free(pmem);
    return false;;

}
bool saveBinaryProgram(const char* Filename, GLuint &ProgramObjectID,unsigned int hash)
{
    HashManager hshMger;
	GLint linked;
	glGetProgramiv(ProgramObjectID, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		return false;
	}

	GLsizei length=0;
	glGetProgramiv(ProgramObjectID,GL_PROGRAM_BINARY_LENGTH_OES,&length);

	GLvoid* ShaderBinary = (GLvoid*)malloc(length);

	GLenum binaryFormat=0;

	GLsizei lengthWritten=0;

	glGetProgramBinaryOES(ProgramObjectID,length,&lengthWritten,&binaryFormat,ShaderBinary);
	if (!lengthWritten)
	{
		free(ShaderBinary);
		return false;
	}

	FILE* outfile = fopen(Filename, "ab+");

	if(!outfile)
	{
#if DEBUG_INFO_OPT	
		ALOGW("Failed to open %s for writing to.\n", Filename);
#endif		
		free(ShaderBinary);
		return false;
	}
    hshMger.hash = hash;
    hshMger.offset = (unsigned int)ftell(outfile);
    hshMger.size = length;
	if(!fwrite((void*)&binaryFormat,sizeof(GLenum),1,outfile)) 
	{
		free(ShaderBinary);
		fclose(outfile);
		return false;
	}

	if(!fwrite(ShaderBinary, length,1,outfile))
	{
		free(ShaderBinary);
		fclose(outfile);
		return false;
	}

	fclose(outfile);

	free(ShaderBinary);

   	FILE* hashfile = fopen(HASHFILE, "ab+");
	if(!hashfile)
	{
	    ALOGW("file=%s open failed",HASHFILE);
	    return false;
	}
	if(!fwrite((void*)&hshMger,sizeof(HashManager),1,hashfile)) 
	{
   	    ALOGW("file=%s fwrite failed");
   	    fclose(hashfile);
	    return false; 
	}
	fclose(hashfile);

	return true;
}

bool loadBinaryProgram(const char* Filename, GLuint &ProgramObjectID,HashManager * HahMager)
{
    FILE* infile = fopen(Filename, "rb");
    int ret = 0;
	if (!infile)
		return false;


    ret = fseek(infile, 0, SEEK_END);
    
    GLsizei length = HahMager->size;

	if (length <=  0)
	{
		fclose(infile);
		return false;
	}

    GLvoid* ShaderBinary = (GLvoid*)malloc(length);

	GLenum format=0;
    fseek(infile, HahMager->offset, SEEK_SET);
    fread(&format, sizeof(GLenum), 1, infile);

    fread(ShaderBinary, length, 1, infile);
    fclose(infile);

	ProgramObjectID = glCreateProgram();
#if DEBUG_INFO_OPT
    ALOGD("ProgramObjectID =%d,length=%ld,format=%d",ProgramObjectID,length,format);
#endif    
    glProgramBinaryOES(ProgramObjectID, format, ShaderBinary, length);

    free(ShaderBinary);

	GLint loaded;
    glGetProgramiv(ProgramObjectID, GL_LINK_STATUS, &loaded);
    if (!loaded)
    {
		int i32InfoLogLength, i32CharsWritten;
		glGetProgramiv(ProgramObjectID, GL_INFO_LOG_LENGTH, &i32InfoLogLength);
		char* pszInfoLog = new char[i32InfoLogLength];
		glGetProgramInfoLog(ProgramObjectID, i32InfoLogLength, &i32CharsWritten, pszInfoLog);
		ALOGW("Failed to load binary program: %s", pszInfoLog);

		delete [] pszInfoLog;
		return false;
    }
	return true;
}

Program::Program(const ProgramDescription& description, const char* vertex, const char* fragment) {
    mInitialized = false;
    mHasColorUniform = false;
    mHasSampler = false;
    mUse = false;
    mLoadbinary = false;
    int len;
    char *hname = NULL;
    unsigned int hash;
    len = strlen(vertex);
    len += strlen(fragment);
    bool ret = 0;
    char layername[100] ;
    bool hashMatched = false;
    HashManager HahMager;
    int binoffset = 0;
#if DEBUG_INFO_OPT    
    struct timeval tpend1, tpend2;
    long usec1 = 0;
    char value[PROPERTY_VALUE_MAX];
    int shctl = 0;
    ALOGD("hwui_debug Program, len=%d",len);
    property_get("sys.hwui_shctl", value, "0");
    shctl = atoi(value);    
    gettimeofday(&tpend1, NULL);
#endif   

#if BuildShaderOPT    
    hname = (char*)malloc(len + 1);
#endif    

    if(hname)
    {
        FILE * pfile = NULL;
        GLuint pID;
        strcpy(hname,vertex);
        strcat(hname,fragment);
        
        hash = JenkinsHashMixBytes(0, (uint8_t*)hname, len);
        for(int i = 0;i < MaxHasList;i++)
        {
            if(hash == ghashlist[i])
            {
                hashMatched = true;
                break;
            }    
        }
        
#if DEBUG_INFO_OPT            
        ALOGD("hwui_debug hash=%u,hashMatched=%d",hash,hashMatched);
       // hashMatched = shctl;
#endif        
        free(hname);
        sprintf(layername, "/data/hwuihas/%u.bin",hash); 
        
        if(hashMatched)
            ret = searchHash(hash,HahMager);
            
        if(ret)
        {

            ret = loadBinaryProgram(DATAFILE,mProgramId,&HahMager);
            if(!ret)
                goto buildshader;      
            else
            {
                position = bindAttrib("position", kBindingPosition);
                if (description.hasTexture || description.hasExternalTexture) {
                    texCoords = bindAttrib("texCoords", kBindingTexCoords);
                } else {
                    texCoords = -1;
                }                
                transform = addUniform("transform");
                projection = addUniform("projection");
#if DEBUG_INFO_OPT                            
                gettimeofday(&tpend2, NULL);
                usec1 = 1000 * (tpend2.tv_sec - tpend1.tv_sec) + (tpend2.tv_usec - tpend1.tv_usec) / 1000;
                ALOGD("hwui_debug loadBinaryProgram use time=%ld ms", usec1); 
#endif                
                mLoadbinary = true;

                return ;
            }    
        }

    }

buildshader:
    // No need to cache compiled shaders, rely instead on Android's
    // persistent shaders cache
#if DEBUG_INFO_OPT                                
    ALOGD("hwui_debug  buildShader");
#endif    
    mVertexShader = buildShader(vertex, GL_VERTEX_SHADER);
    if (mVertexShader) {
        mFragmentShader = buildShader(fragment, GL_FRAGMENT_SHADER);
        if (mFragmentShader) {
            mProgramId = glCreateProgram();

            glAttachShader(mProgramId, mVertexShader);
            glAttachShader(mProgramId, mFragmentShader);

            position = bindAttrib("position", kBindingPosition);
            if (description.hasTexture || description.hasExternalTexture) {
                texCoords = bindAttrib("texCoords", kBindingTexCoords);
            } else {
                texCoords = -1;
            }

            ATRACE_BEGIN("linkProgram");
            glLinkProgram(mProgramId);
            ATRACE_END();

            GLint status;
            glGetProgramiv(mProgramId, GL_LINK_STATUS, &status);
            if (status != GL_TRUE) {
                GLint infoLen = 0;
                glGetProgramiv(mProgramId, GL_INFO_LOG_LENGTH, &infoLen);
                if (infoLen > 1) {
                    GLchar log[infoLen];
                    glGetProgramInfoLog(mProgramId, infoLen, 0, &log[0]);
                    ALOGE("%s", log);
                }
                LOG_ALWAYS_FATAL("Error while linking shaders");
            } else {
                mInitialized = true;
            }
        } else {
            glDeleteShader(mVertexShader);
        }
    }    

#if DEBUG_INFO_OPT                            
    gettimeofday(&tpend2, NULL);
    usec1 = 1000 * (tpend2.tv_sec - tpend1.tv_sec) + (tpend2.tv_usec - tpend1.tv_usec) / 1000;
    ALOGD("hwui_debug buildShader use time=%ld ms", usec1); 
#endif
    if (mInitialized) {
        if(hashMatched)
        {
            #if DEBUG_INFO_OPT                            
            ALOGD("hwui_debug build and save binary");
            #endif
            saveBinaryProgram(DATAFILE,mProgramId,hash);
            
        }    
        transform = addUniform("transform");
        projection = addUniform("projection");
    }
}

Program::~Program() {
    if (mInitialized) {
        // This would ideally happen after linking the program
        // but Tegra drivers, especially when perfhud is enabled,
        // sometimes crash if we do so
        glDetachShader(mProgramId, mVertexShader);
        glDetachShader(mProgramId, mFragmentShader);

        glDeleteShader(mVertexShader);
        glDeleteShader(mFragmentShader);

        glDeleteProgram(mProgramId);
    }
    else if(mLoadbinary)
    {
        glDeleteProgram(mProgramId);
    }
}

int Program::addAttrib(const char* name) {
    int slot = glGetAttribLocation(mProgramId, name);
    mAttributes.add(name, slot);
    return slot;
}

int Program::bindAttrib(const char* name, ShaderBindings bindingSlot) {
    glBindAttribLocation(mProgramId, bindingSlot, name);
    mAttributes.add(name, bindingSlot);
    return bindingSlot;
}

int Program::getAttrib(const char* name) {
    ssize_t index = mAttributes.indexOfKey(name);
    if (index >= 0) {
        return mAttributes.valueAt(index);
    }
    return addAttrib(name);
}

int Program::addUniform(const char* name) {
    int slot = glGetUniformLocation(mProgramId, name);
    mUniforms.add(name, slot);
    return slot;
}

int Program::getUniform(const char* name) {
    ssize_t index = mUniforms.indexOfKey(name);
    if (index >= 0) {
        return mUniforms.valueAt(index);
    }
    return addUniform(name);
}

GLuint Program::buildShader(const char* source, GLenum type) {
    ATRACE_NAME("Build GL Shader");


    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, 0);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        ALOGE("Error while compiling this shader:\n===\n%s\n===", source);
        // Some drivers return wrong values for GL_INFO_LOG_LENGTH
        // use a fixed size instead
        GLchar log[512];
        glGetShaderInfoLog(shader, sizeof(log), 0, &log[0]);
        LOG_ALWAYS_FATAL("Shader info log: %s", log);
        return 0;
    }

    return shader;
}

void Program::set(const mat4& projectionMatrix, const mat4& modelViewMatrix,
        const mat4& transformMatrix, bool offset) {
    if (projectionMatrix != mProjection || offset != mOffset) {
        if (CC_LIKELY(!offset)) {
            glUniformMatrix4fv(projection, 1, GL_FALSE, &projectionMatrix.data[0]);
        } else {
            mat4 p(projectionMatrix);
            // offset screenspace xy by an amount that compensates for typical precision
            // issues in GPU hardware that tends to paint hor/vert lines in pixels shifted
            // up and to the left.
            // This offset value is based on an assumption that some hardware may use as
            // little as 12.4 precision, so we offset by slightly more than 1/16.
            p.translate(Vertex::GeometryFudgeFactor(), Vertex::GeometryFudgeFactor());
            glUniformMatrix4fv(projection, 1, GL_FALSE, &p.data[0]);
        }
        mProjection = projectionMatrix;
        mOffset = offset;
    }

    mat4 t(transformMatrix);
    t.multiply(modelViewMatrix);
    glUniformMatrix4fv(transform, 1, GL_FALSE, &t.data[0]);
}

void Program::setColor(const float r, const float g, const float b, const float a) {
    if (!mHasColorUniform) {
        mColorUniform = getUniform("color");
        mHasColorUniform = true;
    }
    glUniform4f(mColorUniform, r, g, b, a);
}

void Program::use() {
    glUseProgram(mProgramId);
    if (texCoords >= 0 && !mHasSampler) {
        glUniform1i(getUniform("baseSampler"), 0);
        mHasSampler = true;
    }
    mUse = true;
}

void Program::remove() {
    mUse = false;
}

}; // namespace uirenderer
}; // namespace android
