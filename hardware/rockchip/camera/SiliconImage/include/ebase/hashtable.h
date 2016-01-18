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
 * @file hashtable.h
 *
 * @brief
 *   Extended data types: Hashtable
 *
 *****************************************************************************/
/**
 * @defgroup module_ext_hashtable Hashtable
 *
 * @{
 *
 *****************************************************************************/
#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include "types.h"
#include "ext_types.h"

typedef struct _GHashTable GHashTable;

/**
 * @brief Function pointer to
 */
typedef void (*GHFunc) (void* key, void* value, void* user_data);

/**
 * @brief  Function pointer to
 */
typedef bool_t (*GHRFunc) (void* key, void* value, void* user_data);

/**
 * @brief  Function pointer to
 */
typedef void (*GDestroyNotify)(void* data);

/**
 * @brief  Function pointer to
 */
typedef uint32_t (*GHashFunc) (const void* key);
/**
 * @brief  Function pointer to compare elements
 */
typedef bool_t (*GEqualFunc) (const void* a, const void* b);
/**
 * @brief  Function pointer to free data
 */
typedef void (*GFreeFunc) (void* data);


#define hashTableInsert(h,k,v)    hashTableInsertReplace ((h),(k),(v),BOOL_FALSE)
#define hashTableReplace(h,k,v)   hashTableInsertReplace ((h),(k),(v),BOOL_TRUE)


/*****************************************************************************/
/**
 * @brief
 *
 * @param
 *
 * @return
 * @retval
 *
 *****************************************************************************/
GHashTable* hashTableNew(GHashFunc hash_func, GEqualFunc key_equal_func);


/*****************************************************************************/
/**
 * @brief
 *
 * @param
 *
 * @return
 * @retval
 *
 *****************************************************************************/
GHashTable* hashTableNewFull(GHashFunc hash_func, GEqualFunc key_equal_func, GDestroyNotify key_destroy_func, GDestroyNotify value_destroy_func);


/*****************************************************************************/
/**
 * @brief
 *
 * @param
 *
 * @return
 * @retval
 *
 *****************************************************************************/
void hashTableInsertReplace(GHashTable* hash, void* key, void* value, bool_t replace);


/*****************************************************************************/
/**
 * @brief
 *
 * @param
 *
 * @return
 * @retval
 *
 *****************************************************************************/
uint32_t hashTableSize(GHashTable* hash);


/*****************************************************************************/
/**
 * @brief
 *
 * @param
 *
 * @return
 * @retval
 *
 *****************************************************************************/
void* hashTableLookup(GHashTable* hash, const void* key);


/*****************************************************************************/
/**
 * @brief
 *
 * @param
 *
 * @return
 * @retval
 *
 *****************************************************************************/
bool_t hashTableLookupExtended(GHashTable* hash, const void* key, void** orig_key, void** value);


/*****************************************************************************/
/**
 * @brief
 *
 * @param
 *
 * @return
 * @retval
 *
 *****************************************************************************/
void hashTableForeach(GHashTable* hash, GHFunc func, void* user_data);


/*****************************************************************************/
/**
 * @brief
 *
 * @param
 *
 * @return
 * @retval
 *
 *****************************************************************************/
void* hashTableFind(GHashTable* hash, GHRFunc predicate, void* user_data);


/*****************************************************************************/
/**
 * @brief
 *
 * @param
 *
 * @return
 * @retval
 *
 *****************************************************************************/
bool_t hashTableRemove(GHashTable* hash, const void* key);


/*****************************************************************************/
/**
 * @brief
 *
 * @param
 *
 * @return
 * @retval
 *
 *****************************************************************************/
uint32_t hashTableForeachRemove(GHashTable* hash, GHRFunc func, void* user_data);


/*****************************************************************************/
/**
 * @brief
 *
 * @param
 *
 * @return
 * @retval
 *
 *****************************************************************************/
uint32_t hashTableForeachSteal(GHashTable* hash, GHRFunc func, void* user_data);


/*****************************************************************************/
/**
 * @brief
 *
 * @param
 *
 * @return
 * @retval
 *
 *****************************************************************************/
void hashTableDestroy(GHashTable* hash);


/*****************************************************************************/
/**
 * @brief
 *
 * @param
 *
 * @return
 * @retval
 *
 *****************************************************************************/
uint32_t spacedPrimesClosest(uint32_t x);


/*****************************************************************************/
/**
 * @brief
 *
 * @param
 *
 * @return
 * @retval
 *
 *****************************************************************************/
bool_t directEqual(const void* v1, const void* v2);


/*****************************************************************************/
/**
 * @brief
 *
 * @param
 *
 * @return
 * @retval
 *
 *****************************************************************************/
uint32_t directHash(const void* v1);


/*****************************************************************************/
/**
 * @brief
 *
 * @param
 *
 * @return
 * @retval
 *
 *****************************************************************************/
bool_t intEqual(const void* v1, const void* v2);


/*****************************************************************************/
/**
 * @brief
 *
 * @param
 *
 * @return
 * @retval
 *
 *****************************************************************************/
uint32_t intHash(const void* v1);


/*****************************************************************************/
/**
 * @brief
 *
 * @param
 *
 * @return
 * @retval
 *
 *****************************************************************************/
bool_t strEqual(const void* v1, const void* v2);


/*****************************************************************************/
/**
 * @brief
 *
 * @param
 *
 * @return
 * @retval
 *
 *****************************************************************************/
uint32_t strHash(const void* v1);

/* @} module_ext_hashtable */

#endif /* __HASHTABLE_H__ */
