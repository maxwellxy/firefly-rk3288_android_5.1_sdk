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
 * @file impexinfo.h
 *
 * @brief
 *   Image info for import/export C++ API.
 *
 *****************************************************************************/
/**
 *
 * @mainpage Module Documentation
 *
 *
 * Doc-Id: xx-xxx-xxx-xxx (NAME Implementation Specification)\n
 * Author: NAME
 *
 * DESCRIBE_HERE
 *
 *
 * The manual is divided into the following sections:
 *
 * -@subpage module_name_api_page \n
 * -@subpage module_name_page \n
 *
 * @page module_name_api_page Module Name API
 * This module is the API for the NAME. DESCRIBE IN DETAIL / ADD USECASES...
 *
 * for a detailed list of api functions refer to:
 * - @ref module_name_api
 *
 * @defgroup module_name_api Module Name API
 * @{
 */

#ifndef __IMPEXINFO_H__
#define __IMPEXINFO_H__

#include <string>
#include <list>
#include <map>


class Tag
{
public:
    enum Type
    {
        TYPE_INVALID = 0,
        TYPE_BOOL    = 1,
        TYPE_INT     = 2,
        TYPE_UINT32  = 3,
        TYPE_FLOAT   = 4,
        TYPE_STRING  = 5
    };

protected:
    Tag( Type type, const std::string& id )
        : m_type (type), m_id (id) { }
    virtual ~Tag() { }

public:
    Type        type() const { return m_type; };
    std::string id() const { return m_id; };

    template<class T>
    bool        getValue( T& value ) const;
    template<class T>
    bool        setValue( const T& value );

    virtual std::string toString() const= 0;
    virtual void        fromString( const std::string& str ) = 0;

private:
    friend class TagMap;

    Type        m_type;
    std::string m_id;
};


class TagMap
{
public:
    typedef std::list<Tag *>::const_iterator                          const_tag_iterator;
    typedef std::map<std::string, std::list<Tag *> >::const_iterator  const_category_iterator;

public:
    TagMap();
    ~TagMap();

private:
    TagMap (const TagMap& other);
    TagMap& operator = (const TagMap& other);

public:
    void clear();
    bool containes( const std::string& id, const std::string& category = std::string() ) const;
    template<class T>
    void insert( const T& value, const std::string& id, const std::string& category = std::string() );
    void remove( const std::string& id, const std::string& category = std::string() );

    Tag *tag( const std::string& id, const std::string& category = std::string() ) const;

    const_category_iterator begin() const ;
    const_category_iterator end() const ;

    const_tag_iterator begin( const_category_iterator iter ) const ;
    const_tag_iterator end( const_category_iterator iter  ) const ;

private:
    typedef std::list<Tag *>::iterator                         tag_iterator;
    typedef std::map<std::string, std::list<Tag *> >::iterator category_iterator;

    std::map<std::string, std::list<Tag *> > m_data;
};


/**
 * @brief ImageExportInfo class declaration.
 */
class ImageExportInfo
    : public TagMap
{
public:
    ImageExportInfo( const char *fileName );
    ~ImageExportInfo();

public:
    const char *fileName() const;
    void       write() const;

private:
    std::string m_fileName;
};



/* @} module_name_api*/

#endif /*__IMPEXINFO_H__*/
