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
skipPerf=""

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

#buildProjectTags=v20100505
#buildProjectTags=v20100513
buildProjectTags=v20100528a

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
mapVersionTag=R3_6_api_cleanup

# directory in which to export builder projects
builderDir=$WORKSPACE/builds/eclipsebuilder

# buildtype determines whether map file tags are used as entered or are replaced with HEAD
buildType=

# directory where to copy build
postingDirectory=$WORKSPACE/builds/transfer/files/master/downloads/drops

#directory for rss feed - not used 
#rssDirectory=/builds/transfer/files/master

# flag to indicate if test build
testBuild=""

# path to javadoc executable
javadoc=""

# value used in buildLabel and for text replacement in index.php template file
#these should come from hudson
builddate=`date +%Y%m%d`
buildtime=`date +%H%M`
timestamp=$builddate$buildtime


# process command line arguments
usage="usage: $0 [-notify emailaddresses][-textRecipients textaddesses][-test][-buildDirectory directory][-buildId name][-buildLabel directory name][-tagMapFiles][-mapVersionTag tag][-builderTag tag][-bootclasspath path][-compareMaps][-skipPerf] [-skipCleanSites] [-hudson] [-skipTest] [-updateSite site][-skipPack][-sign] M|N|I|S|R"

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
		 		  		 		  -hudson) hudson="-Dhudson=true";;
		 		  		 		  -skipTest) skipTest="-Dskip.tests=true";;
		 		  		 		  -deleteArtifacts) deleteArtifacts="-Ddelete.artifacts=true";;
		 		  		 		  -skipPack) skipPack="-DskipPack=true";;
		 		  		 		  -buildDirectory) builderDir="$2"; shift;;
		 		  		 		  -notify) recipients="$2"; shift;;
		 		 		 		  -textRecipients) textRecipients="$2"; shift;;
		 		  		 		  -test) postingDirectory="$WORKSPACE/builds/transfer/files/bogus/downloads/drops";testBuild="-Dtest=true";;
		 		  		 		  -builderTag) buildProjectTags="$2"; shift;;
		 		  		 		  -compareMaps) compareMaps="-DcompareMaps=true";;
		 		  		 		  -updateSite) updateSite="-DupdateSite=$2";shift;;
		 		  		 		  -sign) sign="-Dsign=true";;
		 		  		 		  -*)
		 		  		 		  		 		  echo >&2 $usage
		 		  		 		  		 		  exit 1;;
		 		  		 		  *) break;;		 		  # terminate while loop
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
#internalToolsTag=$buildProjectTags

# tag for exporting org.eclipse.releng.basebuilder
baseBuilderTag=$buildProjectTags

# tag for exporting the custom builder
customBuilderTag=$buildProjectTags



if [ -e $builderDir ]
then
		 		  builderDir=$builderDir$timestamp
fi

# directory where features and plugins will be compiled
echo builderDir $builderDir
buildDirectory=$builderDir/src

mkdir $builderDir
cd $builderDir

#check out org.eclipse.releng.basebuilder
cvs -d :pserver:anonymous@dev.eclipse.org:/cvsroot/eclipse co -r $baseBuilderTag org.eclipse.releng.basebuilder

#check out org.eclipse.releng.eclipsebuilder
cvs -d :pserver:anonymous@dev.eclipse.org:/cvsroot/eclipse co -r $customBuilderTag org.eclipse.releng.eclipsebuilder

javadoc="-Djavadoc15=/shared/common/jdk-1.5.0_16/bin/javadoc"

mkdir -p $postingDirectory/$buildLabel
chmod -R 755 $builderDir

#default value of the bootclasspath attribute used in ant javac calls.  
bootclasspath="/shared/common/jdk-1.5.0_16/jre/lib/rt.jar:/shared/common/jdk-1.5.0_16/jre/lib/jsse.jar:/shared/common/jdk-1.5.0_16/jre/lib/jce.jar"
bootclasspath_15="/shared/common/jdk-1.5.0_16/jre/lib/rt.jar"
bootclasspath_16="/shared/common/jdk-1.6.0_10/jre/lib/rt.jar"
bootclasspath_foundation="/shared/common/org.eclipse.sdk-feature/libs/ee.foundation-1.0.jar"
bootclasspath_foundation11="/shared/common/org.eclipse.sdk-feature/libs/ee.foundation.jar"

echo builderDir $builderDir

PATH=$BASE_PATH:$builderDir/eclipseInternalBuildTools/bin/linux/:$builderDir/jdk/linux/jdk1.5.0_14/jre/bin;export PATH


cd $builderDir/org.eclipse.releng.eclipsebuilder

echo buildId=$buildId >> monitor.properties 
echo timestamp=$timestamp >> monitor.properties 
echo buildLabel=$buildLabel >> monitor.properties 
echo recipients=$recipients >> monitor.properties
echo log=$postingDirectory/$buildLabel/index.php >> monitor.properties

#the base command used to run AntRunner headless
buildMachineArch=`uname -p`
if [ $buildMachineArch == "ppc64" ]
then
        buildLaunchingVM="/shared/common/ibm-java-ppc-605/jre/bin"
else
        buildLaunchingVM="/shared/common/jdk-1.6.x86_64/jre/bin"
fi
if [ $buildMachineArch == "ppc64" ]
then
        buildLaunching15VM="/shared/common/ibm-java2-ppc64-50/jre/bin"
else
        buildLaunching15VM="/shared/common/jdk-1.5.0-22.x86_64/jre/bin"
fi

antRunner="$buildLaunchingVM/java -Xmx500m -Declipse.p2.MD5Check=false -Dorg.eclipse.update.jarprocessor.pack200=$buildLaunching15VM -jar ../org.eclipse.releng.basebuilder/plugins/org.eclipse.equinox.launcher.jar -Dosgi.os=linux -Dosgi.ws=gtk -Dosgi.arch=ppc -application org.eclipse.ant.core.antRunner -Declipse.p2.MD5Check=false"
antRunnerJDK15="$buildLaunching15VM/java -Xmx500m -Dorg.eclipse.update.jarprocessor.pack200=$buildLaunching15VM -jar ../org.eclipse.releng.basebuilder/plugins/org.eclipse.equinox.launcher.jar -Dosgi.os=linux -Dosgi.ws=gtk -Dosgi.arch=ppc -application org.eclipse.ant.core.antRunner  -Declipse.p2.MD5Check=false"


#clean drop directories
#$antRunner -buildfile eclipse/helper.xml cleanSites

echo recipients=$recipients
echo postingDirectory=$postingDirectory
echo builderTag=$buildProjectTags
echo buildDirectory=$buildDirectory

#full command with args
buildCommand="$antRunner -q -buildfile buildAll.xml $mail $testBuild $compareMaps -DmapVersionTag=$mapVersionTag -DpostingDirectory=$postingDirectory -Dbootclasspath=$bootclasspath -DbuildType=$buildType -D$buildType=true -DbuildId=$buildId -Dbuildid=$buildId -DbuildLabel=$buildLabel -Dtimestamp=$timestamp -DmapCvsRoot=:pserver:anonymous@dev.eclipse.org:/cvsroot/eclipse $skipPerf $skipTest $skipPack $tagMaps $hudson -DJ2SE-1.5=$bootclasspath_15 -DJ2SE-1.4=$bootclasspath -DCDC-1.0/Foundation-1.0=$bootclasspath_foundation -DCDC-1.1/Foundation-1.1=$bootclasspath_foundation11 -DOSGi/Minimum-1.2=/shared/common/org.eclipse.sdk-feature/libs/ee.minimum-1.2.0.jar  -DJavaSE-1.6=$bootclasspath_16 -DlogExtension=.xml $javadoc $updateSite $sign -DgenerateFeatureVersionSuffix=true -Djava15-home=/shared/common/ibm-java2-ppc64-50/jre "

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

#clean up
if [ "$delete.artifacts" == "-Ddelete.artifacts=true"  ]
then
		 rm -rf $builderDir
fi


