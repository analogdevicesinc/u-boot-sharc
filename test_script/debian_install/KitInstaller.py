###############################################################################
# Copyright (c), 2011 - Analog Devices Inc. All Rights Reserved.
# This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
# and its licensors.
###############################################################################
# File:
#   <KitInstaller.py>
# Description:
#   command line kit installer script
###############################################################################
import re,os,sys,platform,shutil,subprocess,optparse,string,datetime,re,ConfigParser,copy,glob
import shlex
import threading
import time
from xml.etree.ElementTree import Element,ElementTree
from support.EasyProcess import EasyProcess

ROOT = os.path.dirname(os.path.abspath(__file__))
sys.path.append(ROOT)



##  use a auto-flush stdout, required for capturing script's realtime stdout
class FlushIO(object):
    def __init__(self, f):
        self.f = f
    def write(self, x):
        self.f.write(x)
        self.f.flush()

sys.stdout = FlushIO(sys.__stdout__)


#################################################################
# A base implementation for MSI based packaging
# etc, Andromeda kit
#################################################################

##  an exception class for the installer 
class InstallerException(Exception): 
    pass


##  the install class
class Installer(object):

    # set 1 hour for the max installation/uninstall timeout
    TIMEOUT=1*60*60
    TEMPDIR=os.path.join(ROOT,'temp')
    SUPPORTDIR=os.path.join(ROOT,'support')
    IGNOREEXE=['instmsiw']


    def __init__(self,productId,kitId='',kitMirror='',installPath='',kitLicense='',supportFiles=False,keepLicense=False,nonDefault=False,multiVersion=False,python=None):
        self.userLicense=os.path.abspath(kitLicense) if kitLicense else ''
        self.python=python
        if platform.system()=='Linux':           
            self.linuxLocalKitMirror='/home/test/kitmirror'       
        # extract product key, version and build type from productId
        productKey=''
        productVersion=''
        buildType=''

        match=re.match(r'([A-Z0-9]{5}-\d{2}[PESC]{0,1})(-\d{1,2}\.\d{1,2}\.\d{1,2}-[B,A,R]){0,1}',productId)
        if match: productKey=match.group(1)                
        match=re.match(r'[A-Z0-9]{5}-\d{2}[PESC]{0,1}-(\d{1,2}\.\d{1,2}\.\d{1,2}|\d{1,2}\.\d{1,2}\.\d{1,2}\.\d{1,2})-([B,A,R])',productId)
        if match: 
            productVersion=self.handleVersion(match.group(1))
            buildType={'B':'BETA','A':'ALPHA','R':'RELEASE','I':'INTERNAL'}[match.group(2)]

        # get profile folder
        if platform.system()=='Windows':            
            if platform.win32_ver()[0] == 'XP':
                self.profileFolder=os.path.join(os.environ['ALLUSERSPROFILE'],'Application Data','Analog Devices')
            else:
                self.profileFolder=os.path.join(os.environ['ALLUSERSPROFILE'],'Analog Devices')
        elif platform.system()=='Linux':
            self.profileFolder='/var/analog'
			
        # read installed Analog Devices products information
        self.adiGPR={}
        self.retrieveAdiGPR()        
        
        # get default settings from XML config for all versions of this product (same product key)
        self.products=[]
        self.product=None
        for productNode in ElementTree().parse(os.path.join(self.SUPPORTDIR,'KitInstaller.xml')).iter('product'):
            if productKey==productNode.attrib['productKey']:
                for versionNode in productNode.iter('version'):                   
                    data=productNode.attrib
                    data = copy.deepcopy(data)
                    data.update(versionNode.attrib)
                    try:
                        data['packageName']
                    except:data['packageName']=''
                    data['kitId']='latest'
                    data['supportFiles']=''		    
                    data['tempDir']=self.TEMPDIR
                    data['supportDir']=self.SUPPORTDIR																				    					
                    data['kitLicense']=os.path.abspath(os.path.join(ROOT,data['kitLicense'])) if data['kitLicense'] else ''
                    data['version'] = self.handleVersion(data['version'])        		
		    if platform.system()=='Windows':                                              
                        if kitMirror: data['kitMirror']=os.path.abspath(kitMirror)
                        data['installPath']=''
                        if installPath: data['installPath']=os.path.abspath(installPath) 
			if nonDefault: data['installPath']=os.path.join(r'C:\Analog Devices',productNode.attrib['productKey'])                        
                        if supportFiles:
                            data['supportFilesCmd']='cmd.exe /C ""$supportDir\\7za.exe" x "$supportFiles" -o"$installPath"" -y'						
                            data['supportFiles']=os.path.join(self.TEMPDIR,'support_files.zip')
                            data['supportFilesCmd']=string.Template(data['supportFilesCmd']).substitute(data)                            

			try:
                            data['binPattern']=data['windows_binPattern']
                        except: data['binPattern']=data['binPattern']						
                        data['installedDir']=self.getInstalledKitInfoKeyValue(data['productGuid'],'installDirectory',data['packageName'],data['version'])                        
                        try:
			    data['installCmd']=string.Template(data['windows_installCmd']).substitute(data)
			except: data['installCmd']=string.Template(data['installCmd']).substitute(data)
			try:
                            data['cleanupCmd']=string.Template(data['windows_cleanupCmd']).substitute(data)
                        except:data['cleanupCmd']=string.Template(data['cleanupCmd']).substitute(data)
                        if not data['kitMirror'].find("\\\\mach5\\")==-1:
                            data['kitMirror'] = data['kitMirror'].replace("mach5", "mach5.spd.analog.com")
                        data['specificKitFolder']=os.path.join(data['kitMirror'],data['kitFolder'])
                        try:
			    data['uninstallCmd']=string.Template(data['windows_uninstallCmd']).substitute(data)
			except: data['uninstallCmd']=string.Template(data['uninstallCmd']).substitute(data)    
		    elif platform.system()=='Linux':                                         	        
                        if kitMirror:data['kitMirror']=os.path.basename(kitMirror)                           
                        else:data['kitMirror']=os.path.basename(data['kitMirror']).replace ('\\\\','\\')                                            
                        if supportFiles: 
			    pass #TODO#
                        data['installPath']=os.path.join('/opt/analog',data['packageName'],data['version'])
                        # workaround for None-default install path: same as default install path in Linux
                        if nonDefault: data['installPath']=os.path.join('/opt/analog',data['packageName'],data['version'])   							
                        try:
                            data['binPattern']=data['linux_binPattern']
                        except: self.log('No binpattern recorded for %s in KitInstaller.xml for Linux'%data['productKey'])			
			if installPath:
                            pass #TODO#
			data['installedDir']=self.getInstalledKitInfoKeyValue(data['productGuid'],'installDirectory',data['packageName'],data['version'])
			try:
                            data['installCmd']=string.Template(data['linux_installCmd']).substitute(data)
                        except: self.log('No installCmd recorded for %s in KitInstaller.xml for Linux'%data['productKey'])
                        try:
                            data['uninstallCmd']=string.Template(data['linux_uninstallCmd']).substitute(data)
                        except: self.log('No uninstallCmd recorded for %s in KitInstaller.xml for Linux'%data['productKey'])
                        if not data['kitMirror'].find("\\mach5")==-1:
                            data['kitMirror'] = data['kitMirror'].replace("mach5", "mach5.spd.analog.com")
                        data['specificKitFolder']=data['kitMirror']+'\\'+data['kitFolder']                                                                                                							                       
		    
		    if kitId: data['kitId']=kitId                                                                                                                                                                                                                                         
                    if data['version']==productVersion and data['buildType']==buildType:
                        self.product=data                   
                    self.products.append(data)
                 
        if not self.product: 
            try: self.product=self.products[0]
            except: raise InstallerException('Could not find an entry of specified product "%s" in KitInstaller.xml'%productId)                                                                                        
        
        if multiVersion:
            self.products=[self.product]                
    
    ## retrieve the adiGPR.xml in Windows and constructed dict in Linux to ensure all the installed products are already existed.
    def retrieveAdiGPR(self):
        if platform.system()=='Windows':
            try:
                for product in ElementTree().parse(os.path.join(self.profileFolder,'adiGPR.xml')).iter('product'):
                    guid='{%s}'%(product.attrib['guid'])
                    self.adiGPR[guid]=product.attrib                
            except: pass
         
        elif platform.system()=='Linux':
            installedFullPackageName=[]
	    inforDict={}			
	    temp=os.popen("dpkg -l | grep adi-").read().strip().split('\n')
            for i in temp:
                if "adi" in i:
                    installedFullPackageName.append(i.split(' ')[2])                                      
            try:
                for fullPackageName in installedFullPackageName:		
		    value=os.popen('dpkg -s %s' %fullPackageName).read().split('\n')
                    for t in value:
                        if t=='':
                            value.remove(t)
                    for t in value:
		        inforDict[t.split(':')[0]]=t.split(':')[1].strip()
			    
		    value=os.popen("dpkg -s %s | grep Package | awk '{print $2}'" %fullPackageName).read()
                    installedPackageName=value.split('\n')[0].split('-')[1]					
		    inforDict['packageName']=installedPackageName
                    inforDict['version']=os.popen("dpkg -s %s | grep Version | grep -v KitVersion | awk '{print $2}'" %fullPackageName).read().strip()
                    inforDict['kitVersion']=os.popen("dpkg -s %s | grep ADIVersion | awk '{print $2}'" %fullPackageName).read().strip()
                    inforDict['installDirectory']=os.path.join('/opt/analog',installedPackageName,inforDict['version'])  
                    inforDict['package_type']=os.popen("dpkg -s %s | grep ADIPackageType | awk '{print $2}'" %fullPackageName).read().strip()
		    inforDict['buildType']=os.popen("dpkg -s %s | grep ADIBuildType | awk '{print $2}'" %fullPackageName).read().strip()           
                    self.adiGPR[fullPackageName]=inforDict 
            except: pass
	    
           
    ##  a simple logger    
    def log(self,msg):
        print "(%s) : %s"%(datetime.datetime.now().strftime("%m-%d-%H:%M:%S"),msg)

    def handleVersion(self, version):
        if platform.system()=='Windows':	
            versions = [v.strip() for v in version.split('.')]
            if len(versions)==3:
                version = version + '.0'
	elif platform.system()=='Linux':
	    version = version
        return version

    def handleSupportFiles(self):        
        self.log("Start handling the support file...")
        
        if os.path.isfile(self.product['supportFiles']):
	    if platform.system()=='Windows':			
                result = self.batchCall('SUPPORTFILES',self.product['supportFilesCmd'])
                if result.return_code != 0:
                    self.log(result.stderr)
                    raise InstallerException('Failed to handle the support file. Error %d.'%(result.return_code))
            elif platform.system()=='Linux':
		self.log("Linux cannot support to handle support file now.")
	else:
            self.log("Cannot find the support_files.zip, nothing need to do.")

        self.log("Finished handling the support file.")

    ##  batch call
    def batchCall(self,action,batch):
        if not os.path.exists(self.TEMPDIR):
            os.makedirs(self.TEMPDIR)

        if platform.system()=='Windows':
            batchFile=os.path.join(self.TEMPDIR,"%s_%s_%s.bat"%(action,self.product['productKey'],datetime.datetime.now().strftime("%Y%m%d%H%M%S")))
            f=open(batchFile,'w')
            f.write(batch)
            f.flush()
            f.close()     
        elif platform.system()=='Linux':
            batchFile=os.path.join(self.TEMPDIR,"%s_%s_%s.sh"%(action,self.product['productKey'],datetime.datetime.now().strftime("%Y%m%d%H%M%S")))                
            f=open(batchFile,'w')
            f.write('#!/bin/sh \n'+batch)
            f.flush()
            f.close()
            os.system('chmod +x %s' %(batchFile))                 
        return EasyProcess([batchFile],logger=self.log).call(timeout=self.TIMEOUT)


    ##  read key value from windows registry
    def getRegistryKeyValue(self,registry,key):
        if platform.system()=='Windows':
            cmd=EasyProcess(['reg','QUERY',registry,'/v',key])      
            result=cmd.call().stdout.replace('\r\n','').replace('"','').replace('\r','')  
            sz=re.match(r'.*%s\s+REG_SZ\s+(.*)'%key,result)
            dword=re.match(r'.*%s\s+REG_DWORD\s+(.*)'%key,result)
            if sz: return sz.group(1)
            elif dword: return int(dword.group(1),0)
            else: return ''
        elif platform.system()=='Linux':
            pass        

    ##  find specific kit from the kit mirror    
    def findKit(self,kitId,kitMirror):
        self.log('Looking for kit on mirror: %s'%kitMirror)
        if platform.system()=='Windows':
            if not os.path.isdir(kitMirror):
                raise InstallerException('Failed to find kit mirror: %s'%kitMirror)
        elif platform.system()=='Linux':            
            self.log('mount server kitMirror to local Linux pc kitMirror.')
            if not os.path.exists(self.linuxLocalKitMirror):		
                os.makedirs(self.linuxLocalKitMirror)
            else:               
                # umount local mirror
                if not os.system('umount -l %s > /dev/null' %self.linuxLocalKitMirror) in [0,256]:
                    self.log('Fail to umount local mirror!')

            kitMirror=kitMirror.replace('\\','\\\\')
            kitMirror='\\\\\\'+kitMirror[1:]            

            if not os.system("mount -t cifs %s %s -o user=testlab2,password=Labrat1 > /dev/null" %(kitMirror,self.linuxLocalKitMirror)) in [0,8192,256]:                
                self.log('Fail to parse the server kitMirror to local Linux kitMirror!')
		sys.exit(-1)
            kitMirror=self.linuxLocalKitMirror                  
        allDir = os.listdir(kitMirror)

        # find the specified kit
        kitsDir=[]
        for kit in allDir:
            path = os.path.join(kitMirror,kit)
            if (os.path.isdir(path)):
                m = re.search(kitId,kit)
                if (m != None):
                    self.log('Found kit with ID %s: %s'%(kitId,path))
                    return (kitId,path)
                m = re.search('w[0-9]+',kit)
                if (m != None):
                    kitsDir.append(kit)

        # specified kit not found, then check for latest
        if kitsDir and kitId=='latest':
            kitsDir.sort()
            kitsDir.reverse()            
            findBin = False
            for k in kitsDir:
                path=os.path.join(kitMirror,k)
                pattern=re.compile(re.sub('\*','.*', self.product['binPattern']))
                for file in os.listdir(path):
                    if pattern.search(file):
                        file = os.path.join(path,file)
                        try:
                            f = open(file)
                            f.close()
                        except:
                            continue
                        findBin = True
                if findBin:
                    kitId = re.match('(w[0-9]+).*',k).group(1)
                    break
            if findBin:
                self.log('Found latest kit %s at %s'%(kitId,path))
                return (kitId,path)
            else:
                raise InstallerException('Failed to find kit with ID: %s'%kitId)
        else:
            raise InstallerException('Failed to find kit with ID: %s'%kitId)     

    ##  download the found kit to local temp folder        
    def downloadKit(self):
        kitId,kitPath=self.findKit(self.product['kitId'],self.product['specificKitFolder'])        

        # download the kit
        self.log('Downloading kit to %s...'%self.TEMPDIR)
        if os.path.exists(self.TEMPDIR):
            time.sleep(10)
            shutil.rmtree(self.TEMPDIR)
        shutil.copytree(kitPath,self.TEMPDIR)
        # locate installation binary
        pattern=re.compile(re.sub('\*','.*',self.product['binPattern']))
        for file in os.listdir(self.TEMPDIR):
            if pattern.search(file):
                # update installation command
                self.product['installCmd']=self.product['installCmd'].replace('INSTALL_BINARY',file)
                self.getExecution()
                self.log('Kit successfully downloaded.')
                return
                
        raise InstallerException('Failed to locate installation binary of kit %s'%kitId)        


    def getExecution(self):       
        if not self.product['installCmd'].find('EXECUTION')==-1:
	    if platform.system()=="Windows":
                pattern = re.compile(r'.*\&\&(.*)\&\&')
                unzipCmd = pattern.search(self.product['installCmd']).group(1)
                self.product['installCmd'] = self.product['installCmd'].replace(unzipCmd, ' echo "" ')
                unzipCmd = 'cd %s && '%self.TEMPDIR + unzipCmd
                result = self.batchCall('UNZIP',unzipCmd)       

                if result.return_code != 0:
                    self.log(result.stderr)
                    raise InstallerException('Failed to unzip the installer of %s: %s'%(self.product['productName'],result.return_code))     
                unzipDir=os.path.join(self.TEMPDIR, self.product['productName'])
                        
                pattern=re.compile(re.sub('\*','.*', '*.exe'))
                for file in os.listdir(unzipDir):
                    if pattern.search(file):
                        for exe in self.IGNOREEXE:
                            if not file.find(exe)==-1:
                                pass
                            else:
                                self.product['installCmd']=self.product['installCmd'].replace('EXECUTION',file)
            elif platform.system()=="Linux":
                self.log('Linux cannot support the EXECUTION in install command now.')
                #TODO#

    def os_platform(self):
        if platform.system()=='Windows':
            true_platform = os.environ['PROCESSOR_ARCHITECTURE']
            try:
                true_platform = os.environ["PROCESSOR_ARCHITEW6432"]
            except KeyError:
                pass
            #true_platform not assigned to if this does not exist
            return true_platform
        elif platform.system()=='Linux':
            pass               
        
    ##  check if the product is installed   
    def isProductInstalled(self,guid,packageName,version):
        if platform.system()=='Windows':
            # look for that particular product using "guid" in adiGPR.xml
            try: settings=self.adiGPR[guid]
            except: return False
        
            # look for "installDirectory"\"keyFile" to validate that product is really installed
            bits=self.os_platform()
            if not bits=='x86':
                registry=os.path.join(r'HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall',guid)
            else:
                registry=os.path.join(r'HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall',guid)
            installLocation=self.getRegistryKeyValue(registry,'InstallLocation')

            if not installLocation:
                return False
            # note installLocation and installDirectory may not match
            installedDir=settings['installDirectory']    
            keyFile=os.path.join(installedDir,settings['keyfile'])
            if not os.path.isfile(keyFile):
                return False

            # at this point, we consider the installation exists
            return True
        
        elif platform.system()=='Linux':
            fullPackageName='adi'+'-'+packageName+'-'+version
	    # look for that particular product using "fullPackageName" in self.adiGPR
	    try: self.adiGPR[fullPackageName]
            except: return False
			
            # to check whether a particular release is installed                             
            value=os.popen("dpkg -l | grep %s | awk '{print $1}'" %fullPackageName).read().strip()
            if value=='ii':
                return True
            else: return False               

    ##  get all information of installed kit 
    def getInstalledKitInfo(self,guid,packageName,version):        
        if not self.isProductInstalled(guid,packageName,version): return {}           
        else:          
            if platform.system()=='Windows':    
		return self.adiGPR[guid]                   
            elif platform.system()=='Linux':
		fullPackageName='adi'+'-'+packageName+'-'+version
                return self.adiGPR[fullPackageName]

    ##  get per installed kit information from global XML
    def getInstalledKitInfoKeyValue(self,guid,key,packageName,version):        
        if not self.isProductInstalled(guid,packageName,version):
            return ''
        if platform.system()=='Windows':
            keyValue=self.adiGPR[guid][key]
			        
            # since version format is not consistent for Windows products, add a workaround here 
            if key=='version':
                match=re.match(r'(\d{1,2}\.\d{1,2}\.\d{1,2}\.\d{1,2}|\d{1,2}\.\d{1,2}\.\d{1,2})',keyValue)
                if match: return match.group(1)

            if key=='installDirectory':
                if 'installation_directory' in self.adiGPR[guid].keys():
                    keyValue=self.adiGPR[guid]['installation_directory']
            
            # remove '\' at the end of string
            if re.match('(.*)\\\\$',keyValue):
                keyValue = keyValue[:-1]
                       
        elif platform.system()=='Linux':           				
            fullPackageName='adi'+'-'+packageName+'-'+version
	    keyValue=self.adiGPR[fullPackageName][key]
			
        return keyValue
        
    def installPython(self, guid, packageName, version):
        self.log('Installing tpsdkserver python bindings to "%s"' % self.python)
        if self.python != 'default' and not os.path.exists(self.python):
            raise InstallerException('Invalid python installation: ' + self.python)
        
        # find the .egg file to install
        self.retrieveAdiGPR()
        installPath = self.getInstalledKitInfoKeyValue(guid,'installDirectory',packageName,version)
        eggPath = os.path.join(installPath,'Eclipse','support','TpsdkServer-*-py%d.%d.egg' % sys.version_info[:2])
        eggFile = glob.glob(eggPath)
        
        if len(eggFile) != 1:
            self.log('No .egg file found in this CCES installation (' + eggPath + ').')
            return
        pythonPath = sys.prefix if self.python == 'default' else self.python
        if platform.system()== 'Windows':
            easyInstall = os.path.join(pythonPath, 'Scripts', 'easy_install.exe')
        elif platform.system()=='Linux':
            easyInstall = os.path.join(pythonPath, 'bin', 'easy_install')
        else:
            raise InstallerException('Unsupported operating system')
        
        if os.path.isabs(easyInstall) and not os.path.exists(easyInstall):
            raise InstallerException('Unable to find easy_install utility. Check your Python distribution to ensure that it has the setuptools package installed.')
        
        # install the .egg
        result = EasyProcess([easyInstall, eggFile[0]], logger=self.log).call(timeout=self.TIMEOUT)
        
        if result.return_code != 0:
            raise InstallerException('Failed to install Python bindings to %s (%s)' % (self.python, result.stderr))         
    
    ##  get per installed kit information from global XML
    def installLicense(self):
        if platform.system()=='Windows':
            licenseDir=os.path.join(self.profileFolder,'CrossCore Embedded Studio')
        elif platform.system()=='Linux':   
            licenseDir=os.path.join(self.profileFolder,'cces')
            
        licenseFile=os.path.join(licenseDir,'license.dat')

        if self.userLicense:
            newLicense=self.userLicense
        elif not os.path.isfile(licenseFile):       
            newLicense=self.product['kitLicense']           
        else: newLicense=''
        newLicense=newLicense.replace('\\','//')
        if newLicense:
            self.log('copy license file %s to %s'%(os.path.basename(newLicense),licenseDir))
            if not os.path.exists(licenseDir): os.makedirs(licenseDir)
            shutil.copy(newLicense,licenseFile)   
       
    ##  do the uninstalltion of a kit
    def uninstall(self):
        uninstall=False
        for product in self.products:
            guid=product['productGuid']
	    packageName=product['packageName']
	    version=product['version']
            if self.isProductInstalled(guid,packageName,version):
                uninstall=True
                installedKit=self.getInstalledKitInfoKeyValue(guid,'kitVersion',packageName,version)
                productVersion=self.getInstalledKitInfoKeyValue(guid,'version',packageName,version)
                buildType=self.getInstalledKitInfoKeyValue(guid,'buildType',packageName,version)

                self.log('Uninstalling %s %s %s kit %s ...'%(product['productName'],productVersion,buildType,installedKit))    

                result = self.batchCall('UNINSTALL',product['uninstallCmd'])

                if result.return_code != 0:
                    # try a registry cleanup with MsiZap tool in Windows
		    if platform.system()=='Windows':					   
                        self.batchCall('CLEANUP',product['cleanupCmd'])
                        self.log(result.stderr)
		    elif platform.system()=='Linux':
			pass
                    raise InstallerException(('Failed to uninstall %s %s %s kit %s. Error %d (timeout if error is 1; Installer process hanging if error is 999).'%(product['productName'],productVersion,buildType,installedKit,result.return_code)))
                    
		self.log('Uninstalled %s %s %s kit %s'%(product['productName'],productVersion,buildType,installedKit))

        if uninstall:
            self.log('Uninstallation successfully completed.')    
        else:
            self.log('No installation can be found for %s. Nothing to do.'%(self.product['productName'])) 

    ##  do the installtion of a kit
    def install(self):       
        self.log('Installing %s %s %s kit %s ...'%(self.product['productName'],self.product['version'],self.product['buildType'], self.product['kitId']))
        foundSameVersion=False
        otherVersion=False
        for product in self.products:
            guid=product['productGuid']
            version=product['version']                 	   
            if self.isProductInstalled(guid,product['packageName'],version):                                                     
                installedId = self.getInstalledKitInfoKeyValue(guid,'kitVersion',product['packageName'],version)                   
                productVersion = self.handleVersion(self.getInstalledKitInfoKeyValue(guid,'version',product['packageName'],version))                
                buildType = self.getInstalledKitInfoKeyValue(guid,'buildType',product['packageName'],version)
                                
                if platform.system()=="Windows":
                    if guid==self.product['productGuid']:
                        kitId,kitPath=self.findKit(self.product['kitId'],self.product['specificKitFolder'])
                        if installedId == kitId and productVersion == self.product['version'] and buildType == self.product['buildType']:
                            foundSameVersion=True
                        else:
                            otherVersion=True
                    else:
                        otherVersion=True
						
                elif platform.system()=="Linux":                                             
                    installedPackageName=self.getInstalledKitInfoKeyValue(guid,'packageName',product['packageName'],version)
                    if productVersion==self.product['version'] and installedPackageName==self.product['packageName']:
                        kitId,kitPath=self.findKit(self.product['kitId'],self.product['specificKitFolder'])                                
                        buildType=buildType.upper()
                        if installedId == kitId and productVersion == self.product['version'] and buildType == self.product['buildType']:
                            foundSameVersion=True
                        else:
                            otherVersion=True
                    else:
                        otherVersion=True
        if foundSameVersion and (not otherVersion):
            self.log('Same version(s) or kit(s) of product already installed (%s %s %s).'%(productVersion,buildType,installedId))
            return ''
        elif (not foundSameVersion) and (not otherVersion):
            pass
        else:
            self.uninstall()  

        self.downloadKit()        
        self.installLicense()

        # call installation batch
        self.log('Start installing...')

        result = self.batchCall('INSTALL',self.product['installCmd'])       

        if result.return_code not in [0,256]:
            if platform.system()=='Windows':
                # try a registry cleanup with MsiZap tool
                self.batchCall('CLEANUP',self.product['cleanupCmd'])
                self.log(result.stderr)               
            elif platform.system()=='Linux':
		pass
            raise InstallerException('Failed to install %s. Error %d (timeout if error is 1;Installer process hanging if error is 999).'%(self.product['productName'],result.return_code))            
        
        if self.python:
            self.installPython(self.product['productGuid'],self.product['packageName'],self.product['version'])

        if self.product['supportFiles']:
            if self.product['installPath']=='':
                self.retrieveAdiGPR()
                installDirectory=self.adiGPR[self.product['productGuid']]['installDirectory']                
                self.product['supportFilesCmd']='cmd.exe /C ""$supportDir\\7za.exe" x "$supportFiles" -o"%s"" -y'%installDirectory
                self.product['supportFilesCmd']=string.Template(self.product['supportFilesCmd']).substitute(self.product)
            self.handleSupportFiles()
        
        if platform.system()=='Linux':
            self.log('umount local mirror. %s' %self.linuxLocalKitMirror)
            if not os.system('umount -l %s > /dev/null' %self.linuxLocalKitMirror) in [0,256]:
                self.log('Fail to umount local mirror!')
            
                		
        
        self.log('Installed %s %s %s kit %s'%(self.product['productName'],self.product['version'],self.product['buildType'],self.product['kitId']))
        self.log('Installation successfully completed.')





#################################################################
# a imlp for VisualDSP++ and its update 
#################################################################
class VDSPInstaller(Installer):
    class PopupHandler(threading.Thread):  

        def __init__(self):
            threading.Thread.__init__(self)
            self.timeToQuit = threading.Event()
            self.timeToQuit.clear()
              
        def run(self):
            try:
                while not self.timeToQuit.isSet():
                    self.handlePopups()
            except:
                pass

        def stop(self):
            self.timeToQuit.set()
            self.join()
              
        def handlePopups(self):
            killList=['drvinst.exe', 'rundll32.exe']
            for item in killList:
                if item in os.popen('tasklist').read():
                    os.system('taskkill /IM %s /F /FI "WINDOWTITLE eq Windows Security*"'%item)
            time.sleep(5)

    def downloadKit(self):
        # cleanup temp folder
        if os.path.exists(self.TEMPDIR):
            shutil.rmtree(self.TEMPDIR)
        os.makedirs(self.TEMPDIR)

        #  download binary:
        self.log('Downloading binary to %s...'%self.TEMPDIR) 
        kitId,kitPath=self.findKit(self.product['kitId'],os.path.join(self.product['kitMirror'],self.product['kitFolder']))
        pattern=re.compile(re.sub('\*','.*',self.product['binPattern']))
        for file in os.listdir(kitPath):
            if pattern.search(file):
                updateBinary=file
                self.product['installCmd']=self.product['installCmd'].replace('INSTALL_BINARY',updateBinary)
                break

        updateFile=os.path.join(kitPath,updateBinary) 
        shutil.copy(updateFile,self.TEMPDIR)            
        self.log('Binary successfully downloaded.')
        
        # download base kit if it has non-empty 'baseKitId' and 'baseBinary' node in KitInstaller.xml
        if self.product['baseKitId'] and self.product['baseBinary']:
            self.log('Downloading base kit to %s...'%self.TEMPDIR)
            mirror=os.path.abspath(os.path.join(self.product['kitMirror'],self.product['kitFolder'],self.product['name']))
            baseKitId,baseKitPath=self.findKit(self.product['baseKitId'],mirror)  
            baseFile=os.path.join(baseKitPath,self.product['baseBinary'])      
            shutil.copy(baseFile,self.TEMPDIR)
            self.log('Base kit successfully downloaded.')        

            
    def isProductInstalled(self,guid): 
        try:
            bits=self.os_platform()
            if not bits=='x86': registry=r'HKLM\SOFTWARE\Wow6432Node'+self.product['registry']
            else: registry=r'HKLM\SOFTWARE'+self.product['registry']
            installedDir=self.getRegistryKeyValue(registry,'Install Path')  
                  
            if not installedDir: return False
            if not os.path.isfile(os.path.join(installedDir,'setup.exe')): return False  
                
            config=ConfigParser.RawConfigParser()
            config.read(os.path.join(installedDir,'system','visualdsp.ini'))
            installedVersion=config.get('VisualDSP','VersionMajor')+'.'+\
                             config.get('VisualDSP','VersionMinor')+'.'+\
                             config.get('VisualDSP','VersionPatch')+'.'+\
                             config.get('VisualDSP','VersionUpdate')
            if not guid==installedVersion: return False
                 
            return True                            
        except:
            return False
        


    def getInstalledKitInfoKeyValue(self,guid,key):
        if not self.isProductInstalled(guid): return ''

        try:            
            bits=self.os_platform()
            if not bits=='x86': registry=r'HKLM\SOFTWARE\Wow6432Node'+self.product['registry']
            else: registry=r'HKLM\SOFTWARE'+self.product['registry']
            installedDir=self.getRegistryKeyValue(registry,'Install Path').rstrip('\\') 
            config=ConfigParser.RawConfigParser()
            config.read(os.path.join(installedDir,'system','visualdsp.ini'))
            productVersion=config.get('VisualDSP','VersionMajor')+'.'+config.get('VisualDSP','VersionMinor')+'.'+config.get('VisualDSP','VersionPatch')
            for node in ElementTree().parse(os.path.join(installedDir,'manifest.xml')).iter('internal-kit-build'):
                kitVersion=node.text
                break
                                
            if   key=='installDirectory': return installedDir
            elif key=='kitVersion': return kitVersion
            elif key=='buildType': return self.product['buildType']
            elif key=='version': return self.product['version']
            else: return ''
        except:
            return ''


    def installLicense(self):
        pass


    def install(self):
        # call installer's install()
        Handler=self.PopupHandler()
        Handler.start()
        Installer.install(self)        
        Handler.stop()
        # copy VisualDSP++ license file
        licenseDir=os.path.join(self.product['installPath'],'system')
        licenseFile=os.path.join(licenseDir,'license.dat')
        
        if self.userLicense: 
            newLicense=self.userLicense
        elif not os.path.isfile(licenseFile): 
            newLicense=self.product['kitLicense']
        else: newLicense=''

        if newLicense:
            self.log('copy license file %s to %s'%(os.path.basename(newLicense),licenseDir))
            if not os.path.exists(licenseDir): os.makedirs(licenseDir)
            shutil.copy(newLicense,licenseFile)
        

#################################################################
# a imlp for SigmaStudio Core
#################################################################
class SSInstaller(Installer):      

    ##  check if the product is installed   
    def isProductInstalled(self,guid):
        # look for that particular product using "guid" in adiGPR.xml
        try: settings=self.adiGPR[guid]
        except: return False
        
        # look for "installDirectory"\"keyFile" to validate that product
        # is really installed
        bits=self.os_platform()
        if bits=='x86':
            registry=os.path.join(r'HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall',guid)
            installLocation=self.getRegistryKeyValue(registry,'InstallLocation')

            if not installLocation:
                return False
        # note installLocation and installDirectory may not match
        installedDir=settings['installDirectory']    
        keyFile=os.path.join(installedDir,settings['keyfile'])
        if not os.path.isfile(keyFile):
            return False

        # at this point, we consider the installation exists
        return True

    def findKit(self,kitId,kitMirror):
        bits=self.os_platform()

        self.log('Looking for kit on mirror: %s'%kitMirror)
        if not os.path.isdir(kitMirror):
            raise InstallerException('Failed to find kit mirror: %s'%kitMirror)
        allDir = os.listdir(kitMirror)

        # find the specified kit
        kitsDir=[]
        path = ''
        for kit in allDir:
            if re.search('w[0-9]+',kit):
                kitsDir.append(kit)

            m = re.search(kitId,kit)
            if (m != None):
                path = os.path.join(kitMirror,kit)
                self.log('Found kit with ID %s: %s'%(kitId,path))


        # specified kit not found, then check for latest
        if kitId=='latest':
            kitsDir.sort()
            path=os.path.join(kitMirror,kitsDir[-1]) 
            kitId = re.match('(w[0-9]+).*',kitsDir[-1]).group(1)
            self.log('Found latest kit %s at %s'%(kitId,path))

        for root, dirs, files in os.walk(path):
            for file in files:
                m=re.search(r'.*64\.exe',file)
                if bits=='x86':
                    if m: continue
                m=re.search(r'.*\.exe',file)                    						
                if m:
                    self.log('Found kit with ID %s: %s'%(kitId,os.path.join(kitMirror,file)))
                    return (kitId,os.path.join(root,file))
        raise InstallerException('Failed to find sigma studio install package in kit with ID: %s'%kitId)  

    def downloadKit(self):
        kitId,kitFile=self.findKit(self.product['kitId'],os.path.join(self.product['kitMirror'],self.product['kitFolder']))           
        self.product['installCmd']=self.product['installCmd'].replace('INSTALL_BINARY',os.path.basename(kitFile))

        self.log('Downloading kit to %s...'%self.TEMPDIR)
        if os.path.exists(self.TEMPDIR):
            shutil.rmtree(self.TEMPDIR)
        os.makedirs(self.TEMPDIR)
        shutil.copy(kitFile,self.TEMPDIR) 
        self.log('Kit successfully downloaded.')
        
#################################################################
# a imlp for CCES patch
#################################################################
class PatchInstaller(Installer):
    def __init__(self,productId,kitId='',kitMirror='',installPath='',kitLicense='',keepLicense=False,nonDefault=False,multiVersion=False):
        self.baseInstaller=None
        Installer.__init__(self,productId,kitId,kitMirror,installPath,kitLicense,keepLicense,nonDefault,multiVersion=True)
        baseId=self.product['baseId']
        baseKit=self.product['baseKit']
        self.baseInstaller = Installer(baseId,baseKit,kitMirror=kitMirror,kitLicense=kitLicense,nonDefault=nonDefault,multiVersion=multiVersion)
    
    ##  do the uninstalltion of a kit
    def uninstall(self):
        super(PatchInstaller, self).uninstall()
        self.baseInstaller.uninstall()
        

    ##  do the installtion of a kit
    def install(self):
        self.baseInstaller.install()
        super(PatchInstaller, self).install()

    ## 
    def isProductInstalled(self,guid):
        if self.baseInstaller:
            baseInstalled = self.baseInstaller.isProductInstalled(self.baseInstaller.product['productGuid'])
            if not baseInstalled: return False
        return super(PatchInstaller, self).isProductInstalled(guid)
        
        
#################################################################
# Command line support
#################################################################
def main(argv):

    #################################################################
    # helper functions
    #################################################################
    def printSupportedProduct():
        for productNode in ElementTree().parse(os.path.join(Installer.SUPPORTDIR,'KitInstaller.xml')).iter('product'):             
            for verNode in productNode.iter('version'):
                try:
                    productId=productNode.attrib['productKey']+'-'+verNode.attrib['version']+'-'+verNode.attrib['buildType'][0]
                    print "%18s : %s %s %s" % (productId,productNode.attrib['productName'],verNode.attrib['version'],verNode.attrib['buildType'].lower())
                except: pass        
        print '\n\nExamples of productId format:'
        print ' ANDTL-00-1.0.0-R  -  CCES Release 1.0.0'
        print ' ANDTL-00-1.0.0-B  -  CCES Beta 1.0.0'
        print ' ANDTL-00-1.0.0-A  -  CCES Alpa 1.0.0'
        return

   
    def queryInfo(infoKey):
        import codecs
        std=sys.stdout
        sys.stdout = codecs.lookup('utf-8')[-1](sys.stdout)
            
        guid=installer.product['productGuid']
        packageName=installer.product['packageName']
        version=installer.product['version']
        if not installer.isProductInstalled(guid,packageName,version): 
            print('No installation can be found for %s\n'%productId)
            if not infoKey and installer.products:
                print('other versions installation(s) of this product:')
                for product in installer.products:
                    guid=product['productGuid']
                    packageName=product['packageName']
                    version=product['version']
                    if installer.isProductInstalled(guid,packageName,version):
                        for (key,value) in installer.getInstalledKitInfo(guid,packageName,version).items():
                            if re.match('.*\\\\$',value):
                                value = value[:-1]                      
                            print "%18s : %s"%(key,value)
        else:         
            if not infoKey:
                for (key,value) in installer.getInstalledKitInfo(guid,packageName,version).items():
                    if re.match('.*\\\\$',value):
                        value = value[:-1]                      
                    print "%18s : %s"%(key,value)
            else: print installer.getInstalledKitInfoKeyValue(guid,infoKey.replace('\'',''),packageName,version)
        sys.stdout=std




    #################################################################
    # Main
    #################################################################
    (options,args) = parser.parse_args(argv[1:])

    if len( args ) == 1 and args[0] == 'list':
        printSupportedProduct()
        sys.exit(-1)
    elif not len( args ) == 2:
        print 'Use "KitInstaller.py list" to see a list of supported products'
        print 'You must specify a product to install, uninstall or query and an action to perform'
        parser.print_help()
        sys.exit(-1)

    (action,productIds) = args

    # support install multi products in single command line
    # multi parameters are separated by comma
    products=[p.strip() for p in productIds.split(',')]
    kits=[k.strip() for k in options.kitIds.split(',')]
    if len(kits)<len(products): 
        kits=kits+['']*(len(products)-len(kits))

    # SIGST-80P should be placed after SIGST-00
    for productId in products: 
        if not productId.find('SIGST-00P')==-1:
            i=products.index(productId) 
            products.insert(0, products[i])
            del products[i+1]
            kits.insert(0, kits[i])
            del kits[i+1]

    # SIGST-02 should be placed after CCES or VDSP
    for productId in products: 
        if not productId.find('SIGST-02')==-1:
            i=products.index(productId) 
            products.insert(0, products[i])
            del products[i+1]
            kits.insert(0, kits[i])
            del kits[i+1]

    # ANDTL-00(CCES) or VDSPT-0x(VDSP5.x) or SigmaStudio Core must be installed first
    for productId in products: 
        if (not productId.find('ANDTL-00')==-1) or (not productId.find('VDSPT-04')==-1) or (not productId.find('VDSPT-03')==-1) or (not productId.find('CCEST-02')==-1) or (not productId.find('CCEST-00')==-1):
            i=products.index(productId)
            products.insert(0, products[i])
            del products[i+1]
            kits.insert(0, kits[i])
            del kits[i+1]

    
    # serve the action
    try:
        for (productId,kitId) in zip(products,kits):
            if productId.find('VDSPT')==0:
                license=options.vdspLicense if options.vdspLicense else options.kitLicense
                installer=VDSPInstaller(productId,
                                kitId=kitId,
                                kitMirror=options.kitMirror,
                                installPath=options.installPath,
                                kitLicense=license,
                                nonDefault=options.nonDefault)

            elif productId.find('SIGST-02')==0:
                installer=SSInstaller(productId,
                                      kitId=kitId,
                                      installPath=options.installPath,
                                      nonDefault=options.nonDefault,
                                      kitMirror=options.kitMirror)


            elif productId.find('ANDTL-00-1.0.0.1-R')==0 or productId.find('ANDTL-00-1.0.1.1-R')==0 or productId.find('ANDTL-00-1.0.1.2-R')==0:
                installer=PatchInstaller(productId,
                                kitId=kitId,
                                kitMirror=options.kitMirror,
                                installPath=options.installPath,
                                kitLicense=options.kitLicense,
                                nonDefault=options.nonDefault,
                                multiVersion=options.multiVersion)                
                
            else:
                installer=Installer(productId,
                                kitId=kitId,
                                kitMirror=options.kitMirror,
                                installPath=options.installPath,
                                kitLicense=options.kitLicense,
                                nonDefault=options.nonDefault,
                                supportFiles=options.supportFiles,
                                multiVersion=options.multiVersion,
                                python=options.python)
                
            if action == 'install':
                installer.install()
            elif action == 'uninstall':
                installer.uninstall()
            elif action == 'query':
                queryInfo(options.kitInfoKey)
                
        return

    except Exception,e:
        print 'Fail to perform action "%s" with error - %s'%(action,e)
        sys.exit(-1)



# main entry
if __name__ == '__main__':
    
    # construct the parameter parser
    global parser
    VERSION='2.3'
    USAGE = '''
        %prog <action> <productId> [options]
    Where:
        <action> is one of "install", "uninstall" or "query"
        <productId> is a product Id defined in KitInstaller.xml'''
    parser = optparse.OptionParser( usage=USAGE, version=VERSION )
    parser.add_option( '-m', '--mirror', help='The kit mirror from which the install will be downloaded', dest='kitMirror', default='' )
    parser.add_option( '-k', '--kit', help='The kit number to install (e.g. w1103071) or "latest" to install the most recent kit. Default is latest', dest='kitIds', default='' )
    parser.add_option( '-d', '--dir', help='The folder to install to.', dest='installPath', default='' )
    parser.add_option( '-l', '--license', help='specify license file to install. keeping existing license file if a license file is not specified.', dest='kitLicense', default='' )
    parser.add_option( '-i', '--info', help='The kit installation information name', dest='kitInfoKey', default='' )
    parser.add_option( '--mv', help='allow co-existence of multi-versions of the product', dest='multiVersion', action='store_true', default=False )
    parser.add_option( '--non-default-location', help='Install product to a none default location. (option does not take effect with -d)', dest='nonDefault', action='store_true', default=False )
    parser.add_option( '-f', help='option deprecated', dest='kitFolders', default='' )
    parser.add_option( '--support-files', help='copy the support files after installation', dest='supportFiles', action='store_true', default=False )
    parser.add_option( '--keep-license', help='option deprecated', dest='keepLicense', action='store_true', default=False )
    parser.add_option( '--vdsp-license', help='specify license file for VDSP, it overwrites -l, --keep-license', dest='vdspLicense', default='C:\\License\\VisualDSP++\\license.dat' )
    parser.add_option( '--python', help='specify a Python environment in which to install the CCES TpsdkServer bindings. use "default" to install to the default Python installation', dest='python' )
    parser.add_option( '--cleanup', help='clean up the machine by uninstalling products which are not in the install list.', dest='cleanup', action='store_true', default=False )
    parser.add_option( '-@', '-@', help='Specifies a file from which options should be read', dest='file', action='store', default=None,  )



    # gather operations
    operations=[]

    # handle the specified @file
    if '-@' in sys.argv:
        scriptFile=sys.argv[0]
        n = sys.argv.index( '-@' )
        atFile = sys.argv[n + 1]
        with open( atFile ) as f:
            for line in f.readlines():
                args=[scriptFile]
                opts = shlex.split(line, posix=False )
                args.extend(opts)
                if platform.system()=='Windows':
                    operations.append([a.replace('"','') for a in args])
                elif platform.system()=='Linux':
                    operations.append([a.replace('"','').replace('\\\\','\\') for a in args])

    else: 
        operations=[sys.argv]


    # handle the clean-up option
    if '--cleanup' in sys.argv:
        # a set of products installed
        installed=set()
        installer=Installer('VDSPT-03')
        try:                        
            if platform.system()=="Windows":                
                guids=installer.adiGPR.keys()
                for productNode in ElementTree().parse(os.path.join(installer.SUPPORTDIR,'KitInstaller.xml')).iter('product'):
                    productKey=productNode.attrib['productKey']
                    for versionNode in productNode.iter('version'):
                        if versionNode.attrib['productGuid'] in guids:
                            installed.add(productKey)
                    
            elif platform.system()=="Linux":               
		fullPackageNames=installer.adiGPR.keys()
                for productNode in ElementTree().parse(os.path.join(installer.SUPPORTDIR,'KitInstaller.xml')).iter('product'):
                    productKey=productNode.attrib['productKey']       
                    for versionNode in productNode.iter('version'):
                        try:   
                            if 'adi'+productNode.attrib['packageName']+'-'+versionNode.attrib['version'] in fullPackageNames:
                                installed.add(productKey)
                        except:pass				  				                                
       
            # a set of products to be installed
            toInstall=set()
            for args in operations:
                (options,para)=parser.parse_args(args[1:])
                (action,productIds) = para
                if action=='install':
                    for productId in [p.strip() for p in productIds.split(',')]:
                        match=re.match(r'([A-Z0-9]{5}-\d{2}[PESC]{0,1})(-\d{1,2}\.\d{1,2}\.\d{1,2}-[B,A,R]){0,1}',productId)
                        if match: toInstall.add(match.group(1))
                # support 'uninstall all'
                if action=='uninstall' and productIds=='all':
                    operations.remove(args)
    
            # add uninstall commands for those irrelevant
            uninstalls=installed-toInstall
            for productKey in uninstalls:
                operations.insert(0,['','uninstall',productKey])
        except Exception,e:
            print 'Fail to perform the clean-up with error - %s'%e
            print 'Uninstall all: %s uninstall all --cleanup' % sys.argv[0]
            sys.exit(-1)       

    
    # perform the operations
    for args in operations:
        main(args)

    # return normally
    sys.exit(0)
