###############################################################################
# Copyright (c), 2015 - Analog Devices Inc. All Rights Reserved.
# This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
# and its licensors.
###############################################################################
# File:
#   <autoInstallDriver.py>
# Description:
#   product USB driver installer script
###############################################################################
import os,sys,platform,optparse
ROOT = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(ROOT,'..'))
from KitInstaller import Installer

class FlushIO(object):
    def __init__(self, f):
        self.f = f
    def write(self, x):
        self.f.write(x)
        self.f.flush()

sys.stdout = FlushIO(sys.__stdout__)

def queryInfo(info):
    installer=Installer(info)       
    guid=installer.product['productGuid']
    packageName=installer.product['packageName']
    productversion=installer.product['version']
    if not installer.isProductInstalled(guid,packageName,productversion):
        print('No installation can be found for %s\n'%info)
    else:
        return installer.getInstalledKitInfoKeyValue(guid,'installDirectory',packageName,productversion)

if __name__ == '__main__':        
    VERSION='1.0'
    USAGE = '''
        %prog [options]
    Where:
        [options] include productKey, productVersion, buildType, driverInstallToolPath, driverFilePath'''
    parser = optparse.OptionParser( usage=USAGE, version=VERSION )
    parser.add_option( '-k', '--productKey', help='product key', dest='productKey', default='' )   
    parser.add_option( '-v', '--productVersion', help='product version', dest='productVersion', default='' )
    parser.add_option( '-t', '--buildType', help='build type', dest='buildType', default='' )
    parser.add_option( '-d', '--driverInstallToolPath', help='driver Install Tool Path', dest='driverInstallToolPath', default='' )
    parser.add_option( '-f', '--driverFilePath', help='driver File Path', dest='driverFilePath', default='' )
    (options,args) = parser.parse_args()

    productId=options.productKey+'-'+options.productVersion+'-'+options.buildType[0]
    installPath=queryInfo(productId)

    if platform.machine()=='x86':
        uninstallDriverCmd=r'cd "%s\%s" && "dpinst32.exe" /U %s\WinUSB_adi.inf /S /C |echo' %(installPath,options.driverInstallToolPath,options.driverFilePath)    
        installDriverCmd=r'cd "%s\%s" && "dpinst32.exe" /PATH %s /S /C | echo' %(installPath,options.driverInstallToolPath,options.driverFilePath)    
    else: 
        uninstallDriverCmd=r'cd "%s\%s" && "dpinst64.exe" /U %s\WinUSB_adi.inf /S /C |echo' %(installPath,options.driverInstallToolPath,options.driverFilePath)    
        installDriverCmd=r'cd "%s\%s" && "dpinst64.exe" /PATH %s /S /C | echo' %(installPath,options.driverInstallToolPath,options.driverFilePath)

    result_uninstall=os.system(uninstallDriverCmd)       
    if result_uninstall not in [0,-2147483648]:
        print "Fail to uninstall USB Driver"
    else:
        result_install=os.system(installDriverCmd)       
        if result_install not in [0,256]:
            sys.exit(-1)
        else: 
            sys.exit(0)

