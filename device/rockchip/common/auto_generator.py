#!/usr/bin/env python
import sys
import os
import re

templet = """include $(CLEAR_VARS)
LOCAL_MODULE := %s
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_PATH := $(TARGET_OUT)/%s
LOCAL_SRC_FILES := $(LOCAL_MODULE)$(COMMON_ANDROID_PACKAGE_SUFFIX)
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_DEX_PREOPT := false
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
include $(BUILD_PREBUILT)

"""
def main(argv):
    preinstall_dir = os.path.dirname(argv[0])
    preinstall_dir = os.path.join(preinstall_dir, '../' + argv[1] + '/' + argv[2])
    if os.path.exists(preinstall_dir):
        #Use to define modules for install
        makefile_path = preinstall_dir + '/Android.mk'
        #Use to include modules
        include_path = preinstall_dir + '/preinstall.mk'

        if os.path.exists(makefile_path):
            os.remove(makefile_path)
        if os.path.exists(include_path):
            os.remove(include_path)

        makefile = file(makefile_path, 'w')
        includefile = file(include_path, 'w')

        makefile.write("LOCAL_PATH := $(my-dir)\n\n")
        for root, dirs, files in os.walk(preinstall_dir):
            for file_name in files:
                p = re.compile(r'\S*(?=.apk\b)')
                found = p.search(file_name)
                if found:
                    makefile.write(templet %(found.group(), argv[2]))
                    includefile.write('PRODUCT_PACKAGES += %s\n' %found.group())
        makefile.close()
        includefile.close()

if __name__=="__main__":
  main(sys.argv)
