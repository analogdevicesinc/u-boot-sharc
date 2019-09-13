###############################################################################
# Copyright (c), 2011 - Analog Devices Inc. All Rights Reserved.
# This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
# and its licensors.
###############################################################################
# File:
#   <InstallNonDevonshireXml.py>
# Description:
#   download ProcessorInfo-non-devonshire-idde.xml to specified folder
###############################################################################
import  os,sys,urllib,optparse,platform
from xml.etree.ElementTree import ElementTree


###################################################################
## main body
###################################################################
if __name__ == "__main__":

    __version__= '1.0'

    parser = optparse.OptionParser( usage="usage: %prog download ProcessorInfo-non-devonshire-idde.xml or ProcessorInfo-sharc-idde.xml to specified folder", version=__version__ )
    parser.add_option( '-n', '--nondevonshire', dest='nd', default=r'http://ctse-team/eclipse/integration/ProcessorInfo-non-devonshire-idde.xml', help='http URL to download non-devonshire XML')
    parser.add_option( '-s', '--sharc', dest='sharc', default=r'http://ctse-team/eclipse/integration/ProcessorInfo-sharc-idde.xml', help='http URL to download sharc XML')
    parser.add_option( '-l', '--headless', dest='headless', default=r'http://testlab2:Labrat1@ctse-team/svn/tests/trunk/headlessbuild_interim/', help='http URL to download headless builder patch')
    (options,args) = parser.parse_args()
    

    # download the XML
    try:        
        if os.name=='nt':       
            if platform.win32_ver()[0]=='XP':
                xml=os.path.join(os.environ['ALLUSERSPROFILE' ],'Application Data','Analog Devices','adiGPR.xml' )
            else:
                xml=os.path.join(os.environ['ALLUSERSPROFILE' ],'Analog Devices','adiGPR.xml' )  
        
            installation=[]
            for product in ElementTree().parse(xml).iter('product'):
                if product.attrib['internal_name']=='CCES':
                    installation.append(product.attrib)
            
            if installation:
                for i in installation:
                    ccesHome=i['installDirectory']
                    if i['buildType']!='BETA':
                        if i['version']=='1.0.0.0' or i['version']=='1.0.0.1':
                            url=options.nd
                            dest=os.path.join(ccesHome,r"System\ArchDef",'ProcessorInfo-non-devonshire-idde.xml')

                            urllib.urlretrieve(url,dest)
                            print 'Finished download patch from %s as %s' % (url,dest)
                            
                        elif i['version']=='1.0.1.0' or i['version']=='1.0.1.1':
                            patch=['com.analog.crosscore.addins.crtldf.core_1.0.1.201211271407.jar',
                                   'com.analog.crosscore.addins.ssldd_1.0.1.201211271407.jar',
                                   'com.analog.crosscore.debug.ui_1.0.1.201211271407.jar',
                                   'com.analog.crosscore.headlesstools_1.0.1.201211271407.jar',
                                   'com.analog.crosscore.system.components_1.0.1.201211271407.jar',
                                   'com.analog.crosscore.system_1.0.1.201211271407.jar',
                                   'com.analog.crosscore.ui_1.0.1.201211271407.jar']

                            for p in patch:
                                url=options.headless+p
                                dest=os.path.join(ccesHome,r"Eclipse\dropins",p)
                                urllib.urlretrieve(url,dest)
                                print 'Finished download patch from %s as %s' % (url,dest)
                        else:
							print 'Version %s of CCES installation does not need any patch XML file.'%i['version']
							break
                        
                    else:
                        print 'CCES installation seems to be BETA version, no need for downloading, quiting...'
                        sys.exit(0)
            else:
                print "No CCES installation or CCES version does not need patch."
                sys.exit(0)
        else:   
            print "No CCES installation found."
            sys.exit(0) 

        
        sys.exit(0)
    except Exception,e:
        print 'Failed to download patch. %s' % (e)
        sys.exit(-1)
    
