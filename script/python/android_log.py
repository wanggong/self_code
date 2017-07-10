#!/usr/bin/python

################################   useage   ##########################
#1. setup build env for android
#2. link the symbols to current,eg:ln -s /home/wanggongzhen/new_camera/out/target/product/lc1861evb_arm64/symbols symbols
#3. adb shell pull /data/local/log/trace ./trace
#4. ln -s trace/trace_xxx.txt trace.txt;
#	ln -s trace/maps_xxx.txt maps.txt; 
#5.	exec this file, ./fconvert.py
################################   end   ################################


import sys
import re
import commands

symbols_dir = './symbols'
	
class android_log:
	'maps for libs'
	def __init__(self , filename):
		self.infile = filename
		self.outfile = filename+'.txt'
		
	def process(self):
		infd = open(self.infile)
		outfd = open(self.outfile,'w')
		pattern = r'#\d+\s+pc\s+([0-9a-f]{8})\s+(\S+)'
		for line in infd:
			outfd.write(line)
			result = re.search(pattern , line)
			if result:
				addr = result.group(1)
				libname = result.group(2)
				#print('%s,%s'%(addr,libname))
				libfile = symbols_dir + libname;
				cmd = 'arm-linux-androideabi-addr2line '
				args = ' -a -i -p -s -f -C -e '
				command_all = cmd + addr + args + libfile
				(status , info) = commands.getstatusoutput(command_all)
				if status == 0:
					#print(info)
					outfd.write('\t\t'+info+'\n')
				else:
					print ('get funcname failed status:(%d),addr:%s,libname:%s'%(status,addr,libname))
		infd.close()
		outfd.close()
	
def main():
	print('%s'%(sys.argv[1]))
	log = android_log(sys.argv[1])
	log.process()
	
main()

