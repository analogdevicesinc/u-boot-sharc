###############################################################################
# Copyright (c), 2011 - Analog Devices Inc. All Rights Reserved.
# This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
# and its licensors.
###############################################################################
# File:
#   <installerEgg.py>
# Description:
#    install CCES 1.1.0 egg for CCES 1.1.1 or 1.1.0   
###############################################################################
import sys,os,datetime,re,optparse,zipfile,platform
from xml.etree.ElementTree import ElementTree

def log(msg): print "(%s) : %s"%(datetime.datetime.now().strftime("%m-%d-%H:%M:%S"),msg)
	
def main():
    # get python version
    if sys.version_info >= (2,7,0,0): pythonVersion='py2.7'
    else: pythonVersion='py2.6'
    log('Python major version is %s'%pythonVersion)

    # get python path
    packagePath=os.path.join(os.path.dirname(os.__file__),'site-packages')       
    pathFile=os.path.join(packagePath,'easy-install.pth') 
    if not os.path.isfile(pathFile): 
        with open(pathFile,'w') as f: pass
    log('Python package dir is %s'%packagePath)    
    
    if platform.win32_ver()[0] == 'XP':
        profileFolder=os.path.join(os.environ['ALLUSERSPROFILE'],'Application Data','Analog Devices')
    else:
        profileFolder=os.path.join(os.environ['ALLUSERSPROFILE'],'Analog Devices')    
    
    for product in ElementTree().parse(os.path.join(profileFolder,'adiGPR.xml')).iter('product'):
        if product.attrib['internal_name']=="CCES":        
            version=product.attrib['version']
   
    repo=r'\\ctse-build-w1\stage\tpsdkserver'
    folder=r'cces.1.1.0.win32'
    if version <= "1.1.1.0":
        ccesRepo=os.path.join(repo,folder)
        eggFile=r'TpsdkServer-1.0.1404171512-py2.7.egg'
        try:
            log('Installing TpsdkServer version to %s...'%(os.path.join(packagePath,eggFile)))
            zip = zipfile.ZipFile(os.path.join(ccesRepo,eggFile))
            zip.extractall(os.path.join(packagePath,eggFile))
            pthContent=[]
            with open(pathFile) as f:
                for line in f:
                    match=re.match('^\./tpsdkserver-([\d\.]*)-%s.egg'%pythonVersion,line)
                    if match:
                        pthContent.append('./'+eggFile.lower()+'\n')
            # write current installation folder to pth file(python import path)            
            with open(pathFile,'w') as f: 
                for line in pthContent:f.write(line)
            log('Installation sucessfully Done.')
        except Exception,e:
            log('Failed to install TpsdkServer egg with error %s.' %e)        
    else:
        print "The CCES version > 1.1.1, do not need to install CCES 1.1.0 egg."
        pass

#################################################################
# Command line support
#################################################################            
if __name__ == "__main__":
    print "TpsdkServer egg installer script"
    ret = main()

