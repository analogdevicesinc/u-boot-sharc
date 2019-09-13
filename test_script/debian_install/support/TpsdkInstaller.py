###############################################################################
# Copyright (c), 2011 - Analog Devices Inc. All Rights Reserved.
# This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
# and its licensors.
###############################################################################
# File:
#   <TpsdkInstaller.py>
# Description:
#   the script install TpsdkServer eggs in following steps
#      - detects current python version
#      - scan python extension, extract installed version number of TpsdkServer
#      - downloads latest egg of current python version
#      - compare tpsdkServer version with download, make a call to install or
#        not
###############################################################################
import sys,os,datetime,re,optparse,zipfile


def log(msg): print "(%s) : %s"%(datetime.datetime.now().strftime("%m-%d-%H:%M:%S"),msg)

#map the release version of CCES
def getProductVersion(p):
	match=re.match(r'([A-Z0-9]{5}-\d{2}[PES]{0,1})-([\d\.]{5})[\.]{0,1}([\d]{0,1})-[BR]{1}',p)
	if match:
				return 'cces'+'.'+match.group(2)
	
def main(repo,p,dst):

    # get python version
    if sys.version_info >= (2,7,0,0): pythonVersion='py2.7'
    else: pythonVersion='py2.6'
    log('Python major version is %s'%pythonVersion)

    # get python path, if the dst virtualenv is pecified, use it as teh python path.
    if dst == 'default':
        packagePath=os.path.join(os.path.dirname(os.__file__),'site-packages')
    else:
        packagePath=os.path.join(dst,'Lib','site-packages')
        
    pathFile=os.path.join(packagePath,'easy-install.pth') 
    if not os.path.isfile(pathFile): 
        with open(pathFile,'w') as f: pass
    log('Python package dir is %s'%packagePath)


    # scan proper version of latest egg from repo for CCES
    try:
        folders=os.listdir(repo)
        folders.sort(reverse=True)

	ccesRepo=''
	#scan for corresponding CCES repo of TpsdkServer egg files
	if p == 'latest':
                for folder in folders:
			match=re.match(r'^cces.*',folder)
			if match:
				ccesRepo=os.path.join(repo, folder)
				break
	else:
                products = [pp.strip() for pp in p.split(',')]

                # Get the CCES productID from products
                for productId in products:
                        if (not productId.find('ANDTL-00')==-1) or (not productId.find('CCEST-02')==-1):
                                ccesProductID=productId
                                version = getProductVersion(ccesProductID)
                                for folder in folders:
                                        if version in folder:
                                                ccesRepo=os.path.join(repo, folder)
                                                break
                                break
			
	if not ccesRepo:
                log('Failed to find proper TpsdkServer egg from %s for product %s. Using the TpsdkServer egg for latest CCES version instead.'%(repo,p))
		for folder in folders:
			match=re.match(r'^cces.*',folder)
			if match:
                                ccesRepo=os.path.join(repo, folder)
				break
				
	#scan for latest egg from corresponding CCES repo
	eggs=os.listdir(ccesRepo)
	eggs.sort(reverse=True)

	for egg in eggs:
		match=re.match('^TpsdkServer-([\d\.]*)-%s.egg'%pythonVersion,egg)
		if match:
			latestVersion=match.group(1)
			eggFile=egg
			print repo
			log('Found latest version egg %s of %s from repository %s'%(latestVersion,pythonVersion,ccesRepo))
			break

    except Exception,e:
	log('Failed to scan TpsdkServer egg of %s from %s. error:%s'%(pythonVersion,repo,e))
	return False


    # scan TpsdkServer installation, locate latest one, generate new .pth content
    currentVersion=''
    pthContent=[]
    with open(pathFile) as f:
        for line in f:
            match=re.match('^\./tpsdkserver-([\d\.]*)-%s.egg'%pythonVersion,line)
            if match:
                currentVersion=match.group(1)
                pthContent.append('./'+eggFile.lower()+'\n')
            else:
                pthContent.append(line)
    if currentVersion:
        log('TpsdkServer current installation version is %s'%currentVersion)
    else:
        pthContent.append('./'+eggFile.lower()+'\n')
        log('No valid TpsdkServer installation can be found.')


    # check if we need to install latest egg, install it
    # then add new folder to python import path
    try:
        if latestVersion!=currentVersion:                
            log('Installing TpsdkServer version %s to %s...'%(latestVersion,os.path.join(packagePath,eggFile)))
            zip = zipfile.ZipFile(os.path.join(ccesRepo,eggFile))
            zip.extractall(os.path.join(packagePath,eggFile))            

            # write current installation folder to pth file(python import path)            
            with open(pathFile,'w') as f: 
                for line in pthContent:f.write(line) 

            log('Installation sucessfully Done.')
            return True
        else:
            log('Same version is already installed, nothing to do.')
            return True
    except Exception,e:
        log('Failed to install TpsdkServer egg of %s from %s for %s with error %s.'%(pythonVersion,repo,p,e))
        return False






            
#################################################################
# Command line support
#################################################################            
if __name__ == "__main__":

    print "TpsdkServer egg installer script, see help with option '--help'"
    parser = optparse.OptionParser( usage='%prog [options]', version='1.0' )
    parser.add_option( '--repo',help=r'Repository of TpsdkServer egg, default is \\ctse-build-w1\stage\tpsdkserver',dest='repo', default=r'\\ctse-build-w1\stage\tpsdkserver' )
    parser.add_option( '--enable-exit-status',help='return non-zero code for failure, default always returns zero',dest='enableExitStatus', action='store_true', default=False)  
    parser.add_option( '--p',help=r'CCES productID to install proper TpsdkServer egg for, default is using latest CCES productID',dest='productID', default='latest' )
    parser.add_option( '--dst',help='Path of virtualenv path, default is default python environment',dest='dst', default='default')  

    (options,args) = parser.parse_args()

    ret=main(options.repo,options.productID,options.dst)

    if options.enableExitStatus:
        if ret: sys.exit(0)
        else: sys.exit(1)
                        
