#!/bin/bash
# User specific environment and startup programs
umask 002

BASE_PATH=.:/bin:/usr/bin:/usr/bin/X11:/usr/local/bin:/usr/bin:/usr/X11R6/bin
LD_LIBRARY_PATH=.
BASH_ENV=$HOME/.bashrc
USERNAME=`whoami`
xhost +$HOSTNAME
DISPLAY=:0.0
CVS_RSH=ssh
ulimit -c unlimited
export CVS_RSH USERNAME BASH_ENV LD_LIBRARY_PATH DISPLAY

proc=$$

#notification list
recipients=

#default text message notification list
textRecipients=

#sets skip.performance.tests Ant property
skipPerformanceTests=""

#sets skip.clean.sites Ant property
skipCleanSites=""

#sets hudson Ant property
hudson=""

#sets skipPack Ant property
skipPack=""

#sets skip.tests Ant property
skipTest=""

#sets sign Ant property
sign=""

tagMaps=""

#delete build artifacts after build is complete
deleteArtifacts=""

#sets fetchTag="HEAD" for nightly builds if required
tag=""

#buildProjectTags=v20091111
#buildProjectTags=v20091112b
#buildProjectTags=v20100113
#buildProjectTags=v20100118a
#buildProjectTags=v20100126
#buildProjectTags=v20100204
#buildProjectTags=v20100208
#buildProjectTags=v20100210
#buildProjectTags=v20100219
#buildProjectTags=v20100222
#buildProjectTags=v20100315
buildProjectTags=v20100423

#updateSite property setting
updateSite=""

#flag indicating whether or not mail should be sent to indicate build has started
mail=""

#flag used to build based on changes in map files
compareMaps=""

#buildId - build name
buildId=""

#buildLabel - name parsed in php scripts <buildType>-<buildId>-<datestamp>
buildLabel=""

# tag for build contribution project containing .map files
mapVersionTag=HEAD

# directory in which to export builder projects
builderDir=/builds/eclipsebuilder

# buildtype determines whether map file tags are used as entered or are replaced with HEAD
buildType=N

# directory where to copy build
postingDirectory=/builds/transfer/files/master/downloads/drops

#directory for rss feed - not used 
#rssDirectory=/builds/transfer/files/master

# flag to indicate if test build
testBuild=""

# path to javadoc executable
javadoc=""

# value used in buildLabel and for text replacement in index.php template file
builddate=`date +%Y%m%d`
buildtime=`date +%H%M`
timestamp=$builddate$buildtime


# process command line arguments
usage="usage: $0 [-notify emailaddresses][-textRecipients textaddesses][-test][-buildDirectory directory][-buildId name][-buildLabel directory name][-tagMapFiles][-mapVersionTag tag][-builderTag tag][-bootclasspath path][-compareMaps][-skipPerf] [-skipCleanSites] [-skipTest] [-updateSite site][-skipPack][-sign] M|N|I|S|R"

if [ $# -lt 1 ]
then
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     echo >&2 "$usage"
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     exit 1
fi

while [ $# -gt 0 ]
do
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     case "$1" in
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     -buildId) buildId="$2"; shift;;
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     -buildLabel) buildLabel="$2"; shift;;
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     -mapVersionTag) mapVersionTag="$2"; shift;;
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     -tagMapFiles) tagMaps="-DtagMaps=true";;
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     -skipPerf) skipPerf="-Dskip.performance.tests=true";;
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     -skipCleanSites) skipCleanSites="-Dskip.clean.sites=true";;
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     -hudson) skipPerf="-Dhudson=true";;
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     -skipTest) skipTest="-Dskip.tests=true";;		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     -deleteArtifacts) deleteArtifacts="-Ddelete.artifacts=true";;
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     -skipPack) skipPack="-DskipPack=true";;
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     -buildDirectory) builderDir="$2"; shift;;
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     -notify) recipients="$2"; shift;;
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     -textRecipients) textRecipients="$2"; shift;;
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     -test) postingDirectory="/builds/transfer/files/bogus/downloads/drops";testBuild="-Dtest=true";;
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     -builderTag) buildProjectTags="$2"; shift;;
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     -compareMaps) compareMaps="-DcompareMaps=true";;
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     -updateSite) updateSite="-DupdateSite=$2";shift;;
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     -sign) sign="-Dsign=true";;
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     -*)
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     echo >&2 $usage
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     exit 1;;
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     *) break;;		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     # terminate while loop
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     esac
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     shift
done

# After the above the build type is left in $1.
buildType=$1

# Set default buildId and buildLabel if none explicitly set
if [ "$buildId" = "" ]
then
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     buildId=$buildType$builddate-$buildtime
fi

if [ "$buildLabel" = "" ]
then
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     buildLabel=$buildId
fi

#Set the tag to HEAD for Nightly builds
if [ "$buildType" = "N" ]
then
        tag="-DfetchTag=HEAD"
        versionQualifier="-DforceContextQualifier=$buildId"
fi

# tag for eclipseInternalBuildTools on ottcvs1
internalToolsTag=$buildProjectTags

# tag for exporting org.eclipse.releng.basebuilder
baseBuilderTag=$buildProjectTags

# tag for exporting the custom builder
customBuilderTag=$buildProjectTags



if [ -e $builderDir ]
then
		 		  		 		   		 		  		 		    		 		  		 		   		 		  		 		     builderDir=$builderDir$timestamp
fi

# directory where features and plugins will be compiled
buildDirectory=$builderDir/src

mkdir $builderDir
cd $builderDir

#check out org.eclipse.releng.basebuilder
cvs -d kmoir@dev.eclipse.org:/cvsroot/eclipse co -r $baseBuilderTag org.eclipse.releng.basebuilder
if [ "$tagMaps" == "-DtagMaps=true" ]; then  
  cvs -d kmoir@dev.eclipse.org:/cvsroot/eclipse rtag -r $baseBuilderTag v$buildId org.eclipse.releng.basebuilder;
fi

#check out org.eclipse.releng.eclipsebuilder
cvs -d kmoir@dev.eclipse.org:/cvsroot/eclipse co -r $customBuilderTag org.eclipse.releng.eclipsebuilder
if [ "$tagMaps" == "-DtagMaps=true" ]; then  
  cvs -d kmoir@dev.eclipse.org:/cvsroot/eclipse rtag -r $customBuilderTag v$buildId  org.eclipse.releng.eclipsebuilder;
fi

#check out eclipseInternalBuildTools and install custom plugins
cvs -d kmoir@ottcvs1:/home/cvs/releng co -r $internalToolsTag eclipseInternalBuildTools
if [ "$tagMaps" == "-DtagMaps=true" ]; then  
  cvs -d kmoir@ottcvs1:/home/cvs/releng rtag -r $internalToolsTag v$buildId eclipseInternalBuildTools;
fi
cp -r eclipseInternalBuildTools/plugins org.eclipse.releng.basebuilder

#The URLs and filenames of vms used in build
linuxJdkArchive=jdks/jdk-1_5_0_22-fcs-bin-b03-linux-i586-09_oct_2009.zip
linuxJdkArchive14=jdks/jdk-1_4_2_14-fcs-bin-b05-linux-i586-14_mar_2007.zip
linuxJdkArchive16=jdks/1.6/jdk-6u17-fcs-bin-b04-linux-i586-11_oct_2009.zip
linuxppcJdkArchive=jdks/IBMJava2-SDK-1.4.2-10.0.ppc.tgz
linuxppcJdkArchive15=jdks/ibm-java2-sdk-5.0-6.0-linux-ppc.tgz
windowsJreArchive=jdks/jdk-1_4_2_16-fcs-bin-b05-windows-i586-16_sep_2007.zip
windows15JdkArchive=jdks/jdk-1_5_0_22-fcs-bin-b03-windows-i586-09_oct_2009.zip
windows16JdkArchive=jdks/1.6/jdk-6u17-fcs-bin-b04-windows-i586-11_oct_2009.zip
windows10FoundationArchive=jdks/weme-win-x86-foundation10_6.1.0.20060317-111429.zip
windows11FoundationArchive=jdks/weme-win-x86-ppro11_6.1.1.20061110-161633.zip

#get then install the Linux 1.6 vm used for running the build
mkdir -p jdk/linux; cvs -d kmoir@ottcvs1:/home/cvs/releng co $linuxJdkArchive; unzip -qq $linuxJdkArchive -d jdk/linux; rm $linuxJdkArchive

mkdir -p jdk/linux14; cvs -d kmoir@ottcvs1:/home/cvs/releng co $linuxJdkArchive14; unzip -qq $linuxJdkArchive14 -d jdk/linux14; rm $linuxJdkArchive14

mkdir -p jdk/linux16; cvs -d kmoir@ottcvs1:/home/cvs/releng co $linuxJdkArchive16; unzip -qq $linuxJdkArchive16 -d jdk/linux16; rm $linuxJdkArchive16

#get the install the Windows jre containing the Java libraries against which to compile
mkdir -p jdk/win32; cvs -d kmoir@ottcvs1:/home/cvs/releng co $windowsJreArchive;unzip -qq $windowsJreArchive -d jdk/win32; rm $windowsJreArchive

#get and install the Windows 1.5 jre containing the 1.5 Java libraries against which to compile
mkdir -p jdk/win32_15; cvs -d kmoir@ottcvs1:/home/cvs/releng co $windows15JdkArchive;unzip -qq $windows15JdkArchive -d jdk/win32_15/; rm $windows15JdkArchive

#get and install the Windows Foundation jre containing the 1.0 Java libraries against which to compile
mkdir -p jdk/win32_foundation; cvs -d kmoir@ottcvs1:/home/cvs/releng co $windows10FoundationArchive;unzip -qq $windows10FoundationArchive -d jdk/win32_foundation/; rm $windows10FoundationArchive

#get and install the Windows Foundation jre containing the 11 Java libraries against which to compile
mkdir -p jdk/win32_foundation11; cvs -d kmoir@ottcvs1:/home/cvs/releng co $windows11FoundationArchive;unzip -qq $windows11FoundationArchive -d jdk/win32_foundation11/; rm $windows11FoundationArchive

#get and install the Windows 1.6 Java libraries against which to compile
mkdir -p jdk/win32_16; cvs -d kmoir@ottcvs1:/home/cvs/releng co $windows16JdkArchive;unzip -qq $windows16JdkArchive -d jdk/win32_16/; rm $windows16JdkArchive

if [ "$HOSTNAME" == "eclipsebuildserv.ottawa.ibm.com" ]
then
    #get then install the Linuxppc vm used for running the build
    mkdir -p jdk/linuxppc; cd jdk/linuxppc; cvs -d kmoir@ottcvs1:/home/cvs/releng co $linuxppcJdkArchive; tar -xzf $linuxppcJdkArchive; rm $linuxppcJdkArchive
    #get then install the Linuxppc 1.5 vm used for generating javadoc
    cd jdk/linuxppc; cvs -d kmoir@ottcvs1:/home/cvs/releng co $linuxppcJdkArchive15; tar -xzf $linuxppcJdkArchive15; rm -rf $linuxppcJdkArchive15; cd ../..
    javadoc="-Djavadoc15=$builderDir/jdk/linuxppc/ibm-java2-ppc-50/bin/javadoc"
else
   javadoc="-Djavadoc15=$builderDir/jdk/linux/jdk1.5.0_22/bin/javadoc"
fi

mkdir -p $postingDirectory/$buildLabel
chmod -R 755 $builderDir

#default value of the bootclasspath attribute used in ant javac calls.  
bootclasspath="$builderDir/jdk/win32/jdk1.4.2_16/jre/lib/rt.jar:$builderDir/jdk/win32/jdk1.4.2_16/jre/lib/jsse.jar:$builderDir/jdk/win32/jdk1.4.2_16/jre/lib/jce.jar"
bootclasspath_15="$builderDir/jdk/win32_15/jdk1.5.0_22/jre/lib/rt.jar"
bootclasspath_16="$builderDir/jdk/win32_16/jdk6_17/jre/lib/rt.jar"
bootclasspath_foundation="$builderDir/jdk/win32_foundation/lib/jclFoundation10/classes.zip"
bootclasspath_foundation11="$builderDir/jdk/win32_foundation11/lib/jclFoundation11/classes.zip"

if [ "$HOSTNAME" == "eclipsebuildserv.ottawa.ibm.com" ]
then
    PATH=$BASE_PATH:$builderDir/eclipseInternalBuildTools/bin/linux/:$builderDir/jdk/linuxppc/IBMJava2-ppc-142/jre/bin;export PATH
else
    PATH=$BASE_PATH:$builderDir/eclipseInternalBuildTools/bin/linux/:$builderDir/jdk/linux/jdk1.5.0_22/jre/bin;export PATH
fi

cd $builderDir/org.eclipse.releng.eclipsebuilder

echo buildId=$buildId >> monitor.properties 
echo timestamp=$timestamp >> monitor.properties 
echo buildLabel=$buildLabel >> monitor.properties 
echo recipients=$recipients >> monitor.properties
#echo textRecipients=$textRecipients >> monitor.properties
echo log=$postingDirectory/$buildLabel/index.php >> monitor.properties

#the base command used to run AntRunner headless
if [ "$HOSTNAME" == "eclipsebuildserv.ottawa.ibm.com" ]
then
antRunner="`which java` -Xmx500m -Dorg.eclipse.update.jarprocessor.pack200=$builderDir/jdk/linuxppc/ibm-java2-ppc-50/bin -jar ../org.eclipse.releng.basebuilder/plugins/org.eclipse.equinox.launcher.jar -Dosgi.os=linux -Dosgi.ws=gtk -Dosgi.arch=ppc -application org.eclipse.ant.core.antRunner -Declipse.p2.MD5Check=false"
antRunnerJDK15="$builderDir/jdk/linuxppc/ibm-java2-ppc-50/jre/bin/java -Xmx500m -Dorg.eclipse.update.jarprocessor.pack200=$builderDir/jdk/linuxppc/ibm-java2-ppc-50/bin -jar ../org.eclipse.releng.basebuilder/plugins/org.eclipse.equinox.launcher.jar -Dosgi.os=linux -Dosgi.ws=gtk -Dosgi.arch=ppc -application org.eclipse.ant.core.antRunner -Declipse.p2.MD5Check=false"
else
#antRunner="$builderDir/jdk/linux14/jdk1.4.2_14/jre/bin/java -Xmx500m -jar ../org.eclipse.releng.basebuilder/plugins/org.eclipse.equinox.launcher.jar -Dosgi.os=linux -Dosgi.ws=gtk -Dosgi.arch=ppc -application org.eclipse.ant.core.antRunner"
antRunner="$builderDir/jdk/linux16/jdk6_17/jre/bin/java -Xmx500m -Declipse.p2.MD5Check=false -Dorg.eclipse.update.jarprocessor.pack200=$builderDir/jdk/linux/jdk1.5.0_22/bin -jar ../org.eclipse.releng.basebuilder/plugins/org.eclipse.equinox.launcher.jar -Dosgi.os=linux -Dosgi.ws=gtk -Dosgi.arch=ppc -application org.eclipse.ant.core.antRunner -Declipse.p2.MD5Check=false"
antRunnerJDK15="$builderDir/jdk/linux/jdk1.5.0_22/jre/bin/java -Xmx500m -Dorg.eclipse.update.jarprocessor.pack200=$builderDir/jdk/linux/jdk1.5.0_22/bin -jar ../org.eclipse.releng.basebuilder/plugins/org.eclipse.equinox.launcher.jar -Dosgi.os=linux -Dosgi.ws=gtk -Dosgi.arch=ppc -application org.eclipse.ant.core.antRunner  -Declipse.p2.MD5Check=false"
fi


#clean drop directories
$antRunner -buildfile eclipse/helper.xml cleanSites

echo recipients=$recipients
echo postingDirectory=$postingDirectory
echo builderTag=$buildProjectTags
echo buildDirectory=$buildDirectory

#full command with args
if [ "$HOSTNAME" == "eclipsebuildserv.ottawa.ibm.com" ]
then
buildCommand="$antRunner -q -buildfile buildAll.xml $mail $testBuild $compareMaps -DmapVersionTag=$mapVersionTag -DpostingDirectory=$postingDirectory -Dbootclasspath=$bootclasspath -DbuildType=$buildType -D$buildType=true -DbuildId=$buildId -Dbuildid=$buildId -DbuildLabel=$buildLabel -Dtimestamp=$timestamp -DmapCvsRoot=:ext:kmoir@dev.eclipse.org:/cvsroot/eclipse $skipPerf $skipTest $skipPack $tagMaps -DJ2SE-1.5=$bootclasspath_15 -DJ2SE-1.4=$bootclasspath -DCDC-1.0/Foundation-1.0=$bootclasspath_foundation -DCDC-1.1/Foundation-1.1=$bootclasspath_foundation11 -DOSGi/Minimum-1.2=$bootclasspath_foundation11 -DJavaSE-1.6=$bootclasspath_16 -DlogExtension=.xml $javadoc $updateSite $sign -DgenerateFeatureVersionSuffix=true -Djava15-home=$builderDir/jdk/linuxppc/ibm-java2-ppc-50/jre -listener org.eclipse.releng.build.listeners.EclipseBuildListener"
else
buildCommand="$antRunner -q -buildfile buildAll.xml $mail $testBuild $compareMaps -DmapVersionTag=$mapVersionTag -DpostingDirectory=$postingDirectory -Dbootclasspath=$bootclasspath -DbuildType=$buildType -D$buildType=true -DbuildId=$buildId -Dbuildid=$buildId -DbuildLabel=$buildLabel -Dtimestamp=$timestamp -DmapCvsRoot=:ext:kmoir@dev.eclipse.org:/cvsroot/eclipse $skipPerf $skipTest $skipPack $tagMaps -DJ2SE-1.5=$bootclasspath_15 -DJ2SE-1.4=$bootclasspath -DCDC-1.0/Foundation-1.0=$bootclasspath_foundation -DCDC-1.1/Foundation-1.1=$bootclasspath_foundation11 -DOSGi/Minimum-1.2=$bootclasspath_foundation11  -DJavaSE-1.6=$bootclasspath_16 -DlogExtension=.xml $javadoc $updateSite $sign -DgenerateFeatureVersionSuffix=true -Djava15-home=$builderDir/jdk/linux/jdk1.5.0_22/jre -listener org.eclipse.releng.build.listeners.EclipseBuildListener"
fi

#capture command used to run the build
echo $buildCommand>command.txt

#run the build
$buildCommand
retCode=$?

if [ $retCode != 0 ]
then
        echo "Build failed (error code $retCode)."
		 		  		 		   		 		  		 		    exit $retCode
fi

if [ "$skip.feed" != "true" ]
then
buildCommandRSS="$antRunnerJDK15 -buildfile $builderDir/org.eclipse.releng.basebuilder/plugins/org.eclipse.build.tools/scripts_rss/feedManipulation.xml"
echo $buildCommandRSS>commandRSS.txt
#run the RSS command
$buildCommandRSS
fi

#clean up
if [ "$delete.artifacts" == "-Ddelete.artifacts=true"  ]
then
		 		  		 		   		 		  		 		    rm -rf $builderDir
fi