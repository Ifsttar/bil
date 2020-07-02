#! /usr/bin/python

import sys
import getopt
from gmshMesh import Mesh

#===============================================================================
def main():
    try:
        optlist,args = getopt.getopt(sys.argv[1:],
                       'hs:c:t:f:r:p:',
                       ['help','side_crack=','crack=','tip_crack=',
                        'file=','region=','physical'])
    except getopt.GetoptError as err:
        print(err)
        sys.exit(2)

    for opt, optval in optlist:
        if opt in ('-h', '--help'):
            usage()
            sys.exit()
        elif opt in ('-c', '--crack'):
            crack_id = set([int(i) for i in optval.split(',')])
        elif opt in ('-t', '--tip_crack'):
            cracktip_id = set([int(i) for i in optval.split(',')])
        elif opt in ('-s','--side_crack'):
            oneside_id = set([int(i) for i in optval.split(',')])
        elif opt in ('-f', '--file'):
            file_name = optval
        elif opt in ('-r', '--region'):
            region_id = set([int(i) for i in optval.split(',')])
        elif opt in ('-p', '--physical'):
            phys_id = int(optval)


    if(not "file_name" in locals()):
        print 'Error: Must specify mesh file with -f'
        sys.exit(1)


    M = Mesh()

    M.read(file_name)
    

    if("crack_id" in locals()):
        if(not "cracktip_id" in locals()): cracktip_id = set()
        if(not "oneside_id" in locals()): oneside_id = set()
        M1 = M.makecrackedmesh(crack_id, oneside_id, cracktip_id)
    elif("region_id" in locals()):
        if(not "phys_id" in locals()): phys_id = -1
        M1 = M.makebrokenmesh(phys_id, region_id)
    else:
        print 'Error: Must specify crack ID with -c or region ID with -r'
        sys.exit(1)
    

    M1.write('cracked_'+file_name)


#===============================================================================
def usage():
    print 'Usage: python %s -f<filename> -c<crack_id> -t<cracktip_id> -s<oneside_id> -r<region_ids> -p<physical_id>' % sys.argv[0]

    print '\n'

    print 'Options:'
    print '-c, --crack:      Comma separated elementary IDs of cracks in .msh file'
    print '-t, --tip_crack:  Comma separated elementary IDs of crack tips in .msh file'
    print '-s, --side_crack: Comma separated elementary IDs of elements touching the cracks on the side'
    print '                  so that the overlapping nodes follow the same local node ordering'
    print '-r, --region:     Comma separated elementary IDs of elements to be cracked'
    print '-p, --physical:   Physical ID of the created zero-thickness elements'
    print '-f, --file:       Mesh file name'
    print '-h, --help:       Display this help message'

    print '\n'

    print 'Restriction:'
    print 'Use options (-c -t -s) or (-r -p) but do not mix them.'

    
#===============================================================================
if __name__=="__main__":
    main()