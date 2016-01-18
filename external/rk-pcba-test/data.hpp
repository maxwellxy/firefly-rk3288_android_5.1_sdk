/*
 * Copyright (C) 2007 The Android Open Source Project
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

#ifndef _DATAMANAGER_HPP_HEADER
#define _DATAMANAGER_HPP_HEADER

#include <string>
#include <utility>
#include <map>

using namespace std;

class DataManager
{
public:
    static int ResetDefaults();
    static int LoadValues(const string filename);
    static int Flush();

    // Core get routines
    static int GetValue(const std::string varName, std::string& value);
    static int GetValue(const std::string varName, int& value);

    // This is a dangerous function. It will create the value if it doesn't exist so it has a valid c_str
    static std::string& GetValueRef(const std::string varName);

    // Helper functions
    static std::string GetStrValue(const std::string varName);
    static int GetIntValue(const std::string varName);

    // Core set routines
    static int SetValue(const std::string varName, std::string value, int persist = 0);
    static int SetValue(const std::string varName, int value, int persist = 0);
    static int SetValue(const std::string varName, float value, int persist = 0);

    static void DumpValues();
	static void SetDefaultValues();
	static void ReadSettingsFile(void);
	
	static std::string GetCurrentStoragePath(void);
	static std::string& CGetCurrentStoragePath();
	static std::string GetCurrentStorageMount(void);
	static std::string& CGetCurrentStorageMount();
	static std::string GetSettingsStoragePath(void);
	static std::string& CGetSettingsStoragePath();
	static std::string GetSettingsStorageMount(void);
	static std::string& CGetSettingsStorageMount();

protected:
    typedef pair<std::string, int> TStrIntPair;
    typedef pair<std::string, TStrIntPair> TNameValuePair;
    static map<std::string, TStrIntPair> mValues;
    static std::string mBackingFile;
    static int mInitialized;

    static map<std::string, std::string> mConstValues;

protected:
    static int SaveValues();

    static int GetMagicValue(std::string varName, std::string& value);

};

#endif // _DATAMANAGER_HPP_HEADER

