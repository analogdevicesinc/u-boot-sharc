import os,sys,optparse

##  a utilty to perform 'touch'
def touch(fname):
    if os.path.exists(fname):
        os.utime(fname, None)
    else:
        open(fname, 'w').close()


##  Main
if __name__ == "__main__":
    parser = optparse.OptionParser( usage='%prog filename', version='1.0' )
    (options,args) = parser.parse_args()

    if not len( args ) == 1:
        print 'You must specify a filename to be touched'
        parser.print_help()
        sys.exit(-1)
    
    (filename,) = args
    touch(filename)
    sys.exit(0)
