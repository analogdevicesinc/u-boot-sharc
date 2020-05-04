#!/bin/bash

# Copyright Analog Devices 2020. All Rights Reserved
# Written by: David Gibson <David.Gibson@analog.com>
# reposync: Tool for synchronizing specific branches between two mirrors of a repository
#           Assuming that the repo you are sync'ing from is the development repo.
#           Assuming that the repo you are sync'ing to is a mirror, that is read-only with no conflicts

# Load configuration information from a config file passed in as an arg

function usage() {
    echo "$0: <repo>"
    echo "    where a corresponding <repo>_rconf.sh exists in the CWD"
    exit $1
}

if [ $# -lt 1 ]
then
    usage -1
fi

FILES=""
while (("$#")); do
    f=$1
    if [ ! -e "${f}_rconf.sh" ]
    then
      echo "Error: ${f}_rconf.sh configuration file not found"
      usage -2
    else
        FILES="${FILES} ${f}_rconf.sh"
    fi
    shift
done

for curFile in $FILES
do
    echo "Parsing repository: $curFile"
    # RESET all the environment variables we support in the config file
    # The source the config file
    REPONAME=
    LOCAL_URL=
    REMOTE_URL=
    . $curFile
    echo "#################################################################"
    echo "Synchronising:  $REPONAME"
    echo "         FROM:  $LOCAL_URL"
    echo "           TO:  $REMOTE_URL"
    echo "     BRANCHES:  ${BRANCHES_TO_PUSH}"
    echo ""
    localDir="`basename $LOCAL_URL .git`_local"
    # TODO : Add option to update local directory
    if [ -d ${localDir} ]
    then
        rm -rf ${localDir}
    fi 
    git clone ${LOCAL_URL} ${localDir}
    cd ${localDir}
    git remote add reposync ${REMOTE_URL}
    for curBranch in ${BRANCHES_TO_PUSH}
    do
        echo "    Syncing: ${curBranch}"
        git checkout -b ${curBranch} origin/${curBranch}
        git push reposync ${curBranch}
    done
done